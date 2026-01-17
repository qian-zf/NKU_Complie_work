#include <middleend/pass/dce.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/module/ir_operand.h>
#include <algorithm>
#include <map>
#include <set>
#include <queue>

namespace ME
{
    /**
     * @brief 运行死代码消除 Pass (Dead Code Elimination)
     * 
     * 这是一个保守的 DCE 实现，旨在移除那些计算结果未被使用的指令。
     * 
     * 与 ADCE 的区别：
     * - 保守性：DCE 认为所有的控制流指令（Branch）都是有副作用的，因此保留所有控制流结构。
     *   - 为什么？不进行控制依赖分析，无法确定分支是否可以安全移除（例如分支可能控制着副作用指令的执行）。
     * - 局限性：无法删除空的基本块或不可达代码（这些通常由 ADCE 或 SimplifyCFG 处理）。
     */
    void DCEPass::runOnFunction(Function& function)
    {
        bool changed = true;
        // 循环执行，因为删除一条指令可能会导致定义它的指令也变成死代码
        // 为什么？例如 a = b + 1; c = a * 2; 如果 c 死掉了，a 也就死掉了。
        // 一次 Mark-Sweep 可能只发现 c 是死的，删除 c 后下一轮才能发现 a 是死的。
        while (changed)
        {
            changed = performDCE(function);
        }
    }

    /**
     * @brief 检查指令是否有副作用 (Side Effect Check)
     * 
     * 有副作用的指令是活跃性传播的根节点 (Roots)。
     * 在保守 DCE 中，以下指令被认为有副作用且不可删除：
     * - STORE: 修改内存
     * - CALL: 可能修改全局状态或进行 I/O
     * - RET: 决定函数返回值
     * - BR: 决定控制流走向 (DCE 不改变 CFG 结构)
     * 
     * @return true 如果指令没有副作用（可以安全删除），否则 false
     */
    bool DCEPass::isSideEffectFree(Instruction* inst)
    {
        switch (inst->opcode)
        {
            case Operator::STORE:
            case Operator::CALL:
            case Operator::RET:
            case Operator::BR_COND:
            case Operator::BR_UNCOND:
                return false;
            default:
                return true;
        }
    }

