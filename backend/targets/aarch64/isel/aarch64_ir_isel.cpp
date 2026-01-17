#include <backend/targets/aarch64/isel/aarch64_ir_isel.h>
#include <backend/targets/aarch64/aarch64_defs.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/module/ir_function.h>
#include <debug.h>
#include <cstring>
#include <transfer.h>

namespace BE::AArch64
{
namespace
{
// 将中间代码的比较操作符转换为 AArch64 的条件码
// AArch64 使用 NZCV 标志位来表示比较结果
int getAArch64CC(ME::FCmpOp op)
{
    switch (op)
    {
    case ME::FCmpOp::OEQ: return 0; // EQ (Equal)
    case ME::FCmpOp::ONE: return 1; // NE (Not Equal)
    case ME::FCmpOp::OGT: return 12; // GT (Greater Than)
    case ME::FCmpOp::OGE: return 10; // GE (Greater or Equal)
    case ME::FCmpOp::OLT: return 11; // LT (Less Than)
    case ME::FCmpOp::OLE: return 13; // LE (Less or Equal)
    case ME::FCmpOp::ORD: return 7; // VC (No overflow / Ordered)
    case ME::FCmpOp::UEQ: return 0; // EQ
    case ME::FCmpOp::UGT: return 8; // HI (Unsigned Higher)
    case ME::FCmpOp::UGE: return 2; // CS (Carry Set / Unsigned Higher or Same)
    case ME::FCmpOp::ULT: return 11; // LT
    case ME::FCmpOp::ULE: return 13; // LE
    case ME::FCmpOp::UNE: return 1; // NE
    case ME::FCmpOp::UNO: return 6; // VS (Overflow / Unordered)
    default: return 0;
    }
}
} // namespace

//入口
void IRIsel::runImpl()
{
    // 1. 导入全局变量
    importGlobals();
    // 2. 对每个函数进行指令选择
    apply(*this, *ir_module_);
}

// 获取或创建与 IR 寄存器对应的后端虚拟寄存器
Register IRIsel::getOrCreateVReg(size_t ir_reg_id, BE::DataType* dt)
{
    // 如果已经存在映射关系，直接返回
    if (ctx_.vregMap.find(ir_reg_id) != ctx_.vregMap.end())
        return ctx_.vregMap[ir_reg_id];

    // 否则创建新的虚拟寄存器并建立映射
    Register vreg = BE::getVReg(dt);
    ctx_.vregMap[ir_reg_id] = vreg;
    return vreg;
}

// 根据 IR 操作数获取对应的后端寄存器
// 如果操作数是立即数，会生成加载立即数的指令序列
Register IRIsel::getReg(ME::Operand* op)
{
    // 1. 处理寄存器操作数
    if (auto* regOp = dynamic_cast<ME::RegOperand*>(op))
    {
        size_t regId = regOp->getRegNum();
        
        // 尝试查找已存在的虚拟寄存器映射
        if (ctx_.vregMap.count(regId)) return ctx_.vregMap[regId];
        
        // 如果未找到，可能是前向引用（如 Phi 节点的回边）或函数参数
        // 创建一个新的 I32 类型的虚拟寄存器作为默认回退
        if (ctx_.vregMap.find(regId) == ctx_.vregMap.end()) {
             return getOrCreateVReg(regId, BE::I32);
        }
        return ctx_.vregMap[regId];
    }
    // 2. 处理 32 位整数立即数
    else if (auto* immOp = dynamic_cast<ME::ImmeI32Operand*>(op))
    {
        Register reg = BE::getVReg(BE::I32);
        int val = immOp->value;
        
        // 优化：0 值直接使用零寄存器 wzr
         if (val == 0) {
             Register r = PR::wzr;
             r.dt = BE::I32;
             return r;
         }
         
         // 如果立即数可以用 16 位表示 (MOVZ 指令范围)
         // MOVZ: Move wide with zero
         if ((val & 0xFFFF0000) == 0) {
             cur_block_->insts.push_back(createInstr2(Operator::MOVZ, new RegOperand(reg), new ImmeOperand(val)));
         } else {
             // 否则分两步加载: 
             // 1. MOVZ 加载低 16 位
             // 2. MOVK (Move wide with keep) 加载高 16 位
             cur_block_->insts.push_back(createInstr2(Operator::MOVZ, new RegOperand(reg), new ImmeOperand(val & 0xFFFF)));
             cur_block_->insts.push_back(createInstr3(Operator::MOVK, new RegOperand(reg), new ImmeOperand((val >> 16) & 0xFFFF), new ImmeOperand(16)));
         }
        return reg;
    }
    // 3. 处理 32 位浮点立即数
    else if (auto* immF32 = dynamic_cast<ME::ImmeF32Operand*>(op))
    {
        Register reg = BE::getVReg(BE::F32);
        float val = immF32->value;
        
        // 优化：0.0f 直接使用整数零寄存器 wzr 移动到浮点寄存器
        if (val == 0.0f) {
            Register zr = PR::wzr;
            zr.dt = BE::I32;
            cur_block_->insts.push_back(createInstr2(Operator::FMOV, new RegOperand(reg), new RegOperand(zr)));
            return reg;
        }
        // 将浮点数位模式转为整数加载，然后转移到浮点寄存器
        // 这样可以复用整数加载大立即数的逻辑
        int bits = FLOAT_TO_INT_BITS(val);
        Register tmp = BE::getVReg(BE::I32);
        
         if ((bits & 0xFFFF0000) == 0) {
             cur_block_->insts.push_back(createInstr2(Operator::MOVZ, new RegOperand(tmp), new ImmeOperand(bits)));
         } else {
             cur_block_->insts.push_back(createInstr2(Operator::MOVZ, new RegOperand(tmp), new ImmeOperand(bits & 0xFFFF)));
             cur_block_->insts.push_back(createInstr3(Operator::MOVK, new RegOperand(tmp), new ImmeOperand((bits >> 16) & 0xFFFF), new ImmeOperand(16)));
         }
        // FMOV: Floating-point Move (General to SIMD&FP)
        cur_block_->insts.push_back(createInstr2(Operator::FMOV, new RegOperand(reg), new RegOperand(tmp)));
        return reg;
    }
    return Register();
}

// 导入 IR 中的全局变量到后端模块
void IRIsel::importGlobals()
{
    for (auto* g : ir_module_->globalVars)
    {
        BE::DataType* dt = BE::I32;
        if (g->dt == ME::DataType::F32) dt = BE::F32;
        else if (g->dt == ME::DataType::I64 || g->dt == ME::DataType::PTR) dt = BE::I64;

        auto* be_g = new BE::GlobalVariable(dt, g->name);
        be_g->dims = g->initList.arrayDims;

        if (g->init)
        {
            if (g->dt == ME::DataType::F32)
            {
                float f = static_cast<ME::ImmeF32Operand*>(g->init)->value;
                be_g->initVals.push_back(FLOAT_TO_INT_BITS(f));
            }
            else
            {
                int v = static_cast<ME::ImmeI32Operand*>(g->init)->value;
                be_g->initVals.push_back(v);
            }
        }
        m_backend_module->globals.push_back(be_g);
    }
}

void IRIsel::collectAllocas(ME::Function* ir_func)
{
    for (auto& pair : ir_func->blocks)
    {
        auto* block = pair.second;
        for (auto* inst : block->insts)
        {
            if (inst->opcode == ME::Operator::ALLOCA)
            {
                auto* allocaInst = static_cast<ME::AllocaInst*>(inst);
                
                int elemSize = 4;
                if (allocaInst->dt == ME::DataType::I64 || allocaInst->dt == ME::DataType::DOUBLE || 
                    allocaInst->dt == ME::DataType::PTR)
                    elemSize = 8;
                
                int totalSize = elemSize;
                for (int d : allocaInst->dims) totalSize *= d;

                int fi = ctx_.mfunc->frameInfo.createSpillSlot(totalSize, 8);
                size_t regId = allocaInst->res->getRegNum();
                ctx_.allocaFI[regId] = fi;
            }
        }
    }
}

//参数传递，把函数参数从物理寄存器搬到虚寄存器
void IRIsel::setupParameters(ME::Function* ir_func)
{
    size_t entryLabel = ir_func->blocks.begin()->first;
    BE::Block* entryBlock = ctx_.mfunc->blocks[entryLabel];

    int gprIdx = 0; // 通用寄存器参数索引 (x0-x7)
    int fprIdx = 0; // 浮点寄存器参数索引 (s0-s7/d0-d7)

    for (auto& arg : ir_func->funcDef->argRegs)
    {
        BE::DataType* dt = BE::I32;
        if (arg.first == ME::DataType::F32) dt = BE::F32;
        else if (arg.first == ME::DataType::I64 || arg.first == ME::DataType::PTR) dt = BE::I64;
        else if (arg.first == ME::DataType::DOUBLE) dt = BE::F64;

        size_t regId = arg.second->getRegNum();
        Register vreg = getOrCreateVReg(regId, dt);
        ctx_.mfunc->params.push_back(vreg);

        // 处理浮点参数
        if (dt == BE::F32 || dt == BE::F64)
        {
            // 前 8 个浮点参数通过寄存器传递
            if (fprIdx < 8)
            {
                Register pReg(fprIdx, dt, false); // 物理寄存器
                // 插入 Move 指令：将物理参数寄存器复制到虚拟寄存器
                entryBlock->insts.push_back(BE::AArch64::createMove(new RegOperand(vreg), new RegOperand(pReg)));
                fprIdx++;
            }
            // 超过 8 个的参数通过栈传递
            else
            {
                int offset = ctx_.mfunc->paramSize;
                ctx_.mfunc->paramSize += 8; // 栈参数按 8 字节对齐
                ctx_.mfunc->hasStackParam = true;
                
                // 计算栈参数地址: FP (x29) + 16 (保存的 FP/LR) + offset
                // 注意：这假设了标准栈帧布局，其中 FP 指向保存的 FP/LR 对的底部
                entryBlock->insts.push_back(createInstr2(Operator::LDR, new RegOperand(vreg), new MemOperand(PR::x29, 16 + offset)));
            }
        }
        // 处理整数参数
        else
        {
            // 前 8 个整数参数通过寄存器传递
            if (gprIdx < 8)
            {
                Register pReg(gprIdx, dt, false);
                entryBlock->insts.push_back(BE::AArch64::createMove(new RegOperand(vreg), new RegOperand(pReg)));
                gprIdx++;
            }
            // 超过 8 个的参数通过栈传递
            else
            {
                int offset = ctx_.mfunc->paramSize;
                ctx_.mfunc->paramSize += 8;
                ctx_.mfunc->hasStackParam = true;
                // 从栈帧中加载参数
                entryBlock->insts.push_back(createInstr2(Operator::LDR, new RegOperand(vreg), new MemOperand(PR::x29, 16 + offset)));
            }
        }
    }
}

void IRIsel::visit(ME::Module& module)
{
    for (auto* func : module.functions)
    {
        visit(*func);
    }
}

void IRIsel::visit(ME::Function& func)
{
    ctx_ = FunctionContext();
    ctx_.mfunc = new BE::Function(func.funcDef->funcName);
    m_backend_module->functions.push_back(ctx_.mfunc);

    // Create all blocks first
    for (auto& pair : func.blocks)
    {
        auto* ir_block = pair.second;
        auto* be_block = new BE::Block(ir_block->blockId);
        ctx_.mfunc->blocks[ir_block->blockId] = be_block;
    }

    setupParameters(&func);
    collectAllocas(&func);

    for (auto& pair : func.blocks)
    {
        visit(*pair.second);
    }
}

void IRIsel::visit(ME::Block& block)
{
    cur_block_ = ctx_.mfunc->blocks[block.blockId];
    for (auto* inst : block.insts)
    {
        // Use apply() helper from ivisitor.h to handle dispatching
        apply(*this, *inst);
    }
}

void IRIsel::visit(ME::LoadInst& inst)
{
    // 确定结果寄存器类型
    Register res = getOrCreateVReg(inst.res->getRegNum(), BE::I32);
    if (inst.dt == ME::DataType::F32) res.dt = BE::F32;
    else if (inst.dt == ME::DataType::I64 || inst.dt == ME::DataType::PTR) res.dt = BE::I64;

    ME::Operand* ptr = inst.ptr;
    
    // 1. 优化：检查指针是否来自 Alloca 指令 (FrameIndex)
    // 如果是，说明是访问栈上的局部变量。
    // 我们不能直接加载地址，而是要计算 SP + offset。
    // 这里生成一个带有 FrameIndexOperand 的 ADD 指令，
    // 后续 StackLowering 会将其替换为实际的 SP 偏移计算。
    if (auto* regOp = dynamic_cast<ME::RegOperand*>(ptr)) {
        size_t ptrId = regOp->getRegNum();
        if (ctx_.allocaFI.count(ptrId)) {
            // 是栈槽索引
            int fi = ctx_.allocaFI[ptrId];
            Register base = BE::getVReg(BE::I64);
            
            // 生成 add base, sp, #offset (通过 FrameIndexOperand 标记)
            Instr* addrInst = createInstr2(Operator::ADD, new RegOperand(base), new RegOperand(PR::sp));
            addrInst->fiop = new FrameIndexOperand(fi);
            addrInst->operands.push_back(addrInst->fiop);
            addrInst->use_fiops = true;
            cur_block_->insts.push_back(addrInst);
            
            // 加载值: ldr res, [base]
            cur_block_->insts.push_back(createInstr2(Operator::LDR, new RegOperand(res), new MemOperand(base, 0)));
            return;
        }
    }
    
    // 2. 处理全局变量 (Global Variable)
    // 全局变量的地址在链接时确定，这里使用 LA (Load Address) 伪指令加载地址
    if (auto* symOp = dynamic_cast<ME::GlobalOperand*>(ptr)) {
         Register addr = BE::getVReg(BE::I64);
         cur_block_->insts.push_back(createInstr2(Operator::LA, new RegOperand(addr), new SymbolOperand(symOp->name)));
         cur_block_->insts.push_back(createInstr2(Operator::LDR, new RegOperand(res), new MemOperand(addr, 0)));
         return;
    }

    // 3. 一般情况：指针在寄存器中
    // 直接使用寄存器间接寻址: ldr res, [ptrReg]
    Register ptrReg = getReg(ptr);
    cur_block_->insts.push_back(createInstr2(Operator::LDR, new RegOperand(res), new MemOperand(ptrReg, 0)));
}

void IRIsel::visit(ME::StoreInst& inst)
{
    ME::Operand* val = inst.val;
    ME::Operand* ptr = inst.ptr;
    Register valReg = getReg(val);

    // 1. 优化：检查指针是否来自 Alloca (FrameIndex)
    // 类似于 Load，计算栈地址并存储
    if (auto* regOp = dynamic_cast<ME::RegOperand*>(ptr)) {
        size_t ptrId = regOp->getRegNum();
        if (ctx_.allocaFI.count(ptrId)) {
            int fi = ctx_.allocaFI[ptrId];
            Register base = BE::getVReg(BE::I64);
            
            // 生成 add base, sp, #offset
            Instr* addrInst = createInstr2(Operator::ADD, new RegOperand(base), new RegOperand(PR::sp));
            addrInst->fiop = new FrameIndexOperand(fi);
            addrInst->operands.push_back(addrInst->fiop);
            addrInst->use_fiops = true;
            cur_block_->insts.push_back(addrInst);
            
            // 存储值: str val, [base]
            cur_block_->insts.push_back(createInstr2(Operator::STR, new RegOperand(valReg), new MemOperand(base, 0)));
            return;
        }
    }
    
    // 2. 处理全局变量
    if (auto* symOp = dynamic_cast<ME::GlobalOperand*>(ptr)) {
         Register addr = BE::getVReg(BE::I64);
         cur_block_->insts.push_back(createInstr2(Operator::LA, new RegOperand(addr), new SymbolOperand(symOp->name)));
         cur_block_->insts.push_back(createInstr2(Operator::STR, new RegOperand(valReg), new MemOperand(addr, 0)));
         return;
    }

    // 3. 一般情况
    Register ptrReg = getReg(ptr);
    cur_block_->insts.push_back(createInstr2(Operator::STR, new RegOperand(valReg), new MemOperand(ptrReg, 0)));
}

void IRIsel::visit(ME::ArithmeticInst& inst)
{
    // 获取左右操作数
    Register lhs = getReg(inst.lhs);
    if (lhs.dt == nullptr) lhs.dt = BE::I32;
    
    Register res = getOrCreateVReg(inst.res->getRegNum(), lhs.dt);
    
    Operator op;
    bool isFloat = (lhs.dt == BE::F32 || lhs.dt == BE::F64);

    Register rhs = getReg(inst.rhs);
    if (rhs.dt == nullptr) rhs.dt = BE::I32;

    // 1. 类型扩展处理
    // 确保整数算术运算的操作数宽度一致。
    // 如果一个是 32 位，一个是 64 位，通常将 32 位扩展为 64 位。
    if (!isFloat) {
        if (lhs.dt == BE::I32 && rhs.dt == BE::I64) {
            // 左操作数扩展: UXTW (Unsigned Extend Word)
            Register ext = BE::getVReg(BE::I64);
            cur_block_->insts.push_back(createInstr2(Operator::UXTW, new RegOperand(ext), new RegOperand(lhs)));
            lhs = ext;
            if (res.dt == BE::I32) {
                res.dt = BE::I64; 
            }
        } else if (lhs.dt == BE::I64 && rhs.dt == BE::I32) {
             // 右操作数扩展
             Register ext = BE::getVReg(BE::I64);
             cur_block_->insts.push_back(createInstr2(Operator::UXTW, new RegOperand(ext), new RegOperand(rhs)));
             rhs = ext;
             if (res.dt == BE::I32) res.dt = BE::I64;
        }
    }

    // 2. 映射 IR 操作码到 AArch64 操作码
    switch (inst.opcode) {
        case ME::Operator::ADD: op = isFloat ? Operator::FADD : Operator::ADD; break;
        case ME::Operator::SUB: op = isFloat ? Operator::FSUB : Operator::SUB; break;
        case ME::Operator::MUL: op = isFloat ? Operator::FMUL : Operator::MUL; break;
        case ME::Operator::DIV: op = isFloat ? Operator::FDIV : Operator::SDIV; break; // SDIV: Signed Divide
        case ME::Operator::BITAND: op = Operator::AND; break;
        case ME::Operator::BITXOR: op = Operator::EOR; break; // EOR: Exclusive OR
        case ME::Operator::SHL: op = Operator::LSL; break;    // LSL: Logical Shift Left
        case ME::Operator::ASHR: op = Operator::ASR; break;   // ASR: Arithmetic Shift Right
        case ME::Operator::LSHR: op = Operator::LSR; break;   // LSR: Logical Shift Right
        case ME::Operator::MOD: 
        {
            // 取模运算 (MOD) 在 AArch64 中没有直接指令
            // 实现为: res = lhs - (lhs / rhs) * rhs
            // MSUB (Multiply-Subtract): d = a - b * c (但 AArch64 只有 MSUB d, b, c, a)
            // 这里拆解为 SDIV, MUL, SUB
            Register divRes = BE::getVReg(lhs.dt);
            Register mulRes = BE::getVReg(lhs.dt);
            cur_block_->insts.push_back(createInstr3(Operator::SDIV, new RegOperand(divRes), new RegOperand(lhs), new RegOperand(rhs)));
            cur_block_->insts.push_back(createInstr3(Operator::MUL, new RegOperand(mulRes), new RegOperand(divRes), new RegOperand(rhs)));
            cur_block_->insts.push_back(createInstr3(Operator::SUB, new RegOperand(res), new RegOperand(lhs), new RegOperand(mulRes)));
            ctx_.vregMap[inst.res->getRegNum()] = res;
            return;
        }
        default: ERROR("Unknown arithmetic op");
    }

    // 3. 窥孔优化：加减零优化
    // 如果操作数之一是零寄存器 (xzr/wzr)，则可以直接转换为 Move 指令
    if (!isFloat && op == Operator::ADD) {
        if (!rhs.isVreg && rhs.rId == A64_REGISTER_ID_XZR) { // x + 0 = x
            cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(res), new RegOperand(lhs)));
            ctx_.vregMap[inst.res->getRegNum()] = res;
            return;
        }
        if (!lhs.isVreg && lhs.rId == A64_REGISTER_ID_XZR) { // 0 + x = x
            cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(res), new RegOperand(rhs)));
            ctx_.vregMap[inst.res->getRegNum()] = res;
            return;
        }
    }
    if (!isFloat && op == Operator::SUB) {
        if (!rhs.isVreg && rhs.rId == A64_REGISTER_ID_XZR) { // x - 0 = x
            cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(res), new RegOperand(lhs)));
            ctx_.vregMap[inst.res->getRegNum()] = res;
            return;
        }
    }

    // 4. 生成指令
    cur_block_->insts.push_back(createInstr3(op, new RegOperand(res), new RegOperand(lhs), new RegOperand(rhs)));
    
    ctx_.vregMap[inst.res->getRegNum()] = res;
}

