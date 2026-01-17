/**
 * @file dag_builder.cpp
 * @brief DAGBuilder 的实现 - 将 LLVM IR 转换为 SelectionDAG
 *
 * 本文件实现了 DAGBuilder 类的所有 visit 方法，每个方法负责
 * 处理一种特定的 IR 指令，并创建相应的 DAG 节点。
 *
 * 核心设计：
 * 1. 值映射管理：通过 reg_value_map_ 维护 IR 虚拟寄存器到 DAG 节点的映射
 * 2. Chain 依赖管理：通过 currentChain_ 串联所有副作用操作
 * 3. 节点去重：所有节点创建都通过 SelectionDAG::getNode()，自动实现 CSE
 *
 * 关键方法：
 * - getValue(): 获取 IR 操作数对应的 DAG 节点
 * - setDef(): 记录 IR 指令的定义结果
 * - visit(XxxInst&): 为每种 IR 指令创建对应的 DAG 节点
 *
 * @brief 若有兴趣了解更多LLVM中DAG的具体内容，可参考
 * https://llvm.org/devmtg/2024-10/slides/tutorial/MacLean-Fargnoli-ABeginnersGuide-to-SelectionDAG.pdf
 * or
 * https://zhuanlan.zhihu.com/p/600170077
 */

#include <backend/dag/dag_builder.h>
#include <cstdint>
#include <debug.h>

namespace BE
{
    namespace DAG
    {
        static inline bool isFloatType(ME::DataType t) { return t == ME::DataType::F32 || t == ME::DataType::DOUBLE; }

        BE::DataType* DAGBuilder::mapType(ME::DataType t)
        {
            // 将中间端(ME)的数据类型映射到后端(BE)的机器数据类型
            switch (t)
            {
                case ME::DataType::I1:
                case ME::DataType::I8:
                case ME::DataType::I32: return BE::I32; // 小整数统一提升为 I32
                case ME::DataType::I64:
                case ME::DataType::PTR: return BE::I64; // 指针在64位机上映射为 I64
                case ME::DataType::F32: return BE::F32;
                case ME::DataType::DOUBLE: return BE::F64;
                default: ERROR("Unsupported IR data type"); return BE::I32;
            }
        }

        void DAGBuilder::visit(ME::Module& module, SelectionDAG& dag)
        {
            for (auto* func : module.functions)
            {
                visit(*func, dag);
            }
        }
        void DAGBuilder::visit(ME::Function& func, SelectionDAG& dag)
        {
            for (auto& pair : func.blocks)
            {
                visit(*pair.second, dag);
            }
        }

        SDValue DAGBuilder::getValue(ME::Operand* op, SelectionDAG& dag, BE::DataType* dtype)
        {
            if (!op) return SDValue();
            switch (op->getType())
            {
                case ME::OperandType::REG:
                {
                    // 虚拟寄存器：查找是否已创建对应节点，否则新建 CopyFromReg 节点
                    ASSERT(dtype != nullptr && "dtype required for REG operands");

                    size_t id = op->getRegNum();
                    auto   it = reg_value_map_.find(id);
                    if (it != reg_value_map_.end()) return it->second;

                    // 如果该虚拟寄存器尚未在 DAG 中出现，创建一个代表它的节点
                    // 这通常对应于函数参数或跨基本块的依赖
                    SDValue v = dag.getRegNode(id, dtype);

                    reg_value_map_[id] = v;
                    return v;
                }
                case ME::OperandType::IMMEI32:
                {
                    // 32位整数常量
                    auto imm = static_cast<ME::ImmeI32Operand*>(op)->value;
                    return dag.getConstantI64(imm, BE::I32);
                }
                case ME::OperandType::IMMEF32:
                {
                    // 32位浮点常量
                    auto imm = static_cast<ME::ImmeF32Operand*>(op)->value;
                    return dag.getConstantF32(imm, BE::F32);
                }
                case ME::OperandType::GLOBAL:
                {
                    // 全局变量地址
                    auto name = static_cast<ME::GlobalOperand*>(op)->name;
                    return dag.getSymNode(static_cast<unsigned>(ISD::SYMBOL), {BE::PTR}, {}, name);
                }
                case ME::OperandType::LABEL:
                {
                    // 基本块标签地址
                    auto labelId = static_cast<ME::LabelOperand*>(op)->lnum;
                    return dag.getImmNode(static_cast<unsigned>(ISD::LABEL), {}, {}, labelId);
                }
                default: ERROR("Unsupported IR operand in DAGBuilder"); return SDValue();
            }
        }

