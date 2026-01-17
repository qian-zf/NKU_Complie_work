// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

// "%code top" blocks.
#line 31 "frontend/parser/yacc.y"

#include <iostream>

#include <frontend/parser/parser.h>
#include <frontend/parser/location.hh>
#include <frontend/parser/scanner.h>
#include <frontend/parser/yacc.h>

using namespace FE;
using namespace FE::AST;

static YaccParser::symbol_type yylex(Scanner& scanner, Parser& parser)
{
    (void)parser;
    return scanner.nextToken();
}

extern size_t errCnt;

#line 59 "frontend/parser/yacc.cpp"

#include "yacc.h"

#ifndef YY_
#if defined YYENABLE_NLS && YYENABLE_NLS
#if ENABLE_NLS
#include <libintl.h>  // FIXME: INFRINGES ON USER NAME SPACE.
#define YY_(msgid) dgettext("bison-runtime", msgid)
#endif
#endif
#ifndef YY_
#define YY_(msgid) msgid
#endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
#if defined __GNUC__ && !defined __EXCEPTIONS
#define YY_EXCEPTIONS 0
#else
#define YY_EXCEPTIONS 1
#endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(Current, Rhs, N)                                  \
    do                                                                   \
        if (N)                                                           \
        {                                                                \
            (Current).begin = YYRHSLOC(Rhs, 1).begin;                    \
            (Current).end   = YYRHSLOC(Rhs, N).end;                      \
        }                                                                \
        else { (Current).begin = (Current).end = YYRHSLOC(Rhs, 0).end; } \
    while (false)
#endif

// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
#define YYCDEBUG \
    if (yydebug_) (*yycdebug_)

#define YY_SYMBOL_PRINT(Title, Symbol)     \
    do {                                   \
        if (yydebug_)                      \
        {                                  \
            *yycdebug_ << Title << ' ';    \
            yy_print_(*yycdebug_, Symbol); \
            *yycdebug_ << '\n';            \
        }                                  \
    } while (false)

#define YY_REDUCE_PRINT(Rule)                 \
    do {                                      \
        if (yydebug_) yy_reduce_print_(Rule); \
    } while (false)

#define YY_STACK_PRINT()                 \
    do {                                 \
        if (yydebug_) yy_stack_print_(); \
    } while (false)

#else  // !YYDEBUG

#define YYCDEBUG \
    if (false) std::cerr
#define YY_SYMBOL_PRINT(Title, Symbol) YY_USE(Symbol)
#define YY_REDUCE_PRINT(Rule) static_cast<void>(0)
#define YY_STACK_PRINT() static_cast<void>(0)

#endif  // !YYDEBUG

#define yyerrok (yyerrstatus_ = 0)
#define yyclearin (yyla.clear())

#define YYACCEPT goto yyacceptlab
#define YYABORT goto yyabortlab
#define YYERROR goto yyerrorlab
#define YYRECOVERING() (!!yyerrstatus_)

#line 4 "frontend/parser/yacc.y"
namespace FE
{
#line 159 "frontend/parser/yacc.cpp"

    /// Build a parser object.
    YaccParser ::YaccParser(FE::Scanner& scanner_yyarg, FE::Parser& parser_yyarg)
#if YYDEBUG
        : yydebug_(false),
          yycdebug_(&std::cerr),
#else
        :
#endif
          scanner(scanner_yyarg),
          parser(parser_yyarg)
    {}

    YaccParser ::~YaccParser() {}

    YaccParser ::syntax_error::~syntax_error() YY_NOEXCEPT YY_NOTHROW {}

    /*---------.
    | symbol.  |
    `---------*/

    // by_state.
    YaccParser ::by_state::by_state() YY_NOEXCEPT : state(empty_state) {}

    YaccParser ::by_state::by_state(const by_state& that) YY_NOEXCEPT : state(that.state) {}

    void YaccParser ::by_state::clear() YY_NOEXCEPT { state = empty_state; }

    void YaccParser ::by_state::move(by_state& that)
    {
        state = that.state;
        that.clear();
    }

    YaccParser ::by_state::by_state(state_type s) YY_NOEXCEPT : state(s) {}

    YaccParser ::symbol_kind_type YaccParser ::by_state::kind() const YY_NOEXCEPT
    {
        if (state == empty_state)
            return symbol_kind::S_YYEMPTY;
        else
            return YY_CAST(symbol_kind_type, yystos_[+state]);
    }

    YaccParser ::stack_symbol_type::stack_symbol_type() {}

    YaccParser ::stack_symbol_type::stack_symbol_type(YY_RVREF(stack_symbol_type) that)
        : super_type(YY_MOVE(that.state), YY_MOVE(that.location))
    {
        switch (that.kind())
        {
            case symbol_kind::S_ASSIGN_EXPR:           // ASSIGN_EXPR
            case symbol_kind::S_EXPR:                  // EXPR
            case symbol_kind::S_NOCOMMA_EXPR:          // NOCOMMA_EXPR
            case symbol_kind::S_LOGICAL_OR_EXPR:       // LOGICAL_OR_EXPR
            case symbol_kind::S_LOGICAL_AND_EXPR:      // LOGICAL_AND_EXPR
            case symbol_kind::S_EQUALITY_EXPR:         // EQUALITY_EXPR
            case symbol_kind::S_RELATIONAL_EXPR:       // RELATIONAL_EXPR
            case symbol_kind::S_ADDSUB_EXPR:           // ADDSUB_EXPR
            case symbol_kind::S_MULDIV_EXPR:           // MULDIV_EXPR
            case symbol_kind::S_UNARY_EXPR:            // UNARY_EXPR
            case symbol_kind::S_BASIC_EXPR:            // BASIC_EXPR
            case symbol_kind::S_FUNC_CALL_EXPR:        // FUNC_CALL_EXPR
            case symbol_kind::S_ARRAY_DIMENSION_EXPR:  // ARRAY_DIMENSION_EXPR
            case symbol_kind::S_LEFT_VAL_EXPR:         // LEFT_VAL_EXPR
            case symbol_kind::S_LITERAL_EXPR:          // LITERAL_EXPR
                value.YY_MOVE_OR_COPY<FE::AST::ExprNode*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_INITIALIZER:  // INITIALIZER
                value.YY_MOVE_OR_COPY<FE::AST::InitDecl*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_UNARY_OP:  // UNARY_OP
                value.YY_MOVE_OR_COPY<FE::AST::Operator>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_PARAM_DECLARATOR:  // PARAM_DECLARATOR
                value.YY_MOVE_OR_COPY<FE::AST::ParamDeclarator*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_PROGRAM:  // PROGRAM
                value.YY_MOVE_OR_COPY<FE::AST::Root*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_STMT:            // STMT
            case symbol_kind::S_CONTINUE_STMT:   // CONTINUE_STMT
            case symbol_kind::S_EXPR_STMT:       // EXPR_STMT
            case symbol_kind::S_VAR_DECL_STMT:   // VAR_DECL_STMT
            case symbol_kind::S_FUNC_BODY:       // FUNC_BODY
            case symbol_kind::S_FUNC_DECL_STMT:  // FUNC_DECL_STMT
            case symbol_kind::S_FOR_STMT:        // FOR_STMT
            case symbol_kind::S_IF_STMT:         // IF_STMT
            case symbol_kind::S_RETURN_STMT:     // RETURN_STMT
            case symbol_kind::S_WHILE_STMT:      // WHILE_STMT
            case symbol_kind::S_BREAK_STMT:      // BREAK_STMT
            case symbol_kind::S_BLOCK_STMT:      // BLOCK_STMT
                value.YY_MOVE_OR_COPY<FE::AST::StmtNode*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_TYPE:  // TYPE
                value.YY_MOVE_OR_COPY<FE::AST::Type*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_VAR_DECLARATION:  // VAR_DECLARATION
                value.YY_MOVE_OR_COPY<FE::AST::VarDeclaration*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_VAR_DECLARATOR:  // VAR_DECLARATOR
                value.YY_MOVE_OR_COPY<FE::AST::VarDeclarator*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_FLOAT_CONST:  // FLOAT_CONST
                value.YY_MOVE_OR_COPY<double>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_INT_CONST:  // INT_CONST
                value.YY_MOVE_OR_COPY<int>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_LL_CONST:  // LL_CONST
                value.YY_MOVE_OR_COPY<long long>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_STR_CONST:      // STR_CONST
            case symbol_kind::S_ERR_TOKEN:      // ERR_TOKEN
            case symbol_kind::S_IDENT:          // IDENT
            case symbol_kind::S_SLASH_COMMENT:  // SLASH_COMMENT
                value.YY_MOVE_OR_COPY<std::string>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_EXPR_LIST:                  // EXPR_LIST
            case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST:  // ARRAY_DIMENSION_EXPR_LIST
                value.YY_MOVE_OR_COPY<std::vector<FE::AST::ExprNode*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_INITIALIZER_LIST:  // INITIALIZER_LIST
                value.YY_MOVE_OR_COPY<std::vector<FE::AST::InitDecl*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_PARAM_DECLARATOR_LIST:  // PARAM_DECLARATOR_LIST
                value.YY_MOVE_OR_COPY<std::vector<FE::AST::ParamDeclarator*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_STMT_LIST:  // STMT_LIST
                value.YY_MOVE_OR_COPY<std::vector<FE::AST::StmtNode*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_VAR_DECLARATOR_LIST:  // VAR_DECLARATOR_LIST
                value.YY_MOVE_OR_COPY<std::vector<FE::AST::VarDeclarator*>*>(YY_MOVE(that.value));
                break;

            default: break;
        }

#if 201103L <= YY_CPLUSPLUS
        // that is emptied.
        that.state = empty_state;
#endif
    }

