#include <backend/ra/linear_scan.h>
#include <backend/mir/m_function.h>
#include <backend/mir/m_instruction.h>
#include <backend/mir/m_block.h>
#include <backend/mir/m_defs.h>
#include <backend/target/target_reg_info.h>
#include <backend/target/target_instr_adapter.h>
#include <backend/common/cfg.h>
#include <backend/common/cfg_builder.h>
#include <utils/dynamic_bitset.h>
#include <debug.h>

#include <map>
#include <set>
#include <unordered_map>
#include <deque>
#include <algorithm>

namespace BE::RA
{
// RA ????????????????
static const BE::Targeting::TargetInstrAdapter* s_raAdapter = nullptr;

void setTargetInstrAdapter(const BE::Targeting::TargetInstrAdapter* adapter)
{
    s_raAdapter = adapter;
}

/*
 * 线性扫描寄存器分配 (Linear Scan Register Allocation)
 *
 * 算法步骤:
 * 1. 活跃区间分析 (Live Interval Analysis): 计算每个虚拟寄存器的活跃范围 (USE/DEF)。
 * 2. 活跃区间构建: 根据活跃变量分析结果，构建活跃区间。
 * 3. 线性扫描分配: 遍历活跃区间，分配物理寄存器，处理溢出 (Spill)。
 * 4. 栈槽分配: 为溢出的寄存器分配栈空间 (Spill Slot)。
 * 5. 代码重写: 将虚拟寄存器替换为物理寄存器或栈访问。
 */
namespace
{
    struct Segment
    {
        int start;
        int end;
        Segment(int s = 0, int e = 0) : start(s), end(e) {}
    };

    // 活跃区间结构体
    struct Interval
    {
        BE::Register         vreg;      // 对应的虚拟寄存器
        std::vector<Segment> segs;      // 活跃段列表 (已排序且不相交)
        bool                 crossesCall = false; // 是否跨越函数调用 (影响 Caller-Saved 寄存器的选择)

        // 分配结果
        int assignedPhys    = -1;       // 分配到的物理寄存器 ID
        int spillFrameIndex = -1;       // 溢出时的栈帧索引
        
        // Coalescing (合并) 指针
        Interval* parent = nullptr;

        Interval* getLeader()
        {
            return parent ? parent->getLeader() : this;
        }

        // 合并另一个区间到当前区间
        void mergeFrom(Interval* other)
        {
            segs.insert(segs.end(), other->segs.begin(), other->segs.end());
            merge(); 
            if (other->crossesCall) crossesCall = true;
        }

        // 检查是否与另一个区间相交
        bool intersects(const Interval* other) const
        {
            auto it1 = segs.begin();
            auto it2 = other->segs.begin();
            while (it1 != segs.end() && it2 != other->segs.end())
            {
                if (it1->end <= it2->start)
                {
                    ++it1;
                }
                else if (it2->end <= it1->start)
                {
                    ++it2;
                }
                else
                {
                    return true;
                }
            }
            return false;
        }

        void addSegment(int s, int e)
        {
            if (s >= e) return;
            segs.emplace_back(s, e);
        }

        // 合并重叠的段
        void merge()
        {
            if (segs.empty()) return;
            // 按 start 排序
            std::sort(segs.begin(), segs.end(), [](const Segment& a, const Segment& b) {
                return a.start < b.start;
            });
            
            std::vector<Segment> merged;
            merged.push_back(segs[0]);
            for (size_t i = 1; i < segs.size(); ++i)
            {
                // 如果当前段与上一个段重叠或相邻，则合并
                if (segs[i].start <= merged.back().end)
                {
                    merged.back().end = std::max(merged.back().end, segs[i].end);
                }
                else
                {
                    merged.push_back(segs[i]);
                }
            }
            segs = std::move(merged);
        }

        int startPoint() const
        {
            if (segs.empty()) return 0;
            return segs.front().start;
        }

        int endPoint() const
        {
            if (segs.empty()) return 0;
            return segs.back().end;
        }