        void DAGBuilder::setDef(ME::Operand* res, const SDValue& val)
        {
            // 记录指令结果：将结果操作数(虚拟寄存器)映射到生成的 DAG 节点
            if (!res || res->getType() != ME::OperandType::REG) return;
            size_t regId          = res->getRegNum();
            reg_value_map_[regId] = val;
            // 设置节点的 IR 寄存器 ID，用于后续调度和寄存器分配
            if (val.getNode()) val.getNode()->setIRRegId(regId);
        }

        uint32_t DAGBuilder::mapArithmeticOpcode(ME::Operator op, bool isFloat)
        {
            // 将中间端(ME)的算术操作符映射到后端(BE)的 DAG 操作码(ISD)
            if (isFloat)
            {
                if (op == ME::Operator::FADD) return static_cast<uint32_t>(ISD::FADD);
                if (op == ME::Operator::FSUB) return static_cast<uint32_t>(ISD::FSUB);
                if (op == ME::Operator::FMUL) return static_cast<uint32_t>(ISD::FMUL);
                if (op == ME::Operator::FDIV) return static_cast<uint32_t>(ISD::FDIV);
                return static_cast<uint32_t>(ISD::FADD);
            }
            if (op == ME::Operator::ADD) return static_cast<uint32_t>(ISD::ADD);
            if (op == ME::Operator::SUB) return static_cast<uint32_t>(ISD::SUB);
            if (op == ME::Operator::MUL) return static_cast<uint32_t>(ISD::MUL);
            if (op == ME::Operator::DIV) return static_cast<uint32_t>(ISD::DIV);
            if (op == ME::Operator::MOD) return static_cast<uint32_t>(ISD::MOD);
            if (op == ME::Operator::SHL) return static_cast<uint32_t>(ISD::SHL);
            if (op == ME::Operator::ASHR) return static_cast<uint32_t>(ISD::ASHR);
            if (op == ME::Operator::LSHR) return static_cast<uint32_t>(ISD::LSHR);
            if (op == ME::Operator::BITAND) return static_cast<uint32_t>(ISD::AND);
            if (op == ME::Operator::BITXOR) return static_cast<uint32_t>(ISD::XOR);
            return static_cast<uint32_t>(ISD::ADD);
        }

        void DAGBuilder::visit(ME::Block& block, SelectionDAG& dag)
        {
            currentChain_ = dag.getNode(static_cast<unsigned>(ISD::ENTRY_TOKEN), {BE::TOKEN}, {});
            for (auto* inst : block.insts) apply(*this, *inst, dag);
        }

        void DAGBuilder::visit(ME::RetInst& inst, SelectionDAG& dag)
        {
            std::vector<SDValue> ops;

            // 结合此处考虑 currentChain_ 的作用是什么
            ops.push_back(currentChain_);

            if (inst.res == nullptr)
            {
                dag.getNode(static_cast<unsigned>(ISD::RET), {}, ops);
                return;
            }

            if (inst.res->getType() == ME::OperandType::IMMEI32)
            {
                auto v = dag.getNode(static_cast<unsigned>(ISD::CONST_I32), {I32}, {});
                v.getNode()->setImmI64(static_cast<ME::ImmeI32Operand*>(inst.res)->value);
                ops.push_back(v);
            }
            else if (inst.res->getType() == ME::OperandType::IMMEF32)
            {
                auto v = dag.getNode(static_cast<unsigned>(ISD::CONST_F32), {F32}, {});
                v.getNode()->setImmF32(static_cast<ME::ImmeF32Operand*>(inst.res)->value);
                ops.push_back(v);
            }
            else if (inst.res->getType() == ME::OperandType::REG)
            {
                auto v = getValue(inst.res, dag, mapType(inst.rt));
                ops.push_back(v);
            }
            else
                ERROR("Unsupported return operand type in DAGBuilder");

            dag.getNode(static_cast<unsigned>(ISD::RET), {}, ops);
        }

