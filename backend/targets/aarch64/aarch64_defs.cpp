#include <backend/targets/aarch64/aarch64_defs.h>
#include <backend/mir/m_instruction.h>

namespace BE::AArch64
{
    // 获取操作符对应的汇编助记符
    std::string getOpInfoAsm(Operator op)
    {
        switch (op)
        {
#define X(n, t, a) \
    case Operator::n: return a;
            A64_INSTS
#undef X
            default: return "nop";
        }
    }

    // 获取操作符的操作数类型 (用于汇编生成和指令分析)
    // OpType 定义了指令的操作数格式，例如寄存器数量、立即数、内存操作数等
    OpType getOpInfoType(Operator op)
    {
        switch (op)
        {
#define X(n, t, a) \
    case Operator::n: return OpType::t;
            A64_INSTS
#undef X
            default: return OpType::Z;
        }
    }
}  // namespace BE::AArch64
