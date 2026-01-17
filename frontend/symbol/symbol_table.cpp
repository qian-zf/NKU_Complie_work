#include <frontend/symbol/symbol_table.h>
#include <debug.h>
#include <map>

namespace FE::Sym
{
    //ջʽ���ű�
    struct Scope
    {
        std::map<Entry*, FE::AST::VarAttr> symbols;     //��ǰ������
        Scope* parent;                                  //ָ���������ָ��
        Scope(Scope* p = nullptr) : parent(p) {}
    };

    // ��ǰ������ָ��
    static Scope* currentScope = nullptr;

    //���÷��ű�
    void SymTable::reset_impl()
    {
        while (currentScope) {
            Scope* tmp = currentScope;
            currentScope = currentScope->parent;
            delete tmp;
        }
    }

    //������������
    void SymTable::enterScope_impl()
    {
        currentScope = new Scope(currentScope);
    }

    //�˳���ǰ������
    void SymTable::exitScope_impl()
    {
        if (currentScope) {
            Scope* tmp = currentScope;
            currentScope = currentScope->parent;
            delete tmp;
        }
    }

    //���ӷ��ŵ���ǰ������
    void SymTable::addSymbol_impl(Entry* entry, FE::AST::VarAttr& attr)
    {
        if (currentScope) {
            currentScope->symbols[entry] = attr;
        }
    }

    //��ȡ��������
    FE::AST::VarAttr* SymTable::getSymbol_impl(Entry* entry)
    {
        Scope* scope = currentScope;
        while (scope) {
            auto it = scope->symbols.find(entry);
            if (it != scope->symbols.end()) {
                return &it->second;
            }
            scope = scope->parent;
        }
        return nullptr;
    }

    bool SymTable::isGlobalScope_impl()
    {
        return currentScope && currentScope->parent == nullptr;
    }

    int SymTable::getScopeDepth_impl()
    {
        int depth = 0;
        Scope* scope = currentScope;
        while (scope) {
            depth++;
            scope = scope->parent;
        }
        return depth;
    }
}  // namespace FE::Sym
