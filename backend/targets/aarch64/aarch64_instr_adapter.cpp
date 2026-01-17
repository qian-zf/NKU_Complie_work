#include <backend/targets/aarch64/aarch64_instr_adapter.h>
#include <backend/targets/aarch64/aarch64_defs.h>
#include <backend/mir/m_block.h>
#include <debug.h>

namespace BE::Targeting::AArch64
{
    using namespace BE::AArch64;

    /*
     * AArch64 InstrAdapter 说明（中文注释）：
     * 该适配器负责将目标架构的 MIR 指令语义暴露给通用后端模块，
     * 包括：CFG 构建、活跃分析（RA）、替换寄存器、插入 spill/reload 等。
     *
     * 主要职责：
     *  - 判定指令类别（isCall/isReturn/isCondBranch/isUncondBranch）
     *  - 枚举指令的 USE/DEF（enumUses/enumDefs），供线性扫描使用
     *  - 在寄存器分配后替换指令中的寄存器（replaceUse/replaceDef）
     *  - 在需要时插入伪指令（insertReloadBefore/insertSpillAfter），后续 Lowering 将其具体化
     *
     * 注意事项：
     *  - 本模块以保守方式列出调用（BL）可能定义/修改的寄存器集合；
     *    具体调用约定在 TargetRegInfo 中定义，RA 可以据此进一步精化。
     *  - 插入的 FILoad/FIStore 为伪指令，最终由 stack_lowering 转换为 LDR/STR。
     */

    // 判断是否为函数调用指令 (BL)
    bool InstrAdapter::isCall(BE::MInstruction* inst) const
    {
        auto* i = static_cast<Instr*>(inst);
        return i->op == Operator::BL;
    }

    // 判断是否为返回指令 (RET)
    bool InstrAdapter::isReturn(BE::MInstruction* inst) const
    {
        auto* i = static_cast<Instr*>(inst);
        return i->op == Operator::RET;
    }

    // 判断是否为无条件跳转 (B)
    bool InstrAdapter::isUncondBranch(BE::MInstruction* inst) const
    {
        auto* i = static_cast<Instr*>(inst);
        return i->op == Operator::B;
    }

    // 判断是否为条件跳转 (BEQ, BNE, BLT, BLE, BGT, BGE)
    bool InstrAdapter::isCondBranch(BE::MInstruction* inst) const
    {
        auto* i = static_cast<Instr*>(inst);
        switch (i->op)
        {
            case Operator::BEQ:
            case Operator::BNE:
            case Operator::BLT:
            case Operator::BLE:
            case Operator::BGT:
            case Operator::BGE:
                return true;
            default:
                return false;
        }
    }

    // 获取分支指令的目标块 ID
    // 用于构建控制流图 (CFG)
    int InstrAdapter::extractBranchTarget(BE::MInstruction* inst) const
    {
        auto* i = static_cast<Instr*>(inst);
        if (i->operands.empty()) return -1;
        
        // 分支指令的第一个操作数通常是 LabelOperand
        if (auto* lab = dynamic_cast<LabelOperand*>(i->operands[0]))
        {
            return lab->targetBlockId;
        }
        return -1;
    }

