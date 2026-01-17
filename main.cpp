#include <frontend/parser/parser.h>
#include <frontend/ast/ast.h>
#include <frontend/ast/visitor/printer/ast_printer.h>
#include <frontend/symbol/symbol_table.h>
#include <frontend/ast/visitor/sementic_check/ast_checker.h>

#include <middleend/visitor/codegen/ast_codegen.h>
#include <middleend/visitor/printer/module_printer.h>
#include <middleend/module/ir_module.h>
#include <middleend/pass/unify_return.h>
#include <middleend/pass/mem2reg.h>
#include <middleend/pass/dce.h>
#include <middleend/pass/adce.h>
#include <backend/mir/m_module.h>
#include <backend/target/registry.h>
#include <backend/target/target.h>

#include <fstream>
#include <iostream>
#include <iomanip>

/* �������˿�ܵ�ʵ��, ���߽���˿���ִ������
   ���������ִ�C++���ԶԿ�ܽ������ع�, ������Ч�ؼ��˴����������˴���ĸ�����
   ���ǹ����������̽�����ϵ����pull request, Ҳ����ĸĽ��ᱻ�õ���һ���ʵ������
*/
/*
   ��ѧ�ڵı�����ҵ������ı����������ٶ���һ��Ҫ�󣬱���󲿷�������ʱ�䲻�ܳ���5s��һЩ���͵Ĳ����������ó���30s
   ����Ҫ��ʵ��ʱע�����ݽṹ��ѡ���Լ��㷨��ʵ��ϸ��
   ����ģ���Ȼ����Ӳ��Ҫ�󣬵����ǽ�������ҵʵ�ֹ�����ע���ڴ�����������ڴ�й©����������

   Makefile ���ṩ�� format Ŀ�꣬����㰲װ�� clang-format ���ߣ�����ͨ�� make format ����ʽ������
       ��ʽ��������� .clang-format �ļ��в鿴

    mem_track.sh �ű������ڵ��� valgrind ������ڴ�й©����
*/

#define STR_PW 30
#define INT_PW 8
#define MIN_GAP 5
#define STR_REAL_WIDTH (STR_PW - MIN_GAP)

using namespace std;

string truncateString(const string& str, size_t width)
{
    if (str.length() > width) return str.substr(0, width - 3) + "...";
    return str;
}

