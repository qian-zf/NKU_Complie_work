#ifndef __MIDDLEEND_PASS_ADCE_H__
#define __MIDDLEEND_PASS_ADCE_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_module.h>
#include <middleend/module/ir_function.h>
#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/pass/analysis/cfg.h>
#include <set>
#include <map>
#include <vector>

namespace ME
{
    class ADCEPass : public FunctionPass
    {
      public:
        ADCEPass()  = default;
        ~ADCEPass() = default;

        void runOnFunction(Function& function) override;

      private:
        bool runADCEIteration(Function& function, Analysis::CFG* cfg);
        bool simplifyControlFlow(Function& function, Analysis::CFG* cfg);
        void cleanupPhiNodes(Function& function);
        bool removeDeadLoops(Function& function, Analysis::CFG* cfg);
        
        // 获取块的最终跳转目标（穿透跳板块）
        size_t getFinalTarget(size_t blockId, Function& function, std::set<size_t>& visited);
        
        bool isRealSideEffect(Instruction* inst);
        Operand* getDefOperand(Instruction* inst);
        std::vector<Operand*> getUsedOperands(Instruction* inst);
    };
}  // namespace ME

#endif  // __MIDDLEEND_PASS_ADCE_H__