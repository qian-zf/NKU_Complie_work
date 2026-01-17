#ifndef __BACKEND_TARGETS_AARCH64_AARCH64_REG_INFO_H__
#define __BACKEND_TARGETS_AARCH64_AARCH64_REG_INFO_H__

#include <backend/target/target_reg_info.h>
#include <backend/targets/aarch64/aarch64_defs.h>
#include <map>

namespace BE::Targeting::AArch64
{
    class RegInfo : public TargetRegInfo
    {
      public:
        RegInfo() {
            // 初始化整数寄存器 (x0-x30)
            for (int i = 0; i <= 30; ++i) {
                intRegs_.push_back(i);
            }
            // 初始化浮点寄存器 (v0-v31)
            for (int i = 0; i <= 31; ++i) {
                floatRegs_.push_back(i);
            }

            // 整数参数寄存器: x0-x7
            for (int i = 0; i < 8; ++i) {
                intArgRegs_.push_back(i);
            }
            // 浮点参数寄存器: v0-v7
            for (int i = 0; i < 8; ++i) {
                floatArgRegs_.push_back(i);
            }

            // 被调者保存的整数寄存器: x19-x28
            for (int i = 19; i <= 28; ++i) {
                calleeSavedIntRegs_.push_back(i);
            }
            // 被调者保存的浮点寄存器: v8-v15
            for (int i = 8; i <= 15; ++i) {
                calleeSavedFloatRegs_.push_back(i);
            }
            
            // 保留寄存器: x16, x17 (过程间调用暂存), x18 (平台相关), x29 (帧指针 FP), x30 (链接寄存器 LR)
            // x9 常在我们的实现中用作临时寄存器
            // reservedRegs_.push_back(9);
            reservedRegs_.push_back(16);
            reservedRegs_.push_back(17);
            reservedRegs_.push_back(18);
            reservedRegs_.push_back(29);
            reservedRegs_.push_back(30);
        }

        int spRegId() const override { return 31; } // SP (栈指针) 通常在指令编码中映射为 31，但在某些上下文中是独立的。这里遵循常见约定或我们的定义。
        int raRegId() const override { return 30; } // x30 是 LR (链接寄存器)
        int zeroRegId() const override { return 32; } // xzr 通常在我们的 IR 中作为伪寄存器 32 或类似的 ID

        const std::vector<int>& intArgRegs() const override { return intArgRegs_; }
        const std::vector<int>& floatArgRegs() const override { return floatArgRegs_; }

        const std::vector<int>& calleeSavedIntRegs() const override { return calleeSavedIntRegs_; }
        const std::vector<int>& calleeSavedFloatRegs() const override { return calleeSavedFloatRegs_; }

        const std::vector<int>& reservedRegs() const override { return reservedRegs_; }

        const std::vector<int>& intRegs() const override { return intRegs_; }
        const std::vector<int>& floatRegs() const override { return floatRegs_; }

    private:
        std::vector<int> intRegs_;
        std::vector<int> floatRegs_;
        std::vector<int> intArgRegs_;
        std::vector<int> floatArgRegs_;
        std::vector<int> calleeSavedIntRegs_;
        std::vector<int> calleeSavedFloatRegs_;
        std::vector<int> reservedRegs_;
    };
}  // namespace BE::Targeting::AArch64

#endif  // __BACKEND_TARGETS_AARCH64_AARCH64_REG_INFO_H__