    YaccParser ::stack_symbol_type::stack_symbol_type(state_type s, YY_MOVE_REF(symbol_type) that)
        : super_type(s, YY_MOVE(that.location))
    {
        switch (that.kind())
        {
            case symbol_kind::S_ASSIGN_EXPR:           // ASSIGN_EXPR
            case symbol_kind::S_EXPR:                  // EXPR
            case symbol_kind::S_NOCOMMA_EXPR:          // NOCOMMA_EXPR
            case symbol_kind::S_LOGICAL_OR_EXPR:       // LOGICAL_OR_EXPR
            case symbol_kind::S_LOGICAL_AND_EXPR:      // LOGICAL_AND_EXPR
            case symbol_kind::S_EQUALITY_EXPR:         // EQUALITY_EXPR
            case symbol_kind::S_RELATIONAL_EXPR:       // RELATIONAL_EXPR
            case symbol_kind::S_ADDSUB_EXPR:           // ADDSUB_EXPR
            case symbol_kind::S_MULDIV_EXPR:           // MULDIV_EXPR
            case symbol_kind::S_UNARY_EXPR:            // UNARY_EXPR
            case symbol_kind::S_BASIC_EXPR:            // BASIC_EXPR
            case symbol_kind::S_FUNC_CALL_EXPR:        // FUNC_CALL_EXPR
            case symbol_kind::S_ARRAY_DIMENSION_EXPR:  // ARRAY_DIMENSION_EXPR
            case symbol_kind::S_LEFT_VAL_EXPR:         // LEFT_VAL_EXPR
            case symbol_kind::S_LITERAL_EXPR:          // LITERAL_EXPR
                value.move<FE::AST::ExprNode*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_INITIALIZER:  // INITIALIZER
                value.move<FE::AST::InitDecl*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_UNARY_OP:  // UNARY_OP
                value.move<FE::AST::Operator>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_PARAM_DECLARATOR:  // PARAM_DECLARATOR
                value.move<FE::AST::ParamDeclarator*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_PROGRAM:  // PROGRAM
                value.move<FE::AST::Root*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_STMT:            // STMT
            case symbol_kind::S_CONTINUE_STMT:   // CONTINUE_STMT
            case symbol_kind::S_EXPR_STMT:       // EXPR_STMT
            case symbol_kind::S_VAR_DECL_STMT:   // VAR_DECL_STMT
            case symbol_kind::S_FUNC_BODY:       // FUNC_BODY
            case symbol_kind::S_FUNC_DECL_STMT:  // FUNC_DECL_STMT
            case symbol_kind::S_FOR_STMT:        // FOR_STMT
            case symbol_kind::S_IF_STMT:         // IF_STMT
            case symbol_kind::S_RETURN_STMT:     // RETURN_STMT
            case symbol_kind::S_WHILE_STMT:      // WHILE_STMT
            case symbol_kind::S_BREAK_STMT:      // BREAK_STMT
            case symbol_kind::S_BLOCK_STMT:      // BLOCK_STMT
                value.move<FE::AST::StmtNode*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_TYPE:  // TYPE
                value.move<FE::AST::Type*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_VAR_DECLARATION:  // VAR_DECLARATION
                value.move<FE::AST::VarDeclaration*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_VAR_DECLARATOR:  // VAR_DECLARATOR
                value.move<FE::AST::VarDeclarator*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_FLOAT_CONST:  // FLOAT_CONST
                value.move<double>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_INT_CONST:  // INT_CONST
                value.move<int>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_LL_CONST:  // LL_CONST
                value.move<long long>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_STR_CONST:      // STR_CONST
            case symbol_kind::S_ERR_TOKEN:      // ERR_TOKEN
            case symbol_kind::S_IDENT:          // IDENT
            case symbol_kind::S_SLASH_COMMENT:  // SLASH_COMMENT
                value.move<std::string>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_EXPR_LIST:                  // EXPR_LIST
            case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST:  // ARRAY_DIMENSION_EXPR_LIST
                value.move<std::vector<FE::AST::ExprNode*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_INITIALIZER_LIST:  // INITIALIZER_LIST
                value.move<std::vector<FE::AST::InitDecl*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_PARAM_DECLARATOR_LIST:  // PARAM_DECLARATOR_LIST
                value.move<std::vector<FE::AST::ParamDeclarator*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_STMT_LIST:  // STMT_LIST
                value.move<std::vector<FE::AST::StmtNode*>*>(YY_MOVE(that.value));
                break;

            case symbol_kind::S_VAR_DECLARATOR_LIST:  // VAR_DECLARATOR_LIST
                value.move<std::vector<FE::AST::VarDeclarator*>*>(YY_MOVE(that.value));
                break;

            default: break;
        }

        // that is emptied.
        that.kind_ = symbol_kind::S_YYEMPTY;
    }

#if YY_CPLUSPLUS < 201103L
    YaccParser ::stack_symbol_type& YaccParser ::stack_symbol_type::operator=(const stack_symbol_type& that)
    {
        state = that.state;
        switch (that.kind())
        {
            case symbol_kind::S_ASSIGN_EXPR:           // ASSIGN_EXPR
            case symbol_kind::S_EXPR:                  // EXPR
            case symbol_kind::S_NOCOMMA_EXPR:          // NOCOMMA_EXPR
            case symbol_kind::S_LOGICAL_OR_EXPR:       // LOGICAL_OR_EXPR
            case symbol_kind::S_LOGICAL_AND_EXPR:      // LOGICAL_AND_EXPR
            case symbol_kind::S_EQUALITY_EXPR:         // EQUALITY_EXPR
            case symbol_kind::S_RELATIONAL_EXPR:       // RELATIONAL_EXPR
            case symbol_kind::S_ADDSUB_EXPR:           // ADDSUB_EXPR
            case symbol_kind::S_MULDIV_EXPR:           // MULDIV_EXPR
            case symbol_kind::S_UNARY_EXPR:            // UNARY_EXPR
            case symbol_kind::S_BASIC_EXPR:            // BASIC_EXPR
            case symbol_kind::S_FUNC_CALL_EXPR:        // FUNC_CALL_EXPR
            case symbol_kind::S_ARRAY_DIMENSION_EXPR:  // ARRAY_DIMENSION_EXPR
            case symbol_kind::S_LEFT_VAL_EXPR:         // LEFT_VAL_EXPR
            case symbol_kind::S_LITERAL_EXPR:          // LITERAL_EXPR
                value.copy<FE::AST::ExprNode*>(that.value);
                break;

            case symbol_kind::S_INITIALIZER:  // INITIALIZER
                value.copy<FE::AST::InitDecl*>(that.value);
                break;

            case symbol_kind::S_UNARY_OP:  // UNARY_OP
                value.copy<FE::AST::Operator>(that.value);
                break;

            case symbol_kind::S_PARAM_DECLARATOR:  // PARAM_DECLARATOR
                value.copy<FE::AST::ParamDeclarator*>(that.value);
                break;

            case symbol_kind::S_PROGRAM:  // PROGRAM
                value.copy<FE::AST::Root*>(that.value);
                break;

            case symbol_kind::S_STMT:            // STMT
            case symbol_kind::S_CONTINUE_STMT:   // CONTINUE_STMT
            case symbol_kind::S_EXPR_STMT:       // EXPR_STMT
            case symbol_kind::S_VAR_DECL_STMT:   // VAR_DECL_STMT
            case symbol_kind::S_FUNC_BODY:       // FUNC_BODY
            case symbol_kind::S_FUNC_DECL_STMT:  // FUNC_DECL_STMT
            case symbol_kind::S_FOR_STMT:        // FOR_STMT
            case symbol_kind::S_IF_STMT:         // IF_STMT
            case symbol_kind::S_RETURN_STMT:     // RETURN_STMT
            case symbol_kind::S_WHILE_STMT:      // WHILE_STMT
            case symbol_kind::S_BREAK_STMT:      // BREAK_STMT
            case symbol_kind::S_BLOCK_STMT:      // BLOCK_STMT
                value.copy<FE::AST::StmtNode*>(that.value);
                break;

            case symbol_kind::S_TYPE:  // TYPE
                value.copy<FE::AST::Type*>(that.value);
                break;

            case symbol_kind::S_VAR_DECLARATION:  // VAR_DECLARATION
                value.copy<FE::AST::VarDeclaration*>(that.value);
                break;

            case symbol_kind::S_VAR_DECLARATOR:  // VAR_DECLARATOR
                value.copy<FE::AST::VarDeclarator*>(that.value);
                break;

            case symbol_kind::S_FLOAT_CONST:  // FLOAT_CONST
                value.copy<double>(that.value);
                break;

            case symbol_kind::S_INT_CONST:  // INT_CONST
                value.copy<int>(that.value);
                break;

            case symbol_kind::S_LL_CONST:  // LL_CONST
                value.copy<long long>(that.value);
                break;

            case symbol_kind::S_STR_CONST:      // STR_CONST
            case symbol_kind::S_ERR_TOKEN:      // ERR_TOKEN
            case symbol_kind::S_IDENT:          // IDENT
            case symbol_kind::S_SLASH_COMMENT:  // SLASH_COMMENT
                value.copy<std::string>(that.value);
                break;

            case symbol_kind::S_EXPR_LIST:                  // EXPR_LIST
            case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST:  // ARRAY_DIMENSION_EXPR_LIST
                value.copy<std::vector<FE::AST::ExprNode*>*>(that.value);
                break;

            case symbol_kind::S_INITIALIZER_LIST:  // INITIALIZER_LIST
                value.copy<std::vector<FE::AST::InitDecl*>*>(that.value);
                break;

            case symbol_kind::S_PARAM_DECLARATOR_LIST:  // PARAM_DECLARATOR_LIST
                value.copy<std::vector<FE::AST::ParamDeclarator*>*>(that.value);
                break;

            case symbol_kind::S_STMT_LIST:  // STMT_LIST
                value.copy<std::vector<FE::AST::StmtNode*>*>(that.value);
                break;

            case symbol_kind::S_VAR_DECLARATOR_LIST:  // VAR_DECLARATOR_LIST
                value.copy<std::vector<FE::AST::VarDeclarator*>*>(that.value);
                break;

            default: break;
        }

        location = that.location;
        return *this;
    }

    YaccParser ::stack_symbol_type& YaccParser ::stack_symbol_type::operator=(stack_symbol_type& that)
    {
        state = that.state;
        switch (that.kind())
        {
            case symbol_kind::S_ASSIGN_EXPR:           // ASSIGN_EXPR
            case symbol_kind::S_EXPR:                  // EXPR
            case symbol_kind::S_NOCOMMA_EXPR:          // NOCOMMA_EXPR
            case symbol_kind::S_LOGICAL_OR_EXPR:       // LOGICAL_OR_EXPR
            case symbol_kind::S_LOGICAL_AND_EXPR:      // LOGICAL_AND_EXPR
            case symbol_kind::S_EQUALITY_EXPR:         // EQUALITY_EXPR
            case symbol_kind::S_RELATIONAL_EXPR:       // RELATIONAL_EXPR
            case symbol_kind::S_ADDSUB_EXPR:           // ADDSUB_EXPR
            case symbol_kind::S_MULDIV_EXPR:           // MULDIV_EXPR
            case symbol_kind::S_UNARY_EXPR:            // UNARY_EXPR
            case symbol_kind::S_BASIC_EXPR:            // BASIC_EXPR
            case symbol_kind::S_FUNC_CALL_EXPR:        // FUNC_CALL_EXPR
            case symbol_kind::S_ARRAY_DIMENSION_EXPR:  // ARRAY_DIMENSION_EXPR
            case symbol_kind::S_LEFT_VAL_EXPR:         // LEFT_VAL_EXPR
            case symbol_kind::S_LITERAL_EXPR:          // LITERAL_EXPR
                value.move<FE::AST::ExprNode*>(that.value);
                break;

            case symbol_kind::S_INITIALIZER:  // INITIALIZER
                value.move<FE::AST::InitDecl*>(that.value);
                break;

            case symbol_kind::S_UNARY_OP:  // UNARY_OP
                value.move<FE::AST::Operator>(that.value);
                break;

            case symbol_kind::S_PARAM_DECLARATOR:  // PARAM_DECLARATOR
                value.move<FE::AST::ParamDeclarator*>(that.value);
                break;

            case symbol_kind::S_PROGRAM:  // PROGRAM
                value.move<FE::AST::Root*>(that.value);
                break;

            case symbol_kind::S_STMT:            // STMT
            case symbol_kind::S_CONTINUE_STMT:   // CONTINUE_STMT
            case symbol_kind::S_EXPR_STMT:       // EXPR_STMT
            case symbol_kind::S_VAR_DECL_STMT:   // VAR_DECL_STMT
            case symbol_kind::S_FUNC_BODY:       // FUNC_BODY
            case symbol_kind::S_FUNC_DECL_STMT:  // FUNC_DECL_STMT
            case symbol_kind::S_FOR_STMT:        // FOR_STMT
            case symbol_kind::S_IF_STMT:         // IF_STMT
            case symbol_kind::S_RETURN_STMT:     // RETURN_STMT
            case symbol_kind::S_WHILE_STMT:      // WHILE_STMT
            case symbol_kind::S_BREAK_STMT:      // BREAK_STMT
            case symbol_kind::S_BLOCK_STMT:      // BLOCK_STMT
                value.move<FE::AST::StmtNode*>(that.value);
                break;

            case symbol_kind::S_TYPE:  // TYPE
                value.move<FE::AST::Type*>(that.value);
                break;

            case symbol_kind::S_VAR_DECLARATION:  // VAR_DECLARATION
                value.move<FE::AST::VarDeclaration*>(that.value);
                break;

            case symbol_kind::S_VAR_DECLARATOR:  // VAR_DECLARATOR
                value.move<FE::AST::VarDeclarator*>(that.value);
                break;

            case symbol_kind::S_FLOAT_CONST:  // FLOAT_CONST
                value.move<double>(that.value);
                break;

            case symbol_kind::S_INT_CONST:  // INT_CONST
                value.move<int>(that.value);
                break;

            case symbol_kind::S_LL_CONST:  // LL_CONST
                value.move<long long>(that.value);
                break;

            case symbol_kind::S_STR_CONST:      // STR_CONST
            case symbol_kind::S_ERR_TOKEN:      // ERR_TOKEN
            case symbol_kind::S_IDENT:          // IDENT
            case symbol_kind::S_SLASH_COMMENT:  // SLASH_COMMENT
                value.move<std::string>(that.value);
                break;

            case symbol_kind::S_EXPR_LIST:                  // EXPR_LIST
            case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST:  // ARRAY_DIMENSION_EXPR_LIST
                value.move<std::vector<FE::AST::ExprNode*>*>(that.value);
                break;

            case symbol_kind::S_INITIALIZER_LIST:  // INITIALIZER_LIST
                value.move<std::vector<FE::AST::InitDecl*>*>(that.value);
                break;

            case symbol_kind::S_PARAM_DECLARATOR_LIST:  // PARAM_DECLARATOR_LIST
                value.move<std::vector<FE::AST::ParamDeclarator*>*>(that.value);
                break;

            case symbol_kind::S_STMT_LIST:  // STMT_LIST
                value.move<std::vector<FE::AST::StmtNode*>*>(that.value);
                break;

            case symbol_kind::S_VAR_DECLARATOR_LIST:  // VAR_DECLARATOR_LIST
                value.move<std::vector<FE::AST::VarDeclarator*>*>(that.value);
                break;

            default: break;
        }

        location = that.location;
        // that is emptied.
        that.state = empty_state;
        return *this;
    }
#endif