    // 枚举指令使用的寄存器 (Uses)
    // 根据指令类型 (OpType) 解析操作数列表，找出作为输入的寄存器
    void InstrAdapter::enumUses(BE::MInstruction* inst, std::vector<BE::Register>& out) const
    {
        if (inst->kind == BE::InstKind::MOVE)
        {
            auto* mov = static_cast<BE::MoveInst*>(inst);
            if (auto* regOp = dynamic_cast<RegOperand*>(mov->src)) out.push_back(regOp->reg);
            return;
        }
        if (inst->kind == BE::InstKind::LSLOT)
        {
            // FILoadInst: dest = load(slot). 使用的是 slot 索引，没有使用寄存器。
            return;
        }
        if (inst->kind == BE::InstKind::SSLOT)
        {
            // FIStoreInst: store(src, slot). 使用了 src 寄存器。
            auto* store = static_cast<BE::FIStoreInst*>(inst);
            out.push_back(store->src);
            return;
        }
        if (inst->kind == BE::InstKind::PHI)
        {
             auto* phi = static_cast<BE::PhiInst*>(inst);
             for (auto& [label, op] : phi->incomingVals) {
                 if (auto* regOp = dynamic_cast<RegOperand*>(op)) out.push_back(regOp->reg);
             }
             return;
        }
        if (inst->kind != BE::InstKind::TARGET) return;

        auto* i = static_cast<Instr*>(inst);
        switch (getOpInfoType(i->op))
        {
            case OpType::SYM:
            {
                // BL with symbol: 隐式参数寄存器被添加到操作数列表中 (从 index 1 开始)
                // operands[0] 是符号; operands[1..] 是 ISel 阶段插入的参数寄存器
                for (size_t k = 1; k < i->operands.size(); ++k)
                {
                    if (auto* ro = dynamic_cast<RegOperand*>(i->operands[k]))
                        out.push_back(ro->reg);
                }
                break;
            }
            case OpType::R:
                // R 类型: add rd, rs1, rs2 -> use rs1, rs2
                if (i->operands.size() >= 2)
                {
                    if (auto* rs1 = dynamic_cast<RegOperand*>(i->operands[1])) out.push_back(rs1->reg);
                    if (i->operands.size() >= 3)
                    {
                        if (auto* rs2 = dynamic_cast<RegOperand*>(i->operands[2])) out.push_back(rs2->reg);
                    }
                }
                break;
            case OpType::R2:
                // R2 类型:
                // mov rd, rs -> use rs
                // cmp rn, rm -> use rn, rm
                if (i->op == Operator::CMP || i->op == Operator::FCMP)
                {
                    // 比较指令: cmp rn, rm (或 imm)
                    if (i->operands.size() >= 1)
                        if (auto* rn = dynamic_cast<RegOperand*>(i->operands[0])) out.push_back(rn->reg);
                    if (i->operands.size() >= 2)
                        if (auto* rm = dynamic_cast<RegOperand*>(i->operands[1])) out.push_back(rm->reg);
                }
                else
                {
                    // 数据移动: mov rd, rs
                    if (i->operands.size() >= 2)
                        if (auto* rs = dynamic_cast<RegOperand*>(i->operands[1])) out.push_back(rs->reg);
                }
                break;
            case OpType::M:
                // M 类型 (内存访问):
                // ldr rt, [base, off] -> use base
                // str rt, [base, off] -> use rt, base
                if (i->op == Operator::LDR)
                {
                    if (i->operands.size() >= 2)
                    {
                        if (auto* mem = dynamic_cast<MemOperand*>(i->operands[1])) out.push_back(mem->base);
                    }
                }
                else if (i->op == Operator::STR)
                {
                    if (i->operands.size() >= 1)
                    {
                        if (auto* rt = dynamic_cast<RegOperand*>(i->operands[0])) out.push_back(rt->reg);
                    }
                    if (i->operands.size() >= 2)
                    {
                        if (auto* mem = dynamic_cast<MemOperand*>(i->operands[1])) out.push_back(mem->base);
                    }
                }
                break;
            case OpType::P:
                // P 类型 (Pair load/store):
                // ldp rt1, rt2, [base, off] -> use base
                // stp rt1, rt2, [base, off] -> use rt1, rt2, base
                if (i->op == Operator::LDP)
                {
                    if (i->operands.size() >= 3)
                    {
                        if (auto* mem = dynamic_cast<MemOperand*>(i->operands[2])) out.push_back(mem->base);
                    }
                }
                else if (i->op == Operator::STP)
                {
                    if (i->operands.size() >= 1)
                        if (auto* rt1 = dynamic_cast<RegOperand*>(i->operands[0])) out.push_back(rt1->reg);
                    if (i->operands.size() >= 2)
                        if (auto* rt2 = dynamic_cast<RegOperand*>(i->operands[1])) out.push_back(rt2->reg);
                    if (i->operands.size() >= 3)
                        if (auto* mem = dynamic_cast<MemOperand*>(i->operands[2])) out.push_back(mem->base);
                }
                break;
            case OpType::Z:
                if (i->op == Operator::RET)
                {
                    // RET 隐式使用 x30 (LR)，通常由 ABI 处理
                }
                break;
            default:
                break;
        }
    }

