#include "backend/targets/aarch64/aarch64_defs.h"
#include <backend/targets/aarch64/aarch64_target.h>
#include <backend/target/registry.h>
#include <backend/targets/aarch64/isel/aarch64_ir_isel.h>
#include <backend/targets/aarch64/aarch64_reg_info.h>
#include <backend/targets/aarch64/aarch64_instr_adapter.h>
#include <backend/targets/aarch64/aarch64_codegen.h>
#include <backend/ra/linear_scan.h>
#include <backend/targets/aarch64/passes/lowering/frame_lowering.h>
#include <backend/targets/aarch64/passes/lowering/stack_lowering.h>
#include <backend/targets/aarch64/passes/lowering/phi_elimination.h>

#include <debug.h>

#include <map>
#include <vector>

namespace BE
{
    namespace RA
    {
        void setTargetInstrAdapter(const BE::Targeting::TargetInstrAdapter* adapter);
    }
}  // namespace BE

namespace BE::Targeting::AArch64
{
    namespace
    {
        struct AutoRegister
        {
            AutoRegister()
            {
                BE::Targeting::TargetRegistry::registerTargetFactory("aarch64", []() { return new AArch64Target(); });
                BE::Targeting::TargetRegistry::registerTargetFactory("armv8", []() { return new AArch64Target(); });
            }
        } s_auto_register;
    }  // namespace

    void AArch64Target::runPipeline(ME::Module* ir, BE::Module* backend, std::ostream* out)
    {
        fprintf(stderr, "DEBUG: Entering AArch64 pipeline\n");
        fflush(stderr);
        
        static BE::Targeting::AArch64::InstrAdapter s_adapter;
        static BE::Targeting::AArch64::RegInfo      s_regInfo;
        BE::Targeting::setTargetInstrAdapter(&s_adapter);

        fprintf(stderr, "DEBUG: Running ISel\n");
        fflush(stderr);
        //TODO("选择一种 Instruction Selector 实现，并完成指令选择");
        // BE::AArch64::DAGIsel isel(ir, backend, this);
        BE::AArch64::IRIsel isel(ir, backend, this);
        isel.run();

        // Pre-RA
        {
            fprintf(stderr, "DEBUG: Running Pre-RA Passes\n");
            fflush(stderr);
            // 对实现了 mem2reg 优化的同学，还需完成 Phi Elimination 
            BE::AArch64::Passes::Lowering::PhiEliminationPass phiElim; 
            phiElim.runOnModule(*backend, &s_adapter); 
 
            // 移动指令消解已包含在 PhiElimination 中（通过生成 MOV/UXTW 指令），无需额外 Pass
        }

        // RA
        {
            fprintf(stderr, "DEBUG: Running RA\n");
            fflush(stderr);
            // TODO("使用你实现的寄存器分配器进行寄存器分配");
            BE::RA::LinearScanRA ra;
            ra.allocate(*backend, s_regInfo);
        }

        // Post-RA
        {
            fprintf(stderr, "DEBUG: Running Post-RA Passes\n");
            fflush(stderr);
            
            fprintf(stderr, "DEBUG: Running FrameLoweringPass\n");
            fflush(stderr);
            BE::AArch64::Passes::Lowering::FrameLoweringPass fl;
            fl.runOnModule(*backend);

            fprintf(stderr, "DEBUG: Running StackLoweringPass\n");
            fflush(stderr);
            BE::AArch64::Passes::Lowering::StackLoweringPass sl;
            sl.runOnModule(*backend);
        }

        fprintf(stderr, "DEBUG: Running Codegen\n");
        fflush(stderr);
        BE::AArch64::Codegen codegen(backend, *out);
        codegen.generateAssembly();
        
        fprintf(stderr, "DEBUG: Pipeline Finished\n");
        fflush(stderr);
    }
}  // namespace BE::Targeting::AArch64