    template <typename Base>
    void YaccParser ::yy_destroy_(const char* yymsg, basic_symbol<Base>& yysym) const
    {
        if (yymsg) YY_SYMBOL_PRINT(yymsg, yysym);
    }

#if YYDEBUG
    template <typename Base>
    void YaccParser ::yy_print_(std::ostream& yyo, const basic_symbol<Base>& yysym) const
    {
        std::ostream& yyoutput = yyo;
        YY_USE(yyoutput);
        if (yysym.empty())
            yyo << "empty symbol";
        else
        {
            symbol_kind_type yykind = yysym.kind();
            yyo << (yykind < YYNTOKENS ? "token" : "nterm") << ' ' << yysym.name() << " (" << yysym.location << ": ";
            switch (yykind)
            {
                case symbol_kind::S_LL_CONST:  // LL_CONST
#line 61 "frontend/parser/yacc.y"
                {
                    yyo << yysym.value.template as<long long>();
                }
#line 715 "frontend/parser/yacc.cpp"
                break;

                default: break;
            }
            yyo << ')';
        }
    }
#endif

    void YaccParser ::yypush_(const char* m, YY_MOVE_REF(stack_symbol_type) sym)
    {
        if (m) YY_SYMBOL_PRINT(m, sym);
        yystack_.push(YY_MOVE(sym));
    }

    void YaccParser ::yypush_(const char* m, state_type s, YY_MOVE_REF(symbol_type) sym)
    {
#if 201103L <= YY_CPLUSPLUS
        yypush_(m, stack_symbol_type(s, std::move(sym)));
#else
        stack_symbol_type ss(s, sym);
        yypush_(m, ss);
#endif
    }

    void YaccParser ::yypop_(int n) YY_NOEXCEPT { yystack_.pop(n); }

#if YYDEBUG
    std::ostream& YaccParser ::debug_stream() const { return *yycdebug_; }

    void YaccParser ::set_debug_stream(std::ostream& o) { yycdebug_ = &o; }

    YaccParser ::debug_level_type YaccParser ::debug_level() const { return yydebug_; }

    void YaccParser ::set_debug_level(debug_level_type l) { yydebug_ = l; }
#endif  // YYDEBUG

    YaccParser ::state_type YaccParser ::yy_lr_goto_state_(state_type yystate, int yysym)
    {
        int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
        if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
            return yytable_[yyr];
        else
            return yydefgoto_[yysym - YYNTOKENS];
    }

    bool YaccParser ::yy_pact_value_is_default_(int yyvalue) YY_NOEXCEPT { return yyvalue == yypact_ninf_; }

    bool YaccParser ::yy_table_value_is_error_(int yyvalue) YY_NOEXCEPT { return yyvalue == yytable_ninf_; }

    int YaccParser ::operator()() { return parse(); }

    int YaccParser ::parse()
    {
        int yyn;
        /// Length of the RHS of the rule being reduced.
        int yylen = 0;

        // Error handling.
        int yynerrs_     = 0;
        int yyerrstatus_ = 0;

        /// The lookahead symbol.
        symbol_type yyla;

        /// The locations where the error started and ended.
        stack_symbol_type yyerror_range[3];

        /// The return value of parse ().
        int yyresult;

#if YY_EXCEPTIONS
        try
#endif  // YY_EXCEPTIONS
        {
            YYCDEBUG << "Starting parse\n";

            /* Initialize the stack.  The initial state will be set in
               yynewstate, since the latter expects the semantical and the
               location values to have been already stored, initialize these
               stacks with a primary value.  */
            yystack_.clear();
            yypush_(YY_NULLPTR, 0, YY_MOVE(yyla));

        /*-----------------------------------------------.
        | yynewstate -- push a new symbol on the stack.  |
        `-----------------------------------------------*/
        yynewstate:
            YYCDEBUG << "Entering state " << int(yystack_[0].state) << '\n';
            YY_STACK_PRINT();

            // Accept?
            if (yystack_[0].state == yyfinal_) YYACCEPT;

            goto yybackup;

        /*-----------.
        | yybackup.  |
        `-----------*/
        yybackup:
            // Try to take a decision without lookahead.
            yyn = yypact_[+yystack_[0].state];
            if (yy_pact_value_is_default_(yyn)) goto yydefault;

            // Read a lookahead token.
            if (yyla.empty())
            {
                YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
                try
#endif  // YY_EXCEPTIONS
                {
                    symbol_type yylookahead(yylex(scanner, parser));
                    yyla.move(yylookahead);
                }
#if YY_EXCEPTIONS
                catch (const syntax_error& yyexc)
                {
                    YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
                    error(yyexc);
                    goto yyerrlab1;
                }
#endif  // YY_EXCEPTIONS
            }
            YY_SYMBOL_PRINT("Next token is", yyla);

            if (yyla.kind() == symbol_kind::S_YYerror)
            {
                // The scanner already issued an error message, process directly
                // to error recovery.  But do not keep the error token as
                // lookahead, it is too special and may lead us to an endless
                // loop in error recovery. */
                yyla.kind_ = symbol_kind::S_YYUNDEF;
                goto yyerrlab1;
            }

            /* If the proper action on seeing token YYLA.TYPE is to reduce or
               to detect an error, take that action.  */
            yyn += yyla.kind();
            if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind()) { goto yydefault; }

            // Reduce or error.
            yyn = yytable_[yyn];
            if (yyn <= 0)
            {
                if (yy_table_value_is_error_(yyn)) goto yyerrlab;
                yyn = -yyn;
                goto yyreduce;
            }

            // Count tokens shifted since error; after three, turn off error status.
            if (yyerrstatus_) --yyerrstatus_;

            // Shift the lookahead token.
            yypush_("Shifting", state_type(yyn), YY_MOVE(yyla));
            goto yynewstate;

        /*-----------------------------------------------------------.
        | yydefault -- do the default action for the current state.  |
        `-----------------------------------------------------------*/
        yydefault:
            yyn = yydefact_[+yystack_[0].state];
            if (yyn == 0) goto yyerrlab;
            goto yyreduce;

        /*-----------------------------.
        | yyreduce -- do a reduction.  |
        `-----------------------------*/
        yyreduce:
            yylen = yyr2_[yyn];
            {
                stack_symbol_type yylhs;
                yylhs.state = yy_lr_goto_state_(yystack_[yylen].state, yyr1_[yyn]);
                /* Variants are always initialized to an empty instance of the
                   correct type. The default '$$ = $1' action is NOT applied
                   when using variants.  */
                switch (yyr1_[yyn])
                {
                    case symbol_kind::S_ASSIGN_EXPR:           // ASSIGN_EXPR
                    case symbol_kind::S_EXPR:                  // EXPR
                    case symbol_kind::S_NOCOMMA_EXPR:          // NOCOMMA_EXPR
                    case symbol_kind::S_LOGICAL_OR_EXPR:       // LOGICAL_OR_EXPR
                    case symbol_kind::S_LOGICAL_AND_EXPR:      // LOGICAL_AND_EXPR
                    case symbol_kind::S_EQUALITY_EXPR:         // EQUALITY_EXPR
                    case symbol_kind::S_RELATIONAL_EXPR:       // RELATIONAL_EXPR
                    case symbol_kind::S_ADDSUB_EXPR:           // ADDSUB_EXPR
                    case symbol_kind::S_MULDIV_EXPR:           // MULDIV_EXPR
                    case symbol_kind::S_UNARY_EXPR:            // UNARY_EXPR
                    case symbol_kind::S_BASIC_EXPR:            // BASIC_EXPR
                    case symbol_kind::S_FUNC_CALL_EXPR:        // FUNC_CALL_EXPR
                    case symbol_kind::S_ARRAY_DIMENSION_EXPR:  // ARRAY_DIMENSION_EXPR
                    case symbol_kind::S_LEFT_VAL_EXPR:         // LEFT_VAL_EXPR
                    case symbol_kind::S_LITERAL_EXPR:          // LITERAL_EXPR
                        yylhs.value.emplace<FE::AST::ExprNode*>();
                        break;

                    case symbol_kind::S_INITIALIZER:  // INITIALIZER
                        yylhs.value.emplace<FE::AST::InitDecl*>();
                        break;

                    case symbol_kind::S_UNARY_OP:  // UNARY_OP
                        yylhs.value.emplace<FE::AST::Operator>();
                        break;

                    case symbol_kind::S_PARAM_DECLARATOR:  // PARAM_DECLARATOR
                        yylhs.value.emplace<FE::AST::ParamDeclarator*>();
                        break;

                    case symbol_kind::S_PROGRAM:  // PROGRAM
                        yylhs.value.emplace<FE::AST::Root*>();
                        break;

                    case symbol_kind::S_STMT:            // STMT
                    case symbol_kind::S_CONTINUE_STMT:   // CONTINUE_STMT
                    case symbol_kind::S_EXPR_STMT:       // EXPR_STMT
                    case symbol_kind::S_VAR_DECL_STMT:   // VAR_DECL_STMT
                    case symbol_kind::S_FUNC_BODY:       // FUNC_BODY
                    case symbol_kind::S_FUNC_DECL_STMT:  // FUNC_DECL_STMT
                    case symbol_kind::S_FOR_STMT:        // FOR_STMT
                    case symbol_kind::S_IF_STMT:         // IF_STMT
                    case symbol_kind::S_RETURN_STMT:     // RETURN_STMT
                    case symbol_kind::S_WHILE_STMT:      // WHILE_STMT
                    case symbol_kind::S_BREAK_STMT:      // BREAK_STMT
                    case symbol_kind::S_BLOCK_STMT:      // BLOCK_STMT
                        yylhs.value.emplace<FE::AST::StmtNode*>();
                        break;

                    case symbol_kind::S_TYPE:  // TYPE
                        yylhs.value.emplace<FE::AST::Type*>();
                        break;

                    case symbol_kind::S_VAR_DECLARATION:  // VAR_DECLARATION
                        yylhs.value.emplace<FE::AST::VarDeclaration*>();
                        break;

                    case symbol_kind::S_VAR_DECLARATOR:  // VAR_DECLARATOR
                        yylhs.value.emplace<FE::AST::VarDeclarator*>();
                        break;

                    case symbol_kind::S_FLOAT_CONST:  // FLOAT_CONST
                        yylhs.value.emplace<double>();
                        break;

                    case symbol_kind::S_INT_CONST:  // INT_CONST
                        yylhs.value.emplace<int>();
                        break;

                    case symbol_kind::S_LL_CONST:  // LL_CONST
                        yylhs.value.emplace<long long>();
                        break;

                    case symbol_kind::S_STR_CONST:      // STR_CONST
                    case symbol_kind::S_ERR_TOKEN:      // ERR_TOKEN
                    case symbol_kind::S_IDENT:          // IDENT
                    case symbol_kind::S_SLASH_COMMENT:  // SLASH_COMMENT
                        yylhs.value.emplace<std::string>();
                        break;

                    case symbol_kind::S_EXPR_LIST:                  // EXPR_LIST
                    case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST:  // ARRAY_DIMENSION_EXPR_LIST
                        yylhs.value.emplace<std::vector<FE::AST::ExprNode*>*>();
                        break;

                    case symbol_kind::S_INITIALIZER_LIST:  // INITIALIZER_LIST
                        yylhs.value.emplace<std::vector<FE::AST::InitDecl*>*>();
                        break;

                    case symbol_kind::S_PARAM_DECLARATOR_LIST:  // PARAM_DECLARATOR_LIST
                        yylhs.value.emplace<std::vector<FE::AST::ParamDeclarator*>*>();
                        break;

                    case symbol_kind::S_STMT_LIST:  // STMT_LIST
                        yylhs.value.emplace<std::vector<FE::AST::StmtNode*>*>();
                        break;

                    case symbol_kind::S_VAR_DECLARATOR_LIST:  // VAR_DECLARATOR_LIST
                        yylhs.value.emplace<std::vector<FE::AST::VarDeclarator*>*>();
                        break;

                    default: break;
                }

                // Default location.
                {
                    stack_type::slice range(yystack_, yylen);
                    YYLLOC_DEFAULT(yylhs.location, range, yylen);
                    yyerror_range[1].location = yylhs.location;
                }

                // Perform the reduction.
                YY_REDUCE_PRINT(yyn);
#if YY_EXCEPTIONS
                try
#endif  // YY_EXCEPTIONS
                {
                    switch (yyn)
                    {
                        case 2:  // PROGRAM: STMT_LIST
#line 189 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Root*>() =
                                new Root(yystack_[0].value.as<std::vector<FE::AST::StmtNode*>*>());
                            parser.ast = yylhs.value.as<FE::AST::Root*>();
                        }
#line 1072 "frontend/parser/yacc.cpp"
                        break;

                        case 3:  // PROGRAM: PROGRAM END
#line 193 "frontend/parser/yacc.y"
                        {
                            YYACCEPT;
                        }
#line 1080 "frontend/parser/yacc.cpp"
                        break;

                        case 4:  // STMT_LIST: STMT
#line 199 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::StmtNode*>*>() = new std::vector<StmtNode*>();
                            if (yystack_[0].value.as<FE::AST::StmtNode*>())
                                yylhs.value.as<std::vector<FE::AST::StmtNode*>*>()->push_back(
                                    yystack_[0].value.as<FE::AST::StmtNode*>());
                        }
#line 1089 "frontend/parser/yacc.cpp"
                        break;

                        case 5:  // STMT_LIST: STMT_LIST STMT
#line 203 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::StmtNode*>*>() =
                                yystack_[1].value.as<std::vector<FE::AST::StmtNode*>*>();
                            if (yystack_[0].value.as<FE::AST::StmtNode*>())
                                yylhs.value.as<std::vector<FE::AST::StmtNode*>*>()->push_back(
                                    yystack_[0].value.as<FE::AST::StmtNode*>());
                        }
#line 1098 "frontend/parser/yacc.cpp"
                        break;

                        case 6:  // STMT: EXPR_STMT
#line 210 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1106 "frontend/parser/yacc.cpp"
                        break;

                        case 7:  // STMT: VAR_DECL_STMT
#line 213 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1114 "frontend/parser/yacc.cpp"
                        break;

                        case 8:  // STMT: IF_STMT
#line 216 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1122 "frontend/parser/yacc.cpp"
                        break;

                        case 9:  // STMT: FOR_STMT
#line 219 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1130 "frontend/parser/yacc.cpp"
                        break;

                        case 10:  // STMT: WHILE_STMT
#line 222 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1138 "frontend/parser/yacc.cpp"
                        break;

                        case 11:  // STMT: BREAK_STMT
#line 225 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1146 "frontend/parser/yacc.cpp"
                        break;

                        case 12:  // STMT: CONTINUE_STMT
#line 228 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1154 "frontend/parser/yacc.cpp"
                        break;

                        case 13:  // STMT: RETURN_STMT
#line 231 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1162 "frontend/parser/yacc.cpp"
                        break;

                        case 14:  // STMT: BLOCK_STMT
#line 234 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1170 "frontend/parser/yacc.cpp"
                        break;

                        case 15:  // STMT: FUNC_DECL_STMT
#line 237 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = yystack_[0].value.as<FE::AST::StmtNode*>();
                        }
#line 1178 "frontend/parser/yacc.cpp"
                        break;

