#ifndef __MIDDLEEND_MODULE_IR_FUNCTION_H__
#define __MIDDLEEND_MODULE_IR_FUNCTION_H__

#include <middleend/module/ir_block.h>
#include <map>

namespace ME
{
    class Function : public Visitable
    {
      public:
        FuncDefInst*             funcDef;
        std::map<size_t, Block*> blocks;

      private:
        size_t maxLabel;  //记录当前已分配的最大标签ID
        size_t maxReg;    //记录函数内已使用的最大虚拟寄存器ID

      public: /*以下2个变量与循环优化相关，如果你正在做Lab3，可以暂时忽略它们 */
        size_t loopStartLabel;
        size_t loopEndLabel;

      public:
        Function(FuncDefInst* fd);
        ~Function();

      public:
        virtual void accept(Visitor& visitor) override { visitor.visit(*this); }

        //创建并返回一个基本块
        Block* createBlock();
        //通过标签ID查找对应Block对象
        Block* getBlock(size_t label);
        void   setMaxReg(size_t reg);
        size_t getMaxReg();
        void   setMaxLabel(size_t label);
        size_t getMaxLabel();
        //获取一个新的唯一的虚拟寄存器ID，并更新maxReg计数器
        size_t getNewRegId();
    };
}  // namespace ME

#endif  // __MIDDLEEND_MODULE_IR_FUNCTION_H__
