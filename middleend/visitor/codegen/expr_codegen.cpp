#include <middleend/visitor/codegen/ast_codegen.h>

namespace ME
{
// from expr_codegen.cpp

void ASTCodeGen::visit(FE::AST::LeftValExpr& node, Module* m)
{
    (void)m;
    Operand* ptrOp = nullptr;
    if (node.entry)
    {
        // 1. ?????????? (????)
        size_t reg = name2reg.getReg(node.entry);
        if (reg != static_cast<size_t>(-1))
            ptrOp = OperandFactory::getInstance().getRegOperand(reg);
        // 2. ?????????? (??????)
        else
        {
            // ? ???????????????????????? ?
            ptrOp = OperandFactory::getInstance().getGlobalOperand(node.entry->getName());
        }
    }
    
    // ???????????????????????????? Load ?????
    if (!ptrOp) return;

    // ?????? (Array/GEP ??????????)
    lval2ptr[&node] = ptrOp; 
    
    // 3. ???? LoadInst ????
    DataType t = convert(node.attr.val.value.type);
    size_t   resReg = getNewRegId(); 
    insert(createLoadInst(t, ptrOp, resReg)); 
    reg2attr[resReg] = FE::AST::VarAttr(node.attr.val.value.type);
}
    void ASTCodeGen::visit(FE::AST::LiteralExpr& node, Module* m)
    {
        (void)m;

        size_t reg = getNewRegId();
        switch (node.literal.type->getBaseType())
        {
            case FE::AST::Type_t::INT:
            case FE::AST::Type_t::LL:  // treat as I32(????????ll???????????????????)
            {
                int             val  = node.literal.getInt();
                ArithmeticInst* inst = createArithmeticI32Inst_ImmeAll(Operator::ADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            case FE::AST::Type_t::FLOAT:
            {
                float           val  = node.literal.getFloat();
                ArithmeticInst* inst = createArithmeticF32Inst_ImmeAll(Operator::FADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            default: ERROR("Unsupported literal type");
        }
    }

    void ASTCodeGen::visit(FE::AST::UnaryExpr& node, Module* m)
    {
        (void)m;
        handleUnaryCalc(*node.expr, node.op, curBlock, m);
        size_t resReg = getMaxReg();
        reg2attr[resReg] = FE::AST::VarAttr(node.attr.val.value.type);
    }

  // from expr_codegen.cpp

void ASTCodeGen::handleAssign(FE::AST::LeftValExpr& lhs, FE::AST::ExprNode& rhs, Module* m)
{
    (void)m;
    
    // 1. ???? RHS????????????? IR
    apply(*this, rhs, m); 
    size_t   valReg = getMaxReg(); 
    DataType rhsT   = convert(rhs.attr.val.value.type);

    // 2. ? ???? LHS ??? (?????????) ?
    Operand* ptr = nullptr;
    if (lhs.entry)
    {
        // ???? A: ????????????? (????????)
        size_t reg = name2reg.getReg(lhs.entry);
        if (reg != static_cast<size_t>(-1)) {
            // ???????????
            ptr = OperandFactory::getInstance().getRegOperand(reg);
        }
        // ???? B: ??????????????????????
        else
        {
            // ? ????????????? getName() ???????????? ?
            // ??????? glbSymbols.find()??????? Entry ???????????????
            ptr = OperandFactory::getInstance().getGlobalOperand(lhs.entry->getName());
        }
    }

    if (!ptr) {
        // ???????????????????????????????????????????????????????????????????
        ERROR("Variable lookup failed in handleAssign: %s", lhs.entry ? lhs.entry->getName().c_str() : "unknown");
        return;
    }
    
    DataType lhsT = convert(lhs.attr.val.value.type);
    
    // 3. ???????????
    if (rhsT != lhsT)
    {
        auto insts = createTypeConvertInst(rhsT, lhsT, valReg);
        for (auto* inst : insts) insert(inst);
        valReg = getMaxReg(); 
    }

    // 4. ???? StoreInst
    insert(createStoreInst(lhsT, valReg, ptr));

    // 5. ????????????
    size_t resReg = getNewRegId();
    insert(createLoadInst(lhsT, ptr, resReg));
    reg2attr[resReg] = FE::AST::VarAttr(lhs.attr.val.value.type);
}

void ASTCodeGen::handleLogicalAnd(
    FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
{
    (void)node;
    (void)m;
    Block* rhsBlock   = curFunc->createBlock();
    Block* falseBlock = curFunc->createBlock(); // ????: ?????????? i1 0 ???
    Block* endBlock   = curFunc->createBlock();
    // size_t origId     = curBlock->blockId; // ??????????????????
    apply(*this, lhs, m);
    size_t   lhsReg = getMaxReg();
    DataType lhsT   = convert(lhs.attr.val.value.type);
    if (lhsT != DataType::I1)
    {
        auto insts = createTypeConvertInst(lhsT, DataType::I1, lhsReg);
        for (auto* inst : insts) insert(inst);
        lhsReg = getMaxReg();
    }
    // ??? LHS ????????? RHS??????????? FalseBlock
    insert(createBranchInst(lhsReg, rhsBlock->blockId, falseBlock->blockId)); 
    
    exitBlock();
    
    // === ???? False Block: ???? i1 0 (false) ????? ===
    enterBlock(falseBlock);
    size_t zeroReg = getNewRegId();
    // ???? i32 0 ???????????????????
    insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, 0, 0, zeroReg)); 
    
    size_t i1FalseReg = getNewRegId();
    // ???? i1 0: %i1FalseReg = icmp eq i32 %zeroReg, 1 -> i1 0
    insert(createIcmpInst_ImmeRight(ICmpOp::EQ, zeroReg, 1, i1FalseReg)); 
    
    // ??????????? End
    insert(createBranchInst(endBlock->blockId));
    
    exitBlock();
    // === False Block ???? ===

    enterBlock(rhsBlock);
    
    // ???? RHS ????? IR
    apply(*this, rhs, m); 
    size_t rhsExitId = curBlock->blockId; 
    
    size_t   rhsReg = getMaxReg();
    DataType rhsT   = convert(rhs.attr.val.value.type);
    if (rhsT != DataType::I1)
    {
        auto insts = createTypeConvertInst(rhsT, DataType::I1, rhsReg);
        for (auto* inst : insts) insert(inst);
        rhsReg = getMaxReg();
    }
    
    // RHS ????????????????? End (rhsExitId -> endBlock)
    if(curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()){
        insert(createBranchInst(endBlock->blockId));
    }
    
    exitBlock();
    enterBlock(endBlock);
    
    size_t  resReg = getNewRegId();
    auto* phi    = new PhiInst(DataType::I1, OperandFactory::getInstance().getRegOperand(resReg));
    
    // ???? 1: LHS ???????? (??? i1FalseReg?????? falseBlock)
    phi->addIncoming(OperandFactory::getInstance().getRegOperand(i1FalseReg), OperandFactory::getInstance().getLabelOperand(falseBlock->blockId)); 
    
    // ???? 2: LHS ??????? RHS (? rhsReg?????? rhsExitId)
    phi->addIncoming(OperandFactory::getInstance().getRegOperand(rhsReg), OperandFactory::getInstance().getLabelOperand(rhsExitId)); 
    
    insert(phi);

    // ???: ?????????? i1???? SysY ?????????? int (i32)??????????????? (zext)
    DataType exprT = convert(node.attr.val.value.type);
    if (exprT == DataType::I32)
    {
        auto insts = createTypeConvertInst(DataType::I1, DataType::I32, resReg);
        for (auto* inst : insts) insert(inst);
    }
}


// from expr_codegen.cpp (????? handleLogicalOr)

void ASTCodeGen::handleLogicalOr(
    FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
{
    (void)node;
    (void)m;
    Block* rhsBlock  = curFunc->createBlock();
    Block* trueBlock = curFunc->createBlock(); // ????: ?????????? i1 1 ???
    Block* endBlock  = curFunc->createBlock();
    // size_t origId    = curBlock->blockId; // ??????????????????
    apply(*this, lhs, m);
    size_t   lhsReg = getMaxReg();
    DataType lhsT   = convert(lhs.attr.val.value.type);
    if (lhsT != DataType::I1)
    {
        auto insts = createTypeConvertInst(lhsT, DataType::I1, lhsReg);
        for (auto* inst : insts) insert(inst);
        lhsReg = getMaxReg();
    }
    // ??? LHS ?????????? TrueBlock??????????? RHS
    insert(createBranchInst(lhsReg, trueBlock->blockId, rhsBlock->blockId)); 
    
    exitBlock();
    
    // === ???? True Block: ???? i1 1 (true) ????? ===
    enterBlock(trueBlock);
    size_t oneReg = getNewRegId();
    // ???? i32 1 ???????????????????
    insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, 1, 0, oneReg)); 
    
    size_t i1TrueReg = getNewRegId();
    // ???? i1 1: %i1TrueReg = icmp eq i32 %oneReg, 1 -> i1 1
    insert(createIcmpInst_ImmeRight(ICmpOp::EQ, oneReg, 1, i1TrueReg)); 
    
    // ??????????? End
    insert(createBranchInst(endBlock->blockId));
    
    exitBlock();
    // === True Block ???? ===
    
    enterBlock(rhsBlock);
    
    // ???? RHS ????? IR
    apply(*this, rhs, m);
    size_t rhsExitId = curBlock->blockId; 
    
    size_t   rhsReg = getMaxReg();
    DataType rhsT   = convert(rhs.attr.val.value.type);
    if (rhsT != DataType::I1)
    {
        auto insts = createTypeConvertInst(rhsT, DataType::I1, rhsReg);
        for (auto* inst : insts) insert(inst);
        rhsReg = getMaxReg();
    }
    
    // RHS ????????????????? End (rhsExitId -> endBlock)
    if(curBlock->insts.empty() || !curBlock->insts.back()->isTerminator()){
        insert(createBranchInst(endBlock->blockId));
    }
    
    exitBlock();
    enterBlock(endBlock);
    
    size_t  resReg = getNewRegId();
    auto* phi    = new PhiInst(DataType::I1, OperandFactory::getInstance().getRegOperand(resReg));
    
    // ???? 1: LHS ???????? (? i1TrueReg?????? trueBlock)
    phi->addIncoming(OperandFactory::getInstance().getRegOperand(i1TrueReg), OperandFactory::getInstance().getLabelOperand(trueBlock->blockId)); 
    
    // ???? 2: LHS ??????? RHS (? rhsReg?????? rhsExitId)
    phi->addIncoming(OperandFactory::getInstance().getRegOperand(rhsReg), OperandFactory::getInstance().getLabelOperand(rhsExitId)); 
    
    insert(phi);
}
   void ASTCodeGen::visit(FE::AST::BinaryExpr& node, Module* m)
{
    (void)m;
    
    // 1. ????????????? (=)
    if (node.op == FE::AST::Operator::ASSIGN)
    {
        // ??????????? (LeftValExpr)
        auto* lhs = dynamic_cast<FE::AST::LeftValExpr*>(node.lhs);
        if (!lhs) return;
        
        // ???? handleAssign ???????????????? IR??
        // handleAssign ?????????????? LHS ?????????????? RHS ???????? Store ???
        handleAssign(*lhs, *node.rhs, m); 
        
        size_t resReg = getMaxReg();
        reg2attr[resReg] = FE::AST::VarAttr(node.lhs->attr.val.value.type);
        return;
    }
    
    // 2. ?????????????? (&&)
    if (node.op == FE::AST::Operator::AND)
    {
        handleLogicalAnd(node, *node.lhs, *node.rhs, m);
        size_t resReg = getMaxReg();
        // ???: ??? node ????????? (i32) ?????? boolType (i1)
        reg2attr[resReg] = FE::AST::VarAttr(node.attr.val.value.type);
        return;
    }
    
    // 3. ?????????????? (||)
    if (node.op == FE::AST::Operator::OR)
    {
        handleLogicalOr(node, *node.lhs, *node.rhs, m);
        size_t resReg = getMaxReg();
        // ???: ??? node ????????? (i32) ?????? boolType (i1)
        reg2attr[resReg] = FE::AST::VarAttr(node.attr.val.value.type);
        return;
    }
    
    // 4. ????????????????????? +, -, *, /, <, >, == ???
    // ?????? handleBinaryCalc ?????????? IR
    handleBinaryCalc(*node.lhs, *node.rhs, node.op, curBlock, m);
    size_t resReg = getMaxReg();
    reg2attr[resReg] = FE::AST::VarAttr(node.attr.val.value.type);
}

    void ASTCodeGen::visit(FE::AST::CallExpr& node, Module* m)
    {
        (void)m;
        auto it = funcDecls.find(node.func);
        DataType retT = DataType::VOID;
        std::vector<CallInst::argPair> args;
        if (it != funcDecls.end())
        {
            auto* decl = it->second;
            retT       = convert(decl->retType);
            if (node.args)
            {
                for (size_t i = 0; i < node.args->size(); ++i)
                {
                    auto* arg = (*node.args)[i];
                    if (!arg) continue;
                    apply(*this, *arg, m);
                    size_t   aReg  = getMaxReg();
                    DataType aType = convert(arg->attr.val.value.type);
                    DataType eType = (decl->params && i < decl->params->size())
                        ? convert((*decl->params)[i]->type)
                        : aType;
                    if (aType != eType)
                    {
                        auto insts = createTypeConvertInst(aType, eType, aReg);
                        for (auto* inst : insts) insert(inst);
                        aReg = getMaxReg();
                        aType = eType;
                    }
                    args.emplace_back(eType, OperandFactory::getInstance().getRegOperand(aReg));
                }
            }
        }
        size_t resReg = 0;
        if (retT != DataType::VOID) resReg = getNewRegId();
        if (retT != DataType::VOID)
            insert(createCallInst(retT, node.func->getName(), args, resReg));
        else
            insert(createCallInst(retT, node.func->getName(), args));
        if (retT != DataType::VOID) reg2attr[resReg] = FE::AST::VarAttr(node.attr.val.value.type);
    }

    void ASTCodeGen::visit(FE::AST::CommaExpr& node, Module* m)
    {
        (void)m;
        if (!node.exprs) return;
        for (auto* e : *node.exprs)
        {
            if (!e) continue;
            apply(*this, *e, m);
        }
        size_t resReg = getMaxReg();
        reg2attr[resReg] = FE::AST::VarAttr(node.attr.val.value.type);
    }
}  // namespace ME