                        case 16:  // STMT: SEMICOLON
#line 240 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = nullptr;
                        }
#line 1186 "frontend/parser/yacc.cpp"
                        break;

                        case 17:  // STMT: SLASH_COMMENT
#line 243 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = nullptr;
                        }
#line 1194 "frontend/parser/yacc.cpp"
                        break;

                        case 18:  // CONTINUE_STMT: CONTINUE SEMICOLON
#line 250 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new ContinueStmt(yystack_[1].location.begin.line, yystack_[1].location.begin.column);
                        }
#line 1202 "frontend/parser/yacc.cpp"
                        break;

                        case 19:  // EXPR_STMT: EXPR SEMICOLON
#line 256 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new ExprStmt(yystack_[1].value.as<FE::AST::ExprNode*>(),
                                    yystack_[1].location.begin.line,
                                    yystack_[1].location.begin.column);
                        }
#line 1210 "frontend/parser/yacc.cpp"
                        break;

                        case 20:  // VAR_DECLARATION: TYPE VAR_DECLARATOR_LIST
#line 262 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::VarDeclaration*>() =
                                new VarDeclaration(yystack_[1].value.as<FE::AST::Type*>(),
                                    yystack_[0].value.as<std::vector<FE::AST::VarDeclarator*>*>(),
                                    false,
                                    yystack_[1].location.begin.line,
                                    yystack_[1].location.begin.column);
                        }
#line 1218 "frontend/parser/yacc.cpp"
                        break;

                        case 21:  // VAR_DECLARATION: CONST TYPE VAR_DECLARATOR_LIST
#line 265 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::VarDeclaration*>() =
                                new VarDeclaration(yystack_[1].value.as<FE::AST::Type*>(),
                                    yystack_[0].value.as<std::vector<FE::AST::VarDeclarator*>*>(),
                                    true,
                                    yystack_[2].location.begin.line,
                                    yystack_[2].location.begin.column);
                        }
#line 1226 "frontend/parser/yacc.cpp"
                        break;

                        case 22:  // VAR_DECL_STMT: VAR_DECLARATION SEMICOLON
#line 271 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new VarDeclStmt(yystack_[1].value.as<FE::AST::VarDeclaration*>(),
                                    yystack_[1].location.begin.line,
                                    yystack_[1].location.begin.column);
                        }
#line 1234 "frontend/parser/yacc.cpp"
                        break;

                        case 23:  // FUNC_BODY: LBRACE RBRACE
#line 279 "frontend/parser/yacc.y"
                        {
                            auto vec = new std::vector<StmtNode*>();
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new BlockStmt(vec, yystack_[1].location.begin.line, yystack_[1].location.begin.column);
                        }
#line 1243 "frontend/parser/yacc.cpp"
                        break;

                        case 24:  // FUNC_BODY: LBRACE STMT_LIST RBRACE
#line 283 "frontend/parser/yacc.y"
                        {
                            if (!yystack_[1].value.as<std::vector<FE::AST::StmtNode*>*>())
                            {
                                auto vec                             = new std::vector<StmtNode*>();
                                yylhs.value.as<FE::AST::StmtNode*>() = new BlockStmt(
                                    vec, yystack_[2].location.begin.line, yystack_[2].location.begin.column);
                            }
                            else
                            {
                                yylhs.value.as<FE::AST::StmtNode*>() =
                                    new BlockStmt(yystack_[1].value.as<std::vector<FE::AST::StmtNode*>*>(),
                                        yystack_[2].location.begin.line,
                                        yystack_[2].location.begin.column);
                            }
                        }
#line 1256 "frontend/parser/yacc.cpp"
                        break;

                        case 25:  // FUNC_DECL_STMT: TYPE IDENT LPAREN PARAM_DECLARATOR_LIST RPAREN FUNC_BODY
#line 294 "frontend/parser/yacc.y"
                        {
                            Entry* entry = Entry::getEntry(yystack_[4].value.as<std::string>());
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new FuncDeclStmt(yystack_[5].value.as<FE::AST::Type*>(),
                                    entry,
                                    yystack_[2].value.as<std::vector<FE::AST::ParamDeclarator*>*>(),
                                    yystack_[0].value.as<FE::AST::StmtNode*>(),
                                    yystack_[5].location.begin.line,
                                    yystack_[5].location.begin.column);
                        }
#line 1265 "frontend/parser/yacc.cpp"
                        break;

                        case 26:  // FOR_STMT: FOR LPAREN VAR_DECLARATION SEMICOLON EXPR SEMICOLON EXPR RPAREN STMT
#line 301 "frontend/parser/yacc.y"
                        {
                            VarDeclStmt* initStmt = new VarDeclStmt(yystack_[6].value.as<FE::AST::VarDeclaration*>(),
                                yystack_[6].location.begin.line,
                                yystack_[6].location.begin.column);
                            yylhs.value.as<FE::AST::StmtNode*>() = new ForStmt(initStmt,
                                yystack_[4].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::StmtNode*>(),
                                yystack_[8].location.begin.line,
                                yystack_[8].location.begin.column);
                        }
#line 1274 "frontend/parser/yacc.cpp"
                        break;

                        case 27:  // FOR_STMT: FOR LPAREN EXPR SEMICOLON EXPR SEMICOLON EXPR RPAREN STMT
#line 305 "frontend/parser/yacc.y"
                        {
                            StmtNode* initStmt = new ExprStmt(yystack_[6].value.as<FE::AST::ExprNode*>(),
                                yystack_[6].value.as<FE::AST::ExprNode*>()->line_num,
                                yystack_[6].value.as<FE::AST::ExprNode*>()->col_num);
                            yylhs.value.as<FE::AST::StmtNode*>() = new ForStmt(initStmt,
                                yystack_[4].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::StmtNode*>(),
                                yystack_[8].location.begin.line,
                                yystack_[8].location.begin.column);
                        }
#line 1283 "frontend/parser/yacc.cpp"
                        break;

                        case 28:  // IF_STMT: IF LPAREN EXPR RPAREN STMT
#line 312 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new IfStmt(yystack_[2].value.as<FE::AST::ExprNode*>(),
                                    yystack_[0].value.as<FE::AST::StmtNode*>(),
                                    nullptr,
                                    yystack_[4].location.begin.line,
                                    yystack_[4].location.begin.column);
                        }
#line 1291 "frontend/parser/yacc.cpp"
                        break;

                        case 29:  // IF_STMT: IF LPAREN EXPR RPAREN STMT ELSE STMT
#line 315 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new IfStmt(yystack_[4].value.as<FE::AST::ExprNode*>(),
                                    yystack_[2].value.as<FE::AST::StmtNode*>(),
                                    yystack_[0].value.as<FE::AST::StmtNode*>(),
                                    yystack_[6].location.begin.line,
                                    yystack_[6].location.begin.column);
                        }
#line 1299 "frontend/parser/yacc.cpp"
                        break;

                        case 30:  // RETURN_STMT: RETURN SEMICOLON
#line 321 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() = new ReturnStmt(
                                nullptr, yystack_[1].location.begin.line, yystack_[1].location.begin.column);
                        }
#line 1307 "frontend/parser/yacc.cpp"
                        break;

                        case 31:  // RETURN_STMT: RETURN EXPR SEMICOLON
#line 324 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new ReturnStmt(yystack_[1].value.as<FE::AST::ExprNode*>(),
                                    yystack_[2].location.begin.line,
                                    yystack_[2].location.begin.column);
                        }
#line 1315 "frontend/parser/yacc.cpp"
                        break;

                        case 32:  // WHILE_STMT: WHILE LPAREN EXPR RPAREN STMT
#line 330 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new WhileStmt(yystack_[2].value.as<FE::AST::ExprNode*>(),
                                    yystack_[0].value.as<FE::AST::StmtNode*>(),
                                    yystack_[4].location.begin.line,
                                    yystack_[4].location.begin.column);
                        }
#line 1323 "frontend/parser/yacc.cpp"
                        break;

                        case 33:  // BREAK_STMT: BREAK SEMICOLON
#line 336 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new BreakStmt(yystack_[1].location.begin.line, yystack_[1].location.begin.column);
                        }
#line 1331 "frontend/parser/yacc.cpp"
                        break;

                        case 34:  // CONTINUE_STMT: CONTINUE SEMICOLON
#line 342 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new ContinueStmt(yystack_[1].location.begin.line, yystack_[1].location.begin.column);
                        }
#line 1339 "frontend/parser/yacc.cpp"
                        break;

                        case 35:  // BLOCK_STMT: LBRACE RBRACE
#line 348 "frontend/parser/yacc.y"
                        {
                            auto vec = new std::vector<StmtNode*>();
                            yylhs.value.as<FE::AST::StmtNode*>() =
                                new BlockStmt(vec, yystack_[1].location.begin.line, yystack_[1].location.begin.column);
                        }
#line 1348 "frontend/parser/yacc.cpp"
                        break;

                        case 36:  // BLOCK_STMT: LBRACE STMT_LIST RBRACE
#line 352 "frontend/parser/yacc.y"
                        {
                            if (!yystack_[1].value.as<std::vector<FE::AST::StmtNode*>*>())
                            {
                                auto vec                             = new std::vector<StmtNode*>();
                                yylhs.value.as<FE::AST::StmtNode*>() = new BlockStmt(
                                    vec, yystack_[2].location.begin.line, yystack_[2].location.begin.column);
                            }
                            else
                            {
                                yylhs.value.as<FE::AST::StmtNode*>() =
                                    new BlockStmt(yystack_[1].value.as<std::vector<FE::AST::StmtNode*>*>(),
                                        yystack_[2].location.begin.line,
                                        yystack_[2].location.begin.column);
                            }
                        }
#line 1361 "frontend/parser/yacc.cpp"
                        break;

                        case 37:  // PARAM_DECLARATOR: TYPE IDENT
#line 365 "frontend/parser/yacc.y"
                        {
                            Entry* entry = Entry::getEntry(yystack_[0].value.as<std::string>());
                            yylhs.value.as<FE::AST::ParamDeclarator*>() =
                                new ParamDeclarator(yystack_[1].value.as<FE::AST::Type*>(),
                                    entry,
                                    nullptr,
                                    yystack_[1].location.begin.line,
                                    yystack_[1].location.begin.column);
                        }
#line 1370 "frontend/parser/yacc.cpp"
                        break;

                        case 38:  // PARAM_DECLARATOR: TYPE IDENT LBRACKET RBRACKET
