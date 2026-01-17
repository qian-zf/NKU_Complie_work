#ifndef __MIDDLEEND_PASS_MEM2REG_H__
#define __MIDDLEEND_PASS_MEM2REG_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_module.h>
#include <middleend/module/ir_function.h>
#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/pass/analysis/dominfo.h>
#include <stack>
#include <map>
#include <set>
#include <vector>

namespace ME
{
    class Mem2RegPass : public FunctionPass
    {
      public:
        Mem2RegPass()  = default;
        ~Mem2RegPass() = default;

        void runOnFunction(Function& function) override;

      private:
        // 存储可提升的 Alloca 指令的信息
        struct AllocaInfo
        {
            AllocaInst*               allocaInst;
            std::set<size_t>          defBlocks;   // 定义该变量的基本块 ID 集合 (Stores)
            std::vector<Instruction*> usingInsts;  // 使用该变量的指令 (Load/Store)
            size_t                    index;       // 在 promotiveAllocas 向量中的索引
        };

        std::vector<AllocaInfo>           promotiveAllocas;
        std::vector<std::stack<Operand*>> renamingStacks;
        std::map<Instruction*, size_t>    phiToAllocaIndex;  // 记录 Phi 指令对应的 Alloca 索引
        std::map<Operand*, Operand*>      replacements;
        std::map<Operand*, size_t>        allocaPtrToIndex;  // alloca ptr -> index for O(1) lookup      // 快速查找索引

        void collectPromotiveAllocas(Function& function);
        void insertPhiNodes(Function& function, Analysis::DomInfo* domInfo);
        void renameVariables(Function& function, Analysis::DomInfo* domInfo, Analysis::CFG* cfg);
        void renameRecursive(size_t blockId, Analysis::DomInfo* domInfo, Analysis::CFG* cfg,
                             std::map<size_t, bool>& visited);
        void replaceOperands(Instruction* inst);
    };
}  // namespace ME

#endif  // __MIDDLEEND_PASS_MEM2REG_H__
