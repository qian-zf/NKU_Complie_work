#include <backend/targets/aarch64/passes/lowering/phi_elimination.h>
#include <debug.h>
#include <algorithm>
#include <transfer.h>
#include <set>

namespace BE::AArch64::Passes::Lowering
{
    using namespace BE;

    void PhiEliminationPass::runOnModule(BE::Module& module, const BE::Targeting::TargetInstrAdapter* adapter)
    {
        if (module.functions.empty()) return;
        for (auto* func : module.functions) runOnFunction(func, adapter);
    }

    void PhiEliminationPass::runOnFunction(BE::Function* func, const BE::Targeting::TargetInstrAdapter* adapter)
    {
        if (!func || func->blocks.empty()) return;

        BE::MIR::CFGBuilder cfgBuilder(adapter);
        BE::MIR::CFG*       cfg = cfgBuilder.buildCFGForFunction(func);
        if (!cfg) return;

        std::vector<BE::Block*> orderedBlocks;
        for (auto& [bid, block] : func->blocks) orderedBlocks.push_back(block);
        std::sort(orderedBlocks.begin(), orderedBlocks.end(), [](BE::Block* a, BE::Block* b) {
            return a->blockId < b->blockId;
        });

        auto insertCopies = [&](BE::Block* blk, std::deque<BE::MInstruction*>::iterator insertIt,
                                const std::vector<std::pair<BE::Register, BE::Operand*>>& copies) {
            std::vector<std::pair<BE::Register, BE::Operand*>> moves = copies;
            auto isSrcReg = [](BE::Operand* op) { return dynamic_cast<BE::RegOperand*>(op) != nullptr; };
            auto srcRegOf = [](BE::Operand* op) { return static_cast<BE::RegOperand*>(op)->reg; };

            while (!moves.empty())
            {
                bool progressed = false;
                std::set<int> destIds;
                for (auto& p : moves) destIds.insert(p.first.rId);

                for (size_t i = 0; i < moves.size(); )
                {
                    auto [dst, srcOp] = moves[i];
                    if (isSrcReg(srcOp))
                    {
                        BE::Register sr = srcRegOf(srcOp);
                        if (destIds.count(sr.rId) && sr.rId != dst.rId)
                        {
                            ++i;
                            continue;
                        }
                        if (sr.rId == dst.rId && sr.isVreg == dst.isVreg)
                        {
                            moves.erase(moves.begin() + i);
                            progressed = true;
                            continue;
                        }
                        auto* inst = BE::AArch64::createMove(new BE::RegOperand(dst), new BE::RegOperand(sr));
                        if (insertIt == blk->insts.end())
                            blk->insts.push_back(inst);
                        else
                            blk->insts.insert(insertIt, inst);
                        moves.erase(moves.begin() + i);
                        progressed = true;
                    }
                    else if (auto* i32 = dynamic_cast<BE::I32Operand*>(srcOp))
                    {
                        auto* inst = BE::AArch64::createMove(new BE::RegOperand(dst), i32->val);
                        if (insertIt == blk->insts.end())
                            blk->insts.push_back(inst);
                        else
                            blk->insts.insert(insertIt, inst);
                        moves.erase(moves.begin() + i);
                        progressed = true;
                    }
                    else
                    {
                        ++i;
                    }
                }

                if (!progressed && !moves.empty())
                {
                    size_t k = 0;
                    for (; k < moves.size(); ++k)
                        if (isSrcReg(moves[k].second)) break;
                    if (k == moves.size()) break;
                    
                    auto [dst, srcOp] = moves[k];
                    BE::Register sr   = srcRegOf(srcOp);
                    BE::Register tmp  = BE::getVReg(dst.dt);
                    
                    auto* tMov = BE::AArch64::createMove(new BE::RegOperand(tmp), new BE::RegOperand(sr));
                    if (insertIt == blk->insts.end())
                        blk->insts.push_back(tMov);
                    else
                        blk->insts.insert(insertIt, tMov);
                    
                    for (auto& p : moves)
                    {
                        if (isSrcReg(p.second) && srcRegOf(p.second).rId == sr.rId)
                        {
                            delete p.second;
                            p.second = new BE::RegOperand(tmp);
                        }
                    }
                }
            }
        };

        auto findFirstBranch = [&](BE::Block* blk) {
            for (auto it = blk->insts.begin(); it != blk->insts.end(); ++it)
            {
                auto* inst = *it;
                if (adapter->isUncondBranch(inst) || adapter->isCondBranch(inst))
                {
                    return it;
                }
            }
            return blk->insts.end();
        };

        auto redirectEdge = [&](BE::Block* fromBlk, uint32_t oldTo, uint32_t newTo) {
            for (auto it = fromBlk->insts.begin(); it != fromBlk->insts.end(); ++it)
            {
                auto* inst = *it;
                if (adapter->isUncondBranch(inst) || adapter->isCondBranch(inst))
                {
                    int t = adapter->extractBranchTarget(inst);
                    if (t == static_cast<int>(oldTo))
                    {
                        auto* a64 = dynamic_cast<BE::AArch64::Instr*>(inst);
                        if (a64 && !a64->operands.empty())
                        {
                            if (auto* lab = dynamic_cast<BE::AArch64::LabelOperand*>(a64->operands[0]))
                                lab->targetBlockId = static_cast<int>(newTo);
                        }
                    }
                }
            }
        };

        uint32_t maxId = 0;
        for (auto& [id, _blk] : func->blocks) 
            if (id > maxId) maxId = id;

        for (auto* block : orderedBlocks)
        {
            std::vector<BE::PhiInst*> phis;
            for (auto* inst : block->insts)
                if (auto* p = dynamic_cast<BE::PhiInst*>(inst)) phis.push_back(p);
            if (phis.empty()) continue;

            uint32_t bid = block->blockId;

            std::vector<uint32_t> preds;
            if (bid < cfg->inv_graph_id.size()) preds = cfg->inv_graph_id[bid];

            std::vector<std::pair<uint32_t, uint32_t>> insertPairs;

            // 2. 遍历所有前驱，决定插入位置
            for (auto pid : preds)
            {
                size_t succCount = (pid < cfg->graph_id.size()) ? cfg->graph_id[pid].size() : 0;
                BE::Block* predBlk = func->blocks[pid];
                
                // Critical Edge Splitting (关键边分割)
                // 如果前驱有多个后继（即它是一个条件跳转），我们不能直接在前驱末尾插入拷贝指令，
                // 因为那样会影响到前驱跳转到的其他块。
                // 解决方案：在它们之间插入一个新的中间块 (Edge Block)，将拷贝指令放在中间块中。
                if (succCount > 1)
                {
                    uint32_t newId = ++maxId;
                    auto* edgeBlk  = new BE::Block(newId);
                    func->blocks[newId] = edgeBlk;
                    cfg->addNewBlock(newId, edgeBlk);
                    cfg->removeEdge(pid, bid);
                    cfg->makeEdge(pid, newId);
                    cfg->makeEdge(newId, bid);
                    redirectEdge(predBlk, bid, newId);
                    
                    // 中间块只需跳转到当前块
                    edgeBlk->insts.push_back(BE::AArch64::createInstr1(BE::AArch64::Operator::B,
                        new BE::AArch64::LabelOperand(static_cast<int>(bid))));
                    
                    insertPairs.emplace_back(newId, pid);
                }
                else
                {
                    // 如果前驱只有一个后继（即只能跳到当前块），可以直接在前驱末尾插入
                    insertPairs.emplace_back(pid, pid);
                }
            }

            // 3. 生成并插入拷贝指令
            for (auto [insertId, origPred] : insertPairs)
            {
                BE::Block* insertBlk = func->blocks[insertId];
                std::vector<std::pair<BE::Register, BE::Operand*>> copies;
                
                for (auto* phi : phis)
                {
                    auto it = phi->incomingVals.find(origPred);
                    if (it == phi->incomingVals.end()) continue;
                    
                    BE::Operand* src = it->second;
                    if (auto* r = dynamic_cast<BE::RegOperand*>(src))
                        copies.emplace_back(phi->resReg, new BE::RegOperand(r->reg));
                    else if (auto* i32 = dynamic_cast<BE::I32Operand*>(src))
                        copies.emplace_back(phi->resReg, new BE::I32Operand(i32->val));
                    else
                        continue;
                }

                if (copies.empty()) continue;

                // 找到插入点：必须在跳转指令之前
                auto insertIt = findFirstBranch(insertBlk);
                insertCopies(insertBlk, insertIt, copies);
            }

            // 4. 移除 Phi 指令
            for (auto it = block->insts.begin(); it != block->insts.end(); )
            {
                if (auto* p = dynamic_cast<BE::PhiInst*>(*it))
                {
                    std::set<BE::Operand*> deletedOps;
                    for (auto& kv : p->incomingVals)
                    {
                        if (kv.second && deletedOps.find(kv.second) == deletedOps.end())
                        {
                            delete kv.second;
                            deletedOps.insert(kv.second);
                        }
                    }
                    BE::MInstruction::delInst(p);
                    it = block->insts.erase(it);
                }
                else
                    ++it;
            }
        }

        delete cfg;
    }
}  // namespace BE::AArch64::Passes::Lowering