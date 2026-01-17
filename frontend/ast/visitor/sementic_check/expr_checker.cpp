#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>

namespace FE::AST
{
    bool ASTChecker::visit(LeftValExpr& node)
    {
        bool   res   = true;
        Entry* entry = node.entry;
        auto*  attr  = symTable.getSymbol(entry);
        if (!attr)
        {
            errors.push_back("Use of undeclared variable '" + entry->getName() + "'");
            node.attr.val.value.type  = voidType;
            node.attr.val.isConstexpr = false;
            res                       = false;
        }
        node.isLval               = (entry != nullptr);
        node.attr.val.value.type  = attr ? (attr->type ? attr->type : FE::AST::TypeFactory::getBasicType(FE::AST::Type_t::UNK)) : voidType;
        node.attr.val.isConstexpr = attr && attr->isConstDecl && attr->arrayDims.empty() && !attr->initList.empty();
        if (node.attr.val.isConstexpr) node.attr.val.value = attr->initList.front();
        if (node.indices)
        {
            for (auto* idx : *node.indices)
            {
                if (!idx) continue;
                res &= apply(*this, *idx);
            }
        }
        return res;
    }

    bool ASTChecker::visit(LiteralExpr& node)
    {
        // 示例实现：字面量表达式的语义检查
        // 字面量总是编译期常量，直接设置属性
        node.attr.val.isConstexpr = true;
        node.attr.val.value       = node.literal;
        return true;
    }

    bool ASTChecker::visit(UnaryExpr& node)
    {
        bool res = true;
        if (!node.expr) return true;
        res &= apply(*this, *node.expr);
        Type* operandType = node.expr->attr.val.value.type;
        if (!operandType || operandType == voidType)
        {
            errors.push_back("Unary operator applied to void operand at line " + std::to_string(node.line_num));
            node.attr.val.value.type  = voidType;
            node.attr.val.isConstexpr = false;
            return false;
        }
        bool      hasError = false;
        ExprValue ev       = typeInfer(node.expr->attr.val, node.op, node, hasError);
        node.attr.val      = ev;
        if (hasError)
        {
            errors.push_back("Invalid unary expression at line " + std::to_string(node.line_num));
            res = false;
        }
        return res;
    }

    bool ASTChecker::visit(BinaryExpr& node)
    {
        if (!node.lhs || !node.rhs) return false;
        bool res = true;
        res &= apply(*this, *node.lhs);
        res &= apply(*this, *node.rhs);
        auto* lhsType = node.lhs->attr.val.value.type;
        auto* rhsType = node.rhs->attr.val.value.type;
        if (node.op == Operator::ASSIGN)
        {
            auto* lhsLval = dynamic_cast<LeftValExpr*>(node.lhs);
            if (!lhsLval)
            {
                errors.push_back("Left-hand side of assignment is not assignable at line " + std::to_string(node.line_num));
                res = false;
            }
            if (!rhsType || rhsType == voidType)
            {
                errors.push_back("Right-hand side of assignment has void type at line " + std::to_string(node.line_num));
                res = false;
            }
        }
        if ((node.op != Operator::ASSIGN) && ((lhsType && lhsType == voidType) || (rhsType && rhsType == voidType)))
        {
            errors.push_back("Binary operator applied to void operand at line " + std::to_string(node.line_num));
            res = false;
        }
        bool      hasError = false;
        ExprValue ev       = typeInfer(node.lhs->attr.val, node.rhs->attr.val, node.op, node, hasError);
        node.attr.val      = ev;
        if (hasError) { res = false; }
        return res;
    }

    bool ASTChecker::visit(CallExpr& node)
    {
        auto* entry = node.func;
        auto  it    = funcDecls.find(entry);
        bool          res    = true;
        FuncDeclStmt* decl   = nullptr;
        if (it == funcDecls.end())
        {
            errors.push_back("Call to undefined function '" + entry->getName() + "'");
            res = false;
        }
        else
        {
            decl = it->second;
        }
        size_t expect = decl && decl->params ? decl->params->size() : 0;
        size_t actual = node.args ? node.args->size() : 0;
        if (decl && expect != actual)
        {
            errors.push_back("Function '" + entry->getName() + "' expects " + std::to_string(expect) + " arguments but " +
                std::to_string(actual) + " provided");
            res = false;
        }
        if (node.args)
        {
            for (size_t idx = 0; idx < node.args->size(); ++idx)
            {
                auto* arg = (*node.args)[idx];
                if (!arg) continue;
                res &= apply(*this, *arg);
                Type* argType = arg->attr.val.value.type;
                if (decl && (!argType || argType == voidType))
                {
                    errors.push_back("Argument " + std::to_string(idx + 1) + " of function '" + entry->getName() +
                        "' has void type");
                    res = false;
                }
            }
        }
        node.attr.val.value.type  = decl ? (decl->retType ? decl->retType : voidType) : voidType;
        node.attr.val.isConstexpr = false;
        return res;
    }

    bool ASTChecker::visit(CommaExpr& node)
    {
        // TODO(Lab3-1): 实现逗号表达式的语义检查
        // 依序访问各子表达式，结果为最后一个表达式的属性
        bool res = true;
        if (!node.exprs || node.exprs->empty()) return true;
        for (auto* e : *node.exprs)
        {
            if (!e) continue;
            res &= apply(*this, *e);
        }
        ExprNode* last = node.exprs->back();
        if (last) node.attr.val = last->attr.val;
        return res;
    }
}  // namespace FE::AST
