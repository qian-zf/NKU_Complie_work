#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::ExprStmt& node, Module* m)
    {
        if (!node.expr) return;
        apply(*this, *node.expr, m);
    }

void ASTCodeGen::visit(FE::AST::FuncDeclStmt& node, Module* m)
{
    // 1. Prepare FuncDefInst and Function
    std::vector<FuncDefInst::argPair> argRegs;
    FuncDefInst* funcdef = new FuncDefInst(convert(node.retType), node.entry->getName(), argRegs);
    funcdef->funcName = node.entry->getName();

    Function* func = new Function(funcdef);
    if (m) m->functions.emplace_back(func);
    enterFunc(func); // 此时 this->curFunc 指向 func

    // 2. Entry Block 的创建和进入
    Block* enter_Block = func->createBlock();
    enterBlock(enter_Block); // 此时 this->curBlock 指向 enter_Block
    
    // ----------------------------------------------------
    // ⚡ 关键：在函数开始时创建函数根作用域 ⚡
    name2reg.enterScope(); 
    // ----------------------------------------------------
    
    // ----------------------------------------------------
    // 2.5 参数的 Alloca、Store 和 name2reg 注册 
    // ----------------------------------------------------
    if (node.params) 
    {
        for (auto* param : *node.params)
        {
            if (!param || !param->entry) continue;

            DataType paramType = convert(param->type);

            // A. 传入参数值 (Value) 的寄存器分配
            // 这个寄存器用于接收 CallInst 传入的值
            size_t valRegId = getNewRegId();
            ME::Operand* valOp = ME::OperandFactory::getInstance().getRegOperand(valRegId); 
            funcdef->argRegs.emplace_back(paramType, valOp); 

            // B. 参数地址 (Address) 的栈空间分配 (Alloca)
            // 这个寄存器是参数的 Alloca 地址
            size_t ptrRegId = getNewRegId();
            AllocaInst* allocaInst = createAllocaInst(paramType, ptrRegId);
            // 插入 AllocaInst。注意：对于 Entry Block 必须在其他指令之前。
            // 由于 VarDeclStmt::visit 会将局部变量 Alloca 前插，这里使用 insert() 即可。
            insert(allocaInst); 

            // C. 将 ADDRESS (ptrRegId) 注册到局部符号表 (name2reg)
            name2reg.addSymbol(param->entry, ptrRegId);

            // D. Store：将传入的 VALUE (valRegId) 存入 ADDRESS (ptrRegId)
            // StoreInst 必须在 AllocaInst 之后
            ME::Operand* ptrOp = ME::OperandFactory::getInstance().getRegOperand(ptrRegId);
            StoreInst* storeInst = createStoreInst(paramType, valRegId, ptrOp);
            insert(storeInst); 
        }
    }
    // ----------------------------------------------------
    // 结束：参数 Alloca/Store
    // ----------------------------------------------------

    // 3. 遍历函数体
    if (node.body)
    {
        // ⚡ 确认：函数体如果是一个 BlockStmt（即 `{}`），其内部的 visit(BlockStmt) 
        // 已经会处理新的子作用域，这里不应再有冗余的 enter/exit Scope。
        apply(*this, *node.body, m);
    }

    // 4. Terminator 补丁逻辑
    // 检查当前基本块是否以终结指令结束 (例如 Ret, Br)
    bool needPatch = (curBlock == nullptr) || curBlock->insts.empty() || !curBlock->insts.back()->isTerminator();
    if (needPatch)
    {
        DataType rt = node.retType ? convert(node.retType) : DataType::VOID;
        if (rt == DataType::VOID)
        {
            insert(createRetInst());
        }
        else if (rt == DataType::I32)
        {
            // return 0;
            size_t z = func->getNewRegId();
            insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, 0, 0, z));
            insert(createRetInst(rt, z));
        }
        else if (rt == DataType::F32)
        {
            // return 0.0f;
            size_t z = func->getNewRegId();
            insert(createArithmeticF32Inst_ImmeAll(Operator::FADD, 0.0f, 0.0f, z));
            insert(createRetInst(rt, z));
        }
        else if (rt == DataType::I64)
        {
            // return 0L;
            size_t z32 = func->getNewRegId();
            insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, 0, 0, z32));
            size_t z64 = func->getNewRegId();
            insert(createZextInst(z32, z64, 32, 64));
            insert(createRetInst(rt, z64));
        }
        else
        {
            insert(createRetInst());
        }
    }

    // 5. 清理工作
    // ⚡ 关键：销毁函数根作用域，回到父作用域 (全局作用域)
    name2reg.exitScope(); 
    
    exitFunc();
}

    // from stmt_codegen.cpp

    void ASTCodeGen::visit(FE::AST::VarDeclStmt& node, Module* m)
{
    // 全局变量的处理保持不变
    if (curFunc == nullptr) {
        handleGlobalVarDecl(&node, m);
        return;
    }

    // 1. 记录当前块（初始化赋值 StoreInst 应该插入的位置）
    ME::Block* currentBlock = curBlock;
    
    // 2. 获取 Entry Block（AllocaInst 必须插入的位置）
    if (curFunc->blocks.empty()) {
        return; 
    }
    // 假设 curFunc->blocks 是一个 map，第一个元素是 Entry Block
    ME::Block* entryBlock = curFunc->blocks.begin()->second; 

    for (auto decl : *node.decl->decls) {
        if (!decl) continue;
        DataType varType = convert(node.decl->type);
        size_t varReg = curFunc->getNewRegId(); // 确保使用 func 级别的注册ID

        // =======================================================
        // 关键修改: 确保 AllocaInst 插入到 Entry Block 的头部
        // =======================================================
        
        // 1. 创建 AllocaInst
        AllocaInst* allocaInst = createAllocaInst(varType, varReg);
        
        // 2. 手动将 AllocaInst 插入到 Entry Block 的指令列表 'insts' 的最前面
        //    !!! 注意: 这里假设 ME::Block 有一个名为 'insts' 的指令列表成员
        //    如果您的框架提供了像 'insertAtFront' 这样的 API，请替换这部分。
        //    如果您的框架将参数的 StoreInst 放在前面，请插入到它们之后、br 之前。
        
        // 假设 `insts` 是 std::vector<Instruction*>, 且第一个非参数相关 storeInst 是插入点
        // 最安全的方式是插入到 entryBlock->insts 的最前面。
        
        // --- 强制前插逻辑 ---
        entryBlock->insts.insert(entryBlock->insts.begin(), allocaInst); 
        // ------------------
        
        // 3. 注册符号
        if (auto* lvalExpr = dynamic_cast<FE::AST::LeftValExpr*>(decl->lval)) {
            if (lvalExpr->entry) {
                name2reg.addSymbol(lvalExpr->entry, varReg);
                lval2ptr[lvalExpr] = ME::OperandFactory::getInstance().getRegOperand(varReg);
            }
        }
        
        // =======================================================
        // 初始化赋值 StoreInst 仍插入到当前块
        // =======================================================

        if (decl->init) {
            // 切换回原始块 (currentBlock) 来生成表达式和 StoreInst
            enterBlock(currentBlock); 
            
            apply(*this, *decl->init, m);
            size_t initReg = curFunc->getMaxReg();
            Operand* ptr = ME::OperandFactory::getInstance().getRegOperand(varReg);
            
            // 确保 StoreInst 被插入到 currentBlock（例如 Block0, 在 br 之前）
            StoreInst* storeInst = createStoreInst(varType, initReg, ptr);
            insert(storeInst); // 这里的 insert() 默认追加到 currentBlock 末尾
        }
    }
}

