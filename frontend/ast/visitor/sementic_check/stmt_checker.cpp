#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>

namespace FE::AST
{
    bool ASTChecker::visit(ExprStmt& node)
    {
        if (!node.expr) return true;
        return apply(*this, *node.expr);
    }

    bool ASTChecker::visit(FuncDeclStmt& node)
    {
        bool   res   = true;
        Entry* entry = node.entry;
        auto   it    = funcDecls.find(entry);
        if (it != funcDecls.end() && it->second->body && node.body)
        {
            errors.push_back("Redefinition of function '" + entry->getName() + "'");
            res = false;
        }
        funcDecls[entry] = &node;
        if (entry && entry->getName() == std::string("main"))
        {
            mainExists = true;
            if (node.retType != intType)
            {
                errors.push_back("Main function must return int.");
                res = false;
            }
            size_t paramCnt = node.params ? node.params->size() : 0;
            if (paramCnt != 0)
            {
                errors.push_back("Main function must not have parameters.");
                res = false;
            }
        }
        curFuncRetType = node.retType ? node.retType : voidType;
        funcHasReturn  = false;
        symTable.enterScope();
        if (node.params)
        {
            for (auto* param : *node.params)
            {
                if (!param) continue;
                res &= apply(*this, *param);
            }
        }
        if (node.body) res &= apply(*this, *node.body);
        symTable.exitScope();
        if (node.body && curFuncRetType != voidType && !funcHasReturn)
        {
            bool isMain = (entry && entry->getName() == std::string("main"));
            if (!isMain)
            {
                errors.push_back("Non-void function '" + entry->getName() + "' has no return statement.");
                res = false;
            }
        }
        return res;
    }

    bool ASTChecker::visit(VarDeclStmt& node)
    {
        if (!node.decl) return true;
        return apply(*this, *node.decl);
    }

    bool ASTChecker::visit(BlockStmt& node)
    {
        bool res = true;
        symTable.enterScope();
        if (!node.stmts)
        {
            symTable.exitScope();
            return true;
        }
        for (auto* stmt : *node.stmts)
        {
            if (!stmt) continue;
            res &= apply(*this, *stmt);
        }
        symTable.exitScope();
        return res;
    }

    bool ASTChecker::visit(ReturnStmt& node)
    {
        bool res = true;
        funcHasReturn = true;
        if (!node.retExpr)
        {
            if (curFuncRetType != voidType)
            {
                errors.push_back(
                    "Return without value in non-void function at line " + std::to_string(node.line_num));
                res = false;
            }
            return res;
        }
        res &= apply(*this, *node.retExpr);
        Type* exprType = node.retExpr->attr.val.value.type;
        if (curFuncRetType == voidType)
        {
            errors.push_back("Return with value in void function at line " + std::to_string(node.line_num));
            res = false;
        }
        if (res && (!exprType || exprType == voidType))
        {
            errors.push_back("Return expression has void type at line " + std::to_string(node.line_num));
            res = false;
        }
        return res;
    }

    bool ASTChecker::visit(WhileStmt& node)
    {
        bool res = true;
        if (node.cond) res &= apply(*this, *node.cond);
        loopDepth++;
        if (node.body) res &= apply(*this, *node.body);
        loopDepth--;
        return res;
    }

    bool ASTChecker::visit(IfStmt& node)
    {
        if (!node.cond) return true;
        if (!apply(*this, *node.cond)) return false;

        if (node.thenStmt) { if (!apply(*this, *node.thenStmt)) return false; }
        if (node.elseStmt) { if (!apply(*this, *node.elseStmt)) return false; }
        return true;
    }

    bool ASTChecker::visit(BreakStmt& node)
    {
        (void)node;
        if (loopDepth == 0)
        {
            errors.push_back("break statement not within loop at line " + std::to_string(node.line_num));
            return false;
        }
        return true;
    }

    bool ASTChecker::visit(ContinueStmt& node)
    {
        (void)node;
        if (loopDepth == 0)
        {
            errors.push_back("continue statement not within loop at line " + std::to_string(node.line_num));
            return false;
        }
        return true;
    }

    bool ASTChecker::visit(ForStmt& node)
    {
        bool res = true;
        symTable.enterScope();
        if (node.init) res &= apply(*this, *node.init);
        if (node.cond) res &= apply(*this, *node.cond);
        loopDepth++;
        if (node.body) res &= apply(*this, *node.body);
        loopDepth--;
        if (node.step) res &= apply(*this, *node.step);
        symTable.exitScope();
        return res;
    }
}  // namespace FE::AST