void IRIsel::visit(ME::IcmpInst& inst)
{
    Register lhs = getReg(inst.lhs);
    if (lhs.dt == nullptr) lhs.dt = BE::I32;
    Register rhs = getReg(inst.rhs);
    if (rhs.dt == nullptr) rhs.dt = BE::I32;

    // Ensure operand widths match (extend 32-bit to 64-bit if needed for comparison)
    if (lhs.dt == BE::I32 && rhs.dt == BE::I64) {
         if (lhs.rId == A64_REGISTER_ID_XZR) {
             lhs = PR::xzr; // Use 64-bit zero
         } else {
             // Extend lhs
             Register ext = BE::getVReg(BE::I64);
             cur_block_->insts.push_back(createInstr2(Operator::UXTW, new RegOperand(ext), new RegOperand(lhs)));
             lhs = ext;
         }
    } else if (lhs.dt == BE::I64 && rhs.dt == BE::I32) {
         if (rhs.rId == A64_REGISTER_ID_XZR) {
             rhs = PR::xzr; // Use 64-bit zero
         } else {
             // Extend rhs
             Register ext = BE::getVReg(BE::I64);
             cur_block_->insts.push_back(createInstr2(Operator::UXTW, new RegOperand(ext), new RegOperand(rhs)));
             rhs = ext;
         }
    }

    cur_block_->insts.push_back(createInstr2(Operator::CMP, new RegOperand(lhs), new RegOperand(rhs)));
    
    Register res = getOrCreateVReg(inst.res->getRegNum(), BE::I32);
    
    int cond = 0;
    switch (inst.cond) {
        case ME::ICmpOp::EQ: cond = 0; break;
        case ME::ICmpOp::NE: cond = 1; break;
        case ME::ICmpOp::UGT: cond = 8; break;
        case ME::ICmpOp::UGE: cond = 2; break;
        case ME::ICmpOp::ULT: cond = 3; break;
        case ME::ICmpOp::ULE: cond = 9; break;
        case ME::ICmpOp::SGT: cond = 12; break;
        case ME::ICmpOp::SGE: cond = 10; break;
        case ME::ICmpOp::SLT: cond = 11; break;
        case ME::ICmpOp::SLE: cond = 13; break;
    }
    
    cur_block_->insts.push_back(createInstr2(Operator::CSET, new RegOperand(res), new ImmeOperand(cond)));
}