int main(int argc, char** argv)
{
    string   inputFile     = "";
    string   outputFile    = "";
    string   step          = "-llvm";
    string   march         = "armv8";
    int      optimizeLevel = 0;
    ostream* outStream     = &cout;
    ofstream outFile;

    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];

        if (arg == "-lexer" || arg == "-parser" || arg == "-llvm" || arg == "-S") { step = arg; }
        else if (arg == "-o")
        {
            if (i + 1 < argc)
                outputFile = argv[++i];
            else
            {
                cerr << "Error: -o option requires a filename" << endl;
                return 1;
            }
        }
        else if (arg == "-march")
        {
            if (i + 1 < argc)
                march = argv[++i];
            else
            {
                cerr << "Error: -march option requires a target (e.g., riscv64)" << endl;
                return 1;
            }
        }
        else if (arg == "-O" || arg == "-O1") { optimizeLevel = 1; }
        else if (arg == "-O0") { optimizeLevel = 0; }
        else if (arg == "-O2") { optimizeLevel = 2; }
        else if (arg == "-O3") { optimizeLevel = 3; }
        else if (arg[0] != '-') { inputFile = arg; }
        else
        {
            cerr << "Unknown option: " << arg << endl;
            return 1;
        }
    }

    if (inputFile.empty())
    {
        cerr << "Error: No input file specified" << endl;
        cerr << "Usage: " << argv[0] << " [-lexer|-parser|-llvm|-S] [-o output_file] input_file [-O]" << endl;
        return 1;
    }

    if (!outputFile.empty())
    {
        outFile.open(outputFile);
        if (!outFile)
        {
            cerr << "Cannot open output file " << outputFile << endl;
            return 1;
        }
        outStream = &outFile;
    }

    cout << "Input file: " << inputFile << endl;
    cout << "Step: " << step << endl;
    cout << "Output: " << (outputFile.empty() ? "standard output" : outputFile) << endl;
    cout << "Optimize level: " << optimizeLevel << endl;

    ifstream       in(inputFile);
    istream*       inStream = &in;
    FE::AST::Node* ast      = nullptr;
    int            ret      = 0;

    if (!in)
    {
        cerr << "Cannot open input file " << inputFile << endl;
        ret = 1;
        goto cleanup_outfile;
    }

    /*
     * Lab 1: �ʷ�����
     *
     * ��ʵ���Ŀ����ʵ�ִʷ�����������Դ�����ı���ת��Ϊ Token ���С�
     *
     * ��Ҫ����:
     * - �� `frontend/parser/parser.cpp` ��Ϊ `FE::Parser::parseTokens_impl` �����ṩ����ʵ�֡�
     * - `iParser` �ӿ� (λ�� `interfaces/frontend/iparser.h`) ʹ�� CRTP ģʽ��
     *   ��ʵ�ʹ����������� `FE::Parser` ��ɡ�
     * - �����ϣ������������������ɱ���ʵ�飬Ҳ�������д� `iParser` �������µ�����ʵ�֡�
     *
     * ����ʵ������ (ʹ�� Flex):
     * 1. `frontend/parser/yacc.y`: �����ʷ���������Ҫʶ��� Token �б���
     * 2. `frontend/parser/lexer.l`: Ϊ Token ��д�������ʽƥ�����ʹ����߼���
     * 3. `frontend/parser/parser.cpp`: ���� Flex ���ɵķ������������ת��Ϊ�Զ���� `Token` �ṹ�塣
     *
     * ע������:
     * - Bison ����: ������﷨����ǰ������ `yacc.y` ʱ�����ľ������ʱ���ԡ�
     * - ��������: �ʷ������׶�Ӧ��������Ϊһ������ (`-`) Token ��һ������ Token ����ϣ�
     *   ���ǵ������� Token��
     *
     * �������ʾ��:
     * �� `testcase/lexer/` Ŀ¼���ṩ��һЩ���������Լ����ǵ�Ԥ��������������в鿴��
     */
    {
        FE::Parser parser(inStream, outStream);

        if (step == "-lexer")
        {
            auto tokens = parser.parseTokens();

            *outStream << left;
            *outStream << setw(STR_PW) << "Token" << setw(STR_PW) << "Lexeme" << setw(STR_PW) << "Property"
                       << setw(INT_PW) << "Line" << setw(INT_PW) << "Column" << endl;

            for (auto& token : tokens)
            {
                *outStream << setw(STR_PW) << truncateString(token.token_name, STR_REAL_WIDTH) << setw(STR_PW)
                           << truncateString(token.lexeme, STR_REAL_WIDTH);

                if (token.type == FE::Token::TokenType::T_INT)
                    *outStream << setw(STR_PW) << token.ival;
                else if (token.type == FE::Token::TokenType::T_LL)
                    *outStream << setw(STR_PW) << token.lval;
                else if (token.type == FE::Token::TokenType::T_FLOAT)
                    *outStream << setw(STR_PW) << token.fval;
                else if (token.type == FE::Token::TokenType::T_DOUBLE)
                    *outStream << setw(STR_PW) << token.dval;
                else if (token.type == FE::Token::TokenType::T_STRING)
                    *outStream << setw(STR_PW) << token.sval;
                else
                    *outStream << setw(STR_PW) << " ";

                *outStream << setw(INT_PW) << token.line_number << setw(INT_PW) << token.column_number << endl;
            }

            ret = 0;
            goto cleanup_files;
        }

        /*
         * Lab 2: �﷨���� (Syntax Analysis)
         *
         * ��ʵ���Ŀ����ʵ���﷨���������� Token ����ת��Ϊ�����﷨�� (AST)��
         * �﷨������Բο� `doc/SysY2022���Զ���-V1.pdf`����������ơ�
         * ��ʵ�ֵ��﷨����Ӧ�����Ը��� SysY ���Ե��﷨�ṹ��
         *
         * ��Ҫ����:
         * - ���� AST: �� `frontend/parser/yacc.y` �ж����﷨���������� AST��
         * - AST ��Ա: ������ AST �ڵ�ʱ����ȷ������ `Entry* entry` ��Ա��
         *   ʹ��ָ���Ӧ�ķ��ű��ͬʱ��ȷ�����������ԣ���ڵ���������������ȡ�
         *
         * ����ļ�:
         * - `frontend/parser/yacc.y`: Bison �﷨�����塣
         * - `frontend/ast/`: AST �ڵ㶨��Ŀ¼ (`ast.h`, `expr.h`, `decl.h`, `stmt.h`)��
         * - `interfaces/frontend/symbol/symbol_entry.h`: ���ű���塣
         *
         * ��ʾ:
         * - AST �Ĵ�ӡ�������ṩ��λ�� `frontend/ast/visitor/printer/`��
         *   ����ͨ�����·�ʽ��ʹ�������ں�����ʵ����Ҳ��ʹ�õ����Ƶķ�����ģʽ����Ҳ����ʹ�� `apply`
         *   �������򻯷����ߵĵ��á�
         *   ```
         *   FE::AST::ASTPrinter printer;
         *   apply(printer, *ast, outStream);
         *   ```
         *
         * �������ʾ��:
         * �� `testcase/parser/` Ŀ¼���ṩ��һЩ���������Լ����ǵ�Ԥ��������������в鿴��
         */
        ast = parser.parseAST();
        if (!ast)
        {
            cerr << "Parsing failed." << endl;
            ret = 1;
            goto cleanup_files;
        }

        if (step == "-parser")
        {
            FE::AST::ASTPrinter printer;
            std::ostream*       osPtr = outStream;
            apply(printer, *ast, osPtr);

            ret = 0;
            goto cleanup_ast;
        }

        /*
         * Lab 3-1: �������
         *
         * ���׶���Ҫʵ����������������� AST �����Դ����ľ�̬������ȷ�ԡ�
         * ��ı�������Ҫ��ʶ�𲢱������´���
         * - ����/�����ض��塢����δ����
         * - ȫ�ֱ����������ڳ�ʼ��
         * - �Ƿ�������ά��/�±�/��ʼ��
         * - �������ò�����ƥ��
         * - �Ƿ���ֵ���Ƿ�����ֵ
         * - `main` ����δ����
         * - ѭ����� `break`/`continue`
         * - �Ƿ��Ĳ��������� (��Ը�����ȡģ)
         *
         * ��Ҫ����:
         * 1. ʵ�ַ��ű�:
         *    - ���ű��������������й������Ŷ��塣�ӿ�λ�� `interfaces/frontend/symbol/isymbol_table.h`��
         *    - ����Ҫʵ�� `frontend/symbol/symbol_table.h`�е� `enterScope`, `exitScope`, `addSymbol`, `getSymbol`
         * �ȷ�����
         *
         * 2. ʵ����������:
         *    - ʵ�ַ��ű�������Ҫͨ������ AST ��ִ�м�顣
         *    - ����ṩ�� `frontend/ast/visitor/sementic_check/ast_checker.h` �е� `ASTChecker` �����ߡ�
         *    - ����Ҫ������ `visit` �������߼������÷��ű������ֲ���¼�������
         *
         * ��ʾ:
         * - ������ʼ����������һ���ֵ� "ʵ����������"
         * ����Ҫ����Щ����������˵���ǣ�����Ƿ������������������Լ�
         * ά���������ԣ�����������͡������Ĳ������Թ������� IR ����ʹ�á�
         * ��˿���б����˽�Ϊ�򵥵ļ��� `visit` ������ʵ����Ϊʾ��������Բο�������ʵ�������ڵ�ļ���߼���
         */
        FE::AST::ASTChecker checker;
        bool                accept = apply(checker, *ast);
        if (!accept)
        {
            cerr << "Semantic check failed with " << checker.errors.size() << " errors." << endl;
            for (const auto& err : checker.errors) cerr << "Error: " << err << endl;
            ret = 1;
            goto cleanup_ast;
        }

        /*
         * Lab 3-2: �м�������� (IR Generation)
         *
         * Ŀ��:
         * - �� AST ����Ϊ�м��ʾ (IR)��ʵ��һ�� AST ������, ������/����ʽ/���ӳ��Ϊ IR
         * Module/Function/Block/Instruction��
         *
         * ��Ҫ����:
         * - �� middleend/visitor/codegen/ �в�ȫ ASTCodeGen �ĸ��� visit �ӿ�
         * - ����: ���ɺ��������������; ��������ӳ���뷵��; ά��ѭ����ֹ��ǩ
         * - ����: �ֲ��������� (alloca/store); ȫ�ֱ����������ֵ; ����Ѱַ (GEP)
         * - ����ʽ: ������/һԪ/��Ԫ����; ��·�߼�; ��Ҫ������ת��
         * - ������: if/while/for/break/continue �Ļ�����ƴ��������/��������֧
         * - ����: �⺯�������뺯������
         *
         * ����ļ�:
         * - middleend/visitor/codegen/ast_codegen.{h,cpp}
         * - middleend/visitor/codegen/expr_codegen.cpp
         * - middleend/visitor/codegen/stmt_codegen.cpp
         * - middleend/visitor/codegen/decl_codegen.cpp
         * - middleend/visitor/codegen/type_convert.cpp (����ת�������ʵ��)
         * - middleend/module/ir_module.{h,cpp} (IR ���ݽṹ)
         * - middleend/visitor/printer/module_printer.{h,cpp} (IR ��ӡ)
         *
         * ��ʾ:
         * - ����ʵ������������������˳�����, ����֧�������������
         * - ͨ�� -llvm �����֤ IR �Ƿ����Ԥ��
         */
        ME::ASTCodeGen codegen(checker.getGlbSymbols(), checker.getFuncDecls());
        ME::Module     m;

        apply(codegen, *ast, &m);

        if (optimizeLevel > 0)
        {
            /*
             * Lab 4: �м�����Ż�
             *
             * ѡ���˲��ֵ�ͬѧ���������������ʽ�� mem2reg �Լ����������ɴ��/������������������
             * ��ɱ�Ҫ�Ż��󣬿ɼ���ʵ��������Ż���
             * - ϡ������������������ʵ�ֿ�ô�Ĵ�����
             * - ���������ѭ������������
             * - ��������Ĺ����ӱ���ʽ����
             * - ��������
             * - �������������������ڿ�������ͼ����ɾ����ѭ����
             * - �ѶȲ��������� pass �������Ż�
             */
            // ������� pass ������Ϊ�ο�����Ҫ��ʾ�����ͨ��cache��ȡ����pass�Ľ��
            ME::UnifyReturnPass unifyReturnPass;
            unifyReturnPass.runOnModule(m);

            ME::Mem2RegPass mem2RegPass;
            ME::DCEPass dcePass;
            ME::ADCEPass adcePass; 
            for (auto* func : m.functions)
    {
        if (func->blocks.empty()) continue;
        
        // Pass ִ��˳��
        // 1. Mem2Reg - ���ڴ��������Ϊ�Ĵ�������
        mem2RegPass.runOnFunction(*func);
        
        // 2. ADCE - ��������������
        adcePass.runOnFunction(*func);
        
        // 3. DCE - ��ͨ����������������ʣ�ࣩ
        dcePass.runOnFunction(*func);
    }
}

        if (step == "-llvm")
        {
            // ��һ���ֵĴ�ӡ������ʵ���ṩ�������δ�� IR �ṹ�иĶ�������ֱ��ʹ��
            ME::IRPrinter printer;
            printer.visit(m, *outStream);
            ret = 0;
            goto cleanup_ast;
        }

        if (step != "-S")
        {
            cerr << "Unknown step: " << step << endl;
            ret = 1;
            goto cleanup_ast;
        }

        /*
         * Lab 5: ��˴�������
         *
         * �����￪ʼ�����ˡ������Ķ� `backend/README.md` �˽���Ŀ¼�ṹ����ˮ������׶�ְ��
         * ��ָ��ѡ�� �� ֡���� �� �Ĵ������� �� ջ���ͣ���
         *
         * ��˵�"���������"���ڴ��ļ������ڸ�Ŀ��ܹ��� Target �ࣺ
         * - AArch64: `backend/targets/aarch64/aarch64_target.cpp` �� `AArch64Target::runPipeline`
         * - RISC-V:  `backend/targets/riscv64/rv64_target.cpp` �� `RV64::Target::runPipeline`
         * main ��ͨ�� `BE::Targeting::TargetRegistry::getTarget(march)` ��ȡ������ʵ����
         * Ȼ����� `target->runPipeline(&m, &backendModule, outStream)` ����������衣
         *
         * ���� `BE::Targeting::TargetRegistry`��
         * - ���ã�ά��"Ŀ���ַ��� �� Target ����/ʵ��"��ȫ��ע�������Ϊ���ѡ���븴�õ���ڡ�
         * - ע�᣺���ܹ����� target Դ�ļ���ͨ����̬����ע�Ṥ�����������磺
         *   AArch64 �� `aarch64_target.cpp` ��ע�� "aarch64"/"armv8"��
         *   RISC?V �� `rv64_target.cpp` ��ע�� "riscv64"/"riscv"/"rv64"��
         * - ��ȡ��ʹ�������� `-march` ���ַ�����Ĭ�� "riscv64"������ `getTarget(march)`��
         *     `auto* tgt = BE::Targeting::TargetRegistry::getTarget(march);`
         *   ���Ѵ������򷵻ػ���ʵ��������ͨ����Ӧ�������������棻�Ҳ����򷵻� `nullptr`��
         *
         * ��Ҫ����
         * - ѡ��������ָ��ѡ��DAG ISel ��ֱ�� IR��MIR������������ FrameIndex �ȳ���� MIR��
         * - �� AArch64/RV64 �� `runPipeline` �ڰ� README.md ������˳����� Pass��
         *
         * ����˵����
         * - ��Ŀ��Ŀ¼���ṩ�� arm2bin.sh �� rv2bin.sh �ű����������ڽ� AArch64 �� RISC-V �Ļ�����ת��Ϊ�������ļ���
         *     Ĭ������Ϊ `test.s`�����Ϊ `test.bin`����ͨ�������в����޸ġ�
         * - ���ں�˵��ԣ���ͬѧ������ gdb ���ߡ�arm2bin.sh �� rv2bin.sh �ű����Ѿ������� -g ѡ��������ɵ�����Ϣ��
         *     ��������ʹ�� gdb-multiarch ���������� AArch64 �� RISC-V �ĳ��򡣾�������ʾ�����£�
         *     ```bash
         *     qemu-riscv64 -g {port} {exec} &      // {port} Ϊ���Զ˿ڣ�{exec} Ϊ��ִ���ļ���& ��ʾ��̨����
         *                                          // �� `qemu-riscv64 -g 1234 test.bin &`
         *                                          // ��Ȼ���� & Ҳ���ԣ��¿�һ���ն����� gdb ����
         *     gdb-multiarch {exec}
         *     (gdb) target remote:{port}
         *     ```
         *     gdb ��ʹ�����Ŵ���� OS �����Ѿ������˽⣬������Թ���
         */
        BE::Module backendModule;
        auto*      tgt = BE::Targeting::TargetRegistry::getTarget(march);
        if (!tgt)
        {
            cerr << "Unknown target: " << march << endl;
            ret = 1;
            goto cleanup_ast;
        }

        tgt->runPipeline(&m, &backendModule, outStream);

        ret = 0;
    }

cleanup_ast:
    delete ast;
    ast = nullptr;

cleanup_files:
    if (in.is_open()) in.close();

cleanup_outfile:
    if (outFile.is_open()) outFile.close();

    return ret;
}