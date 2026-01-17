#include <backend/targets/aarch64/passes/lowering/stack_lowering.h>
#include <backend/targets/aarch64/aarch64_defs.h>
#include <backend/targets/aarch64/aarch64_reg_info.h>
#include <backend/mir/m_block.h>
#include <backend/mir/m_function.h>
#include <backend/mir/m_instruction.h>

namespace BE::AArch64::Passes::Lowering
{
    void StackLoweringPass::runOnModule(BE::Module& module)
    {
        for (auto* func : module.functions)
        {
            if (!func) continue;
            lowerFunction(func);
        }
    }

    void StackLoweringPass::lowerFunction(BE::Function* func)
    {
        // 确保偏移量已计算 (FrameInfo 负责维护栈帧中的对象)
        func->frameInfo.calculateOffsets();

        for (auto& [id, block] : func->blocks)
        {
            if (!block) continue;

            for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
            {
                auto* inst = *it;

                // 1. 处理 SSLOT (Spill Slot Store) 伪指令
                // SSLOT 用于将寄存器值保存到溢出槽中
                if (inst->kind == InstKind::SSLOT)
                {
                    auto* store = static_cast<FIStoreInst*>(inst);
                    int   offset = func->frameInfo.getSpillSlotOffset(store->frameIndex);
                    Register src = store->src;

                    // Determine scale for immediate offset check
                    // AArch64 的 LDR/STR 指令支持 scaled immediate offset
                    // 例如 STR x0, [sp, #8] (scale=8, imm=1)
                    int scale = 8;
                    if (src.dt && (src.dt->equal(I32) || src.dt->equal(F32))) scale = 4;
                    
                    if (fitsUnsignedScaledOffset(offset, scale))
                    {
                        auto* str = createInstr2(Operator::STR, new RegOperand(src), new MemOperand(PR::sp, offset));
                        *it = str;
                    }
                    else
                    {
                        // 偏移量超出范围，需要使用临时寄存器构造地址
                        // Materialize offset into x16
                        Register tmpReg = PR::x16;
                        
                        // 1. MOVZ x16, offset & 0xFFFF
                        auto* movz = createInstr2(Operator::MOVZ, new RegOperand(tmpReg), new ImmeOperand(offset & 0xFFFF));
                        block->insts.insert(it, movz);

                        // 2. MOVK if needed
                        if (offset > 0xFFFF)
                        {
                            auto* movk = createInstr3(Operator::MOVK, new RegOperand(tmpReg),
                                                      new ImmeOperand((offset >> 16) & 0xFFFF), new ImmeOperand(16));
                            block->insts.insert(it, movk);
                        }

                        // 3. ADD x16, sp, x16
                        auto* add = createInstr3(Operator::ADD, new RegOperand(tmpReg), new RegOperand(PR::sp), new RegOperand(tmpReg));
                        block->insts.insert(it, add);

                        // 4. STR src, [x16, #0]
                        auto* str = createInstr2(Operator::STR, new RegOperand(src), new MemOperand(tmpReg, 0));
                        *it = str;
                    }
                    
                    BE::MInstruction::delInst(inst);
                    continue;
                }

                // 2. 处理 LSLOT (Spill Slot Load) 伪指令
                // LSLOT 用于从溢出槽加载值到寄存器
                if (inst->kind == InstKind::LSLOT)
                {
                    auto* load = static_cast<FILoadInst*>(inst);
                    int   offset = func->frameInfo.getSpillSlotOffset(load->frameIndex);
                    Register dest = load->dest;

                    // Determine scale
                    int scale = 8;
                    if (dest.dt && (dest.dt->equal(I32) || dest.dt->equal(F32))) scale = 4;

                    if (fitsUnsignedScaledOffset(offset, scale))
                    {
                        auto* ldr = createInstr2(Operator::LDR, new RegOperand(dest), new MemOperand(PR::sp, offset));
                        *it = ldr;
                    }
                    else
                    {
                         // Materialize offset into x16
                        Register tmpReg = PR::x16;
                        
                        // 1. MOVZ x16, offset & 0xFFFF
                        auto* movz = createInstr2(Operator::MOVZ, new RegOperand(tmpReg), new ImmeOperand(offset & 0xFFFF));
                        block->insts.insert(it, movz);

                        // 2. MOVK if needed
                        if (offset > 0xFFFF)
                        {
                            auto* movk = createInstr3(Operator::MOVK, new RegOperand(tmpReg),
                                                      new ImmeOperand((offset >> 16) & 0xFFFF), new ImmeOperand(16));
                            block->insts.insert(it, movk);
                        }

                        // 3. ADD x16, sp, x16
                        auto* add = createInstr3(Operator::ADD, new RegOperand(tmpReg), new RegOperand(PR::sp), new RegOperand(tmpReg));
                        block->insts.insert(it, add);

                        // 4. LDR dest, [x16, #0]
                        auto* ldr = createInstr2(Operator::LDR, new RegOperand(dest), new MemOperand(tmpReg, 0));
                        *it = ldr;
                    }

                    BE::MInstruction::delInst(inst);
                    continue;
                }

                if (inst->kind != InstKind::TARGET) continue;

                // 3. 处理指令中的 FrameIndexOperand
                // 有些目标指令可能直接引用了 FrameIndex (如 add x0, sp, %stack.0)
                // 这种情况较少见，通常在 FrameLowering 或 ISel 阶段产生
                auto* a64Inst = dynamic_cast<Instr*>(inst);
                if (!a64Inst || !a64Inst->use_fiops || !a64Inst->fiop) continue;

                // 获取 FrameIndex 对应的偏移量
                auto* fiOp = dynamic_cast<FrameIndexOperand*>(a64Inst->fiop);
                if (!fiOp) continue;
                int fi     = fiOp->frameIndex;
                int offset = func->frameInfo.getSpillSlotOffset(fi);

                if (offset < 0)
                {
                    // 尝试从 local objects 中查找（如果使用的是 createLocalObject）
                    // 但 DAGIsel 目前使用 createSpillSlot，所以这里应该能找到
                    // 如果没找到，可能是逻辑错误或使用了未初始化的 FrameIndex
                    // 这里为了健壮性，可以打印警告或跳过
                    // ERROR("Invalid frame index %d in function %s", fi, func->name.c_str());
                    continue;
                }

                // 移除原有的 FrameIndexOperand
                // 注意：fiop 通常是最后一个操作数，但也可能不是，遍历删除比较安全
                for (auto opIt = a64Inst->operands.begin(); opIt != a64Inst->operands.end();)
                {
                    if (*opIt == a64Inst->fiop)
                    {
                        opIt = a64Inst->operands.erase(opIt);
                    }
                    else
                    {
                        ++opIt;
                    }
                }

                // 清理 FrameIndexOperand 相关状态
                delete a64Inst->fiop;
                a64Inst->fiop      = nullptr;
                a64Inst->use_fiops = false;

                // 生成具体的地址计算指令
                if (fitsUnsignedImm12(offset))
                {
                    // 偏移量在 12 位立即数范围内，直接使用 ADD dst, sp, #imm
                    a64Inst->operands.push_back(new ImmeOperand(offset));
                }
                else
                {
                    // 偏移量较大，需要使用临时寄存器 materialize
                    // 使用 x16 (IP0) 作为临时寄存器
                    Register tmpReg = PR::x16;

                    // 1. MOVZ x16, offset & 0xFFFF
                    auto* movz = createInstr2(Operator::MOVZ, new RegOperand(tmpReg), new ImmeOperand(offset & 0xFFFF));
                    block->insts.insert(it, movz);

                    // 2. 如果高位不为 0，生成 MOVK
                    if (offset > 0xFFFF)
                    {
                        auto* movk = createInstr3(Operator::MOVK, new RegOperand(tmpReg),
                                                  new ImmeOperand((offset >> 16) & 0xFFFF), new ImmeOperand(16));
                        block->insts.insert(it, movk);
                    }

                    // 3. 修改原指令为 ADD dst, sp, x16
                    a64Inst->operands.push_back(new RegOperand(tmpReg));
                }
            }
        }
    }
}  // namespace BE::AArch64::Passes::Lowering