void IRIsel::visit(ME::FcmpInst& inst)
{
    Register lhs = getReg(inst.lhs);
    Register rhs = getReg(inst.rhs);
    cur_block_->insts.push_back(createInstr2(Operator::FCMP, new RegOperand(lhs), new RegOperand(rhs)));
    
    Register res = getOrCreateVReg(inst.res->getRegNum(), BE::I32);
    int cond = getAArch64CC(inst.cond);
    cur_block_->insts.push_back(createInstr2(Operator::CSET, new RegOperand(res), new ImmeOperand(cond)));
}

void IRIsel::visit(ME::AllocaInst& inst)
{
    // Already handled in collectAllocas
    // But we need to define the vreg -> address
    // materialize address of stack slot
    size_t regId = inst.res->getRegNum();
    if (ctx_.allocaFI.count(regId)) {
        // We usually don't need to do anything here if we handle address materialization at use sites (Load/Store)
        // But if the address itself is used (e.g. passed to function), we need to materialize it.
        // Let's materialize it just in case.
        int fi = ctx_.allocaFI[regId];
        Register res = getOrCreateVReg(regId, BE::I64);
        Instr* addrInst = createInstr2(Operator::ADD, new RegOperand(res), new RegOperand(PR::sp));
        addrInst->fiop = new FrameIndexOperand(fi);
        addrInst->operands.push_back(addrInst->fiop);
        addrInst->use_fiops = true;
        cur_block_->insts.push_back(addrInst);
    }
}

