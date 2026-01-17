#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>

namespace ME
{
    void ASTCodeGen::visit(FE::AST::Initializer& node, Module* m)
    {
        // ??????????????????? IR????????????????????????��????????????
        if (node.init_val) apply(*this, *node.init_val, m);
    }
    void ASTCodeGen::visit(FE::AST::InitializerList& node, Module* m)
    {
        // ????????????��??��????????????
        if (!node.init_list) return;
        for (auto* sub : *(node.init_list))
        {
            if (!sub) continue;
            apply(*this, *sub, m);
        }
    }
    void ASTCodeGen::visit(FE::AST::VarDeclarator& node, Module* m)
    {
        // ?????????????????????????????????????????????????????????
        if (node.lval) apply(*this, *node.lval, m);
        if (node.init) apply(*this, *node.init, m);
    }
    void ASTCodeGen::visit(FE::AST::ParamDeclarator& node, Module* m)
    {
        // ?????????????? visit(FuncDeclStmt) ????????????????????? IR
        (void)node;
        (void)m;
    }

    void ASTCodeGen::visit(FE::AST::VarDeclaration& node, Module* m)
    {
        (void)m;
        DataType type = convert(node.type);
        if (!node.decls) return;

        for (auto* declarator : *node.decls)
        {
            if (!declarator) continue;

            size_t reg = getNewRegId();
            insert(createAllocaInst(type, reg));

            if (auto* lval = dynamic_cast<FE::AST::LeftValExpr*>(declarator->lval))
            {
                if (lval->entry) name2reg.addSymbol(lval->entry, reg);
            }

            if (declarator->init)
            {
                if (declarator->init->singleInit)
                {
                    if (declarator->init->attr.val.isConstexpr)
                    {
                        ME::Operand* ptr = ME::OperandFactory::getInstance().getRegOperand(reg);
                        if (type == DataType::I64)
                        {
                            long long vll = declarator->init->attr.val.getLL();
                            int       lo  = static_cast<int>(vll);
                            size_t    r32 = curFunc->getNewRegId();
                            insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, lo, 0, r32));
                            size_t r64 = curFunc->getNewRegId();
                            insert(createZextInst(r32, r64, 32, 64));
                            insert(createStoreInst(type, r64, ptr));
                        }
                        else if (type == DataType::I32 || type == DataType::I1)
                        {
                            int v = declarator->init->attr.val.getInt();
                            ME::Operand* valOp = ME::OperandFactory::getInstance().getImmeI32Operand(v);
                            insert(createStoreInst(type, valOp, ptr));
                        }
                        else if (type == DataType::F32)
                        {
                            float v = declarator->init->attr.val.getFloat();
                            ME::Operand* valOp = ME::OperandFactory::getInstance().getImmeF32Operand(v);
                            insert(createStoreInst(type, valOp, ptr));
                        }
                    }
                    else
                    {
                        auto* init = dynamic_cast<FE::AST::Initializer*>(declarator->init);
                        if (init && init->init_val)
                        {
                            apply(*this, *init->init_val, m);
                            size_t valReg = curFunc->getMaxReg();
                            DataType exprType = reg2attr.count(valReg) ? convert(reg2attr[valReg].type) : type;
                            if (exprType != type)
                            {
                                auto convInsts = createTypeConvertInst(exprType, type, valReg);
                                for (auto* inst : convInsts) insert(inst);
                                valReg = curFunc->getMaxReg();
                            }
                            ME::Operand* ptr = ME::OperandFactory::getInstance().getRegOperand(reg);
                            insert(createStoreInst(type, valReg, ptr));
                        }
                    }
                }
                else
                {
                    apply(*this, *declarator->init, m);
                    size_t   valReg   = curFunc->getMaxReg();
                    DataType exprType = reg2attr.count(valReg) ? convert(reg2attr[valReg].type) : type;
                    if (exprType != type)
                    {
                        auto convInsts = createTypeConvertInst(exprType, type, valReg);
                        for (auto* inst : convInsts) insert(inst);
                        valReg = curFunc->getMaxReg();
                    }
                    ME::Operand* ptr = ME::OperandFactory::getInstance().getRegOperand(reg);
                    insert(createStoreInst(type, valReg, ptr));
                }
            }
            else
            {
                ME::Operand* ptr = ME::OperandFactory::getInstance().getRegOperand(reg);
                if (type == DataType::I64)
                {
                    size_t r32 = curFunc->getNewRegId();
                    insert(createArithmeticI32Inst_ImmeAll(Operator::ADD, 0, 0, r32));
                    size_t r64 = curFunc->getNewRegId();
                    insert(createZextInst(r32, r64, 32, 64));
                    insert(createStoreInst(type, r64, ptr));
                }
                else if (type == DataType::I32 || type == DataType::I1)
                {
                    ME::Operand* valOp = ME::OperandFactory::getInstance().getImmeI32Operand(0);
                    insert(createStoreInst(type, valOp, ptr));
                }
                else if (type == DataType::F32)
                {
                    ME::Operand* valOp = ME::OperandFactory::getInstance().getImmeF32Operand(0.0f);
                    insert(createStoreInst(type, valOp, ptr));
                }
            }
        }
    }

}  // namespace ME