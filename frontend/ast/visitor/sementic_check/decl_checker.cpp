#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>

namespace FE::AST
{
    bool ASTChecker::visit(Initializer& node)
    {
        // 示例实现：单个初始化器的语义检查
        // 1) 访问初始化值表达式
        // 2) 将子表达式的属性拷贝到当前节点
        ASSERT(node.init_val && "Null initializer value");
        bool res  = apply(*this, *node.init_val);
        node.attr = node.init_val->attr;
        return res;
    }

    bool ASTChecker::visit(InitializerList& node)
    {
        // 示例实现：初始化器列表的语义检查
        // 遍历列表中的每个初始化器并逐个访问
        if (!node.init_list) return true;
        bool res = true;
        for (auto* init : *(node.init_list))
        {
            if (!init) continue;
            res &= apply(*this, *init);
        }
        return res;
    }

    bool ASTChecker::visit(VarDeclarator& node)
    {
        bool res = true;
        
        if (!node.lval)
        {
            errors.push_back("Variable declarator missing left value at line " + std::to_string(node.line_num));
            res = false;
        }
        if (node.init) res &= apply(*this, *node.init);
        if (node.lval) node.attr = node.lval->attr;
        return res;
    }

    bool ASTChecker::visit(ParamDeclarator& node)
    {
        bool res = true;
        FE::AST::VarAttr* exist = symTable.getSymbol(node.entry);
        bool redef = exist && exist->scopeLevel == symTable.getScopeDepth();
        if (redef)
        {
            errors.push_back("Redefinition of parameter.");
            res = false;
        }

        FE::AST::VarAttr attr;
        attr.type       = node.type;
        attr.scopeLevel = symTable.getScopeDepth();

        if (!redef) symTable.addSymbol(node.entry, attr);
        return res;
    }

    bool ASTChecker::visit(VarDeclaration& node)
    {
        bool res = true;

        if (!node.decls) return true;

        for (auto* decl : *(node.decls))
        {
            if (!decl) continue;

            res &= apply(*this, *decl);

            auto* lval = dynamic_cast<LeftValExpr*>(decl->lval);
            if (!lval)
            {
                errors.push_back("Invalid left value in declaration at line " + std::to_string(decl->line_num));
                res = false;
                continue;
            }

            Entry* entry = lval->entry;
            if (!entry)
            {
                errors.push_back("LeftValExpr has no Entry at line " + std::to_string(decl->line_num));
                res = false;
                continue;
            }

            FE::AST::VarAttr* exist = symTable.getSymbol(entry);
            if (exist && exist->scopeLevel == symTable.getScopeDepth())
            {
                errors.push_back("Redefinition of variable '" + entry->getName() +
                    "' at line " + std::to_string(decl->line_num));
                res = false;
                continue;
            }

            FE::AST::VarAttr attr(node.type, node.isConstDecl, symTable.getScopeDepth());
            if (!attr.type) { ERROR("1"); }

            if (decl->init)
            {
                if (auto* single = dynamic_cast<Initializer*>(decl->init))
                {
                    if (single->init_val)
                    {
                        attr.initList.push_back(single->init_val->attr.val.value);
                        Type* initType = single->init_val->attr.val.value.type;
                        if (!initType || initType == voidType)
                        {
                            errors.push_back("Initializer has void type at line " + std::to_string(single->line_num));
                            res = false;
                            continue;
                        }
                    }
                }
            }

            symTable.addSymbol(entry, attr);
            if (symTable.isGlobalScope()) glbSymbols[entry] = attr;
        }

        return res;
    }
}  // namespace FE::AST
