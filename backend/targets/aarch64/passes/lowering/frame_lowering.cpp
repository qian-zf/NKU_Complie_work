#include <backend/targets/aarch64/passes/lowering/frame_lowering.h>
#include <backend/targets/aarch64/aarch64_defs.h>
#include <backend/mir/m_block.h>
#include <backend/mir/m_function.h>
#include <debug.h>
#include <algorithm>

namespace BE::AArch64::Passes::Lowering
{

void FrameLoweringPass::runOnModule(BE::Module& module)
{
    for (auto* func : module.functions)
    {
        if (!func) continue;
        lowerFunction(func);
    }
}

void FrameLoweringPass::lowerFunction(BE::Function* func)
{
    if (func->blocks.empty()) return;

    // 0. 检测是否为叶子函数 (Leaf Function)
    // 叶子函数是指不调用其他函数的函数。
    // 优化：叶子函数不需要保存返回地址寄存器 (LR/x30)，也不一定需要建立栈帧 (FP/x29)，
    // 除非它使用了栈上的参数或局部变量过多导致需要溢出。
    bool isLeaf = true;
    for (auto& [id, block] : func->blocks)
    {
        for (auto* inst : block->insts)
        {
            auto* i = dynamic_cast<Instr*>(inst);
            if (i && (i->op == Operator::BL || i->op == Operator::BLR))
            {
                isLeaf = false;
                break;
            }
        }
        if (!isLeaf) break;
    }

    // 0.5. 扫描用到的 Callee-Saved 寄存器 (被调者保存寄存器)
    // 根据 AArch64 调用约定 (AAPCS64)：
    // - x19-x28 是 Callee-Saved 寄存器，如果函数使用了它们，必须在入口处保存，在出口处恢复。
    // - d8-d15 是 Callee-Saved 浮点寄存器。
    std::set<int> usedCSInt;
    std::set<int> usedCSFloat;

    auto checkReg = [&](BE::Register reg) {
        if (reg.isVreg) return;
        int rId = reg.rId;
        
        bool isFloat = false;
        if (reg.dt == BE::F32 || reg.dt == BE::F64)
        {
            isFloat = true;
        }
        else if (reg.dt == BE::I32 || reg.dt == BE::I64 || reg.dt == BE::PTR || reg.dt == BE::TOKEN || reg.dt == nullptr)
        {
            isFloat = false;
        }
        else
        {
            // Fallback: unsafe check if pointer is valid? 
            // We assume any other pointer is invalid or at least treat as int (safe default)
            // to avoid segfault.
            // fprintf(stderr, "Warning: Unknown DataType pointer %p for reg %d\n", reg.dt, rId);
            isFloat = false;
        }

        if (isFloat)
        {
            if (rId >= 8 && rId <= 15) usedCSFloat.insert(rId);
        }
        else
        {
            if (rId >= 19 && rId <= 28) usedCSInt.insert(rId);
        }
    };

    auto checkOp = [&](BE::Operand* op) {
        if (auto* regOp = dynamic_cast<RegOperand*>(op))
        {
            checkReg(regOp->reg);
        }
    };

    for (auto& [id, block] : func->blocks)
    {
        for (auto* inst : block->insts)
        {
            if (auto* a64Inst = dynamic_cast<Instr*>(inst))
            {
                for (auto* op : a64Inst->operands) checkOp(op);
            }
            else if (auto* moveInst = dynamic_cast<MoveInst*>(inst))
            {
                checkOp(moveInst->src);
                checkOp(moveInst->dest);
            }
            else if (auto* fiLoad = dynamic_cast<FILoadInst*>(inst))
            {
                checkReg(fiLoad->dest);
            }
            else if (auto* fiStore = dynamic_cast<FIStoreInst*>(inst))
            {
                checkReg(fiStore->src);
            }
        }
    }

    // 将 set 转为 vector 并排序，方便配对
    std::vector<int> sortedCSInt(usedCSInt.begin(), usedCSInt.end());
    std::vector<int> sortedCSFloat(usedCSFloat.begin(), usedCSFloat.end());
    std::sort(sortedCSInt.begin(), sortedCSInt.end());
    std::sort(sortedCSFloat.begin(), sortedCSFloat.end());

    int csIntSize   = sortedCSInt.size() * 8;
    int csFloatSize = sortedCSFloat.size() * 8;
    int csTotalSize = csIntSize + csFloatSize;

    // 1. 计算栈帧布局
    // AArch64 栈帧通常向下增长，且必须保持 16 字节对齐 (SP % 16 == 0)。
    // 典型的栈帧布局如下 (地址从高到低):
    //    +-----------------------+
    //    | Caller's SP           |
    //    +-----------------------+
    //    | Saved FP (x29)        | <- FP (Frame Pointer) 指向这里
    //    | Saved LR (x30)        |
    //    +-----------------------+
    //    | Saved CS Float (d8..) | (被调者保存的浮点寄存器)
    //    +-----------------------+
    //    | Saved CS Int (x19..)  | (被调者保存的通用寄存器)
    //    +-----------------------+
    //    | Locals / Spills       | (局部变量、溢出槽、参数传递区)
    //    +-----------------------+ <- SP (Stack Pointer)

    // 计算栈帧各部分大小
    func->frameInfo.calculateOffsets(); // Ensure offsets are calculated including spill slots
    int localSize = func->frameInfo.getStackSize();  // 局部变量 + spill slots + outgoing args
    
    // Ensure (localSize + csTotalSize) is 16-byte aligned
    // 确保 (localSize + csTotalSize) 是 16 字节对齐的。
    // 这样做的目的是为了让 FP/LR 保存的位置相对于 SP 也是 16 字节对齐的，符合 ABI 要求。
    if ((localSize + csTotalSize) % 16 != 0)
    {
        localSize += (16 - ((localSize + csTotalSize) % 16));
    }

    // 优化: 叶子函数不需要保存 FP/LR，除非有栈参数需要通过 FP 访问
    int fpLrSize  = (isLeaf && !func->hasStackParam) ? 0 : 16;  

    int totalFrameSize = localSize + csTotalSize + fpLrSize;

    // 确保整个栈帧大小是 16 字节对齐的
    totalFrameSize = (totalFrameSize + 15) & ~15;

    // 保存计算后的栈大小
    func->stackSize = totalFrameSize;

    // 如果栈帧大小为 0（只有 FP/LR），且非叶子函数，仍然需要保存 FP/LR
    if (totalFrameSize == 0 && !isLeaf)
    {
        totalFrameSize = 16;
        fpLrSize = 16; // 确保一致性
    }

    // ========== 找到入口块 ==========
    Block* entryBlock = nullptr;
    uint32_t minBlockId = UINT32_MAX;
    for (auto& [id, block] : func->blocks)
    {
        if (id < minBlockId)
        {
            minBlockId = id;
            entryBlock = block;
        }
    }

    if (!entryBlock) return;

    // ========== 生成 Prologue ==========
    std::deque<MInstruction*> prologueInsts;

    // STP 的立即数范围: -512 到 504，且必须是 8 的倍数
    // 对于 pre-indexed: stp x29, x30, [sp, #-imm]!
    // 我们使用分步方式更安全

    if (totalFrameSize > 0)
    {
        // 方案: 先分配栈空间，再保存 FP/LR，再设置 FP
        
        // Step 1: sub sp, sp, #totalFrameSize
        if (totalFrameSize <= 4095)
        {
            // 立即数在 12 位范围内，可以直接使用
            auto* subSp = createInstr3(
                Operator::SUB,
                new RegOperand(PR::sp),
                new RegOperand(PR::sp),
                new ImmeOperand(totalFrameSize)
            );
            subSp->comment = "prologue: allocate stack frame";
            prologueInsts.push_back(subSp);
        }
        else
        {
            // 立即数太大，需要先加载到临时寄存器
            // 使用 x16 作为临时寄存器 (IP0, caller-saved)
            auto* movz = createInstr2(
                Operator::MOVZ,
                new RegOperand(PR::x16),
                new ImmeOperand(totalFrameSize & 0xFFFF)
            );
            movz->comment = "prologue: load frame size (low 16 bits)";
            prologueInsts.push_back(movz);

            if (totalFrameSize > 0xFFFF)
            {
                auto* movk = createInstr3(
                    Operator::MOVK,
                    new RegOperand(PR::x16),
                    new ImmeOperand((totalFrameSize >> 16) & 0xFFFF),
                    new ImmeOperand(16)  // shift amount
                );
                movk->comment = "prologue: load frame size (high 16 bits)";
                prologueInsts.push_back(movk);
            }

            auto* subSp = createInstr3(
                Operator::SUB,
                new RegOperand(PR::sp),
                new RegOperand(PR::sp),
                new RegOperand(PR::x16)
            );
            subSp->comment = "prologue: allocate stack frame";
            prologueInsts.push_back(subSp);
        }

        // Step 1.5: Save Callee-Saved Registers
        // Int: [sp, #localSize]
        // Float: [sp, #localSize + csIntSize]
        
        int currentOffset = localSize;
        
        // Handle large stack frames where offsets exceed STP immediate range (-512, 504)
        bool useLargeOffset = (localSize + csTotalSize > 504);
        BE::Register baseReg = PR::sp;
        int baseOffset = 0;

        if (useLargeOffset)
        {
            // Compute x16 = sp + localSize
            if (localSize <= 4095)
            {
                auto* add = createInstr3(
                    Operator::ADD,
                    new RegOperand(PR::x16),
                    new RegOperand(PR::sp),
                    new ImmeOperand(localSize)
                );
                add->comment = "prologue: compute base for cs saves";
                prologueInsts.push_back(add);
            }
            else
            {
                auto* movz = createInstr2(
                    Operator::MOVZ,
                    new RegOperand(PR::x16),
                    new ImmeOperand(localSize & 0xFFFF)
                );
                prologueInsts.push_back(movz);

                if (localSize > 0xFFFF)
                {
                    auto* movk = createInstr3(
                        Operator::MOVK,
                        new RegOperand(PR::x16),
                        new ImmeOperand((localSize >> 16) & 0xFFFF),
                        new ImmeOperand(16)
                    );
                    prologueInsts.push_back(movk);
                }

                auto* add = createInstr3(
                    Operator::ADD,
                    new RegOperand(PR::x16),
                    new RegOperand(PR::sp),
                    new RegOperand(PR::x16)
                );
                add->comment = "prologue: compute base for cs saves";
                prologueInsts.push_back(add);
            }
            baseReg = PR::x16;
            baseOffset = localSize;
        }

        // Save Ints
        for (size_t i = 0; i < sortedCSInt.size(); i += 2)
        {
            int r1 = sortedCSInt[i];
            int r2 = (i + 1 < sortedCSInt.size()) ? sortedCSInt[i + 1] : -1;
            
            if (r2 != -1)
            {
                auto* stp = createInstr4(
                    Operator::STP,
                    new RegOperand(BE::Register(r1, BE::I64, false)),
                    new RegOperand(BE::Register(r2, BE::I64, false)),
                    new RegOperand(baseReg),
                    new ImmeOperand(currentOffset - baseOffset)
                );
                stp->comment = "prologue: save cs int";
                prologueInsts.push_back(stp);
                currentOffset += 16;
            }
            else
            {
                auto* str = createInstr2(
                    Operator::STR,
                    new RegOperand(BE::Register(r1, BE::I64, false)),
                    new MemOperand(baseReg, currentOffset - baseOffset)
                );
                str->comment = "prologue: save cs int";
                prologueInsts.push_back(str);
                currentOffset += 8;
            }
        }
        
        // Save Floats
        for (size_t i = 0; i < sortedCSFloat.size(); i += 2)
        {
            int r1 = sortedCSFloat[i];
            int r2 = (i + 1 < sortedCSFloat.size()) ? sortedCSFloat[i + 1] : -1;
            
            if (r2 != -1)
            {
                auto* stp = createInstr4(
                    Operator::STP,
                    new RegOperand(BE::Register(r1, BE::F64, false)),
                    new RegOperand(BE::Register(r2, BE::F64, false)),
                    new RegOperand(baseReg),
                    new ImmeOperand(currentOffset - baseOffset)
                );
                stp->comment = "prologue: save cs float";
                prologueInsts.push_back(stp);
                currentOffset += 16;
            }
            else
            {
                auto* str = createInstr2(
                    Operator::STR,
                    new RegOperand(BE::Register(r1, BE::F64, false)),
                    new MemOperand(baseReg, currentOffset - baseOffset)
                );
                str->comment = "prologue: save cs float";
                prologueInsts.push_back(str);
                currentOffset += 8;
            }
        }

        // Step 2: stp x29, x30, [sp, #fpLrOffset]
        // FP/LR 保存在栈帧顶部（高地址端）
        int fpLrOffset = localSize + csTotalSize;
        if (fpLrSize > 0)
        {
            auto* stpFpLr = createInstr4(
                Operator::STP,
                new RegOperand(PR::x29),
                new RegOperand(PR::x30),
                new RegOperand(baseReg),
                new ImmeOperand(fpLrOffset - baseOffset)
            );
            stpFpLr->comment = "prologue: save FP and LR";
            prologueInsts.push_back(stpFpLr);

            // Step 3: add x29, sp, #localSize (设置 FP 指向保存的 FP 位置)
            if (fpLrOffset == 0)
            {
                auto* movFp = createInstr2(
                    Operator::MOV,
                    new RegOperand(PR::x29),
                    new RegOperand(PR::sp)
                );
                movFp->comment = "prologue: set frame pointer";
                prologueInsts.push_back(movFp);
            }
            else if (fpLrOffset <= 4095)
            {
                auto* addFp = createInstr3(
                    Operator::ADD,
                    new RegOperand(PR::x29),
                    new RegOperand(PR::sp),
                    new ImmeOperand(fpLrOffset)
                );
                addFp->comment = "prologue: set frame pointer";
                prologueInsts.push_back(addFp);
            }
            else
            {
                // 偏移量太大，需要使用临时寄存器
                auto* movz = createInstr2(
                    Operator::MOVZ,
                    new RegOperand(PR::x16),
                    new ImmeOperand(fpLrOffset & 0xFFFF)
                );
                prologueInsts.push_back(movz);

                if (fpLrOffset > 0xFFFF)
                {
                    auto* movk = createInstr3(
                        Operator::MOVK,
                        new RegOperand(PR::x16),
                        new ImmeOperand((fpLrOffset >> 16) & 0xFFFF),
                        new ImmeOperand(16)
                    );
                    prologueInsts.push_back(movk);
                }

                auto* addFp = createInstr3(
                    Operator::ADD,
                    new RegOperand(PR::x29),
                    new RegOperand(PR::sp),
                    new RegOperand(PR::x16)
                );
                addFp->comment = "prologue: set frame pointer";
                prologueInsts.push_back(addFp);
            }
        }
    }

    // 将 prologue 插入到入口块开头
    for (auto it = prologueInsts.rbegin(); it != prologueInsts.rend(); ++it)
    {
        entryBlock->insts.push_front(*it);
    }

    // ========== 生成 Epilogue ==========
    // 在每个 RET 指令前插入 epilogue
    for (auto& [id, block] : func->blocks)
    {
        for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
        {
            auto* inst = *it;
            if (inst->kind != InstKind::TARGET) continue;

            auto* a64Inst = dynamic_cast<Instr*>(inst);
            if (!a64Inst || a64Inst->op != Operator::RET) continue;

            std::vector<MInstruction*> epilogueInsts;

            if (totalFrameSize > 0)
            {
                int fpLrOffset = localSize + csTotalSize;

                // Handle large stack frames
                bool useLargeOffset = (localSize + csTotalSize > 504);
                BE::Register baseReg = PR::sp;
                int baseOffset = 0;

                if (useLargeOffset)
                {
                    // Compute x16 = sp + localSize
                    if (localSize <= 4095)
                    {
                        auto* add = createInstr3(
                            Operator::ADD,
                            new RegOperand(PR::x16),
                            new RegOperand(PR::sp),
                            new ImmeOperand(localSize)
                        );
                        add->comment = "epilogue: compute base for cs restores";
                        epilogueInsts.push_back(add);
                    }
                    else
                    {
                        auto* movz = createInstr2(
                            Operator::MOVZ,
                            new RegOperand(PR::x16),
                            new ImmeOperand(localSize & 0xFFFF)
                        );
                        epilogueInsts.push_back(movz);

                        if (localSize > 0xFFFF)
                        {
                            auto* movk = createInstr3(
                                Operator::MOVK,
                                new RegOperand(PR::x16),
                                new ImmeOperand((localSize >> 16) & 0xFFFF),
                                new ImmeOperand(16)
                            );
                            epilogueInsts.push_back(movk);
                        }

                        auto* add = createInstr3(
                            Operator::ADD,
                            new RegOperand(PR::x16),
                            new RegOperand(PR::sp),
                            new RegOperand(PR::x16)
                        );
                        add->comment = "epilogue: compute base for cs restores";
                        epilogueInsts.push_back(add);
                    }
                    baseReg = PR::x16;
                    baseOffset = localSize;
                }

                // Step 1: ldp x29, x30, [sp, #fpLrOffset]
                if (fpLrSize > 0)
                {
                    auto* ldpFpLr = createInstr4(
                        Operator::LDP,
                        new RegOperand(PR::x29),
                        new RegOperand(PR::x30),
                        new RegOperand(baseReg),
                        new ImmeOperand(fpLrOffset - baseOffset)
                    );
                    ldpFpLr->comment = "epilogue: restore FP and LR";
                    epilogueInsts.push_back(ldpFpLr);
                }

                // Step 1.5: Restore Callee-Saved Registers
                int currentOffset = localSize;
                
                // Restore Ints
                for (size_t i = 0; i < sortedCSInt.size(); i += 2)
                {
                    int r1 = sortedCSInt[i];
                    int r2 = (i + 1 < sortedCSInt.size()) ? sortedCSInt[i + 1] : -1;
                    
                    if (r2 != -1)
                    {
                        auto* ldp = createInstr4(
                            Operator::LDP,
                            new RegOperand(BE::Register(r1, BE::I64, false)),
                            new RegOperand(BE::Register(r2, BE::I64, false)),
                            new RegOperand(baseReg),
                            new ImmeOperand(currentOffset - baseOffset)
                        );
                        ldp->comment = "epilogue: restore cs int";
                        epilogueInsts.push_back(ldp);
                        currentOffset += 16;
                    }
                    else
                    {
                        auto* ldr = createInstr2(
                            Operator::LDR,
                            new RegOperand(BE::Register(r1, BE::I64, false)),
                            new MemOperand(baseReg, currentOffset - baseOffset)
                        );
                        ldr->comment = "epilogue: restore cs int";
                        epilogueInsts.push_back(ldr);
                        currentOffset += 8;
                    }
                }
                
                // Restore Floats
                for (size_t i = 0; i < sortedCSFloat.size(); i += 2)
                {
                    int r1 = sortedCSFloat[i];
                    int r2 = (i + 1 < sortedCSFloat.size()) ? sortedCSFloat[i + 1] : -1;
                    
                    if (r2 != -1)
                    {
                        auto* ldp = createInstr4(
                            Operator::LDP,
                            new RegOperand(BE::Register(r1, BE::F64, false)),
                            new RegOperand(BE::Register(r2, BE::F64, false)),
                            new RegOperand(baseReg),
                            new ImmeOperand(currentOffset - baseOffset)
                        );
                        ldp->comment = "epilogue: restore cs float";
                        epilogueInsts.push_back(ldp);
                        currentOffset += 16;
                    }
                    else
                    {
                        auto* ldr = createInstr2(
                            Operator::LDR,
                            new RegOperand(BE::Register(r1, BE::F64, false)),
                            new MemOperand(baseReg, currentOffset - baseOffset)
                        );
                        ldr->comment = "epilogue: restore cs float";
                        epilogueInsts.push_back(ldr);
                        currentOffset += 8;
                    }
                }

                // Step 2: add sp, sp, #totalFrameSize
                if (totalFrameSize <= 4095)
                {
                    auto* addSp = createInstr3(
                        Operator::ADD,
                        new RegOperand(PR::sp),
                        new RegOperand(PR::sp),
                        new ImmeOperand(totalFrameSize)
                    );
                    addSp->comment = "epilogue: deallocate stack frame";
                    epilogueInsts.push_back(addSp);
                }
                else
                {
                    auto* movz = createInstr2(
                        Operator::MOVZ,
                        new RegOperand(PR::x16),
                        new ImmeOperand(totalFrameSize & 0xFFFF)
                    );
                    movz->comment = "epilogue: load frame size";
                    epilogueInsts.push_back(movz);

                    if (totalFrameSize > 0xFFFF)
                    {
                        auto* movk = createInstr3(
                            Operator::MOVK,
                            new RegOperand(PR::x16),
                            new ImmeOperand((totalFrameSize >> 16) & 0xFFFF),
                            new ImmeOperand(16)
                        );
                        epilogueInsts.push_back(movk);
                    }

                    auto* addSp = createInstr3(
                        Operator::ADD,
                        new RegOperand(PR::sp),
                        new RegOperand(PR::sp),
                        new RegOperand(PR::x16)
                    );
                    addSp->comment = "epilogue: deallocate stack frame";
                    epilogueInsts.push_back(addSp);
                }
            }

            // 在 RET 前插入 epilogue 指令
            for (auto* eInst : epilogueInsts)
            {
                it = block->insts.insert(it, eInst);
                ++it;
            }
        }
    }

    // ========== 处理 FILoadInst 和 FIStoreInst ==========
    // 将抽象的帧索引操作转换为具体的 LDR/STR 指令
    for (auto& [id, block] : func->blocks)
    {
        for (auto it = block->insts.begin(); it != block->insts.end(); )
        {
            auto* inst = *it;

            if (auto* fiLoad = dynamic_cast<FILoadInst*>(inst))
            {
                int offset = func->frameInfo.getSpillSlotOffset(fiLoad->frameIndex);
                if (offset < 0)
                {
                    ++it;
                    continue;
                }

                // ldr dest, [sp, #offset]
                auto* ldr = createInstr2(
                    Operator::LDR,
                    new RegOperand(fiLoad->dest),
                    new MemOperand(PR::sp, offset)
                );
                ldr->comment = "spill reload from slot " + std::to_string(fiLoad->frameIndex);

                it = block->insts.erase(it);
                it = block->insts.insert(it, ldr);
                ++it;
            }
            else if (auto* fiStore = dynamic_cast<FIStoreInst*>(inst))
            {
                int offset = func->frameInfo.getSpillSlotOffset(fiStore->frameIndex);
                if (offset < 0)
                {
                    ++it;
                    continue;
                }

                // str src, [sp, #offset]
                auto* str = createInstr2(
                    Operator::STR,
                    new RegOperand(fiStore->src),
                    new MemOperand(PR::sp, offset)
                );
                str->comment = "spill store to slot " + std::to_string(fiStore->frameIndex);

                it = block->insts.erase(it);
                it = block->insts.insert(it, str);
                ++it;
            }
            else
            {
                ++it;
            }
        }
    }
}

} // namespace BE::AArch64::Passes::Lowering