#line 370 "frontend/parser/yacc.y"
                        {
                            std::vector<ExprNode*>* dim = new std::vector<ExprNode*>();
                            dim->emplace_back(new LiteralExpr(
                                -1, yystack_[1].location.begin.line, yystack_[1].location.begin.column));
                            Entry* entry = Entry::getEntry(yystack_[2].value.as<std::string>());
                            yylhs.value.as<FE::AST::ParamDeclarator*>() =
                                new ParamDeclarator(yystack_[3].value.as<FE::AST::Type*>(),
                                    entry,
                                    dim,
                                    yystack_[3].location.begin.line,
                                    yystack_[3].location.begin.column);
                        }
#line 1381 "frontend/parser/yacc.cpp"
                        break;

                        case 39:  // PARAM_DECLARATOR: TYPE IDENT ARRAY_DIMENSION_EXPR_LIST
#line 379 "frontend/parser/yacc.y"
                        {
                            Entry* entry = Entry::getEntry(yystack_[1].value.as<std::string>());
                            yylhs.value.as<FE::AST::ParamDeclarator*>() =
                                new ParamDeclarator(yystack_[2].value.as<FE::AST::Type*>(),
                                    entry,
                                    yystack_[0].value.as<std::vector<FE::AST::ExprNode*>*>(),
                                    yystack_[2].location.begin.line,
                                    yystack_[2].location.begin.column);
                        }
#line 1390 "frontend/parser/yacc.cpp"
                        break;

                        case 40:  // PARAM_DECLARATOR: TYPE IDENT LBRACKET RBRACKET ARRAY_DIMENSION_EXPR_LIST
#line 386 "frontend/parser/yacc.y"
                        {
                            /* 第一维空 []，后面跟剩余维度 */
                            yystack_[0].value.as<std::vector<FE::AST::ExprNode*>*>()->insert(
                                yystack_[0].value.as<std::vector<FE::AST::ExprNode*>*>()->begin(),
                                new LiteralExpr(
                                    -1, yystack_[2].location.begin.line, yystack_[2].location.begin.column));
                            Entry* entry = Entry::getEntry(yystack_[3].value.as<std::string>());
                            yylhs.value.as<FE::AST::ParamDeclarator*>() =
                                new ParamDeclarator(yystack_[4].value.as<FE::AST::Type*>(),
                                    entry,
                                    yystack_[0].value.as<std::vector<FE::AST::ExprNode*>*>(),
                                    yystack_[4].location.begin.line,
                                    yystack_[4].location.begin.column);
                        }
#line 1402 "frontend/parser/yacc.cpp"
                        break;

                        case 41:  // PARAM_DECLARATOR_LIST: %empty
#line 398 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::ParamDeclarator*>*>() =
                                new std::vector<ParamDeclarator*>();
                        }
#line 1410 "frontend/parser/yacc.cpp"
                        break;

                        case 42:  // PARAM_DECLARATOR_LIST: PARAM_DECLARATOR
#line 402 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::ParamDeclarator*>*>() =
                                new std::vector<ParamDeclarator*>();
                            yylhs.value.as<std::vector<FE::AST::ParamDeclarator*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::ParamDeclarator*>());
                        }
#line 1419 "frontend/parser/yacc.cpp"
                        break;

                        case 43:  // PARAM_DECLARATOR_LIST: PARAM_DECLARATOR_LIST COMMA PARAM_DECLARATOR
#line 406 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::ParamDeclarator*>*>() =
                                yystack_[2].value.as<std::vector<FE::AST::ParamDeclarator*>*>();
                            yylhs.value.as<std::vector<FE::AST::ParamDeclarator*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::ParamDeclarator*>());
                        }
#line 1428 "frontend/parser/yacc.cpp"
                        break;

                        case 44:  // VAR_DECLARATOR: LEFT_VAL_EXPR
#line 413 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::VarDeclarator*>() =
                                new VarDeclarator(yystack_[0].value.as<FE::AST::ExprNode*>(),
                                    nullptr,
                                    yystack_[0].location.begin.line,
                                    yystack_[0].location.begin.column);
                        }
#line 1436 "frontend/parser/yacc.cpp"
                        break;

                        case 45:  // VAR_DECLARATOR: LEFT_VAL_EXPR ASSIGN INITIALIZER
#line 417 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::VarDeclarator*>() =
                                new VarDeclarator(yystack_[2].value.as<FE::AST::ExprNode*>(),
                                    yystack_[0].value.as<FE::AST::InitDecl*>(),
                                    yystack_[2].location.begin.line,
                                    yystack_[2].location.begin.column);
                        }
#line 1444 "frontend/parser/yacc.cpp"
                        break;

                        case 46:  // VAR_DECLARATOR_LIST: VAR_DECLARATOR
#line 423 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::VarDeclarator*>*>() = new std::vector<VarDeclarator*>();
                            yylhs.value.as<std::vector<FE::AST::VarDeclarator*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::VarDeclarator*>());
                        }
#line 1453 "frontend/parser/yacc.cpp"
                        break;

                        case 47:  // VAR_DECLARATOR_LIST: VAR_DECLARATOR_LIST COMMA VAR_DECLARATOR
#line 427 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::VarDeclarator*>*>() =
                                yystack_[2].value.as<std::vector<FE::AST::VarDeclarator*>*>();
                            yylhs.value.as<std::vector<FE::AST::VarDeclarator*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::VarDeclarator*>());
                        }
#line 1462 "frontend/parser/yacc.cpp"
                        break;

                        case 48:  // INITIALIZER: ASSIGN_EXPR
#line 434 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::InitDecl*>() =
                                new Initializer(yystack_[0].value.as<FE::AST::ExprNode*>(),
                                    yystack_[0].location.begin.line,
                                    yystack_[0].location.begin.column);
                        }
#line 1470 "frontend/parser/yacc.cpp"
                        break;

                        case 49:  // INITIALIZER: LBRACE INITIALIZER_LIST RBRACE
#line 438 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::InitDecl*>() =
                                new InitializerList(yystack_[1].value.as<std::vector<FE::AST::InitDecl*>*>(),
                                    yystack_[2].location.begin.line,
                                    yystack_[2].location.begin.column);
                        }
#line 1478 "frontend/parser/yacc.cpp"
                        break;

                        case 50:  // INITIALIZER_LIST: INITIALIZER
#line 444 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::InitDecl*>*>() = new std::vector<InitDecl*>();
                            yylhs.value.as<std::vector<FE::AST::InitDecl*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::InitDecl*>());
                        }
#line 1487 "frontend/parser/yacc.cpp"
                        break;

                        case 51:  // INITIALIZER_LIST: INITIALIZER_LIST COMMA INITIALIZER
#line 448 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::InitDecl*>*>() =
                                yystack_[2].value.as<std::vector<FE::AST::InitDecl*>*>();
                            yylhs.value.as<std::vector<FE::AST::InitDecl*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::InitDecl*>());
                        }
#line 1496 "frontend/parser/yacc.cpp"
                        break;

                        case 52:  // ASSIGN_EXPR: LOGICAL_OR_EXPR
#line 455 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1504 "frontend/parser/yacc.cpp"
                        break;

                        case 53:  // ASSIGN_EXPR: LEFT_VAL_EXPR ASSIGN ASSIGN_EXPR
#line 458 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::ASSIGN,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1512 "frontend/parser/yacc.cpp"
                        break;

                        case 54:  // EXPR_LIST: NOCOMMA_EXPR
#line 464 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>() = new std::vector<ExprNode*>();
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::ExprNode*>());
                        }
#line 1521 "frontend/parser/yacc.cpp"
                        break;

                        case 55:  // EXPR_LIST: EXPR_LIST COMMA NOCOMMA_EXPR
#line 468 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>() =
                                yystack_[2].value.as<std::vector<FE::AST::ExprNode*>*>();
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::ExprNode*>());
                        }
#line 1530 "frontend/parser/yacc.cpp"
                        break;

                        case 56:  // EXPR: NOCOMMA_EXPR
#line 475 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1538 "frontend/parser/yacc.cpp"
                        break;

                        case 57:  // EXPR: EXPR COMMA NOCOMMA_EXPR
#line 478 "frontend/parser/yacc.y"
                        {
                            if (yystack_[2].value.as<FE::AST::ExprNode*>()->isCommaExpr())
                            {
                                CommaExpr* ce = static_cast<CommaExpr*>(yystack_[2].value.as<FE::AST::ExprNode*>());
                                ce->exprs->push_back(yystack_[0].value.as<FE::AST::ExprNode*>());
                                yylhs.value.as<FE::AST::ExprNode*>() = ce;
                            }
                            else
                            {
                                auto vec = new std::vector<ExprNode*>();
                                vec->push_back(yystack_[2].value.as<FE::AST::ExprNode*>());
                                vec->push_back(yystack_[0].value.as<FE::AST::ExprNode*>());
                                yylhs.value.as<FE::AST::ExprNode*>() = new CommaExpr(vec,
                                    yystack_[2].value.as<FE::AST::ExprNode*>()->line_num,
                                    yystack_[2].value.as<FE::AST::ExprNode*>()->col_num);
                            }
                        }
#line 1555 "frontend/parser/yacc.cpp"
                        break;

                        case 58:  // NOCOMMA_EXPR: ASSIGN_EXPR
#line 493 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1563 "frontend/parser/yacc.cpp"
                        break;

                        case 59:  // LOGICAL_OR_EXPR: LOGICAL_AND_EXPR
#line 501 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1571 "frontend/parser/yacc.cpp"
                        break;

                        case 60:  // LOGICAL_OR_EXPR: LOGICAL_OR_EXPR OR LOGICAL_AND_EXPR
#line 504 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::OR,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1579 "frontend/parser/yacc.cpp"
                        break;

                        case 61:  // LOGICAL_AND_EXPR: EQUALITY_EXPR
#line 510 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1587 "frontend/parser/yacc.cpp"
                        break;

                        case 62:  // LOGICAL_AND_EXPR: LOGICAL_AND_EXPR AND LOGICAL_AND_EXPR
#line 513 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::AND,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1595 "frontend/parser/yacc.cpp"
                        break;

                        case 63:  // EQUALITY_EXPR: RELATIONAL_EXPR
#line 519 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1603 "frontend/parser/yacc.cpp"
                        break;

                        case 64:  // EQUALITY_EXPR: EQUALITY_EXPR EQ EQUALITY_EXPR
#line 522 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::EQ,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1611 "frontend/parser/yacc.cpp"
                        break;

                        case 65:  // EQUALITY_EXPR: EQUALITY_EXPR NEQ EQUALITY_EXPR
#line 525 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::NEQ,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1619 "frontend/parser/yacc.cpp"
                        break;

                        case 66:  // RELATIONAL_EXPR: ADDSUB_EXPR
#line 532 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1627 "frontend/parser/yacc.cpp"
                        break;

                        case 67:  // RELATIONAL_EXPR: RELATIONAL_EXPR GT RELATIONAL_EXPR
#line 535 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::GT,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1635 "frontend/parser/yacc.cpp"
                        break;

                        case 68:  // RELATIONAL_EXPR: RELATIONAL_EXPR GE RELATIONAL_EXPR
#line 538 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::GE,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1643 "frontend/parser/yacc.cpp"
                        break;

                        case 69:  // RELATIONAL_EXPR: RELATIONAL_EXPR LT RELATIONAL_EXPR
#line 541 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::LT,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1651 "frontend/parser/yacc.cpp"
                        break;

                        case 70:  // RELATIONAL_EXPR: RELATIONAL_EXPR LE RELATIONAL_EXPR
#line 544 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::LE,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1659 "frontend/parser/yacc.cpp"
                        break;

                        case 71:  // ADDSUB_EXPR: MULDIV_EXPR
#line 551 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1667 "frontend/parser/yacc.cpp"
                        break;

                        case 72:  // ADDSUB_EXPR: ADDSUB_EXPR PLUS ADDSUB_EXPR
#line 554 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::ADD,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1675 "frontend/parser/yacc.cpp"
                        break;

                        case 73:  // ADDSUB_EXPR: ADDSUB_EXPR MINUS ADDSUB_EXPR
#line 557 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::SUB,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1683 "frontend/parser/yacc.cpp"
                        break;

                        case 74:  // MULDIV_EXPR: UNARY_EXPR
#line 564 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1691 "frontend/parser/yacc.cpp"
                        break;

                        case 75:  // MULDIV_EXPR: MULDIV_EXPR MUL MULDIV_EXPR
#line 567 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::MUL,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1699 "frontend/parser/yacc.cpp"
                        break;

                        case 76:  // MULDIV_EXPR: MULDIV_EXPR DIV MULDIV_EXPR
#line 570 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::DIV,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1707 "frontend/parser/yacc.cpp"
                        break;

                        case 77:  // MULDIV_EXPR: MULDIV_EXPR MOD MULDIV_EXPR
#line 574 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new BinaryExpr(Operator::MOD,
                                yystack_[2].value.as<FE::AST::ExprNode*>(),
                                yystack_[0].value.as<FE::AST::ExprNode*>(),
                                yystack_[2].location.begin.line,
                                yystack_[2].location.begin.column);
                        }
