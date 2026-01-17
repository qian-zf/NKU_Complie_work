#ifndef __MIDDLEEND_PASS_DCE_H__
#define __MIDDLEEND_PASS_DCE_H__

#include <interfaces/middleend/pass.h>
#include <middleend/module/ir_module.h>
#include <middleend/module/ir_function.h>
#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <set>
#include <vector>

namespace ME
{
    class DCEPass : public FunctionPass
    {
      public:
        DCEPass()  = default;
        ~DCEPass() = default;

        void runOnFunction(Function& function) override;

      private:
        bool performDCE(Function& function);
        bool isSideEffectFree(Instruction* inst);
    };
}  // namespace ME

#endif  // __MIDDLEEND_PASS_DCE_H__