    // 枚举指令定义的寄存器 (Defs)
    // 根据指令类型 (OpType) 解析操作数列表，找出作为输出的寄存器
    void InstrAdapter::enumDefs(BE::MInstruction* inst, std::vector<BE::Register>& out) const
    {
        if (inst->kind == BE::InstKind::MOVE)
        {
            auto* mov = static_cast<BE::MoveInst*>(inst);
            if (auto* regOp = dynamic_cast<RegOperand*>(mov->dest)) out.push_back(regOp->reg);
            return;
        }
        if (inst->kind == BE::InstKind::LSLOT)
        {
            // FILoadInst: dest = load(slot). Defs dest.
            auto* load = static_cast<BE::FILoadInst*>(inst);
            out.push_back(load->dest);
            return;
        }
        if (inst->kind == BE::InstKind::SSLOT)
        {
            // FIStoreInst: store(src, slot). No defs.
            return;
        }
        if (inst->kind == BE::InstKind::PHI)
        {
            auto* phi = static_cast<BE::PhiInst*>(inst);
            out.push_back(phi->resReg);
            return;
        }
        if (inst->kind != BE::InstKind::TARGET) return;

        auto* i = static_cast<Instr*>(inst);
        switch (getOpInfoType(i->op))
        {
            case OpType::R:
                // R ????: add rd, ... -> ???? rd
                if (!i->operands.empty())
                {
                    if (auto* rd = dynamic_cast<RegOperand*>(i->operands[0])) out.push_back(rd->reg);
                }
                break;
            case OpType::R2:
                // R2 ????:
                // mov rd, ... -> ???? rd
                // cmp ... -> ?????????? (????????????????????????)
                if (i->op != Operator::CMP && i->op != Operator::FCMP)
                {
                    if (!i->operands.empty())
                    {
                        if (auto* rd = dynamic_cast<RegOperand*>(i->operands[0])) out.push_back(rd->reg);
                    }
                }
                break;
            case OpType::M:
                // M ????:
                // ldr rt, ... -> ???? rt
                // str ... -> ??????????
                if (i->op == Operator::LDR)
                {
                    if (!i->operands.empty())
                    {
                        if (auto* rt = dynamic_cast<RegOperand*>(i->operands[0])) out.push_back(rt->reg);
                    }
                }
                break;
            case OpType::P:
                // P ????:
                // ldp rt1, rt2, ... -> ???? rt1, rt2
                if (i->op == Operator::LDP)
                {
                    if (i->operands.size() >= 1)
                        if (auto* rt1 = dynamic_cast<RegOperand*>(i->operands[0])) out.push_back(rt1->reg);
                    if (i->operands.size() >= 2)
                        if (auto* rt2 = dynamic_cast<RegOperand*>(i->operands[1])) out.push_back(rt2->reg);
                }
                break;
            case OpType::SYM:
                // SYM ???? (BL symbol):
                // ???????????????? (x0-x18, v0-v7, v16-v31) ?? LR (x30)
                out.push_back(BE::Register(30, nullptr, false)); // LR
                // x0-x18
                for (int k = 0; k <= 18; ++k) out.push_back(BE::Register(k, BE::I64, false));
                // v0-v7
                for (int k = 0; k <= 7; ++k) out.push_back(BE::Register(k, BE::F64, false)); // Use F64/I64 generic
                // v16-v31
                for (int k = 16; k <= 31; ++k) out.push_back(BE::Register(k, BE::F64, false));
                break;
            default:
                break;
        }
    }