#line 1715 "frontend/parser/yacc.cpp"
                        break;

                        case 78:  // UNARY_EXPR: BASIC_EXPR
#line 579 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1723 "frontend/parser/yacc.cpp"
                        break;

                        case 79:  // UNARY_EXPR: UNARY_OP UNARY_EXPR
#line 582 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() =
                                new UnaryExpr(yystack_[1].value.as<FE::AST::Operator>(),
                                    yystack_[0].value.as<FE::AST::ExprNode*>(),
                                    yystack_[0].value.as<FE::AST::ExprNode*>()->line_num,
                                    yystack_[0].value.as<FE::AST::ExprNode*>()->col_num);
                        }
#line 1731 "frontend/parser/yacc.cpp"
                        break;

                        case 80:  // BASIC_EXPR: LITERAL_EXPR
#line 588 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1739 "frontend/parser/yacc.cpp"
                        break;

                        case 81:  // BASIC_EXPR: LEFT_VAL_EXPR
#line 591 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1747 "frontend/parser/yacc.cpp"
                        break;

                        case 82:  // BASIC_EXPR: LPAREN EXPR RPAREN
#line 594 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[1].value.as<FE::AST::ExprNode*>();
                        }
#line 1755 "frontend/parser/yacc.cpp"
                        break;

                        case 83:  // BASIC_EXPR: FUNC_CALL_EXPR
#line 597 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = yystack_[0].value.as<FE::AST::ExprNode*>();
                        }
#line 1763 "frontend/parser/yacc.cpp"
                        break;

                        case 84:  // FUNC_CALL_EXPR: IDENT LPAREN RPAREN
#line 603 "frontend/parser/yacc.y"
                        {
                            std::string funcName = yystack_[2].value.as<std::string>();
                            if (funcName != "starttime" && funcName != "stoptime")
                            {
                                Entry* entry                         = Entry::getEntry(funcName);
                                yylhs.value.as<FE::AST::ExprNode*>() = new CallExpr(
                                    entry, nullptr, yystack_[2].location.begin.line, yystack_[2].location.begin.column);
                            }
                            else
                            {
                                funcName                     = "_sysy_" + funcName;
                                std::vector<ExprNode*>* args = new std::vector<ExprNode*>();
                                args->emplace_back(new LiteralExpr(static_cast<int>(yystack_[2].location.begin.line),
                                    yystack_[2].location.begin.line,
                                    yystack_[2].location.begin.column));
                                yylhs.value.as<FE::AST::ExprNode*>() = new CallExpr(Entry::getEntry(funcName),
                                    args,
                                    yystack_[2].location.begin.line,
                                    yystack_[2].location.begin.column);
                            }
                        }
#line 1783 "frontend/parser/yacc.cpp"
                        break;

                        case 85:  // FUNC_CALL_EXPR: IDENT LPAREN EXPR_LIST RPAREN
#line 618 "frontend/parser/yacc.y"
                        {
                            Entry* entry                         = Entry::getEntry(yystack_[3].value.as<std::string>());
                            yylhs.value.as<FE::AST::ExprNode*>() = new CallExpr(entry,
                                yystack_[1].value.as<std::vector<FE::AST::ExprNode*>*>(),
                                yystack_[3].location.begin.line,
                                yystack_[3].location.begin.column);
                        }
#line 1792 "frontend/parser/yacc.cpp"
                        break;

                        case 86:  // ARRAY_DIMENSION_EXPR: LBRACKET NOCOMMA_EXPR RBRACKET
#line 625 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() =
                                yystack_[1].value.as<FE::AST::ExprNode*>();  // 已知维度表达式
                        }
#line 1800 "frontend/parser/yacc.cpp"
                        break;

                        case 87:  // ARRAY_DIMENSION_EXPR_LIST: ARRAY_DIMENSION_EXPR
#line 631 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>() = new std::vector<ExprNode*>();
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::ExprNode*>());
                        }
#line 1809 "frontend/parser/yacc.cpp"
                        break;

                        case 88:  // ARRAY_DIMENSION_EXPR_LIST: ARRAY_DIMENSION_EXPR_LIST ARRAY_DIMENSION_EXPR
#line 635 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>() =
                                yystack_[1].value.as<std::vector<FE::AST::ExprNode*>*>();
                            yylhs.value.as<std::vector<FE::AST::ExprNode*>*>()->push_back(
                                yystack_[0].value.as<FE::AST::ExprNode*>());
                        }
#line 1818 "frontend/parser/yacc.cpp"
                        break;

                        case 89:  // LEFT_VAL_EXPR: IDENT
#line 643 "frontend/parser/yacc.y"
                        {
                            Entry* entry                         = Entry::getEntry(yystack_[0].value.as<std::string>());
                            yylhs.value.as<FE::AST::ExprNode*>() = new LeftValExpr(
                                entry, nullptr, yystack_[0].location.begin.line, yystack_[0].location.begin.column);
                        }
#line 1827 "frontend/parser/yacc.cpp"
                        break;

                        case 90:  // LEFT_VAL_EXPR: IDENT ARRAY_DIMENSION_EXPR_LIST
#line 647 "frontend/parser/yacc.y"
                        {
                            Entry* entry                         = Entry::getEntry(yystack_[1].value.as<std::string>());
                            yylhs.value.as<FE::AST::ExprNode*>() = new LeftValExpr(entry,
                                yystack_[0].value.as<std::vector<FE::AST::ExprNode*>*>(),
                                yystack_[1].location.begin.line,
                                yystack_[1].location.begin.column);
                        }
#line 1836 "frontend/parser/yacc.cpp"
                        break;

                        case 91:  // LITERAL_EXPR: INT_CONST
#line 654 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() = new LiteralExpr((int)yystack_[0].value.as<int>(),
                                yystack_[0].location.begin.line,
                                yystack_[0].location.begin.column);
                        }
#line 1844 "frontend/parser/yacc.cpp"
                        break;

                        case 92:  // LITERAL_EXPR: FLOAT_CONST
#line 657 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() =
                                new LiteralExpr((float)yystack_[0].value.as<double>(),
                                    yystack_[0].location.begin.line,
                                    yystack_[0].location.begin.column);
                        }
#line 1852 "frontend/parser/yacc.cpp"
                        break;

                        case 93:  // LITERAL_EXPR: LL_CONST
#line 660 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::ExprNode*>() =
                                new LiteralExpr((long long)yystack_[0].value.as<long long>(),
                                    yystack_[0].location.begin.line,
                                    yystack_[0].location.begin.column);
                        }
#line 1860 "frontend/parser/yacc.cpp"
                        break;

                        case 94:  // TYPE: INT
#line 666 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Type*>() = intType;
                        }
#line 1868 "frontend/parser/yacc.cpp"
                        break;

                        case 95:  // TYPE: FLOAT
#line 669 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Type*>() = floatType;
                        }
#line 1876 "frontend/parser/yacc.cpp"
                        break;

                        case 96:  // TYPE: VOID
#line 672 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Type*>() = voidType;
                        }
#line 1884 "frontend/parser/yacc.cpp"
                        break;

                        case 97:  // TYPE: LL
#line 675 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Type*>() = llType;
                        }
#line 1892 "frontend/parser/yacc.cpp"
                        break;

                        case 98:  // UNARY_OP: PLUS
#line 681 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Operator>() = Operator::ADD;  // TODO: 添加一元加
                        }
#line 1900 "frontend/parser/yacc.cpp"
                        break;

                        case 99:  // UNARY_OP: MINUS
#line 684 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Operator>() = Operator::SUB;  // 一元减
                        }
#line 1908 "frontend/parser/yacc.cpp"
                        break;

                        case 100:  // UNARY_OP: NOT
#line 687 "frontend/parser/yacc.y"
                        {
                            yylhs.value.as<FE::AST::Operator>() = Operator::NOT;  // 逻辑非
                        }
#line 1916 "frontend/parser/yacc.cpp"
                        break;

#line 1920 "frontend/parser/yacc.cpp"

                        default: break;
                    }
                }
#if YY_EXCEPTIONS
                catch (const syntax_error& yyexc)
                {
                    YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
                    error(yyexc);
                    YYERROR;
                }
#endif  // YY_EXCEPTIONS
                YY_SYMBOL_PRINT("-> $$ =", yylhs);
                yypop_(yylen);
                yylen = 0;

                // Shift the result of the reduction.
                yypush_(YY_NULLPTR, YY_MOVE(yylhs));
            }
            goto yynewstate;

        /*--------------------------------------.
        | yyerrlab -- here on detecting error.  |
        `--------------------------------------*/
        yyerrlab:
            // If not already recovering from an error, report this error.
            if (!yyerrstatus_)
            {
                ++yynerrs_;
                context     yyctx(*this, yyla);
                std::string msg = yysyntax_error_(yyctx);
                error(yyla.location, YY_MOVE(msg));
            }

            yyerror_range[1].location = yyla.location;
            if (yyerrstatus_ == 3)
            {
                /* If just tried and failed to reuse lookahead token after an
                   error, discard it.  */

                // Return failure if at end of input.
                if (yyla.kind() == symbol_kind::S_YYEOF)
                    YYABORT;
                else if (!yyla.empty())
                {
                    yy_destroy_("Error: discarding", yyla);
                    yyla.clear();
                }
            }

            // Else will try to reuse lookahead token after shifting the error token.
            goto yyerrlab1;

        /*---------------------------------------------------.
        | yyerrorlab -- error raised explicitly by YYERROR.  |
        `---------------------------------------------------*/
        yyerrorlab:
            /* Pacify compilers when the user code never invokes YYERROR and
               the label yyerrorlab therefore never appears in user code.  */
            if (false) YYERROR;

            /* Do not reclaim the symbols of the rule whose action triggered
               this YYERROR.  */
            yypop_(yylen);
            yylen = 0;
            YY_STACK_PRINT();
            goto yyerrlab1;

        /*-------------------------------------------------------------.
        | yyerrlab1 -- common code for both syntax error and YYERROR.  |
        `-------------------------------------------------------------*/
        yyerrlab1:
            yyerrstatus_ = 3;  // Each real token shifted decrements this.
            // Pop stack until we find a state that shifts the error token.
            for (;;)
            {
                yyn = yypact_[+yystack_[0].state];
                if (!yy_pact_value_is_default_(yyn))
                {
                    yyn += symbol_kind::S_YYerror;
                    if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == symbol_kind::S_YYerror)
                    {
                        yyn = yytable_[yyn];
                        if (0 < yyn) break;
                    }
                }

                // Pop the current state because it cannot handle the error token.
                if (yystack_.size() == 1) YYABORT;

                yyerror_range[1].location = yystack_[0].location;
                yy_destroy_("Error: popping", yystack_[0]);
                yypop_();
                YY_STACK_PRINT();
            }
            {
                stack_symbol_type error_token;

                yyerror_range[2].location = yyla.location;
                YYLLOC_DEFAULT(error_token.location, yyerror_range, 2);

                // Shift the error token.
                error_token.state = state_type(yyn);
                yypush_("Shifting", YY_MOVE(error_token));
            }
            goto yynewstate;

        /*-------------------------------------.
        | yyacceptlab -- YYACCEPT comes here.  |
        `-------------------------------------*/
        yyacceptlab:
            yyresult = 0;
            goto yyreturn;

        /*-----------------------------------.
        | yyabortlab -- YYABORT comes here.  |
        `-----------------------------------*/
        yyabortlab:
            yyresult = 1;
            goto yyreturn;

        /*-----------------------------------------------------.
        | yyreturn -- parsing is finished, return the result.  |
        `-----------------------------------------------------*/
        yyreturn:
            if (!yyla.empty()) yy_destroy_("Cleanup: discarding lookahead", yyla);

            /* Do not reclaim the symbols of the rule whose action triggered
               this YYABORT or YYACCEPT.  */
            yypop_(yylen);
            YY_STACK_PRINT();
            while (1 < yystack_.size())
            {
                yy_destroy_("Cleanup: popping", yystack_[0]);
                yypop_();
            }

            return yyresult;
        }
#if YY_EXCEPTIONS
        catch (...)
        {
            YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
            // Do not try to display the values of the reclaimed symbols,
            // as their printers might throw an exception.
            if (!yyla.empty()) yy_destroy_(YY_NULLPTR, yyla);

            while (1 < yystack_.size())
            {
                yy_destroy_(YY_NULLPTR, yystack_[0]);
                yypop_();
            }
            throw;
        }