        void DAGBuilder::visit(ME::LoadInst& inst, SelectionDAG& dag)
        {
            auto    vt  = mapType(inst.dt);
            SDValue ptr = getValue(inst.ptr, dag, BE::PTR);
            // LOAD 节点：
            // 输入：Chain (依赖链), Address (地址)
            // 输出：Value (加载的值), NewChain (新的依赖链)
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::LOAD), {vt, BE::TOKEN}, {currentChain_, ptr});
            setDef(inst.res, SDValue(node.getNode(), 0));  // 结果0是加载的值
            currentChain_ = SDValue(node.getNode(), 1);    // 结果1是更新后的 Chain
        }

        void DAGBuilder::visit(ME::StoreInst& inst, SelectionDAG& dag)
        {
            auto    val  = getValue(inst.val, dag, mapType(inst.dt));
            SDValue ptr  = getValue(inst.ptr, dag, BE::PTR);
            // STORE 节点：
            // 输入：Chain, Value (要存储的值), Address (地址)
            // 输出：NewChain (新的依赖链)
            // Store 操作产生副作用，必须更新 currentChain_ 以保持顺序
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::STORE), {BE::TOKEN}, {currentChain_, val, ptr});
            currentChain_ = node;
        }

        void DAGBuilder::visit(ME::ArithmeticInst& inst, SelectionDAG& dag)
        {
            bool     f    = isFloatType(inst.dt);
            auto     vt   = mapType(inst.dt);
            SDValue  lhs  = getValue(inst.lhs, dag, vt);
            SDValue  rhs  = getValue(inst.rhs, dag, vt);
            uint32_t opc  = mapArithmeticOpcode(inst.opcode, f);
            SDValue  node = dag.getNode(opc, {vt}, {lhs, rhs});
            setDef(inst.res, node);
        }

        void DAGBuilder::visit(ME::IcmpInst& inst, SelectionDAG& dag)
        {
            auto    lhs  = getValue(inst.lhs, dag, BE::I32);
            auto    rhs  = getValue(inst.rhs, dag, BE::I32);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::ICMP), {BE::I32}, {lhs, rhs});
            node.getNode()->setImmI64(static_cast<int64_t>(inst.cond));
            setDef(inst.res, node);
        }

        void DAGBuilder::visit(ME::FcmpInst& inst, SelectionDAG& dag)
        {
            auto    lhs  = getValue(inst.lhs, dag, BE::F32);
            auto    rhs  = getValue(inst.rhs, dag, BE::F32);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::FCMP), {BE::I32}, {lhs, rhs});
            node.getNode()->setImmI64(static_cast<int64_t>(inst.cond));
            setDef(inst.res, node);
        }

        void DAGBuilder::visit(ME::AllocaInst& inst, SelectionDAG& dag)
        {
            size_t dest_id = static_cast<ME::RegOperand*>(inst.res)->getRegNum();

            DataType* ptr_ty = BE::I64;
            SDValue   v      = dag.getFrameIndexNode(static_cast<int>(dest_id), ptr_ty);

            v.getNode()->setIRRegId(dest_id);

            reg_value_map_[dest_id] = v;
        }

        void DAGBuilder::visit(ME::BrCondInst& inst, SelectionDAG& dag)
        {
            auto cond       = getValue(inst.cond, dag, BE::I32);
            auto trueLabel  = getValue(inst.trueTar, dag);
            auto falseLabel = getValue(inst.falseTar, dag);
            // 条件跳转：Chain, Cond, TrueLabel, FalseLabel
            dag.getNode(static_cast<unsigned>(ISD::BRCOND), {BE::TOKEN}, {currentChain_, cond, trueLabel, falseLabel});
        }

        void DAGBuilder::visit(ME::BrUncondInst& inst, SelectionDAG& dag)
        {
            auto target = getValue(inst.target, dag);
            // 无条件跳转：Chain, TargetLabel
            dag.getNode(static_cast<unsigned>(ISD::BR), {BE::TOKEN}, {currentChain_, target});
        }

        void DAGBuilder::visit(ME::GlbVarDeclInst& inst, SelectionDAG& dag)
        {
            (void)inst;
            (void)dag;
            ERROR("GlbVarDeclInst should not appear in DAGBuilder");
        }

        void DAGBuilder::visit(ME::CallInst& inst, SelectionDAG& dag)
        {
            std::vector<SDValue> ops;
            // 1. Chain 作为第一个操作数
            ops.push_back(currentChain_);
            // 2. 目标函数地址（符号）
            ops.push_back(dag.getSymNode(static_cast<unsigned>(ISD::SYMBOL), {BE::PTR}, {}, inst.funcName));

            // 3. 参数列表
            for (auto& argPair : inst.args)
            {
                ops.push_back(getValue(argPair.second, dag, mapType(argPair.first)));
            }

            if (inst.retType == ME::DataType::VOID)
            {
                // Void 函数调用：只返回 Chain
                SDValue node = dag.getNode(static_cast<unsigned>(ISD::CALL), {BE::TOKEN}, ops);
                currentChain_ = node;
            }
            else
            {
                // 非 Void 函数调用：返回 {Value, Chain}
                auto    vt   = mapType(inst.retType);
                SDValue node = dag.getNode(static_cast<unsigned>(ISD::CALL), {vt, BE::TOKEN}, ops);
                setDef(inst.res, SDValue(node.getNode(), 0)); // 结果值
                currentChain_ = SDValue(node.getNode(), 1);   // 更新 Chain
            }
        }

        void DAGBuilder::visit(ME::FuncDeclInst& inst, SelectionDAG& dag)
        {
            (void)inst;
            (void)dag;
            ERROR("FuncDeclInst should not appear in DAGBuilder");
        }
        void DAGBuilder::visit(ME::FuncDefInst& inst, SelectionDAG& dag)
        {
            (void)inst;
            (void)dag;
            ERROR("FuncDefInst should not appear in DAGBuilder");
        }

        [[maybe_unused]]
        static inline int elemByteSize(ME::DataType t)
        {
            switch (t)
            {
                case ME::DataType::I1:
                case ME::DataType::I8:
                case ME::DataType::I32: return 4;
                case ME::DataType::I64:
                case ME::DataType::PTR: return 8;
                case ME::DataType::F32: return 4;
                case ME::DataType::DOUBLE: return 8;
                default: return 4;
            }
        }

        void DAGBuilder::visit(ME::GEPInst& inst, SelectionDAG& dag)
        {
            // GetElementPtr 指令：计算内存地址偏移
            SDValue base        = getValue(inst.basePtr, dag, BE::PTR);
            SDValue totalOffset = dag.getConstantI64(0, BE::I64);
            int     elementSize = elemByteSize(inst.idxType);

            if (inst.dims.empty())
            {
                // 一维数组或指针解引用
                if (!inst.idxs.empty())
                {
                    SDValue idx = getValue(inst.idxs[0], dag, BE::I32);
                    idx         = dag.getNode(static_cast<unsigned>(ISD::ZEXT), {BE::I64}, {idx}); // 索引扩展为64位
                    SDValue sz  = dag.getConstantI64(elementSize, BE::I64);
                    totalOffset = dag.getNode(static_cast<unsigned>(ISD::MUL), {BE::I64}, {idx, sz}); // 偏移量 = 索引 * 元素大小
                }
            }
            else
            {
                // 多维数组
                // 计算 strides (步长)
                std::vector<int> strides(inst.dims.size());
                int              currentStride = elementSize;
                for (int i = (int)inst.dims.size() - 1; i >= 0; --i)
                {
                    strides[i] = currentStride;
                    currentStride *= inst.dims[i];
                }

                // 累加每个维度的偏移
                for (size_t i = 0; i < inst.idxs.size(); ++i)
                {
                    SDValue idx = getValue(inst.idxs[i], dag, BE::I32);
                    idx         = dag.getNode(static_cast<unsigned>(ISD::ZEXT), {BE::I64}, {idx});

                    int stride = 0;
                    if (i < inst.dims.size())
                    {
                        stride = strides[i];
                    }
                    else
                    {
                        stride = elementSize; // 最后一个维度之后的索引（如果有）通常按元素大小计算
                    }

                    SDValue strideNode = dag.getConstantI64(stride, BE::I64);
                    SDValue term       = dag.getNode(static_cast<unsigned>(ISD::MUL), {BE::I64}, {idx, strideNode});
                    totalOffset        = dag.getNode(static_cast<unsigned>(ISD::ADD), {BE::I64}, {totalOffset, term});
                }
            }

            // 最终地址 = 基地址 + 总偏移
            SDValue addr = dag.getNode(static_cast<unsigned>(ISD::ADD), {BE::PTR}, {base, totalOffset});
            setDef(inst.res, addr);
        }

        void DAGBuilder::visit(ME::ZextInst& inst, SelectionDAG& dag)
        {
            auto    val  = getValue(inst.src, dag, mapType(inst.from));
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::ZEXT), {mapType(inst.to)}, {val});
            setDef(inst.dest, node);
        }

        void DAGBuilder::visit(ME::SI2FPInst& inst, SelectionDAG& dag)
        {
            auto    val  = getValue(inst.src, dag, BE::I32);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::SITOFP), {BE::F32}, {val});
            setDef(inst.dest, node);
        }

        void DAGBuilder::visit(ME::FP2SIInst& inst, SelectionDAG& dag)
        {
            auto    val  = getValue(inst.src, dag, BE::F32);
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::FPTOSI), {BE::I32}, {val});
            setDef(inst.dest, node);
        }

        void DAGBuilder::visit(ME::PhiInst& inst, SelectionDAG& dag)
        {
            std::vector<SDValue> ops;
            auto                 vt = mapType(inst.dt);
            // PHI 节点：操作数成对出现 (IncomingValue, IncomingBlock)
            // 注意：DAG 构建通常在基本块内部进行。对于 PHI 节点，
            // LLVM SelectionDAG 通常将其转换为 CopyFromReg/CopyToReg 或特殊的 PHI 节点
            // 这里我们假设后端能处理这种形式的 PHI 节点，或者后续有 Pass 进行处理
            for (auto& pair : inst.incomingVals)
            {
                ops.push_back(getValue(pair.first, dag));
                ops.push_back(getValue(pair.second, dag, vt));
            }
            SDValue node = dag.getNode(static_cast<unsigned>(ISD::PHI), {vt}, ops);
            setDef(inst.res, node);
        }

    }  // namespace DAG
}  // namespace BE