    // ????????????MOV???????? true ??????????
    bool InstrAdapter::isCopy(BE::MInstruction* inst, BE::Register& dst, BE::Register& src) const
    {
        auto* i = static_cast<Instr*>(inst);
        if (i->op == Operator::MOV || i->op == Operator::FMOV)
        {
            if (i->operands.size() >= 2)
            {
                auto* rd = dynamic_cast<RegOperand*>(i->operands[0]);
                auto* rs = dynamic_cast<RegOperand*>(i->operands[1]);
                if (rd && rs)
                {
                    dst = rd->reg;
                    src = rs->reg;
                    return true;
                }
            }
        }
        return false;
    }

    // ????????????I??????????????????
    static void replaceOne(Operand* op, const BE::Register& from, const BE::Register& to)
    {
        if (auto* regOp = dynamic_cast<RegOperand*>(op))
        {
            if (regOp->reg == from) regOp->reg = to;
        }
        else if (auto* memOp = dynamic_cast<MemOperand*>(op))
        {
            if (memOp->base == from) memOp->base = to;
        }
    }

    // ??I???????? (Use) ??????
    void InstrAdapter::replaceUse(BE::MInstruction* inst, const BE::Register& from, const BE::Register& to) const
    {
        if (inst->kind == BE::InstKind::MOVE)
        {
            auto* mov = static_cast<BE::MoveInst*>(inst);
            replaceOne(mov->src, from, to);
            return;
        }
        if (inst->kind == BE::InstKind::LSLOT)
        {
            return;
        }
        if (inst->kind == BE::InstKind::SSLOT)
        {
            auto* store = static_cast<BE::FIStoreInst*>(inst);
            if (store->src == from) store->src = to;
            return;
        }
        if (inst->kind == BE::InstKind::PHI)
        {
             auto* phi = static_cast<BE::PhiInst*>(inst);
             for (auto& [label, op] : phi->incomingVals) {
                 replaceOne(op, from, to);
             }
             return;
        }
        if (inst->kind != BE::InstKind::TARGET) return;

        auto* i = static_cast<Instr*>(inst);
        switch (getOpInfoType(i->op))
        {
            case OpType::R:
                // add rd, rs1, rs2 -> ??I???? 1, 2 ????????
                if (i->operands.size() >= 2) replaceOne(i->operands[1], from, to);
                if (i->operands.size() >= 3) replaceOne(i->operands[2], from, to);
                break;
            case OpType::R2:
                // mov rd, rs -> ??I???? 1
                // cmp rn, rm -> ??I???? 0, 1
                if (i->op == Operator::CMP || i->op == Operator::FCMP)
                {
                    if (i->operands.size() >= 1) replaceOne(i->operands[0], from, to);
                    if (i->operands.size() >= 2) replaceOne(i->operands[1], from, to);
                }
                else
                {
                    if (i->operands.size() >= 2) replaceOne(i->operands[1], from, to);
                }
                break;
            case OpType::M:
                // ldr rt, [base] -> ??I???? 1 (base)
                // str rt, [base] -> ??I???? 0 (rt), 1 (base)
                if (i->op == Operator::LDR)
                {
                    if (i->operands.size() >= 2) replaceOne(i->operands[1], from, to);
                }
                else if (i->op == Operator::STR)
                {
                    if (i->operands.size() >= 1) replaceOne(i->operands[0], from, to);
                    if (i->operands.size() >= 2) replaceOne(i->operands[1], from, to);
                }
                break;
            case OpType::P:
                // ldp rt1, rt2, [base] -> ??I???? 2 (base)
                // stp rt1, rt2, [base] -> ??I???? 0, 1, 2
                if (i->op == Operator::LDP)
                {
                    if (i->operands.size() >= 3) replaceOne(i->operands[2], from, to);
                }
                else if (i->op == Operator::STP)
                {
                    if (i->operands.size() >= 1) replaceOne(i->operands[0], from, to);
                    if (i->operands.size() >= 2) replaceOne(i->operands[1], from, to);
                    if (i->operands.size() >= 3) replaceOne(i->operands[2], from, to);
                }
                break;
            case OpType::SYM:
                // SYM (BL) arguments
                for (size_t k = 1; k < i->operands.size(); ++k) replaceOne(i->operands[k], from, to);
                break;
            default:
                break;
        }
    }