        bool covers(int point) const
        {
            for (const auto& seg : segs)
            {
                if (point >= seg.start && point < seg.end) return true;
            }
            return false;
        }
    };

    struct IntervalOrder
    {
        bool operator()(const Interval* a, const Interval* b) const
        {
            // 按起始位置排序
            if (a->startPoint() != b->startPoint())
                return a->startPoint() < b->startPoint();
            return a->vreg.rId < b->vreg.rId;
        }
    };

    struct ActiveOrder
    {
        bool operator()(const Interval* a, const Interval* b) const
        {
            // 按结束位置排序 (用于 Active 列表)
            if (a->endPoint() != b->endPoint())
                return a->endPoint() < b->endPoint();
            return a->vreg.rId < b->vreg.rId;
        }
    };

    bool isFloatType(BE::DataType* dt)
    {
        if (!dt) return false;
        return dt->dt == BE::DataType::Type::FLOAT;
    }

    const BE::Targeting::TargetInstrAdapter* getAdapter()
    {
        if (s_raAdapter) return s_raAdapter;
        return BE::Targeting::g_adapter;
    }
}  // namespace

static std::vector<int> buildAllocatableInt(const BE::Targeting::TargetRegInfo& ri)
{
    std::vector<int> result;
    const auto& reserved = ri.reservedRegs();
    std::set<int> reservedSet(reserved.begin(), reserved.end());

    for (int reg : ri.intRegs())
    {
        if (reservedSet.find(reg) == reservedSet.end())
        {
            result.push_back(reg);
        }
    }
    return result;
}

static std::vector<int> buildAllocatableFloat(const BE::Targeting::TargetRegInfo& ri)
{
    std::vector<int> result;
    const auto& reserved = ri.reservedRegs();
    std::set<int> reservedSet(reserved.begin(), reserved.end());

    for (int reg : ri.floatRegs())
    {
        if (reservedSet.find(reg) == reservedSet.end())
        {
            result.push_back(reg);
        }
    }
    return result;
}

void LinearScanRA::allocateFunction(BE::Function& func, const BE::Targeting::TargetRegInfo& regInfo)
{
    fprintf(stderr, "DEBUG: RA allocateFunction for %s\n", func.name.c_str());
    fflush(stderr);
    
    const auto* adapter = getAdapter();
    ASSERT(adapter && "TargetInstrAdapter is not set");

    if (func.blocks.empty()) return;

    // 1. 给指令编号 (Number instructions)
    fprintf(stderr, "DEBUG: RA Step 1: Number instructions\n");
    fflush(stderr);
    std::map<BE::Block*, std::pair<int, int>> blockRange;
    std::set<int>                             callPoints;
    int                                       ins_id = 0;

    // 按 blockId 排序 Block，确保顺序确定性
    std::vector<BE::Block*> orderedBlocks;
    for (auto& [bid, block] : func.blocks)
    {
        orderedBlocks.push_back(block);
    }
    std::sort(orderedBlocks.begin(), orderedBlocks.end(), [](BE::Block* a, BE::Block* b) {
        return a->blockId < b->blockId;
    });

    for (auto* block : orderedBlocks)
    {
        int start = ins_id;
        // 每条指令 ID 递增 2，以便在指令之间插入 Move 指令（例如 Spill/Reload 代码）
        // 奇数 ID 可用于后续插入代码，保持原指令 ID 为偶数
        for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
        {
            if (!*it) continue;
            // adapter->setInstructionId(*it, ins_id); // 可选：保存指令 ID
            if (adapter->isCall(*it)) callPoints.insert(ins_id);
            ins_id++;
        }
        // 记录基本块的指令范围 [start, ins_id)
        blockRange[block] = {start, ins_id};
    }

    // 2. USE/DEF 分析
    fprintf(stderr, "DEBUG: RA Step 2: USE/DEF analysis\n");
    fflush(stderr);
    // USE[B]: 在 Block B 中被使用，且在使用前未在 B 中被定义的变量集合
    // DEF[B]: 在 Block B 中被定义的变量集合
    std::map<int, BE::Register> vregInfo;
    std::map<BE::Block*, std::set<int>> USE, DEF;
    
    for (auto* block : orderedBlocks)
    {
        std::set<int> use, def;
        
        
        for (auto it = block->insts.begin(); it != block->insts.end(); ++it)
        {
            if (!*it) continue;
            std::vector<BE::Register> uses, defs;
            adapter->enumUses(*it, uses);
            adapter->enumDefs(*it, defs);

            // 调试输出
            if (block->blockId == 4) {
                fprintf(stderr, "  Inst: uses=%zu defs=%zu\n", uses.size(), defs.size());
                for (auto& u : uses) {
                    if (u.isVreg && u.rId == 32) {
                        fprintf(stderr, "    Found vreg32 in uses!\n");
                    }
                }
            }

            // 处理 USE：如果在当前 Block 中尚未被定义（def 集合），则加入 use 集合
            // 意味着该变量是 live-in 的候选
            for (auto& u : uses)
            {
                if (u.isVreg) {
                    int key = u.rId;
                    if (!def.count(key)) 
                        use.insert(key);
                    if (vregInfo.find(key) == vregInfo.end() || vregInfo[key].dt == nullptr)
                        vregInfo[key] = u;
                }
            }

            // 处理 DEF：加入 def 集合，屏蔽后续的 use
            for (auto& d : defs)
            {
                if (d.isVreg) {
                    int key = d.rId;
                    def.insert(key);
                    if (vregInfo.find(key) == vregInfo.end() || vregInfo[key].dt == nullptr)
                        vregInfo[key] = d;
                }
            }
        }
        
        // 调试输出 Block4 的最终 USE 集合
        if (block->blockId == 4) {
            fprintf(stderr, "  Block4 final USE set: ");
            for (int id : use) fprintf(stderr, "%d ", id);
            fprintf(stderr, "\n");
        }
        
        USE[block] = std::move(use);
        DEF[block] = std::move(def);
    }

    // 3. 构建控制流图 (CFG)
    fprintf(stderr, "DEBUG: RA Step 3: Build CFG\n");
    fflush(stderr);
    BE::MIR::CFGBuilder cfgBuilder(adapter);
    BE::MIR::CFG*       cfg = cfgBuilder.buildCFGForFunction(&func);
    std::map<BE::Block*, std::vector<BE::Block*>> succs;

    for (auto* block : orderedBlocks)
    {
        if (block->blockId < cfg->graph.size())
        {
            for (auto* succBlock : cfg->graph[block->blockId])
            {
                succs[block].push_back(succBlock);
            }
        }
    }

    // 4. 活跃变量分析 (Backward Dataflow: IN[B] = USE[B] U (OUT[B] - DEF[B]))
    fprintf(stderr, "DEBUG: RA Step 4: Liveness Analysis\n");
    fflush(stderr);
    std::map<BE::Block*, std::set<int>> IN, OUT;
    
    // 初始化 IN = USE (初始假设)
    for (auto* block : orderedBlocks)
    {
        IN[block] = USE[block];
    }

    bool changed = true;
    while (changed)
    {
        changed = false;
        // 反向遍历基本块 (reverse order)，加速收敛
        for (auto it = orderedBlocks.rbegin(); it != orderedBlocks.rend(); ++it)
        {
            BE::Block* block = *it;

            std::set<int> newOUT;
            for (auto* s : succs[block])
            {
                // OUT[B] = U(IN[Successors])
                const auto& succIN = IN[s];
                newOUT.insert(succIN.begin(), succIN.end());
            }

            // IN[B] = USE[B] U (OUT[B] - DEF[B])
            std::set<int> newIN = USE[block];
            for (auto& r : newOUT)
            {
                if (!DEF[block].count(r))
                {
                    newIN.insert(r);
                }
            }

            if (newOUT != OUT[block] || newIN != IN[block])
            {
                OUT[block] = std::move(newOUT);
                IN[block]  = std::move(newIN);
                changed    = true;
            }
        }
    }
    delete cfg;

    fprintf(stderr, "DEBUG: Liveness for vreg32 (rId=32):\n");
    for (auto* block : orderedBlocks) {
        bool inUSE = USE[block].count(32);
        bool inDEF = DEF[block].count(32);
        bool inIN = IN[block].count(32);
        bool inOUT = OUT[block].count(32);
        if (inUSE || inDEF || inIN || inOUT) {
            fprintf(stderr, "  Block%u: USE=%d DEF=%d IN=%d OUT=%d\n",
                    block->blockId, inUSE, inDEF, inIN, inOUT);
        }
    }

    // 5. 构建活跃区间 (Build Intervals)
    fprintf(stderr, "DEBUG: RA Step 5: Build Intervals\n");
    fflush(stderr);
    std::map<int, Interval> intervals;

    for (auto* block : orderedBlocks)
    {
        int blockStart = blockRange[block].first;
        int blockEnd   = blockRange[block].second;

        // live 集合初始化为 block 的 OUT 集合
        // 代表这些变量在 block 结束时仍然活跃
        std::set<int> live = OUT[block];

        // 记录活跃范围的终点。对于 OUT 集合中的变量，它们在 block 内的活跃范围延伸到 blockEnd
        std::map<int, int> rangeEnd;
        for (int rId : live)
        {
            rangeEnd[rId] = blockEnd;
            if (intervals.find(rId) == intervals.end() && vregInfo.count(rId)) {
                intervals[rId].vreg = vregInfo[rId];
            }
        }

        int pos = blockEnd - 1;
        // 反向遍历指令，更新活跃区间
        for (auto it = block->insts.rbegin(); it != block->insts.rend(); ++it, --pos)
        {
            BE::MInstruction* inst = *it;
            if (!inst) continue;

            std::vector<BE::Register> uses, defs;
            adapter->enumUses(inst, uses);
            adapter->enumDefs(inst, defs);

            // 处理 DEF：缩短活跃区间
            for (auto& d : defs)
            {
                if (!d.isVreg) continue;
                int rId = d.rId;
                
                if (intervals.find(rId) == intervals.end()) {
                    intervals[rId].vreg = d;
                } else if (intervals[rId].vreg.dt == nullptr && d.dt != nullptr) {
                    intervals[rId].vreg.dt = d.dt;
                }

                if (live.count(rId))
                {
                    // 变量在当前位置被定义，所以它的活跃范围是 [pos, rangeEnd)
                    // 同时从 live 集合中移除，因为它在此之前（向上）不再活跃（直到遇到更早的 USE）
                    intervals[rId].addSegment(pos, rangeEnd[rId]);
                    live.erase(rId);
                    rangeEnd.erase(rId);
                }
                else
                {
                    // 这是一个 Dead Definition (定义了但未使用)，或者只是局部定义
                    // 为了正确性，添加一个长度为 1 的区间 [pos, pos+1)
                    intervals[rId].addSegment(pos, pos + 1);
                }
            }

            // 处理 USE：延长活跃区间（向前延伸）
            for (auto& u : uses)
            {
                if (!u.isVreg) continue;
                int rId = u.rId;
                
                if (intervals.find(rId) == intervals.end()) {
                    intervals[rId].vreg = u;
                } else if (intervals[rId].vreg.dt == nullptr && u.dt != nullptr) {
                    intervals[rId].vreg.dt = u.dt;
                }

                if (!live.count(rId))
                {
                    // 变量在此处被使用，标记为活跃
                    // 它的活跃范围终点暂定为 pos+1 (因为指令执行需要输入)
                    live.insert(rId);
                    rangeEnd[rId] = pos + 1;
                }
                // 如果已经在 live 集合中，说明它是从更后面的位置传过来的，这里只是中间的使用点，不需要改变 rangeEnd
            }
        }

        // 处理 block 的 live-in 变量
        // 它们在 block 开始处就是活跃的，范围是 [blockStart, rangeEnd)
        for (int rId : live)
        {
            intervals[rId].addSegment(blockStart, rangeEnd[rId]);
        }
    }

    // 处理跨越函数调用的情况
    fprintf(stderr, "DEBUG: CallPoints: ");
    for (int cp : callPoints) fprintf(stderr, "%d ", cp);
    fprintf(stderr, "\n");

    for (auto& [rId, interval] : intervals)
    {
        interval.merge();
        for (int callPt : callPoints)
        {
            if (interval.covers(callPt))
            {
                interval.crossesCall = true;
                break;
            }
        }
    }

    fprintf(stderr, "DEBUG: Interval for vreg32 (b8):\n");
    for (auto& [rId, interval] : intervals) {
        if (rId == 32) {
            fprintf(stderr, "  vreg32: segments = ");
            for (auto& seg : interval.segs) {
                fprintf(stderr, "[%d,%d) ", seg.start, seg.end);
            }
            fprintf(stderr, ", crossesCall = %d\n", interval.crossesCall);
            break;
        }
    }

    // 6. 寄存器分配 (Allocate Registers)
    fprintf(stderr, "DEBUG: RA Step 6: Allocate Registers\n");
    fflush(stderr);
    
    // === Move Coalescing (合并) ===
    // 简单的合并尝试，主要是在构建区间时完成的，这里再次确保区间是合并的
    for (auto& [rId, interval] : intervals)
    {
        interval.merge();
    }

    auto allIntRegs   = buildAllocatableInt(regInfo);
    auto allFloatRegs = buildAllocatableFloat(regInfo);

    // 保留 Scratch 寄存器用于 Spill 代码生成
    // 通常需要保留 1-2 个通用寄存器来处理栈地址计算或内存移动
    // 这里简单策略：从可分配列表中移除最后 2 个作为 Scratch
    std::vector<int> scratchInts;
    std::vector<int> scratchFloats;
    
    // 保留 2 个整数寄存器
    for (int i = 0; i < 2; ++i)
    {
        if (!allIntRegs.empty())
        {
            scratchInts.push_back(allIntRegs.back());
            allIntRegs.pop_back();
        }
    }
    
    // 保留 2 个浮点寄存器
    for (int i = 0; i < 2; ++i)
    {
        if (!allFloatRegs.empty())
        {
            scratchFloats.push_back(allFloatRegs.back());
            allFloatRegs.pop_back();
        }
    }

    // 分离整数和浮点区间 (因为它们使用不同的寄存器文件)
    std::vector<Interval*> intIntervals, floatIntervals;
    for (auto& [rId, interval] : intervals)
    {
        // 跳过被合并的区间 (非 Leader)
        if (interval.parent != nullptr) continue;
        
        if (interval.segs.empty()) continue;
        if (isFloatType(interval.vreg.dt))
            floatIntervals.push_back(&interval);
        else
            intIntervals.push_back(&interval);
    }

    // 按起始位置排序
    std::sort(intIntervals.begin(), intIntervals.end(), IntervalOrder());
    std::sort(floatIntervals.begin(), floatIntervals.end(), IntervalOrder());

    // 线性扫描分配算法核心
    auto allocateRegs = [&](std::vector<Interval*>& intervalList,
                            const std::vector<int>& allocatable,
                            const std::vector<int>& calleeSaved) {
        std::set<int> calleeSavedSet(calleeSaved.begin(), calleeSaved.end());
        // Active 列表：当前占用寄存器的区间，按结束位置排序
        std::set<Interval*, ActiveOrder> active;
        std::set<int> freeRegs(allocatable.begin(), allocatable.end());

        for (auto* interval : intervalList)
        {
            int curStart = interval->startPoint();

            // 1. 过期旧区间：释放结束位置在当前区间开始位置之前的寄存器
            for (auto it = active.begin(); it != active.end(); )
            {
                if ((*it)->endPoint() <= curStart)
                {
                    freeRegs.insert((*it)->assignedPhys);
                    it = active.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            int selectedReg = -1;

            // 2. 选择寄存器策略
            // 如果区间跨越了函数调用，优先使用 Callee-Saved 寄存器 (被调用者保存)，
            // 这样在调用期间不需要保存/恢复（由被调用者负责，或者只需要保存一次）。
            if (interval->crossesCall)
            {
                for (int r : freeRegs)
                {
                    if (calleeSavedSet.count(r))
                    {
                        selectedReg = r;
                        break;
                    }
                }
            }
            else
            {
                // 如果不跨越调用，优先使用 Caller-Saved (临时寄存器)，避免不必要的 Callee-Saved 保存/恢复开销
                for (int r : freeRegs)
                {
                    if (!calleeSavedSet.count(r))
                    {
                        selectedReg = r;
                        break;
                    }
                }
                // 如果 Caller-Saved 用完了，再尝试 Callee-Saved
                if (selectedReg == -1)
                {
                    for (int r : freeRegs)
                    {
                        if (calleeSavedSet.count(r))
                        {
                            selectedReg = r;
                            break;
                        }
                    }
                }
            }

            if (selectedReg != -1)
            {
                // 分配成功
                interval->assignedPhys = selectedReg;
                freeRegs.erase(selectedReg);
                active.insert(interval);
            }
            else
            {
                // 3. 溢出处理 (Spilling)
                // 启发式：溢出结束位置最晚的区间 (Spill the interval that ends the latest)
                // 这样可以最大程度地释放寄存器资源
                Interval* spillCandidate = interval;
                for (auto* act : active)
                {
                    // 注意：如果是跨越调用的区间，且占用了 Caller-Saved，通常应该 Spill 它？
                    // 这里简化逻辑：比较结束位置
                    
                    // 特殊情况：如果当前 interval 必须跨越调用，而 active 中的是 Callee-Saved，
                    // 我们不能简单抢占，除非我们确定该 Callee-Saved 也能被 Spill。
                    // 这里的逻辑主要基于结束位置。
                    if (interval->crossesCall && !calleeSavedSet.count(act->assignedPhys))
                    {
                        // 略过
                        continue;
                    }

                    if (act->endPoint() > spillCandidate->endPoint())
                    {
                        spillCandidate = act;
                    }
                }

                if (spillCandidate != interval)
                {
                    // 抢占寄存器：Spill 候选者，将其寄存器分配给当前区间
                    interval->assignedPhys = spillCandidate->assignedPhys;
                    spillCandidate->assignedPhys = -1;
                    spillCandidate->spillFrameIndex = func.frameInfo.createSpillSlot(8, 8); // 分配 8 字节栈槽
                    active.erase(spillCandidate);
                    active.insert(interval);
                }
                else
                {
                    // 当前区间被 Spill
                    interval->spillFrameIndex = func.frameInfo.createSpillSlot(8, 8);
                }
            }
        }
    };

    allocateRegs(intIntervals, allIntRegs, regInfo.calleeSavedIntRegs());
    allocateRegs(floatIntervals, allFloatRegs, regInfo.calleeSavedFloatRegs());

    // 7. 重写 MIR (Rewrite MIR)
    fprintf(stderr, "DEBUG: RA Step 7: Rewrite MIR\n");
    fflush(stderr);
    std::map<int, Interval*> vregToInterval;
    for (auto& [rId, interval] : intervals)
    {
        vregToInterval[rId] = interval.getLeader();
    }

    for (auto* block : orderedBlocks)
    {
        for (size_t idx = 0; idx < block->insts.size(); ++idx)
        {
            BE::MInstruction* inst = block->insts[idx];
            if (!inst) continue;

            std::vector<BE::Register> uses, defs;
            adapter->enumUses(inst, uses);
            adapter->enumDefs(inst, defs);

            // 处理 USE
            int spillUseCountInt = 0;
            int spillUseCountFloat = 0;
            int reloadInserted = 0;  // 记录插入了多少条 reload 指令，以便调整 idx

            for (auto& u : uses)
            {
                if (!u.isVreg) continue;
                int rId = u.rId;
                auto intervalIt = vregToInterval.find(rId);
                if (intervalIt == vregToInterval.end()) continue;

                Interval* interval = intervalIt->second;
                if (interval->assignedPhys >= 0)
                {
                    // 已分配物理寄存器，直接替换
                    DataType* finalDt = interval->vreg.dt != nullptr ? interval->vreg.dt : u.dt;
                    BE::Register physReg(interval->assignedPhys, finalDt, false);
                    adapter->replaceUse(inst, u, physReg);
                }
                else if (interval->spillFrameIndex >= 0)
                {
                    // 寄存器已溢出到栈
                    // 需要使用 Scratch 寄存器从栈中 Reload
                    DataType* finalDt = interval->vreg.dt != nullptr ? interval->vreg.dt : u.dt;
                    bool isFloat = isFloatType(finalDt);
                    
                    int scratch = -1;
                    if (isFloat)
                    {
                         if (scratchFloats.empty()) {
                            ASSERT(false && "No float scratch register available!");
                         }
                         // 简单策略：轮询使用 Scratch 寄存器
                         scratch = scratchFloats[spillUseCountFloat % scratchFloats.size()];
                         spillUseCountFloat++;
                    }
                    else
                    {
                         if (scratchInts.empty()) {
                            ASSERT(false && "No int scratch register available!");
                         }
                         scratch = scratchInts[spillUseCountInt % scratchInts.size()];
                         spillUseCountInt++;
                    }

                    BE::Register scratchReg(scratch, finalDt, false);
                    // 在当前指令前插入 reload
                    auto it = block->insts.begin() + idx + reloadInserted;
                    adapter->insertReloadBefore(block, it, scratchReg, interval->spillFrameIndex);
                    reloadInserted++;  // 更新偏移
                    adapter->replaceUse(inst, u, scratchReg);
                }
            }

            // 更新 idx，跳过新插入的 reload 指令
            idx += reloadInserted;

            // 处理 DEF
            for (auto& d : defs)
            {
                if (!d.isVreg) continue;
                int rId = d.rId;
                auto intervalIt = vregToInterval.find(rId);
                if (intervalIt == vregToInterval.end()) continue;

                Interval* interval = intervalIt->second;
                if (interval->assignedPhys >= 0)
                {
                    DataType* finalDt = interval->vreg.dt != nullptr ? interval->vreg.dt : d.dt;
                    BE::Register physReg(interval->assignedPhys, finalDt, false);
                    adapter->replaceDef(inst, d, physReg);
                }
                else if (interval->spillFrameIndex >= 0)
                {
                    // 结果需要写回栈
                    // 先写入 Scratch，然后 Spill 到栈
                    DataType* finalDt = interval->vreg.dt != nullptr ? interval->vreg.dt : d.dt;
                    bool isFloat = isFloatType(finalDt);
                    
                    int scratch = -1;
                    if (isFloat)
                    {
                         if (scratchFloats.empty()) {
                            ASSERT(false && "No float scratch register available!");
                         }
                         scratch = scratchFloats[0];
                    }
                    else
                    {
                         if (scratchInts.empty()) {
                            ASSERT(false && "No int scratch register available!");
                         }
                         scratch = scratchInts[0];
                    }

                    BE::Register scratchReg(scratch, finalDt, false);
                    adapter->replaceDef(inst, d, scratchReg);
                    // 在当前指令后插入 spill
                    auto it = block->insts.begin() + idx;
                    adapter->insertSpillAfter(block, it, scratchReg, interval->spillFrameIndex);
                    // 注意：insertSpillAfter 不会改变 idx，因为它是在 idx 之后插入
                }
            }

            // 移除冗余的 Move 指令 (Coalescing 效果: MOV x, x)
            BE::Register pDst, pSrc;
            inst = block->insts[idx];  // 重新获取指令，以防 vector 扩容（虽然在这里不太可能失效，但保险起见）
            if (adapter->isCopy(inst, pDst, pSrc) && pDst.rId == pSrc.rId && pDst.isVreg == pSrc.isVreg)
            {
                block->insts.erase(block->insts.begin() + idx);
                --idx;  // 回退 idx，因为删除了当前元素
            }
        }
    }
}
} // namespace BE::RA