void IRIsel::visit(ME::BrCondInst& inst)
{
    Register cond = getReg(inst.cond);
    int trueLabel = dynamic_cast<ME::LabelOperand*>(inst.trueTar)->lnum;
    int falseLabel = dynamic_cast<ME::LabelOperand*>(inst.falseTar)->lnum;
    
    cur_block_->insts.push_back(createInstr2(Operator::CMP, new RegOperand(cond), new ImmeOperand(0)));
    cur_block_->insts.push_back(createInstr1(Operator::BNE, new LabelOperand(trueLabel)));
    cur_block_->insts.push_back(createInstr1(Operator::B, new LabelOperand(falseLabel)));
}

void IRIsel::visit(ME::BrUncondInst& inst)
{
    int targetLabel = dynamic_cast<ME::LabelOperand*>(inst.target)->lnum;
    cur_block_->insts.push_back(createInstr1(Operator::B, new LabelOperand(targetLabel)));
}

void IRIsel::visit(ME::CallInst& inst)
{
    int gprIdx = 0;
    int fprIdx = 0;
    int stackOffset = 0;
    std::vector<Register> paramRegs;
    
    // 第一步：收集所有参数信息
    struct ArgInfo {
        Register vreg;
        bool isFloat;
        int regIdx;      // 寄存器参数的索引 (0-7)
        int stackOff;    // 栈参数的偏移
        bool isStackArg;
    };
    std::vector<ArgInfo> argInfos;
    
    for (auto const& [type, op] : inst.args) {
        Register argReg = getReg(op);  // 这会生成 ldr 指令或立即数加载
        bool isFloat = (argReg.dt == BE::F32 || argReg.dt == BE::F64);
        
        ArgInfo info;
        info.vreg = argReg;
        info.isFloat = isFloat;
        
        // 根据参数类型和已用寄存器数量，决定是通过寄存器传递还是栈传递
        // AArch64 调用约定：前 8 个整数参数用 x0-x7，前 8 个浮点参数用 s0-s7/d0-d7
        if (isFloat) {
            if (fprIdx < 8) {
                info.isStackArg = false;
                info.regIdx = fprIdx;
                fprIdx++;
            } else {
                info.isStackArg = true;
                info.stackOff = stackOffset;
                stackOffset += 8; // 简单起见，所有栈参数都按 8 字节对齐
            }
        } else {
            if (gprIdx < 8) {
                info.isStackArg = false;
                info.regIdx = gprIdx;
                gprIdx++;
            } else {
                info.isStackArg = true;
                info.stackOff = stackOffset;
                stackOffset += 8;
            }
        }
        argInfos.push_back(info);
    }
    
    // 第二步：【关键修复】先存储栈参数
    // 必须在覆盖任何临时寄存器（参数寄存器 x0-x7）之前完成栈参数的存储
    // 因为计算栈参数的地址或值可能需要用到临时寄存器
    for (auto& info : argInfos) {
        if (info.isStackArg) {
            cur_block_->insts.push_back(createInstr2(Operator::STR, 
                new RegOperand(info.vreg), new MemOperand(PR::sp, info.stackOff)));
        }
    }
    
    // 第三步：处理寄存器参数（vreg -> temp）
    // 为了避免参数寄存器之间的冲突（例如 swap(x0, x1)），先移动到临时寄存器
    // 注意：这里使用了特殊的临时寄存器编号，这可能是在寄存器分配前的临时约定
    for (auto& info : argInfos) {
        if (!info.isStackArg) {
            if (info.isFloat) {
                Register pReg(info.regIdx, info.vreg.dt, false);
                Register tempReg(16 + info.regIdx, info.vreg.dt, false); // 使用 d16-d23 作为临时
                cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(tempReg), new RegOperand(info.vreg)));
                paramRegs.push_back(pReg);
            } else {
                Register pReg(info.regIdx, info.vreg.dt, false);
                // 尽可能避开 x0-x7 和 x8 (返回值结构体指针)，使用 x9-x15 或 x16+
                int tempId = (info.regIdx < 7) ? (9 + info.regIdx) : 16;
                Register tempReg(tempId, info.vreg.dt, false);
                cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(tempReg), new RegOperand(info.vreg)));
                paramRegs.push_back(pReg);
            }
        }
    }
    
    // 第四步：处理寄存器参数（temp -> arg reg）
    // 将值从临时寄存器移动到真正的参数寄存器 (x0-x7, s0-s7)
    for (auto& info : argInfos) {
        if (!info.isStackArg) {
            if (info.isFloat) {
                Register pReg(info.regIdx, info.vreg.dt, false);
                Register tempReg(16 + info.regIdx, info.vreg.dt, false);
                cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(pReg), new RegOperand(tempReg)));
            } else {
                Register pReg(info.regIdx, info.vreg.dt, false);
                int tempId = (info.regIdx < 7) ? (9 + info.regIdx) : 16;
                Register tempReg(tempId, info.vreg.dt, false);
                cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(pReg), new RegOperand(tempReg)));
            }
        }
    }
    
    // 第五步：发起调用
    // BL: Branch with Link (跳转并保存返回地址到 x30)
    auto* blInst = createInstr1(Operator::BL, new SymbolOperand(inst.funcName));
    // 将用到的参数寄存器添加到指令的操作数中，以标记它们被使用了（活跃区间分析需要）
    for (auto r : paramRegs) {
        blInst->operands.push_back(new RegOperand(r));
    }
    cur_block_->insts.push_back(blInst);
    
    // 更新最大参数区大小
    // FrameLowering 阶段会根据这个值调整栈指针
    ctx_.mfunc->frameInfo.setParamAreaSize(stackOffset);

    // 处理返回值
    if (inst.res) {
        Register res = getOrCreateVReg(inst.res->getRegNum(), BE::I32);
        if (inst.retType == ME::DataType::F32) res.dt = BE::F32;
        
        Register retReg;
        // 浮点返回值在 s0/d0，整数返回值在 w0/x0
        if (res.dt == BE::F32) retReg = PR::s0;
        else retReg = PR::w0;
        
        if (res.dt == BE::I64 || res.dt == BE::PTR) retReg = PR::x0;
        
        cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(res), new RegOperand(retReg)));
    }
}