#endif  // YY_EXCEPTIONS
    }

    void YaccParser ::error(const syntax_error& yyexc) { error(yyexc.location, yyexc.what()); }

    /* Return YYSTR after stripping away unnecessary quotes and
       backslashes, so that it's suitable for yyerror.  The heuristic is
       that double-quoting is unnecessary unless the string contains an
       apostrophe, a comma, or backslash (other than backslash-backslash).
       YYSTR is taken from yytname.  */
    std::string YaccParser ::yytnamerr_(const char* yystr)
    {
        if (*yystr == '"')
        {
            std::string yyr;
            char const* yyp = yystr;

            for (;;) switch (*++yyp)
                {
                    case '\'':
                    case ',': goto do_not_strip_quotes;

                    case '\\':
                        if (*++yyp != '\\')
                            goto do_not_strip_quotes;
                        else
                            goto append;

                    append:
                    default: yyr += *yyp; break;

                    case '"': return yyr;
                }
        do_not_strip_quotes:;
        }

        return yystr;
    }

    std::string YaccParser ::symbol_name(symbol_kind_type yysymbol) { return yytnamerr_(yytname_[yysymbol]); }

    //  YaccParser ::context.
    YaccParser ::context::context(const YaccParser& yyparser, const symbol_type& yyla)
        : yyparser_(yyparser), yyla_(yyla)
    {}

    int YaccParser ::context::expected_tokens(symbol_kind_type yyarg[], int yyargn) const
    {
        // Actual number of expected tokens
        int yycount = 0;

        const int yyn = yypact_[+yyparser_.yystack_[0].state];
        if (!yy_pact_value_is_default_(yyn))
        {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            const int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            const int yychecklim = yylast_ - yyn + 1;
            const int yyxend     = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
                if (yycheck_[yyx + yyn] == yyx && yyx != symbol_kind::S_YYerror &&
                    !yy_table_value_is_error_(yytable_[yyx + yyn]))
                {
                    if (!yyarg)
                        ++yycount;
                    else if (yycount == yyargn)
                        return 0;
                    else
                        yyarg[yycount++] = YY_CAST(symbol_kind_type, yyx);
                }
        }

        if (yyarg && yycount == 0 && 0 < yyargn) yyarg[0] = symbol_kind::S_YYEMPTY;
        return yycount;
    }

    int YaccParser ::yy_syntax_error_arguments_(const context& yyctx, symbol_kind_type yyarg[], int yyargn) const
    {
        /* There are many possibilities here to consider:
           - If this state is a consistent state with a default action, then
             the only way this function was invoked is if the default action
             is an error action.  In that case, don't check for expected
             tokens because there are none.
           - The only way there can be no lookahead present (in yyla) is
             if this state is a consistent state with a default action.
             Thus, detecting the absence of a lookahead is sufficient to
             determine that there is no unexpected or expected token to
             report.  In that case, just report a simple "syntax error".
           - Don't assume there isn't a lookahead just because this state is
             a consistent state with a default action.  There might have
             been a previous inconsistent state, consistent state with a
             non-default action, or user semantic action that manipulated
             yyla.  (However, yyla is currently not documented for users.)
           - Of course, the expected token list depends on states to have
             correct lookahead information, and it depends on the parser not
             to perform extra reductions after fetching a lookahead from the
             scanner and before detecting a syntax error.  Thus, state merging
             (from LALR or IELR) and default reductions corrupt the expected
             token list.  However, the list is correct for canonical LR with
             one exception: it will still contain any token that will not be
             accepted due to an error action in a later state.
        */

        if (!yyctx.lookahead().empty())
        {
            if (yyarg) yyarg[0] = yyctx.token();
            int yyn = yyctx.expected_tokens(yyarg ? yyarg + 1 : yyarg, yyargn - 1);
            return yyn + 1;
        }
        return 0;
    }

    // Generate an error message.
    std::string YaccParser ::yysyntax_error_(const context& yyctx) const
    {
        // Its maximum.
        enum
        {
            YYARGS_MAX = 5
        };
        // Arguments of yyformat.
        symbol_kind_type yyarg[YYARGS_MAX];
        int              yycount = yy_syntax_error_arguments_(yyctx, yyarg, YYARGS_MAX);

        char const* yyformat = YY_NULLPTR;
        switch (yycount)
        {
#define YYCASE_(N, S) \
    case N: yyformat = S; break
            default:  // Avoid compiler warnings.
                YYCASE_(0, YY_("syntax error"));
                YYCASE_(1, YY_("syntax error, unexpected %s"));
                YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
                YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
                YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
                YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
        }

        std::string yyres;
        // Argument number.
        std::ptrdiff_t yyi = 0;
        for (char const* yyp = yyformat; *yyp; ++yyp)
            if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
            {
                yyres += symbol_name(yyarg[yyi++]);
                ++yyp;
            }
            else
                yyres += *yyp;
        return yyres;
    }

    const short YaccParser ::yypact_ninf_ = -138;

    const signed char YaccParser ::yytable_ninf_ = -1;

    const short YaccParser ::yypact_[] = {283,
        -138,
        -138,
        -138,
        0,
        -138,
        -138,
        -138,
        -138,
        4,
        -20,
        -11,
        21,
        26,
        56,
        318,
        -138,
        -138,
        377,
        143,
        -138,
        -138,
        -138,
        13,
        283,
        -138,
        -138,
        -138,
        67,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        59,
        -138,
        -31,
        33,
        51,
        -23,
        63,
        -13,
        -138,
        -138,
        -138,
        69,
        -138,
        118,
        377,
        326,
        377,
        -138,
        100,
        125,
        377,
        86,
        377,
        -138,
        -138,
        -138,
        74,
        11,
        -138,
        178,
        -138,
        -138,
        -138,
        -138,
        -138,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        377,
        20,
        -138,
        105,
        87,
        -138,
        -138,
        -138,
        41,
        -138,
        103,
        -138,
        100,
        105,
        42,
        112,
        76,
        125,
        46,
        -138,
        -138,
        -138,
        -138,
        33,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        4,
        125,
        334,
        377,
        -138,
        -138,
        283,
        377,
        377,
        283,
        -138,
        47,
        141,
        -138,
        334,
        -138,
        -138,
        -138,
        133,
        82,
        96,
        -138,
        4,
        122,
        128,
        -138,
        -17,
        283,
        377,
        377,
        -138,
        213,
        -138,
        369,
        100,
        334,
        -138,
        -138,
        50,
        54,
        -138,
        248,
        100,
        -138,
        283,
        283,
        -138,
        100,
        -138,
        -138};

    const signed char YaccParser ::yydefact_[] = {0,
        91,
        93,
        92,
        89,
        17,
        94,
        96,
        95,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        97,
        16,
        0,
        0,
        98,
        99,
        100,
        0,
        2,
        4,
        12,
        6,
        0,
        7,
        15,
        9,
        8,
        13,
        10,
        11,
        14,
        58,
        0,
        56,
        52,
        59,
        61,
        63,
        66,
        71,
        74,
        78,
        83,
        81,
        80,
        0,
        0,
        0,
        0,
        87,
        90,
        0,
        0,
        0,
        0,
        18,
        33,
        30,
        0,
        0,
        35,
        0,
        1,
        3,
        5,
        22,
        19,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        89,
        46,
        20,
        44,
        79,
        81,
        84,
        0,
        54,
        0,
        88,
        89,
        21,
        0,
        0,
        0,
        0,
        0,
        31,
        82,
        36,
        57,
        60,
        62,
        64,
        65,
        69,
        67,
        70,
        68,
        72,
        73,
        75,
        76,
        77,
        53,
        41,
        0,
        0,
        0,
        85,
        86,
        0,
        0,
        0,
        0,
        42,
        0,
        0,
        47,
        0,
        45,
        48,
        55,
        28,
        0,
        0,
        32,
        0,
        0,
        37,
        50,
        0,
        0,
        0,
        0,
        43,
        0,
        25,
        0,
        39,
        0,
        49,
        29,
        0,
        0,
        23,
        0,
        38,
        51,
        0,
        0,
        24,
        40,
        26,
        27};

    const short YaccParser ::yypgoto_[] = {-138,
        -138,
        -12,
        -24,
        -138,
        -138,
        98,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        -138,
        19,
        -138,
        43,
        109,
        -135,
        -138,
        -85,
        -138,
        -14,
        -45,
        -138,
        40,
        52,
        -43,
        48,
        -37,
        115,
        -138,
        -138,
        -50,
        -137,
        -18,
        -138,
        -4,
        -138};

    const unsigned char YaccParser ::yydefgoto_[] = {0,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        156,
        30,
        31,
        32,
        33,
        34,
        35,
        36,
        134,
        135,
        89,
        90,
        139,
        150,
        37,
        95,
        38,
        39,
        40,
        41,
        42,
        43,
        44,
        45,
        46,
        47,
        48,
        55,
        56,
        49,
        50,
        51,
        52};

    const unsigned char YaccParser ::yytable_[] = {70,
        64,
        123,
        149,
        65,
        57,
        98,
        67,
        96,
        97,
        58,
        158,
        159,
        68,
        6,
        7,
        8,
        74,
        160,
        59,
        78,
        79,
        80,
        81,
        167,
        84,
        85,
        86,
        109,
        171,
        53,
        16,
        54,
        91,
        93,
        114,
        115,
        116,
        117,
        91,
        73,
        140,
        107,
        70,
        101,
        103,
        105,
        120,
        121,
        122,
        124,
        60,
        54,
        140,
        61,
        104,
        93,
        93,
        93,
        93,
        93,
        93,
        93,
        93,
        93,
        93,
        93,
        93,
        93,
        69,
        127,
        73,
        128,
        130,
        140,
        73,
        146,
        133,
        147,
        73,
        75,
        168,
        141,
        73,
        62,
        169,
        91,
        72,
        73,
        1,
        2,
        3,
        76,
        77,
        4,
        71,
        6,
        7,
        8,
        82,
        83,
        9,
        106,
        73,
        132,
        73,
        142,
        91,
        98,
        145,
        152,
        73,
        97,
        16,
        110,
        111,
        18,
        143,
        144,
        87,
        136,
        98,
        20,
        21,
        153,
        73,
        88,
        161,
        112,
        113,
        118,
        119,
        54,
        99,
        125,
        22,
        129,
        126,
        162,
        163,
        131,
        70,
        136,
        165,
        172,
        173,
        1,
        2,
        3,
        148,
        151,
        4,
        5,
        6,
        7,
        8,
        155,
        102,
        9,
        10,
        157,
        11,
        12,
        13,
        14,
        154,
        100,
        92,
        137,
        15,
        16,
        17,
        0,
        18,
        0,
        0,
        0,
        19,
        66,
        20,
        21,
        1,
        2,
        3,
        0,
        0,
        4,
        5,
        6,
        7,
        8,
        0,
        22,
        9,
        10,
        0,
        11,
        12,
        13,
        14,
        0,
        0,
        0,
        0,
        15,
        16,
        17,
        0,
        18,
        0,
        0,
        0,
        19,
        108,
        20,
        21,
        1,
        2,
        3,
        0,
        0,
        4,
        5,
        6,
        7,
        8,
        0,
        22,
        9,
        10,
        0,
        11,
        12,
        13,
        14,
        0,
        0,
        0,
        0,
        15,
        16,
        17,
        0,
        18,
        0,
        0,
        0,
        19,
        164,
        20,
        21,
        1,
        2,
        3,
        0,
        0,
        4,
        5,
        6,
        7,
        8,
        0,
        22,
        9,
        10,
        0,
        11,
        12,
        13,
        14,
        0,
        0,
        0,
        0,
        15,
        16,
        17,
        0,
        18,
        0,
        0,
        0,
        19,
        170,
        20,
        21,
        1,
        2,
        3,
        0,
        0,
        4,
        5,
        6,
        7,
        8,
        0,
        22,
        9,
        10,
        0,
        11,
        12,
        13,
        14,
        0,
        0,
        0,
        0,
        15,
        16,
        17,
        0,
        18,
        0,
        0,
        0,
        19,
        0,
        20,
        21,
        1,
        2,
        3,
        0,
        0,
        4,
        0,
        0,
        1,
        2,
        3,
        22,
        0,
        4,
        0,
        0,
        1,
        2,
        3,
        0,
        0,
        4,
        0,
        0,
        0,
        63,
        0,
        18,
        0,
        0,
        0,
        0,
        0,
        20,
        21,
        18,
        94,
        0,
        0,
        0,
        0,
        20,
        21,
        18,
        0,
        0,
        22,
        138,
        0,
        20,
        21,
        1,
        2,
        3,
        22,
        0,
        4,
        0,
        0,
        1,
        2,
        3,
        22,
        0,
        4,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        18,
        0,
        0,
        166,
        0,
        0,
        20,
        21,
        18,
        0,
        0,
        0,
        0,
        0,
        20,
        21,
        0,
        0,
        0,
        22,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        22};

    const short YaccParser ::yycheck_[] = {24,
        15,
        87,
        138,
        18,
        9,
        56,
        19,
        53,
        54,
        30,
        148,
        29,
        0,
        10,
        11,
        12,
        48,
        35,
        30,
        43,
        44,
        45,
        46,
        159,
        38,
        39,
        40,
        73,
        166,
        30,
        27,
        32,
        51,
        52,
        78,
        79,
        80,
        81,
        57,
        29,
        126,
        31,
        67,
        58,
        59,
        60,
        84,
        85,
        86,
        30,
        30,
        32,
        138,
        28,
        59,
        74,
        75,
        76,
        77,
        78,
        79,
        80,
        81,
        82,
        83,
        84,
        85,
        86,
        56,
        29,
        29,
        31,
        31,
        159,
        29,
        29,
        31,
        31,
        29,
        47,
        31,
        127,
        29,
        28,
        31,
        104,
        28,
        29,
        3,
        4,
        5,
        41,
        42,
        8,
        28,
        10,
        11,
        12,
        36,
        37,
        15,
        28,
        29,
        28,
        29,
        130,
        125,
        158,
        133,
        28,
        29,
        157,
        27,
        74,
        75,
        30,
        131,
        132,
        50,
        124,
        171,
        36,
        37,
        28,
        29,
        8,
        151,
        76,
        77,
        82,
        83,
        32,
        8,
        29,
        49,
        33,
        50,
        152,
        153,
        28,
        165,
        146,
        155,
        168,
        169,
        3,
        4,
        5,
        8,
        17,
        8,
        9,
        10,
        11,
        12,
        34,
        59,
        15,
        16,
        32,
        18,
        19,
        20,
        21,
        146,
        57,
        52,
        125,
        26,
        27,
        28,
        -1,
        30,
        -1,
        -1,
        -1,
        34,
        35,
        36,
        37,
        3,
        4,
        5,
        -1,
        -1,
        8,
        9,
        10,
        11,
        12,
        -1,
        49,
        15,
        16,
        -1,
        18,
        19,
        20,
        21,
        -1,
        -1,
        -1,
        -1,
        26,
        27,
        28,
        -1,
        30,
        -1,
        -1,
        -1,
        34,
        35,
        36,
        37,
        3,
        4,
        5,
        -1,
        -1,
        8,
        9,
        10,
        11,
        12,
        -1,
        49,
        15,
        16,
        -1,
        18,
        19,
        20,
        21,
        -1,
        -1,
        -1,
        -1,
        26,
        27,
        28,
        -1,
        30,
        -1,
        -1,
        -1,
        34,
        35,
        36,
        37,
        3,
        4,
        5,
        -1,
        -1,
        8,
        9,
        10,
        11,
        12,
        -1,
        49,
        15,
        16,
        -1,
        18,
        19,
        20,
        21,
        -1,
        -1,
        -1,
        -1,
        26,
        27,
        28,
        -1,
        30,
        -1,
        -1,
        -1,
        34,
        35,
        36,
        37,
        3,
        4,
        5,
        -1,
        -1,
        8,
        9,
        10,
        11,
        12,
        -1,
        49,
        15,
        16,
        -1,
        18,
        19,
        20,
        21,
        -1,
        -1,
        -1,
        -1,
        26,
        27,
        28,
        -1,
        30,
        -1,
        -1,
        -1,
        34,
        -1,
        36,
        37,
        3,
        4,
        5,
        -1,
        -1,
        8,
        -1,
        -1,
        3,
        4,
        5,
        49,
        -1,
        8,
        -1,
        -1,
        3,
        4,
        5,
        -1,
        -1,
        8,
        -1,
        -1,
        -1,
        28,
        -1,
        30,
        -1,
        -1,
        -1,
        -1,
        -1,
        36,
        37,
        30,
        31,
        -1,
        -1,
        -1,
        -1,
        36,
        37,
        30,
        -1,
        -1,
        49,
        34,
        -1,
        36,
        37,
        3,
        4,
        5,
        49,
        -1,
        8,
        -1,
        -1,
        3,
        4,
        5,
        49,
        -1,
        8,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        30,
        -1,
        -1,
        33,
        -1,
        -1,
        36,
        37,
        30,
        -1,
        -1,
        -1,
        -1,
        -1,
        36,
        37,
        -1,
        -1,
        -1,
        49,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        49};

    const signed char YaccParser ::yystos_[] = {0,
        3,
        4,
        5,
        8,
        9,
        10,
        11,
        12,
        15,
        16,
        18,
        19,
        20,
        21,
        26,
        27,
        28,
        30,
        34,
        36,
        37,
        49,
        63,
        64,
        65,
        66,
        67,
        68,
        69,
        71,
        72,
        73,
        74,
        75,
        76,
        77,
        84,
        86,
        87,
        88,
        89,
        90,
        91,
        92,
        93,
        94,
        95,
        96,
        99,
        100,
        101,
        102,
        30,
        32,
        97,
        98,
        101,
        30,
        30,
        30,
        28,
        28,
        28,
        86,
        86,
        35,
        64,
        0,
        56,
        65,
        28,
        28,
        29,
        48,
        47,
        41,
        42,
        43,
        44,
        45,
        46,
        36,
        37,
        38,
        39,
        40,
        50,
        8,
        80,
        81,
        99,
        94,
        99,
        31,
        85,
        87,
        87,
        97,
        8,
        81,
        86,
        68,
        86,
        101,
        86,
        28,
        31,
        35,
        87,
        89,
        89,
        90,
        90,
        91,
        91,
        91,
        91,
        92,
        92,
        93,
        93,
        93,
        84,
        30,
        29,
        50,
        29,
        31,
        33,
        31,
        28,
        28,
        31,
        78,
        79,
        101,
        80,
        34,
        82,
        84,
        87,
        65,
        86,
        86,
        65,
        29,
        31,
        8,
        82,
        83,
        17,
        28,
        28,
        78,
        34,
        70,
        32,
        98,
        29,
        35,
        65,
        86,
        86,
        35,
        64,
        33,
        82,
        31,
        31,
        35,
        98,
        65,
        65};

    const signed char YaccParser ::yyr1_[] = {0,
        62,
        63,
        63,
        64,
        64,
        65,
        65,
        65,
        65,
        65,
        65,
        65,
        65,
        65,
        65,
        65,
        65,
        66,
        67,
        68,
        68,
        69,
        70,
        70,
        71,
        72,
        72,
        73,
        73,
        74,
        74,
        75,
        76,
        66,
        77,
        77,
        78,
        78,
        78,
        78,
        79,
        79,
        79,
        80,
        80,
        81,
        81,
        82,
        82,
        83,
        83,
        84,
        84,
        85,
        85,
        86,
        86,
        87,
        88,
        88,
        89,
        89,
        90,
        90,
        90,
        91,
        91,
        91,
        91,
        91,
        92,
        92,
        92,
        93,
        93,
        93,
        93,
        94,
        94,
        95,
        95,
        95,
        95,
        96,
        96,
        97,
        98,
        98,
        99,
        99,
        100,
        100,
        100,
        101,
        101,
        101,
        101,
        102,
        102,
        102};

    const signed char YaccParser ::yyr2_[] = {0,
        2,
        1,
        2,
        1,
        2,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        2,
        2,
        2,
        3,
        2,
        2,
        3,
        6,
        9,
        9,
        5,
        7,
        2,
        3,
        5,
        2,
        2,
        2,
        3,
        2,
        4,
        3,
        5,
        0,
        1,
        3,
        1,
        3,
        1,
        3,
        1,
        3,
        1,
        3,
        1,
        3,
        1,
        3,
        1,
        3,
        1,
        1,
        3,
        1,
        3,
        1,
        3,
        3,
        1,
        3,
        3,
        3,
        3,
        1,
        3,
        3,
        1,
        3,
        3,
        3,
        1,
        2,
        1,
        1,
        3,
        1,
        3,
        4,
        3,
        1,
        2,
        1,
        2,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1};