void ASTCodeGen::visit(FE::AST::BlockStmt& node, Module* m)
{
    // 1. 进入新的作用域：创建新的符号表层级。
    name2reg.enterScope();

    if (node.stmts)
    {
        for (auto* stmt : *node.stmts)
        {
            if (!stmt) continue;
            // 递归访问块内的所有语句和声明 (包括 VarDeclStmt)
            apply(*this, *stmt, m);
        }
    }

    // 2. 退出当前作用域：移除块内定义的所有局部符号。
    // 这是修复全局变量 'a' 地址查找错误的关键。
    name2reg.exitScope();
}
    void ASTCodeGen::visit(FE::AST::ReturnStmt& node, Module* m)
    {
        // ... (保持 ReturnStmt 原有代码不变) ...
        DataType retT = curFunc->funcDef->retType;
        if (!node.retExpr)
        {
            insert(createRetInst());
            return;
        }
        if (retT != DataType::VOID)
        {
            if (auto* lit = dynamic_cast<FE::AST::LiteralExpr*>(node.retExpr))
            {
                auto bt = lit->literal.type->getBaseType();
                if (bt == FE::AST::Type_t::INT || bt == FE::AST::Type_t::LL)
                {
                    if (retT == DataType::I64)
                    {
                        long long v = (bt == FE::AST::Type_t::LL) ? lit->literal.getLL() : static_cast<long long>(lit->literal.getInt());
                        int lo = static_cast<int>(v);
                        size_t r32 = getNewRegId();
                        insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, lo, 0, r32));
                        size_t r64 = getNewRegId();
                        insert(createZextInst(r32, r64, 32, 64));
                        insert(createRetInst(DataType::I64, r64));
                        return;
                    }
                    insert(createRetInst(lit->literal.getInt()));
                    return;
                }
                if (bt == FE::AST::Type_t::FLOAT)
                {
                    insert(createRetInst(lit->literal.getFloat()));
                    return;
                }
            }
        }
        apply(*this, *node.retExpr, m);
        size_t  exprReg = getMaxReg();
        DataType exprT  = convert(node.retExpr->attr.val.value.type);
        if (retT == DataType::VOID)
        {
            insert(createRetInst());
            return;
        }
        if (exprT != retT)
        {
            auto insts = createTypeConvertInst(exprT, retT, exprReg);
            for (auto* inst : insts) insert(inst);
            exprReg = getMaxReg();
        }
        insert(createRetInst(retT, exprReg));
    }

    void ASTCodeGen::visit(FE::AST::WhileStmt& node, Module* m)
    {
        Block* condBlock = curFunc->createBlock();
        Block* bodyBlock = curFunc->createBlock();
        Block* endBlock  = curFunc->createBlock();

        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
            insert(createBranchInst(condBlock->blockId));

        size_t oldStart = curFunc->loopStartLabel;
        size_t oldEnd   = curFunc->loopEndLabel;
        curFunc->loopStartLabel = condBlock->blockId; // continue 跳条件块
        curFunc->loopEndLabel   = endBlock->blockId; // break 跳结束块

        exitBlock();
        enterBlock(condBlock);
        apply(*this, *node.cond, m);

        size_t condReg = getMaxReg();
        DataType condT = convert(node.cond->attr.val.value.type);
        if (condT != DataType::I1) {
            for (auto* i : createTypeConvertInst(condT, DataType::I1, condReg))
                insert(i);
            condReg = getMaxReg();
        }
        insert(createBranchInst(condReg, bodyBlock->blockId, endBlock->blockId));

        exitBlock();
        enterBlock(bodyBlock);
        if (node.body) {
            // --- 关键修复：为循环体创建新的作用域，解决变量重复声明问题 ---
            //name2reg.enterScope(); 
            apply(*this, *node.body, m);
            //name2reg.exitScope();
            // ----------------------------------------------------------------
        }

        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
            insert(createBranchInst(condBlock->blockId));

        exitBlock();
        enterBlock(endBlock);

        curFunc->loopStartLabel = oldStart;
        curFunc->loopEndLabel   = oldEnd;
    }

    void ASTCodeGen::visit(FE::AST::IfStmt& node, Module* m)
    {
        ME::Block* thenBlock = curFunc->createBlock();
        ME::Block* endBlock = curFunc->createBlock();
        ME::Block* elseBlock = nullptr;

        bool hasElse = node.elseStmt!=nullptr;
        if(hasElse){
            elseBlock = curFunc->createBlock();
        }

        apply(*this,*node.cond,m);
        size_t condReg = curFunc->getMaxReg();
        DataType condT   = convert(node.cond->attr.val.value.type);
        if (condT != DataType::I1)
        {
            auto insts = createTypeConvertInst(condT, DataType::I1, condReg);
            for (auto* inst : insts) insert(inst);
            condReg = getMaxReg();
        }

        size_t falseTarId = hasElse?elseBlock->blockId:endBlock->blockId;
        insert(createBranchInst(condReg,thenBlock->blockId,falseTarId));

        exitBlock();
        enterBlock(thenBlock);

        if(node.thenStmt){
            //name2reg.enterScope();
            apply(*this,*node.thenStmt,m);
            //name2reg.exitScope();
        }

        if(curBlock->insts.empty()||!curBlock->insts.back()->isTerminator()){
            insert(createBranchInst(endBlock->blockId));
        }

        if(hasElse){
            exitBlock();
            enterBlock(elseBlock);

            //name2reg.enterScope();
            apply(*this, *node.elseStmt, m);
            //name2reg.exitScope();
        
            if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()) {
                insert(createBranchInst(endBlock->blockId));
            }
        }

        exitBlock();
        enterBlock(endBlock);
    }

    void ASTCodeGen::visit(FE::AST::BreakStmt& node, Module* m)
    {
        (void)node;
        (void)m;
        // ... (保持 BreakStmt 原有代码不变) ...
        size_t endBlockId = curFunc->loopEndLabel;

        if(endBlockId == 0){
            return;
        }

        insert(createBranchInst(endBlockId));
    }

    void ASTCodeGen::visit(FE::AST::ContinueStmt& node, Module* m)
    {
        (void)node;
        (void)m;
        // ... (保持 ContinueStmt 原有代码不变) ...
        size_t startBlockId = curFunc->loopStartLabel;

        if(startBlockId == 0)return;

        insert(createBranchInst(startBlockId));
    }

    void ASTCodeGen::visit(FE::AST::ForStmt& node, Module* m)
    {
        Block* condBlock = curFunc->createBlock();
        Block* bodyBlock = curFunc->createBlock();
        Block* stepBlock = curFunc->createBlock();
        Block* endBlock = curFunc->createBlock();

        // for 作用域（包含 init 声明的变量）
        name2reg.enterScope();

        if (node.init)
            apply(*this, *node.init, m);

        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
            insert(createBranchInst(condBlock->blockId));

        size_t oldStart = curFunc->loopStartLabel;
        size_t oldEnd   = curFunc->loopEndLabel;
        curFunc->loopStartLabel = stepBlock->blockId; // continue 跳 step
        curFunc->loopEndLabel   = endBlock->blockId; // break 跳 end

        exitBlock();
        enterBlock(condBlock);
        if (node.cond) {
            apply(*this, *node.cond, m);
            size_t r = getMaxReg();
            DataType t = convert(node.cond->attr.val.value.type);
            if (t != DataType::I1) {
                for (auto* i : createTypeConvertInst(t, DataType::I1, r))
                    insert(i);
                r = getMaxReg();
            }
            insert(createBranchInst(r, bodyBlock->blockId, endBlock->blockId));
        } else {
            insert(createBranchInst(bodyBlock->blockId));
        }

        exitBlock();
        enterBlock(bodyBlock);
        if (node.body) {
            // --- 关键修复：为循环体创建新的作用域 ---
            //name2reg.enterScope(); 
            apply(*this, *node.body, m);
            //name2reg.exitScope();
            // ----------------------------------------
        }

        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
            insert(createBranchInst(stepBlock->blockId));

        exitBlock();
        enterBlock(stepBlock);
        if (node.step)
            apply(*this, *node.step, m);

        if (curBlock->insts.empty() || !curBlock->insts.back()->isTerminator())
            insert(createBranchInst(condBlock->blockId));

        exitBlock();
        enterBlock(endBlock);

        curFunc->loopStartLabel = oldStart;
        curFunc->loopEndLabel   = oldEnd;

        name2reg.exitScope(); // for 作用域结束 (包括 init 声明的变量)
    }
}
// namespace ME