void IRIsel::visit(ME::RetInst& inst)
{
    if (inst.res) {
        Register retVal = getReg(inst.res);
        Register targetReg;
        if (retVal.dt == BE::F32) targetReg = PR::s0;
        else if (retVal.dt == BE::F64) targetReg = PR::d0;
        else if (retVal.dt == BE::I64 || retVal.dt == BE::PTR) targetReg = PR::x0;
        else targetReg = PR::w0;
        
        cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(targetReg), new RegOperand(retVal)));
    }
    cur_block_->insts.push_back(createInstr0(Operator::RET));
}

void IRIsel::visit(ME::GEPInst& inst)
{
    // GEP (GetElementPtr) 指令通常在进入后端之前应该被 Lowering Pass 转换为基本的算术指令 (ADD/MUL)。
    // 如果 IR 中仍然存在 GEP 指令，说明 MiddleEnd 没有完成降级。
    // 在当前的 IRIsel 实现中，我们没有足够的信息（如类型系统的大小信息）来正确计算复杂的 GEP 偏移。
    // 因此，如果遇到 GEP 指令，这是一个错误。
    // 
    // 解决方案：
    // 1. 确保在 MiddleEnd 运行 GEP Lowering pass。
    // 2. 或者在这里实现简化的 GEP 处理（假设字节寻址或固定大小）。
    
    ERROR("GEP 指令未在 IRIsel 中实现。请确保在中端进行了 GEP Lowering。");
}