    /**
     * @brief 执行单次死代码消除
     * 
     * 算法流程 (Mark-Sweep):
     * 1. Mark (标记):
     *    - 扫描所有指令，识别有副作用的指令作为活跃根节点 (Roots)。
     *    - 将 Roots 加入 Worklist。
     *    - 迭代 Worklist，将活跃指令所使用的操作数的定义指令 (Def-Use) 标记为活跃。
     *      - 为什么？如果指令 I 是活的，那么它计算所依赖的输入数据也必须被计算出来。
     * 
     * 2. Sweep (清除):
     *    - 再次遍历所有指令。
     *    - 物理删除所有未被标记为活跃的指令。
     * 
     * @return true 如果有指令被删除，否则 false
     */
    bool DCEPass::performDCE(Function& function)
    {
        std::set<Instruction*> liveInsts;
        std::vector<Instruction*> worklist;
        std::map<Operand*, Instruction*> defMap;

        // 1. 初始化阶段：构建定义映射 (DefMap) 并识别种子指令 (Seeds)
        for (auto& [blockId, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                // 1.1 记录该指令定义了哪个操作数 (用于反向查找)
                Operand* res = nullptr;
                switch (inst->opcode)
                {
                    case Operator::ADD:
                    case Operator::SUB:
                    case Operator::MUL:
                    case Operator::DIV:
                    case Operator::MOD:
                    case Operator::FADD:
                    case Operator::FSUB:
                    case Operator::FMUL:
                    case Operator::FDIV:
                    case Operator::BITAND:
                    case Operator::BITXOR:
                    case Operator::SHL:
                    case Operator::LSHR:
                    case Operator::ASHR: {
                        auto* i = static_cast<ArithmeticInst*>(inst);
                        res = i->res;
                        break;
                    }
                    case Operator::ICMP: {
                        auto* i = static_cast<IcmpInst*>(inst);
                        res = i->res;
                        break;
                    }
                    case Operator::FCMP: {
                        auto* i = static_cast<FcmpInst*>(inst);
                        res = i->res;
                        break;
                    }
                    case Operator::LOAD: {
                        auto* i = static_cast<LoadInst*>(inst);
                        res = i->res;
                        break;
                    }
                    case Operator::GETELEMENTPTR: {
                        auto* i = static_cast<GEPInst*>(inst);
                        res = i->res;
                        break;
                    }
                    case Operator::ZEXT: {
                        auto* i = static_cast<ZextInst*>(inst);
                        res = i->dest;
                        break;
                    }
                    case Operator::SITOFP: {
                        auto* i = static_cast<SI2FPInst*>(inst);
                        res = i->dest;
                        break;
                    }
                    case Operator::FPTOSI: {
                        auto* i = static_cast<FP2SIInst*>(inst);
                        res = i->dest;
                        break;
                    }
                    case Operator::PHI: {
                        auto* i = static_cast<PhiInst*>(inst);
                        res = i->res;
                        break;
                    }
                    case Operator::CALL: {
                        auto* i = static_cast<CallInst*>(inst);
                        res = i->res;
                        break;
                    }
                    case Operator::ALLOCA: {
                        auto* i = static_cast<AllocaInst*>(inst);
                        res = i->res;
                        break;
                    }
                    default:
                        break;
                }

                if (res)
                {
                    defMap[res] = inst;
                }

                // 1.2 如果指令有副作用，它就是活跃的根源
                if (!isSideEffectFree(inst))
                {
                    liveInsts.insert(inst);
                    worklist.push_back(inst);
                }
            }
        }

        // 2. 活跃性传播阶段 (Propagate Liveness)
        // 只要一个指令是活跃的，它所需要的操作数（Operands）的生成者也必须是活跃的。
        size_t head = 0;
        while (head < worklist.size())
        {
            Instruction* inst = worklist[head++];
            
            // 辅助 Lambda: 将操作数的定义指令标记为活跃
            auto addOperand = [&](Operand* op) {
                if (op && defMap.count(op))
                {
                    Instruction* defInst = defMap[op];
                    if (liveInsts.find(defInst) == liveInsts.end())
                    {
                        liveInsts.insert(defInst);
                        worklist.push_back(defInst);
                    }
                }
            };

            // 根据指令类型，找出它使用的所有操作数
            switch (inst->opcode)
            {
                case Operator::ADD:
                case Operator::SUB:
                case Operator::MUL:
                case Operator::DIV:
                case Operator::MOD:
                case Operator::FADD:
                case Operator::FSUB:
                case Operator::FMUL:
                case Operator::FDIV:
                case Operator::BITAND:
                case Operator::BITXOR:
                case Operator::SHL:
                case Operator::LSHR:
                case Operator::ASHR: {
                    auto* i = static_cast<ArithmeticInst*>(inst);
                    addOperand(i->lhs);
                    addOperand(i->rhs);
                    break;
                }
                case Operator::ICMP: {
                    auto* i = static_cast<IcmpInst*>(inst);
                    addOperand(i->lhs);
                    addOperand(i->rhs);
                    break;
                }
                case Operator::FCMP: {
                    auto* i = static_cast<FcmpInst*>(inst);
                    addOperand(i->lhs);
                    addOperand(i->rhs);
                    break;
                }
                case Operator::LOAD: {
                    auto* i = static_cast<LoadInst*>(inst);
                    addOperand(i->ptr);
                    break;
                }
                case Operator::STORE: {
                    auto* i = static_cast<StoreInst*>(inst);
                    addOperand(i->ptr);
                    addOperand(i->val);
                    break;
                }
                case Operator::BR_COND: {
                    auto* i = static_cast<BrCondInst*>(inst);
                    addOperand(i->cond);
                    break;
                }
                case Operator::RET: {
                    auto* i = static_cast<RetInst*>(inst);
                    addOperand(i->res);
                    break;
                }
                case Operator::CALL: {
                    auto* i = static_cast<CallInst*>(inst);
                    for (auto& arg : i->args)
                    {
                        addOperand(arg.second);
                    }
                    break;
                }
                case Operator::GETELEMENTPTR: {
                    auto* i = static_cast<GEPInst*>(inst);
                    addOperand(i->basePtr);
                    for (auto* idx : i->idxs)
                    {
                        addOperand(idx);
                    }
                    break;
                }
                case Operator::ZEXT: {
                    auto* i = static_cast<ZextInst*>(inst);
                    addOperand(i->src);
                    break;
                }
                case Operator::SITOFP: {
                    auto* i = static_cast<SI2FPInst*>(inst);
                    addOperand(i->src);
                    break;
                }
                case Operator::FPTOSI: {
                    auto* i = static_cast<FP2SIInst*>(inst);
                    addOperand(i->src);
                    break;
                }
                case Operator::PHI: {
                    auto* i = static_cast<PhiInst*>(inst);
                    for (auto& pair : i->incomingVals)
                    {
                        addOperand(pair.second);
                    }
                    break;
                }
                default:
                    break;
            }
        }

        // 3. 清除阶段 (Sweep)
        // 移除所有未被标记为 Live 的指令
        bool changed = false;
        for (auto& [blockId, block] : function.blocks)
        {
            auto it = block->insts.begin();
            while (it != block->insts.end())
            {
                if (liveInsts.find(*it) == liveInsts.end())
                {
                    it = block->insts.erase(it);
                    changed = true;
                }
                else
                {
                    ++it;
                }
            }
        }
        
        return changed;
    }

}  // namespace ME