    // ??I????????? (Def) ??????
    void InstrAdapter::replaceDef(BE::MInstruction* inst, const BE::Register& from, const BE::Register& to) const
    {
        if (inst->kind == BE::InstKind::MOVE)
        {
            auto* mov = static_cast<BE::MoveInst*>(inst);
            replaceOne(mov->dest, from, to);
            return;
        }
        if (inst->kind == BE::InstKind::LSLOT)
        {
            auto* load = static_cast<BE::FILoadInst*>(inst);
            if (load->dest == from) load->dest = to;
            return;
        }
        if (inst->kind == BE::InstKind::SSLOT)
        {
            return;
        }
        if (inst->kind == BE::InstKind::PHI)
        {
            auto* phi = static_cast<BE::PhiInst*>(inst);
            if (phi->resReg == from) phi->resReg = to;
            return;
        }
        if (inst->kind != BE::InstKind::TARGET) return;

        auto* i = static_cast<Instr*>(inst);
        switch (getOpInfoType(i->op))
        {
            case OpType::R:
                // add rd, ... -> ??I???? 0 (rd)
                if (!i->operands.empty()) replaceOne(i->operands[0], from, to);
                break;
            case OpType::R2:
                // mov rd, ... -> ??I???? 0 (rd)
                // cmp ... -> ?????
                if (i->op != Operator::CMP && i->op != Operator::FCMP)
                {
                    if (!i->operands.empty()) replaceOne(i->operands[0], from, to);
                }
                break;
            case OpType::M:
                // ldr rt, ... -> ??I???? 0 (rt)
                if (i->op == Operator::LDR)
                {
                    if (!i->operands.empty()) replaceOne(i->operands[0], from, to);
                }
                break;
            case OpType::P:
                // ldp rt1, rt2, ... -> ??I???? 0, 1 (rt1, rt2)
                if (i->op == Operator::LDP)
                {
                    if (i->operands.size() >= 1) replaceOne(i->operands[0], from, to);
                    if (i->operands.size() >= 2) replaceOne(i->operands[1], from, to);
                }
                break;
            default:
                break;
        }
    }


    // ??????????p?????????????????????????????
    void InstrAdapter::enumPhysRegs(BE::MInstruction* inst, std::vector<BE::Register>& out) const
    {
        std::vector<BE::Register> uses, defs;
        enumUses(inst, uses);
        enumDefs(inst, defs);
        for (auto& r : uses)
            if (!r.isVreg) out.push_back(r);
        for (auto& r : defs)
            if (!r.isVreg) out.push_back(r);
    }

    // ??????????????????????? (Reload) ???
    // ??????????????????
    void InstrAdapter::insertReloadBefore(
        BE::Block* block, std::deque<BE::MInstruction*>::iterator it, const BE::Register& physReg, int frameIndex) const
    {
        // ????????? FILoadInst physReg, frameIndex
        auto* reload = new BE::FILoadInst(physReg, frameIndex);
        block->insts.insert(it, reload);
    }

    // ?????????????????????? (Spill) ???
    // ????????????????????
    void InstrAdapter::insertSpillAfter(
        BE::Block* block, std::deque<BE::MInstruction*>::iterator it, const BE::Register& physReg, int frameIndex) const
    {
        // ????????? FIStoreInst physReg, frameIndex
        auto* spill = new BE::FIStoreInst(physReg, frameIndex);
        
        // ?????????????
        if (it != block->insts.end()) {
            block->insts.insert(std::next(it), spill);
        } else {
            block->insts.push_back(spill);
        }
    }
}  // namespace BE::Targeting::AArch64