void IRIsel::visit(ME::FP2SIInst& inst)
{
    Register res = getOrCreateVReg(inst.dest->getRegNum(), BE::I32);
    Register src = getReg(inst.src);
    cur_block_->insts.push_back(createInstr2(Operator::FCVTZS, new RegOperand(res), new RegOperand(src)));
}

void IRIsel::visit(ME::SI2FPInst& inst)
{
    Register res = getOrCreateVReg(inst.dest->getRegNum(), BE::F32);
    Register src = getReg(inst.src);
    cur_block_->insts.push_back(createInstr2(Operator::SCVTF, new RegOperand(res), new RegOperand(src)));
}

void IRIsel::visit(ME::ZextInst& inst)
{
    // Default to I64 as before, but check if we should produce I32
    BE::DataType* destType = BE::I64;
    if (inst.to == ME::DataType::I32) destType = BE::I32;

    Register res = getOrCreateVReg(inst.dest->getRegNum(), destType); // destination usually larger
    Register src = getReg(inst.src);

    if (inst.from == ME::DataType::I1 && inst.to == ME::DataType::I32) {
         // Zext i1 to i32.
         // We need to mask to ensure 0/1 value, although CSET usually produces clean 0/1.
         // Using AND ensures correctness if src was not clean.
         cur_block_->insts.push_back(createInstr3(Operator::AND, new RegOperand(res), new RegOperand(src), new ImmeOperand(1)));
    } else if (destType == BE::I64 && src.dt == BE::I32) {
         // Zext i32 to i64
         cur_block_->insts.push_back(createInstr2(Operator::UXTW, new RegOperand(res), new RegOperand(src)));
    } else {
         // Move or other cases
         cur_block_->insts.push_back(BE::AArch64::createMove(new RegOperand(res), new RegOperand(src)));
    }
}

