#include <middleend/pass/adce.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/module/ir_operand.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/pass/analysis/analysis_manager.h>
#include <set>
#include <map>
#include <vector>
#include <queue>

namespace ME
{
    /**
     * @brief 运行激进死代码消除 Pass (Aggressive Dead Code Elimination)
     * 
     * ADCE 与普通 DCE 的区别在于：
     * 1. 假设：默认所有代码都是“死”的，只有被证明“活”的代码才会被保留。
     *    - 为什么？这样可以一次性消除不可达代码和死循环，而无需多次迭代普通 DCE。
     * 2. 能力：不仅能删除无用指令，还能删除不可达基本块、死循环，并简化控制流图（CFG）。
     * 
     * 整体流程是一个迭代收敛的过程：
     * 1. 移除死循环 (Dead Loop Elimination)
     *    - 为什么先做这个？死循环是控制流图中的强连通分量，普通的活跃性分析难以处理（循环依赖会导致它们互相保活）。
     * 2. 核心 ADCE 活跃性分析与指令删除
     * 3. 控制流简化 (Control Flow Simplification)
     *    - 为什么？删除指令后会留下空的跳转块或冗余分支，需要清理以减少代码体积和执行开销。
     */
    void ADCEPass::runOnFunction(Function& function)
    {
        if (function.blocks.empty()) return;
        
        bool globalChanged = true;
        int maxOuterIterations = 50; // 防止无限循环的安全上限
        
        for (int outer = 0; outer < maxOuterIterations && globalChanged; ++outer)
        {
            globalChanged = false;
            
            // 每次迭代前更新 CFG 分析
            Analysis::AM.invalidate(function);
            auto* cfg = Analysis::AM.get<Analysis::CFG>(function);
            
            // 步骤 1: 移除死循环
            // 策略：如果一个循环没有副作用（无 IO/全局写），且计算结果不被外部使用，直接删除整个循环。
            // 为什么？编译器优化应当消除无效的计算资源消耗，即使是无限循环，如果它不产生可观测行为，也是可以移除的。
            bool changed = true;
            while (changed)
            {
                changed = removeDeadLoops(function, cfg);
                if (changed)
                {
                    globalChanged = true;
                    Analysis::AM.invalidate(function);
                    cfg = Analysis::AM.get<Analysis::CFG>(function);
                }
            }
            
            // 步骤 2: ADCE 核心迭代 (活跃性传播 + 删除)
            // 策略：从 Side-Effect 指令开始，沿着数据依赖和控制依赖反向传播活跃性。
            // 为什么？这是基于“可观测性”的优化。只有影响最终输出（Side Effect）的指令才是必须执行的。
            changed = true;
            while (changed)
            {
                changed = runADCEIteration(function, cfg);
                if (changed)
                {
                    globalChanged = true;
                    Analysis::AM.invalidate(function);
                    cfg = Analysis::AM.get<Analysis::CFG>(function);
                }
            }
            
            // 步骤 3: 控制流简化
            // 策略：清理 ADCE 留下的碎片，如合并跳转块、删除不可达块、简化条件分支。
            for (int i = 0; i < 20; ++i)
            {
                Analysis::AM.invalidate(function);
                cfg = Analysis::AM.get<Analysis::CFG>(function);
                if (!simplifyControlFlow(function, cfg))
                    break;
                globalChanged = true;
            }
        }
        
        // 最后清理 PHI 节点，确保没有悬空的引用
        cleanupPhiNodes(function);
    }