#if YYDEBUG || 1
    // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
    // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
    const char* const YaccParser ::yytname_[] = {"\"end of file\"",
        "error",
        "\"invalid token\"",
        "INT_CONST",
        "LL_CONST",
        "FLOAT_CONST",
        "STR_CONST",
        "ERR_TOKEN",
        "IDENT",
        "SLASH_COMMENT",
        "INT",
        "VOID",
        "FLOAT",
        "DOUBLE",
        "CHAR",
        "CONST",
        "IF",
        "ELSE",
        "FOR",
        "WHILE",
        "CONTINUE",
        "BREAK",
        "SWITCH",
        "CASE",
        "GOTO",
        "DO",
        "RETURN",
        "LL",
        "SEMICOLON",
        "COMMA",
        "LPAREN",
        "RPAREN",
        "LBRACKET",
        "RBRACKET",
        "LBRACE",
        "RBRACE",
        "PLUS",
        "MINUS",
        "MUL",
        "DIV",
        "MOD",
        "EQ",
        "NEQ",
        "LT",
        "GT",
        "LE",
        "GE",
        "AND",
        "OR",
        "NOT",
        "ASSIGN",
        "ADD_ASSIGN",
        "SUB_ASSIGN",
        "MUL_ASSIGN",
        "DIV_ASSIGN",
        "MOD_ASSIGN",
        "END",
        "POS",
        "NEG",
        "INC",
        "DEC",
        "THEN",
        "$accept",
        "PROGRAM",
        "STMT_LIST",
        "STMT",
        "CONTINUE_STMT",
        "EXPR_STMT",
        "VAR_DECLARATION",
        "VAR_DECL_STMT",
        "FUNC_BODY",
        "FUNC_DECL_STMT",
        "FOR_STMT",
        "IF_STMT",
        "RETURN_STMT",
        "WHILE_STMT",
        "BREAK_STMT",
        "BLOCK_STMT",
        "PARAM_DECLARATOR",
        "PARAM_DECLARATOR_LIST",
        "VAR_DECLARATOR",
        "VAR_DECLARATOR_LIST",
        "INITIALIZER",
        "INITIALIZER_LIST",
        "ASSIGN_EXPR",
        "EXPR_LIST",
        "EXPR",
        "NOCOMMA_EXPR",
        "LOGICAL_OR_EXPR",
        "LOGICAL_AND_EXPR",
        "EQUALITY_EXPR",
        "RELATIONAL_EXPR",
        "ADDSUB_EXPR",
        "MULDIV_EXPR",
        "UNARY_EXPR",
        "BASIC_EXPR",
        "FUNC_CALL_EXPR",
        "ARRAY_DIMENSION_EXPR",
        "ARRAY_DIMENSION_EXPR_LIST",
        "LEFT_VAL_EXPR",
        "LITERAL_EXPR",
        "TYPE",
        "UNARY_OP",
        YY_NULLPTR};
#endif

#if YYDEBUG
    const short YaccParser ::yyrline_[] = {0,
        189,
        189,
        193,
        199,
        203,
        210,
        213,
        216,
        219,
        222,
        225,
        228,
        231,
        234,
        237,
        240,
        243,
        250,
        256,
        262,
        265,
        271,
        279,
        283,
        294,
        301,
        305,
        312,
        315,
        321,
        324,
        330,
        336,
        342,
        348,
        352,
        364,
        369,
        378,
        385,
        398,
        402,
        406,
        413,
        417,
        423,
        427,
        434,
        438,
        444,
        448,
        455,
        458,
        464,
        468,
        475,
        478,
        493,
        501,
        504,
        510,
        513,
        519,
        522,
        525,
        532,
        535,
        538,
        541,
        544,
        551,
        554,
        557,
        564,
        567,
        570,
        574,
        579,
        582,
        588,
        591,
        594,
        597,
        603,
        618,
        625,
        631,
        635,
        643,
        647,
        654,
        657,
        660,
        666,
        669,
        672,
        675,
        681,
        684,
        687};

    void YaccParser ::yy_stack_print_() const
    {
        *yycdebug_ << "Stack now";
        for (stack_type::const_iterator i = yystack_.begin(), i_end = yystack_.end(); i != i_end; ++i)
            *yycdebug_ << ' ' << int(i->state);
        *yycdebug_ << '\n';
    }

    void YaccParser ::yy_reduce_print_(int yyrule) const
    {
        int yylno  = yyrline_[yyrule];
        int yynrhs = yyr2_[yyrule];
        // Print the symbols being reduced, and their result.
        *yycdebug_ << "Reducing stack by rule " << yyrule - 1 << " (line " << yylno << "):\n";
        // The symbols being reduced.
        for (int yyi = 0; yyi < yynrhs; yyi++)
            YY_SYMBOL_PRINT("   $" << yyi + 1 << " =", yystack_[(yynrhs) - (yyi + 1)]);
    }
#endif  // YYDEBUG

#line 4 "frontend/parser/yacc.y"
}  // namespace FE
#line 2571 "frontend/parser/yacc.cpp"

#line 692 "frontend/parser/yacc.y"

void FE::YaccParser::error(const FE::location& location, const std::string& message)
{
    std::cerr << "msg: " << message << ", error happened at: " << location << std::endl;
}