void IRIsel::visit(ME::PhiInst& inst)
{
    // 调试输出 (可选保留)
    fprintf(stderr, "DEBUG: PhiInst res=%zu, incoming values:\n", inst.res->getRegNum());
    for (auto const& [labelOp, valOp] : inst.incomingVals) {
        if (auto* regOp = dynamic_cast<ME::RegOperand*>(valOp)) {
            fprintf(stderr, "  from label %zu: reg %zu\n", 
                    dynamic_cast<ME::LabelOperand*>(labelOp)->lnum,
                    regOp->getRegNum());
        }
    }
    
    // 1. 确定 Phi 节点的结果类型
    BE::DataType* resDt = BE::I32;
    if (inst.dt == ME::DataType::F32) resDt = BE::F32;
    else if (inst.dt == ME::DataType::I64 || inst.dt == ME::DataType::PTR) resDt = BE::I64;
    
    Register res = getOrCreateVReg(inst.res->getRegNum(), resDt);

    // 2. 创建后端 Phi 指令
    auto* phi = new BE::PhiInst(res);
    
    for (auto const& [labelOp, valOp] : inst.incomingVals) {
        int labelId = dynamic_cast<ME::LabelOperand*>(labelOp)->lnum;
        
        if (auto* regOp = dynamic_cast<ME::RegOperand*>(valOp)) {
            // 关键逻辑：使用 getOrCreateVReg 确保一致性
            // Phi 节点的输入可能来自尚未访问的基本块（后向边），或者尚未定义的指令
            // getOrCreateVReg 保证对于同一个 IR 寄存器 ID，总是返回同一个后端 vreg
            size_t srcRegId = regOp->getRegNum();
            Register srcVReg = getOrCreateVReg(srcRegId, resDt);
            phi->incomingVals[labelId] = new BE::RegOperand(srcVReg);
        }
        else if (auto* immOp = dynamic_cast<ME::ImmeI32Operand*>(valOp)) {
            // 如果输入是立即数，直接作为 Phi 的操作数
            // 后续 PhiElimination 会处理这种混合情况（通常会插入 move 指令）
            phi->incomingVals[labelId] = new BE::I32Operand(immOp->value);
        }
        else {
            // 对于其他类型的立即数（如浮点），需要特殊处理
            // 这里尝试用 getReg 获取，但这可能会在当前块插入指令，这对于 Phi 来说是不合法的
            // 因为 Phi 必须在块的开头。
            // 理想情况下，中端应该将常量 Phi 输入提升为前驱块中的常量定义。
            fprintf(stderr, "WARNING: PHI has non-reg, non-imm operand\n");
            Register val = getReg(valOp);
            phi->incomingVals[labelId] = new BE::RegOperand(val);
        }
    }
    
    cur_block_->insts.push_back(phi);
}

void IRIsel::visit(ME::GlbVarDeclInst& inst)
{
    ERROR("Global variable declarations should not appear in IR during instruction selection.");
}
void IRIsel::visit(ME::FuncDeclInst& inst)
{
    ERROR("Function declarations should not appear in IR during instruction selection.");
}
void IRIsel::visit(ME::FuncDefInst& inst)
{
    ERROR("Function definitions should not appear in IR during instruction selection.");
}
} // namespace BE::AArch64