    /**
     * @brief 简化控制流图 (Control Flow Simplification)
     * 
     * 执行一系列 CFG 变换以减少基本块数量和跳转开销：
     * 1. Jump Threading (跳板块合并): A -> B -> C，若 B 仅含跳转，优化为 A -> C。
     *    - 为什么？消除无意义的跳转指令，减少流水线冲刷（Pipeline Flush）风险，提高执行效率。
     * 2. Branch Simplification (分支简化): 若条件跳转的 True/False 目标相同，转为无条件跳转。
     *    - 为什么？消除冗余的条件判断，减少分支预测压力。
     * 3. Unreachable Block Removal (不可达块删除): 删除从入口无法到达的块。
     *    - 为什么？减小代码体积，改善指令缓存（I-Cache）利用率。
     * 
     * @return true 如果 CFG 发生了变化
     */
    bool ADCEPass::simplifyControlFlow(Function& function, Analysis::CFG* cfg)
    {
        bool changed = false;
        size_t entryBlockId = function.blocks.begin()->first;
        
        // Phase 1: 识别纯跳板块 (Basic Block with only Unconditional Branch)
        // 这种块只起到转发作用，可以被跳过。
        std::map<size_t, size_t> jumpTargets;
        for (auto& [blockId, block] : function.blocks)
        {
            if (blockId == entryBlockId) continue;
            
            // 条件：块内只有一条 BR_UNCOND 指令
            if (block->insts.size() != 1) continue;
            if (block->insts[0]->opcode != Operator::BR_UNCOND) continue;
            
            auto* br = static_cast<BrUncondInst*>(block->insts[0]);
            if (!br->target || br->target->getType() != OperandType::LABEL)
                continue;
            
            size_t targetId = static_cast<LabelOperand*>(br->target)->lnum;
            if (targetId == blockId) continue; // 忽略自环
            
            // 安全性检查：如果目标块有 PHI 节点引用当前块，直接跳过可能会破坏 SSA 结构
            // (除非我们也更新 PHI，这里选择保守策略：有 PHI 引用则不合并)
            auto targetIt = function.blocks.find(targetId);
            bool hasPhiRef = false;
            if (targetIt != function.blocks.end())
            {
                for (auto* inst : targetIt->second->insts)
                {
                    if (inst->opcode != Operator::PHI) break;
                    PhiInst* phi = static_cast<PhiInst*>(inst);
                    Operand* label = getLabelOperand(blockId);
                    if (phi->incomingVals.find(label) != phi->incomingVals.end())
                    {
                        hasPhiRef = true;
                        break;
                    }
                }
            }
            
            if (!hasPhiRef)
            {
                jumpTargets[blockId] = targetId;
            }
        }
        
        // Phase 2: 路径压缩 (Path Compression)
        // 将跳转链 A -> B -> C -> D 压缩为 A -> D
        for (auto& [blockId, targetId] : jumpTargets)
        {
            size_t finalTarget = targetId;
            std::set<size_t> visited;
            visited.insert(blockId);
            
            while (jumpTargets.count(finalTarget) && !visited.count(finalTarget))
            {
                visited.insert(finalTarget);
                finalTarget = jumpTargets[finalTarget];
            }
            
            jumpTargets[blockId] = finalTarget;
        }
        
        // Phase 3: 简化条件分支
        // 检查 BR_COND 的 True 和 False 路径是否最终指向同一个块
        for (auto& [blockId, block] : function.blocks)
        {
            if (block->insts.empty()) continue;
            
            Instruction* term = block->insts.back();
            if (term->opcode != Operator::BR_COND) continue;
            
            auto* br = static_cast<BrCondInst*>(term);
            
            if (!br->trueTar || !br->falseTar) continue;
            if (br->trueTar->getType() != OperandType::LABEL) continue;
            if (br->falseTar->getType() != OperandType::LABEL) continue;
            
            size_t trueId = static_cast<LabelOperand*>(br->trueTar)->lnum;
            size_t falseId = static_cast<LabelOperand*>(br->falseTar)->lnum;
            
            // 计算经过跳板块后的最终目标
            size_t trueFinal = jumpTargets.count(trueId) ? jumpTargets[trueId] : trueId;
            size_t falseFinal = jumpTargets.count(falseId) ? jumpTargets[falseId] : falseId;
            
            // 如果两个分支殊途同归，条件判断就是多余的
            if (trueFinal == falseFinal)
            {
                // 再次检查 PHI 引用，确保合并是安全的
                auto finalIt = function.blocks.find(trueFinal);
                bool hasProblem = false;
                
                if (finalIt != function.blocks.end())
                {
                    for (auto* inst : finalIt->second->insts)
                    {
                        if (inst->opcode != Operator::PHI) break;
                        PhiInst* phi = static_cast<PhiInst*>(inst);
                        
                        Operand* trueLabel = getLabelOperand(trueId);
                        Operand* falseLabel = getLabelOperand(falseId);
                        
                        bool refTrue = phi->incomingVals.find(trueLabel) != phi->incomingVals.end();
                        bool refFalse = phi->incomingVals.find(falseLabel) != phi->incomingVals.end();
                        
                        // 如果 PHI 区分了来源路径，我们不能简单合并
                        if ((refTrue && !jumpTargets.count(trueId)) || 
                            (refFalse && !jumpTargets.count(falseId)))
                        {
                            hasProblem = true;
                            break;
                        }
                    }
                }
                
                if (hasProblem) continue;
                
                // 优化：删除计算条件的死代码 (ICMP 等)，因为条件不再被使用了
                std::set<Operand*> usedByCond;
                if (br->cond) usedByCond.insert(br->cond);
                
                while (block->insts.size() > 1)
                {
                    auto it = block->insts.end();
                    --it; --it; // 查看倒数第二条指令
                    Instruction* prevInst = *it;
                    
                    Operand* def = getDefOperand(prevInst);
                    if (def && usedByCond.count(def))
                    {
                        // 这是一个计算条件的指令，将其加入删除队列并追踪其操作数
                        std::vector<Operand*> uses = getUsedOperands(prevInst);
                        for (auto* op : uses)
                            if (op) usedByCond.insert(op);
                        
                        if (prevInst->opcode == Operator::ICMP ||
                            prevInst->opcode == Operator::FCMP ||
                            prevInst->opcode == Operator::ADD || // 简单的算术运算也可删除
                            prevInst->opcode == Operator::SUB ||
                            prevInst->opcode == Operator::MUL ||
                            prevInst->opcode == Operator::LOAD ||
                            prevInst->opcode == Operator::ZEXT)
                        {
                            block->insts.erase(it);
                            changed = true;
                        }
                        else
                        {
                            break; // 遇到复杂指令停止
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                
                // 将 BR_COND 替换为 BR_UNCOND
                block->insts.back() = new BrUncondInst(getLabelOperand(trueFinal));
                changed = true;
            }
        }
        
        // Phase 4: 更新所有跳转指令的目标 (应用 Jump Threading 结果)
        for (auto& [blockId, block] : function.blocks)
        {
            if (block->insts.empty()) continue;
            
            Instruction* term = block->insts.back();
            
            auto updateTarget = [&](Operand*& targetOp) {
                if (targetOp && targetOp->getType() == OperandType::LABEL)
                {
                    size_t targetId = static_cast<LabelOperand*>(targetOp)->lnum;
                    if (jumpTargets.count(targetId) && jumpTargets[targetId] != targetId)
                    {
                        targetOp = getLabelOperand(jumpTargets[targetId]);
                        changed = true;
                    }
                }
            };

            if (term->opcode == Operator::BR_UNCOND)
            {
                auto* br = static_cast<BrUncondInst*>(term);
                updateTarget(br->target);
            }
            else if (term->opcode == Operator::BR_COND)
            {
                auto* br = static_cast<BrCondInst*>(term);
                updateTarget(br->trueTar);
                updateTarget(br->falseTar);
            }
        }
        
        // Phase 5: 删除不可达的基本块 (Unreachable Block Elimination)
        // 使用 BFS 从入口块开始搜索所有可达块
        std::set<size_t> reachable;
        std::queue<size_t> workQueue;
        workQueue.push(entryBlockId);
        reachable.insert(entryBlockId);
        
        while (!workQueue.empty())
        {
            size_t cur = workQueue.front();
            workQueue.pop();
            
            auto blockIt = function.blocks.find(cur);
            if (blockIt == function.blocks.end() || blockIt->second->insts.empty())
                continue;
            
            Instruction* term = blockIt->second->insts.back();
            
            auto visit = [&](Operand* target) {
                if (target && target->getType() == OperandType::LABEL)
                {
                    size_t targetId = static_cast<LabelOperand*>(target)->lnum;
                    if (!reachable.count(targetId))
                    {
                        reachable.insert(targetId);
                        workQueue.push(targetId);
                    }
                }
            };

            if (term->opcode == Operator::BR_UNCOND)
            {
                visit(static_cast<BrUncondInst*>(term)->target);
            }
            else if (term->opcode == Operator::BR_COND)
            {
                visit(static_cast<BrCondInst*>(term)->trueTar);
                visit(static_cast<BrCondInst*>(term)->falseTar);
            }
        }
        
        // 收集并删除不可达块
        std::vector<size_t> blocksToRemove;
        for (auto& [blockId, block] : function.blocks)
        {
            if (!reachable.count(blockId))
            {
                blocksToRemove.push_back(blockId);
            }
        }
        
        for (size_t id : blocksToRemove)
        {
            function.blocks.erase(id);
            changed = true;
        }
        
        return changed;
    }

    // 辅助：清理 PHI 节点中引用了已删除块的条目
    void ADCEPass::cleanupPhiNodes(Function& function)
    {
        std::set<size_t> existingBlocks;
        for (auto& [blockId, block] : function.blocks)
        {
            existingBlocks.insert(blockId);
        }
        
        for (auto& [blockId, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                if (inst->opcode != Operator::PHI) continue;
                
                PhiInst* phi = static_cast<PhiInst*>(inst);
                std::vector<Operand*> toRemove;
                
                for (auto& [labelOp, valOp] : phi->incomingVals)
                {
                    if (labelOp && labelOp->getType() == OperandType::LABEL)
                    {
                        size_t predId = static_cast<LabelOperand*>(labelOp)->lnum;
                        if (!existingBlocks.count(predId))
                        {
                            toRemove.push_back(labelOp);
                        }
                    }
                }
                
                for (auto* label : toRemove)
                {
                    phi->incomingVals.erase(label);
                }
            }
        }
    }

    /**
     * @brief 执行单次 ADCE 核心迭代
     * 
     * 算法核心 (Mark-Sweep based on Dependence):
     * 1. Mark (标记):
     *    - Root Set: 将所有 Side-Effect 指令 (RET, CALL, Global STORE) 标记为 Live。
     *      - 为什么？这些指令产生了外部可见的影响，必须保留。
     *    - Propagation (传播):
     *      a. Data Dependence: 如果指令 Live，它使用的操作数定义指令也 Live。
     *         - 为什么？为了计算出 Live 的结果，必须先计算出它的输入。
     *      b. Memory Dependence: 如果 LOAD Live，相关的 STORE (别名分析) 也 Live。
     *         - 为什么？确保读取到的内存值是正确的。
     *      c. Control Dependence (via PHI): 如果 PHI Live，前驱块的跳转指令也 Live。
     *         - 为什么？PHI 的值取决于从哪条路径到达，因此控制流路径必须保留。
     *      d. Control Dependence (via Block Liveness): 如果一个块内有 Live 指令，控制流跳转到该块的分支指令也 Live。
     *         - 为什么？如果不跳转到该块，Live 指令就无法执行。
     * 
     * 2. Sweep (清除):
     *    - 删除所有未标记为 Live 的指令。
     *    - 对于死掉的 BR_COND (条件不影响结果)，替换为 BR_UNCOND 以简化流图。
     */
    bool ADCEPass::runADCEIteration(Function& function, Analysis::CFG* cfg)
    {
        std::set<Instruction*> liveInsts;
        std::vector<Instruction*> worklist;
        std::map<Operand*, Instruction*> defMap;
        std::map<Instruction*, Block*> instToBlock;
        std::set<Operand*> localAllocaPtrs;
        
        // 0. 预处理：构建 Def-Use 映射和 Block 映射
        for (auto& [blockId, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                instToBlock[inst] = block;
                Operand* def = getDefOperand(inst);
                if (def) defMap[def] = inst;
                
                // 记录局部变量 Alloca，用于简单的内存依赖分析
                if (inst->opcode == Operator::ALLOCA)
                {
                    auto* alloca = static_cast<AllocaInst*>(inst);
                    if (alloca->dims.empty() && alloca->res)
                    {
                        localAllocaPtrs.insert(alloca->res);
                    }
                }
            }
        }

        // 1. 初始化活跃集合 (Seeds)
        // 将所有具有“真实副作用”的指令加入 Worklist
        for (auto& [blockId, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                if (isRealSideEffect(inst))
                {
                    liveInsts.insert(inst);
                    worklist.push_back(inst);
                }
            }
        }

        // 2. 活跃性传播 (Liveness Propagation)
        size_t head = 0;
        while (head < worklist.size())
        {
            Instruction* inst = worklist[head++];
            
            // 2.1 数据依赖传播 (Data Dependence)
            // 我活着，我用的数据定义者也得活着
            std::vector<Operand*> uses = getUsedOperands(inst);
            for (Operand* op : uses)
            {
                if (!op) continue;
                auto it = defMap.find(op);
                if (it != defMap.end() && !liveInsts.count(it->second))
                {
                    liveInsts.insert(it->second);
                    worklist.push_back(it->second);
                }
            }
            
            // 2.2 内存依赖传播 (Memory Dependence for Local Vars)
            // 如果读取局部变量的 LOAD 活着，写入该变量的 STORE 也得活着
            if (inst->opcode == Operator::LOAD)
            {
                auto* load = static_cast<LoadInst*>(inst);
                if (load->ptr && localAllocaPtrs.count(load->ptr))
                {
                    // 简单的别名分析：遍历所有 STORE 寻找写入同一地址的指令
                    // (注意：这是 O(N*M) 的简化实现，实际编译器会用 MemorySSA 或 AliasAnalysis)
                    for (auto& [bid, blk] : function.blocks)
                    {
                        for (auto* other : blk->insts)
                        {
                            if (other->opcode == Operator::STORE)
                            {
                                auto* store = static_cast<StoreInst*>(other);
                                if (store->ptr == load->ptr && !liveInsts.count(other))
                                {
                                    liveInsts.insert(other);
                                    worklist.push_back(other);
                                }
                            }
                        }
                    }
                    // Alloca 本身也需要活着
                    auto allocaIt = defMap.find(load->ptr);
                    if (allocaIt != defMap.end() && !liveInsts.count(allocaIt->second))
                    {
                        liveInsts.insert(allocaIt->second);
                        worklist.push_back(allocaIt->second);
                    }
                }
            }
            
            // 2.3 控制依赖传播 - PHI (Control Dependence via PHI)
            // 如果 PHI 节点活着，意味着控制流来自哪个前驱很重要，因此前驱的跳转指令必须活着。
            // 为什么？PHI 节点根据前驱块的不同选择不同的值 (v1 from BB1, v2 from BB2)。
            // 如果我们删除了 BB1 跳转到当前块的控制流，PHI 就无法正确选择 v1。
            if (inst->opcode == Operator::PHI)
            {
                PhiInst* phi = static_cast<PhiInst*>(inst);
                for (auto& [labelOp, valOp] : phi->incomingVals)
                {
                    if (labelOp && labelOp->getType() == OperandType::LABEL)
                    {
                        size_t predId = static_cast<LabelOperand*>(labelOp)->lnum;
                        auto blockIt = function.blocks.find(predId);
                        if (blockIt != function.blocks.end() && !blockIt->second->insts.empty())
                        {
                            Instruction* term = blockIt->second->insts.back();
                            if (term->isTerminator() && !liveInsts.count(term))
                            {
                                liveInsts.insert(term);
                                worklist.push_back(term);
                            }
                        }
                    }
                }
            }
        }

        // 3. 控制依赖传播 - 块活跃性 (Control Dependence via Block Liveness)
        // 如果一个块内有任何活跃指令，那么该块就是“活跃块”。
        // 为了到达这个活跃块，其前驱块的跳转指令（控制流向）必须保留。
        std::set<size_t> liveBlockIds;
        for (auto* inst : liveInsts)
        {
            auto it = instToBlock.find(inst);
            if (it != instToBlock.end())
            {
                liveBlockIds.insert(it->second->blockId);
            }
        }
        
        size_t entryBlockId = function.blocks.begin()->first;
        liveBlockIds.insert(entryBlockId);

        // 迭代传播块的活跃性
        bool changed = true;
        while (changed)
        {
            changed = false;
            std::vector<size_t> toAdd;
            
            for (size_t bid : liveBlockIds)
            {
                if (bid >= cfg->invG_id.size()) continue;
                
                for (size_t predId : cfg->invG_id[bid])
                {
                    // 标记前驱块本身为活跃
                    if (!liveBlockIds.count(predId))
                    {
                        toAdd.push_back(predId);
                        changed = true;
                    }
                    
                    // 关键：前驱块的 Terminator 必须活着，因为它决定了控制流是否流向当前活跃块
                    auto blockIt = function.blocks.find(predId);
                    if (blockIt != function.blocks.end() && !blockIt->second->insts.empty())
                    {
                        Instruction* term = blockIt->second->insts.back();
                        if (term->isTerminator() && !liveInsts.count(term))
                        {
                            liveInsts.insert(term);
                            worklist.push_back(term);
                        }
                    }
                }
            }
            
            for (size_t id : toAdd)
                liveBlockIds.insert(id);
        }

        // 3.1 继续处理因 Terminator 变活而产生的新数据依赖
        // (因为之前 worklist 处理完了，但步骤 3 又加了新指令进去)
        while (head < worklist.size())
        {
            Instruction* inst = worklist[head++];
            std::vector<Operand*> uses = getUsedOperands(inst);
            for (Operand* op : uses)
            {
                if (!op) continue;
                auto it = defMap.find(op);
                if (it != defMap.end() && !liveInsts.count(it->second))
                {
                    liveInsts.insert(it->second);
                    worklist.push_back(it->second);
                }
            }
            // (省略了重复的 LOAD/STORE 处理逻辑，假设 Terminator 通常不涉及复杂内存依赖)
            // 为完整性，这里应该也检查 LOAD，但 BR 指令通常只用基本类型操作数
        }

        // 4. 执行删除操作 (Sweep)
        size_t deleteCount = 0;

        for (auto& [blockId, block] : function.blocks)
        {
            auto it = block->insts.begin();
            while (it != block->insts.end())
            {
                Instruction* inst = *it;
                
                if (!liveInsts.count(inst))
                {
                    // 特殊处理 BR_COND:
                    // 如果条件跳转死了（说明条件本身不重要），但我们需要保持控制流连通。
                    // 策略：检查哪个分支目标是活跃的，将其改为无条件跳转到该活跃目标。
                    if (inst->opcode == Operator::BR_COND)
                    {
                        BrCondInst* br = static_cast<BrCondInst*>(inst);
                        Operand* target = nullptr;
                        
                        // 优先选择活跃的目标块
                        if (br->trueTar && br->trueTar->getType() == OperandType::LABEL)
                        {
                            size_t trueId = static_cast<LabelOperand*>(br->trueTar)->lnum;
                            if (liveBlockIds.count(trueId))
                                target = br->trueTar;
                        }
                        if (!target && br->falseTar && br->falseTar->getType() == OperandType::LABEL)
                        {
                            size_t falseId = static_cast<LabelOperand*>(br->falseTar)->lnum;
                            if (liveBlockIds.count(falseId))
                                target = br->falseTar;
                        }
                        
                        if (target)
                        {
                            *it = new BrUncondInst(target);
                            ++it;
                            continue;
                        }
                    }
                    
                    deleteCount++;
                    it = block->insts.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        // 移除整个从未被标记为活跃的基本块
        std::vector<size_t> blocksToRemove;
        for (auto& [blockId, block] : function.blocks)
        {
            if (blockId == entryBlockId) continue;
            if (!liveBlockIds.count(blockId))
                blocksToRemove.push_back(blockId);
        }

        for (size_t id : blocksToRemove)
        {
            function.blocks.erase(id);
            deleteCount++;
        }
        
        return deleteCount > 0;
    }

    /**
     * @brief 移除无用循环 (Dead Loop Deletion)
     * 
     * 识别并删除那些“自嗨”的循环。
     * 判定标准：
     * 1. 循环内没有副作用指令 (No Side Effects)。
     *    - 为什么？如果没有副作用，循环体执行多少次都不会改变程序对外的行为。
     * 2. 循环内定义的值没有在循环外被使用 (No External Uses)。
     *    - 为什么？如果结果不被外部使用，那么计算过程也是多余的。
     * 
     * 算法流程：
     * 1. DFS 寻找回边 (Back Edge) 以识别循环。
     *    - 为什么？回边 (u->v, v dom u) 是自然循环的结构特征。
     * 2. 收集循环体所有基本块。
     * 3. 验证上述两个判定标准。
     * 4. 若为死循环，修改入口跳转，直接跳过循环。
     */
    bool ADCEPass::removeDeadLoops(Function& function, Analysis::CFG* cfg)
    {
        size_t numBlocks = 0;
        for (auto& [id, _] : function.blocks)
            numBlocks = std::max(numBlocks, id + 1);
        
        if (numBlocks == 0) return false;
        
        size_t entryId = function.blocks.begin()->first;
        
        // 1. 寻找自然循环 (Natural Loops)
        // 使用 DFS 识别回边 (Back Edge: 指向 DFS 祖先的边)
        std::map<size_t, int> dfsOrder;
        std::set<size_t> visited;
        std::vector<size_t> dfsStack;
        
        dfsStack.push_back(entryId);
        int order = 0;
        
        while (!dfsStack.empty())
        {
            size_t cur = dfsStack.back();
            dfsStack.pop_back();
            
            if (visited.count(cur)) continue;
            visited.insert(cur);
            dfsOrder[cur] = order++;
            
            if (cur < cfg->G_id.size())
            {
                for (auto it = cfg->G_id[cur].rbegin(); it != cfg->G_id[cur].rend(); ++it)
                {
                    if (!visited.count(*it))
                        dfsStack.push_back(*it);
                }
            }
        }
        
        std::vector<std::pair<size_t, size_t>> backEdges;
        for (auto& [blockId, block] : function.blocks)
        {
            if (blockId >= cfg->G_id.size()) continue;
            
            for (size_t succId : cfg->G_id[blockId])
            {
                if (dfsOrder.count(blockId) && dfsOrder.count(succId))
                {
                    // 如果后继节点的 DFS 序小于当前节点，说明是回边
                    if (dfsOrder[succId] <= dfsOrder[blockId])
                        backEdges.push_back({blockId, succId});
                }
            }
        }
        
        if (backEdges.empty()) return false;
        
        // 2. 处理每个发现的循环
        for (auto& [tailId, headId] : backEdges)
        {
            if (headId == entryId) continue; // 不删除包含入口的循环
            
            // 收集循环体 (从 tail 逆向 BFS 直到 head)
            std::set<size_t> loopBlocks;
            loopBlocks.insert(headId);
            
            if (tailId != headId)
            {
                std::queue<size_t> workQueue;
                workQueue.push(tailId);
                loopBlocks.insert(tailId);
                
                while (!workQueue.empty())
                {
                    size_t cur = workQueue.front();
                    workQueue.pop();
                    
                    if (cur >= cfg->invG_id.size()) continue;
                    
                    for (size_t predId : cfg->invG_id[cur])
                    {
                        if (predId == headId) continue;
                        if (!loopBlocks.count(predId))
                        {
                            loopBlocks.insert(predId);
                            workQueue.push(predId);
                        }
                    }
                }
            }
            
            if (loopBlocks.count(entryId)) continue; // 无法删除包含入口的循环
            if (loopBlocks.empty()) continue;
            
            // 寻找循环出口
            std::set<size_t> exitBlocks;
            for (size_t bid : loopBlocks)
            {
                if (bid >= cfg->G_id.size()) continue;
                for (size_t succId : cfg->G_id[bid])
                {
                    if (!loopBlocks.count(succId))
                        exitBlocks.insert(succId);
                }
            }
            
            // 简化处理：只处理单出口循环
            if (exitBlocks.size() != 1) continue;
            
            size_t exitBlockId = *exitBlocks.begin();
            
            // 3. 检查副作用 (Side Effects)
            bool hasSideEffect = false;
            std::set<Operand*> defsInLoop;
            
            for (size_t bid : loopBlocks)
            {
                auto blockIt = function.blocks.find(bid);
                if (blockIt == function.blocks.end()) continue;
                
                for (auto* inst : blockIt->second->insts)
                {
                    // 检查 IO, 全局内存写
                    if (inst->opcode == Operator::CALL ||
                        inst->opcode == Operator::STORE ||
                        (inst->opcode == Operator::LOAD &&
                         static_cast<LoadInst*>(inst)->ptr &&
                         static_cast<LoadInst*>(inst)->ptr->getType() == OperandType::GLOBAL))
                    {
                        hasSideEffect = true;
                        break;
                    }
                    
                    Operand* def = getDefOperand(inst);
                    if (def) defsInLoop.insert(def);
                }
                
                if (hasSideEffect) break;
            }
            
            if (hasSideEffect) continue;
            
            // 4. 检查活跃性 (External Uses)
            // 检查循环内定义的变量是否被循环外的指令使用
            bool usedOutside = false;
            for (auto& [bid, block] : function.blocks)
            {
                if (loopBlocks.count(bid)) continue;
                
                for (auto* inst : block->insts)
                {
                    // 检查 PHI 节点引用
                    if (inst->opcode == Operator::PHI)
                    {
                        PhiInst* phi = static_cast<PhiInst*>(inst);
                        for (auto& [labelOp, valOp] : phi->incomingVals)
                        {
                            if (labelOp && labelOp->getType() == OperandType::LABEL)
                            {
                                size_t predId = static_cast<LabelOperand*>(labelOp)->lnum;
                                if (loopBlocks.count(predId))
                                {
                                    usedOutside = true;
                                    break;
                                }
                            }
                        }
                        if (usedOutside) break;
                    }
                    
                    // 检查数据引用
                    std::vector<Operand*> uses = getUsedOperands(inst);
                    for (Operand* op : uses)
                    {
                        if (op && defsInLoop.count(op))
                        {
                            usedOutside = true;
                            break;
                        }
                    }
                    if (usedOutside) break;
                }
                if (usedOutside) break;
            }
            
            if (usedOutside) continue;
            
            // 5. 执行删除：重定向循环入口的前驱，跳过循环
            std::set<size_t> loopPreds;
            if (headId < cfg->invG_id.size())
            {
                for (size_t predId : cfg->invG_id[headId])
                {
                    if (!loopBlocks.count(predId))
                        loopPreds.insert(predId);
                }
            }
            
            if (loopPreds.empty()) continue;
            
            for (size_t predId : loopPreds)
            {
                auto blockIt = function.blocks.find(predId);
                if (blockIt == function.blocks.end() || blockIt->second->insts.empty())
                    continue;
                
                Instruction* term = blockIt->second->insts.back();
                
                // 修改跳转目标为 exitBlockId
                if (term->opcode == Operator::BR_UNCOND)
                {
                    auto* br = static_cast<BrUncondInst*>(term);
                    if (br->target && br->target->getType() == OperandType::LABEL)
                    {
                        size_t targetId = static_cast<LabelOperand*>(br->target)->lnum;
                        if (targetId == headId)
                            br->target = getLabelOperand(exitBlockId);
                    }
                }
                else if (term->opcode == Operator::BR_COND)
                {
                    auto* br = static_cast<BrCondInst*>(term);
                    
                    if (br->trueTar && br->trueTar->getType() == OperandType::LABEL)
                    {
                        size_t targetId = static_cast<LabelOperand*>(br->trueTar)->lnum;
                        if (targetId == headId)
                            br->trueTar = getLabelOperand(exitBlockId);
                    }
                    if (br->falseTar && br->falseTar->getType() == OperandType::LABEL)
                    {
                        size_t targetId = static_cast<LabelOperand*>(br->falseTar)->lnum;
                        if (targetId == headId)
                            br->falseTar = getLabelOperand(exitBlockId);
                    }
                }
            }
            
            // 物理删除循环块
            for (size_t bid : loopBlocks)
                function.blocks.erase(bid);
            
            return true; // 每次只删除一个循环，然后重新分析
        }
        
        return false;
    }

    /**
     * @brief 辅助：判断指令是否具有“真实副作用” (Real Side Effect)
     * 
     * 这些指令是 ADCE 活跃性传播的根节点 (Roots)。
     * 注意：BR 指令不在此列，因为控制流的活跃性是推导出来的。
     */
    bool ADCEPass::isRealSideEffect(Instruction* inst)
    {
        switch (inst->opcode)
        {
            case Operator::RET:  // 改变函数控制流出口
            case Operator::CALL: // 可能修改外部状态
                return true;
            case Operator::STORE:
            {
                // 只有写全局变量才算绝对的副作用
                // 写局部 Alloca 变量不算，因为如果该变量没人读，写操作也可删
                auto* store = static_cast<StoreInst*>(inst);
                return store->ptr && store->ptr->getType() == OperandType::GLOBAL;
            }
            default:
                return false;
        }
    }

    Operand* ADCEPass::getDefOperand(Instruction* inst)
    {
        switch (inst->opcode)
        {
            case Operator::ADD: case Operator::SUB: case Operator::MUL:
            case Operator::DIV: case Operator::MOD: case Operator::FADD:
            case Operator::FSUB: case Operator::FMUL: case Operator::FDIV:
            case Operator::BITAND: case Operator::BITXOR: case Operator::SHL:
            case Operator::LSHR: case Operator::ASHR:
                return static_cast<ArithmeticInst*>(inst)->res;
            case Operator::ICMP: return static_cast<IcmpInst*>(inst)->res;
            case Operator::FCMP: return static_cast<FcmpInst*>(inst)->res;
            case Operator::LOAD: return static_cast<LoadInst*>(inst)->res;
            case Operator::GETELEMENTPTR: return static_cast<GEPInst*>(inst)->res;
            case Operator::ZEXT: return static_cast<ZextInst*>(inst)->dest;
            case Operator::SITOFP: return static_cast<SI2FPInst*>(inst)->dest;
            case Operator::FPTOSI: return static_cast<FP2SIInst*>(inst)->dest;
            case Operator::PHI: return static_cast<PhiInst*>(inst)->res;
            case Operator::CALL: return static_cast<CallInst*>(inst)->res;
            case Operator::ALLOCA: return static_cast<AllocaInst*>(inst)->res;
            default: return nullptr;
        }
    }

    std::vector<Operand*> ADCEPass::getUsedOperands(Instruction* inst)
    {
        std::vector<Operand*> ops;
        switch (inst->opcode)
        {
            case Operator::ADD: case Operator::SUB: case Operator::MUL:
            case Operator::DIV: case Operator::MOD: case Operator::FADD:
            case Operator::FSUB: case Operator::FMUL: case Operator::FDIV:
            case Operator::BITAND: case Operator::BITXOR: case Operator::SHL:
            case Operator::LSHR: case Operator::ASHR:
            {
                auto* i = static_cast<ArithmeticInst*>(inst);
                if (i->lhs) ops.push_back(i->lhs);
                if (i->rhs) ops.push_back(i->rhs);
                break;
            }
            case Operator::ICMP:
            {
                auto* i = static_cast<IcmpInst*>(inst);
                if (i->lhs) ops.push_back(i->lhs);
                if (i->rhs) ops.push_back(i->rhs);
                break;
            }
            case Operator::FCMP:
            {
                auto* i = static_cast<FcmpInst*>(inst);
                if (i->lhs) ops.push_back(i->lhs);
                if (i->rhs) ops.push_back(i->rhs);
                break;
            }
            case Operator::LOAD:
            {
                auto* i = static_cast<LoadInst*>(inst);
                if (i->ptr) ops.push_back(i->ptr);
                break;
            }
            case Operator::STORE:
            {
                auto* i = static_cast<StoreInst*>(inst);
                if (i->ptr) ops.push_back(i->ptr);
                if (i->val) ops.push_back(i->val);
                break;
            }
            case Operator::BR_COND:
            {
                auto* i = static_cast<BrCondInst*>(inst);
                if (i->cond) ops.push_back(i->cond);
                break;
            }
            case Operator::RET:
            {
                auto* i = static_cast<RetInst*>(inst);
                if (i->res) ops.push_back(i->res);
                break;
            }
            case Operator::CALL:
            {
                auto* i = static_cast<CallInst*>(inst);
                for (auto& arg : i->args)
                    ops.push_back(arg.second);
                break;
            }
            case Operator::GETELEMENTPTR:
            {
                auto* i = static_cast<GEPInst*>(inst);
                if (i->basePtr) ops.push_back(i->basePtr);
                for (auto* idx : i->idxs)
                    ops.push_back(idx);
                break;
            }
            case Operator::ZEXT:
            {
                auto* i = static_cast<ZextInst*>(inst);
                if (i->src) ops.push_back(i->src);
                break;
            }
            case Operator::SITOFP:
            {
                auto* i = static_cast<SI2FPInst*>(inst);
                if (i->src) ops.push_back(i->src);
                break;
            }
            case Operator::FPTOSI:
            {
                auto* i = static_cast<FP2SIInst*>(inst);
                if (i->src) ops.push_back(i->src);
                break;
            }
            case Operator::PHI:
            {
                auto* i = static_cast<PhiInst*>(inst);
                for (auto& pair : i->incomingVals)
                    ops.push_back(pair.second);
                break;
            }
            default:
                break;
        }
        return ops;
    }

}  // namespace ME
