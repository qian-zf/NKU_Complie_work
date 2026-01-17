#include <dom_analyzer.h>
#include <debug.h>
#include <cassert>
#include <functional>
#include <algorithm>

/**
 * @file dom_analyser.cpp
 * @brief 支配树 (Dominator Tree) 与支配边界 (Dominance Frontier) 分析器实现
 * 
 * 核心算法基于 Lengauer-Tarjan (LT) 算法，这是一种高效求解支配树的算法，时间复杂度接近线性 O(M alpha(N))。
 * 
 * 基本概念：
 * - 支配 (Dominates): 如果从入口到节点 u 的所有路径都必须经过节点 d，则称 d 支配 u (d dom u)。
 * - 直接支配 (Immediate Dominator, idom): 在所有严格支配 u 的节点中，距离 u 最近的那个。
 * - 支配边界 (Dominance Frontier, DF): 节点 d 的支配边界是所有不被 d 严格支配，但 d 支配其某个前驱的节点集合。
 *   DF 是构建 SSA 形式时插入 PHI 节点的关键依据。
 */

using namespace std;

DomAnalyzer::DomAnalyzer() {}

/**
     * @brief 求解支配树和支配边界的主入口
     * 
     * 该函数负责构建辅助图（如添加虚拟源点），并调用核心构建算法。
     * 
     * 为什么要添加虚拟源点？
     * - 保证流图有唯一的入口节点 (Single Entry)，简化算法实现。
     * - 对于有多个入口（如死代码导致的孤立块）或计算后支配树（多个出口）的情况，虚拟源点充当统一的根。
     * 
     * @param graph 输入图的邻接表表示
     * @param entry_points 图的入口节点列表 (通常只有一个，但在计算 Post-Dom 时可能有多个出口)
     * @param reverse 是否反向计算。
     *                - false: 计算支配树 (Dominator Tree)。
     *                - true: 计算后支配树 (Post-Dominator Tree)，此时会将图的边反向。
     */
    void DomAnalyzer::solve(const vector<vector<int>>& graph, const vector<int>& entry_points, bool reverse)
{
    int node_count = graph.size();

    // 创建一个虚拟源点，连接所有实际的入口点
    // 这样做可以统一处理多入口函数或多出口（反向时）的情况，保证图的单源特性
    int                 virtual_source = node_count;
    vector<vector<int>> working_graph;

    if (!reverse)
    {
        // 正向支配树：虚拟源点 -> 所有入口点
        working_graph = graph;
        working_graph.push_back(vector<int>());
        for (int entry : entry_points) working_graph[virtual_source].push_back(entry);
    }
    else
    {
        // 后支配树：构建反图 (Reverse Graph)
        // 原图边 u->v 变为 v->u
        // 虚拟源点 -> 所有出口点 (原图的入口点变为反图的出口，这里参数命名 entry_points 实际上在 reverse=true 时指代 exit_points)
        working_graph.resize(node_count + 1);
        for (int u = 0; u < node_count; ++u)
            for (int v : graph[u]) working_graph[v].push_back(u);

        working_graph.push_back(vector<int>());
        for (int exit : entry_points) working_graph[virtual_source].push_back(exit);
    }

    // 调用核心构建函数
    build(working_graph, node_count + 1, virtual_source, entry_points);
}

/**
 * @brief Lengauer-Tarjan 算法的核心实现
 * 
 * 算法步骤：
 * 1. DFS 遍历：计算 DFS 序 (dfn) 和父节点 (parent)，建立半支配点 (semi-dominator) 的初始值。
 * 2. 逆 DFS 序遍历：
 *    - 计算半支配点 (sdom)。
 *    - 维护一个并查集 (DSU) 用于路径压缩和快速查询最小 sdom。
 *    - 隐式计算直接支配点 (idom)。
 * 3. 修正 idom：处理 sdom != idom 的情况。
 * 4. 构建支配树：根据 idom 数组构建树结构。
 * 5. 计算支配边界 (Dominance Frontier)。
 */
void DomAnalyzer::build(
    const vector<vector<int>>& working_graph, int node_count, int virtual_source, const std::vector<int>& entry_points)
{
    (void)entry_points; // 未使用参数，避免警告
    
    // 1. 构建反向图 (Preds)，方便查找前驱节点
    // 算法需要知道每个节点的前驱 (predecessors)
    vector<vector<int>> backward_edges(node_count);
    for (int u = 0; u < node_count; ++u)
    {
        for (int v : working_graph[u])
        {
            backward_edges[v].push_back(u);
        }
    }

    // 初始化结果容器
    dom_tree.clear();
    dom_tree.resize(node_count);
    dom_frontier.clear();
    dom_frontier.resize(node_count);
    imm_dom.clear();
    imm_dom.resize(node_count);

    // 2. DFS 遍历初始化
    // block_to_dfs: 节点 ID -> DFS 序
    // dfs_to_block: DFS 序 -> 节点 ID
    int                 dfs_count = -1;
    vector<int>         block_to_dfs(node_count, 0), dfs_to_block(node_count), parent(node_count, 0);
    vector<int>         semi_dom(node_count);
    vector<int>         dsu_parent(node_count), min_ancestor(node_count);
    vector<vector<int>> semi_children(node_count);

    for (int i = 0; i < node_count; ++i)
    {
        dsu_parent[i]   = i; // 并查集父节点
        min_ancestor[i] = i; // 路径上具有最小半支配 dfn 的节点
        semi_dom[i]     = i; // 初始半支配节点为自身
    }

    // DFS Lambda 函数
    function<void(int)> dfs = [&](int block) {
        block_to_dfs[block]     = ++dfs_count;
        dfs_to_block[dfs_count] = block;
        semi_dom[block]         = block_to_dfs[block]; // 初始 sdom 为其 dfn
        for (int next : working_graph[block])
            if (!block_to_dfs[next])
            {
                dfs(next);
                parent[next] = block;
            }
    };
    dfs(virtual_source);

    // 并查集查找 (Find) 与路径压缩 (Path Compression)
    // 作用：在森林中查找 u 的祖先中，半支配节点 dfn 最小的那个节点
    // 返回：当前并查集的根
    // 为什么需要路径压缩？
    // - 为了快速查询 `dsu_query`，避免退化成 O(N)。
    // - 压缩过程中同时维护 `min_ancestor` 属性，保证查询结果正确。
    auto dsu_find = [&](int u, const auto& self) -> int {
        if (dsu_parent[u] == u) return u;
        int root = self(dsu_parent[u], self);
        
        // 路径压缩的同时维护 min_ancestor
        // 如果父节点的 min_ancestor 的半支配节点 dfn 更小，则更新当前节点的 min_ancestor
        if (semi_dom[min_ancestor[dsu_parent[u]]] < semi_dom[min_ancestor[u]])
        {
            min_ancestor[u] = min_ancestor[dsu_parent[u]];
        }
        dsu_parent[u] = root;
        return root;
    };

    // 查询辅助函数：返回 u 到其并查集根路径上 sdom 最小的节点
    auto dsu_query = [&](int u) -> int {
        dsu_find(u, dsu_find);
        return min_ancestor[u];
    };
    
    // 消除未使用警告 (实际上 dsu_query 被使用了)
    (void)dsu_find;
    (void)dsu_query;

    // 3. 逆 DFS 序遍历，计算半支配点 (Semi-Dominator) 并隐式计算直接支配点 (idom)
    for (int dfs_id = dfs_count; dfs_id >= 1; --dfs_id)
    {
        int curr = dfs_to_block[dfs_id];
        
        // 步骤 3.1: 计算半支配点 sdom(curr)
        // 公式：sdom(w) = min({dfn(v) | (v, w) 是树边} U {sdom(eval(v)) | (v, w) 是非树边})
        for (int pred : backward_edges[curr])
        {
            // 跳过不可达节点
            if (pred != virtual_source && block_to_dfs[pred] == 0) continue;

            int eval_node;
            if (block_to_dfs[pred] < block_to_dfs[curr])
            {
                // 如果 pred 是 DFS 树上的祖先，直接考虑 pred
                eval_node = pred;
            }
            else
            {
                // 如果 pred 是后代或横向节点，需要在并查集中查找
                // dsu_query 返回 pred 在 DFS 树上的祖先中（但在 curr 之下）具有最小 sdom 的节点
                eval_node = dsu_query(pred);
            }

            // 更新 sdom(curr) 为 dfn 最小的那个
            if (semi_dom[eval_node] < semi_dom[curr])
            {
                semi_dom[curr] = semi_dom[eval_node];
            }
        }

        // 步骤 3.2: 将 curr 加入其 sdom 的桶中 (semi_children)
        // 延迟计算 idom，直到处理到 sdom(curr) 时
        int sdom_block = dfs_to_block[semi_dom[curr]];
        semi_children[sdom_block].push_back(curr);
        
        // 链接：将 curr 加入并查集，链接到其 DFS 树父节点
        dsu_parent[curr] = parent[curr];

        // 步骤 3.3: 计算父节点的半支配孩子们的 idom
        // 此时 parent[curr] 的子树中的相关 sdom 信息已计算完毕
        int p = parent[curr];
        for (int child : semi_children[p])
        {
            // 找到 child 到 p 路径上 sdom 最小的节点 u
            int u = dsu_query(child);
            
            // LT 定理：
            // 如果 sdom(u) == sdom(child)，则 idom(child) = sdom(child) = p
            // 否则，idom(child) = idom(u)
            if (semi_dom[u] < semi_dom[child])
            {
                imm_dom[child] = u; // 暂存 u，稍后在步骤 4 修正
            }
            else
            {
                imm_dom[child] = p; // 确认为 p
            }
        }
        semi_children[p].clear();
    }

    // 4. 修正直接支配点 idom
    // 按照 DFS 序正向遍历，处理之前暂存的情况
    for (int dfs_id = 1; dfs_id <= dfs_count; ++dfs_id)
    {
        int curr = dfs_to_block[dfs_id];
        if (imm_dom[curr] != dfs_to_block[semi_dom[curr]]) 
        {
            // 对应上述定理中的 case 2: idom(curr) = idom(u)
            imm_dom[curr] = imm_dom[imm_dom[curr]];
        }
    }

    // 5. 构建支配树 (Dominator Tree)
    // imm_dom[i] 是 i 的父节点
    for (int i = 0; i < node_count; ++i)
        if (block_to_dfs[i]) dom_tree[imm_dom[i]].push_back(i);

    // 移除虚拟源点带来的影响，调整大小
    dom_tree.resize(virtual_source);
    dom_frontier.resize(virtual_source);
    imm_dom.resize(virtual_source);

    // 修正 imm_dom：将指向虚拟源点的 idom 更新为自身（表示根节点或无支配者）
    for (int i = 0; i < virtual_source; ++i)
    {
        if (imm_dom[i] == virtual_source)
        {
            imm_dom[i] = i;
        }
    }

    // 6. 计算支配边界 (Dominance Frontier)
    // DF(n) 是所有满足 n 支配其前驱但不严格支配其本身的节点集合
    // 算法：对于每个汇合点 (有多个前驱的节点)，其前驱沿着支配树向上回溯，直到遇到 idom
    for (int block = 0; block < node_count; ++block)
    {
        // 确保是 CFG 中的有效节点
        if (block_to_dfs[block] == 0) continue;

        // 遍历 block 的所有后继节点 succ
        for (int succ : working_graph[block])
        {
            if (block_to_dfs[succ] == 0) continue;

            // 沿着支配树向上回溯，直到遇到 succ 的直接支配者
            // 路径上的每个节点 runner，succ 都在其支配边界中
            int runner = block;
            while (runner != imm_dom[succ] && runner != virtual_source && static_cast<size_t>(runner) < dom_frontier.size())
            {
                // succ 在 runner 的支配边界中
                if (succ < virtual_source) // 排除虚拟节点
                {
                    dom_frontier[runner].insert(succ);
                }
                
                runner = imm_dom[runner];
                // 防止死循环（如果图不连通或 idom 构建错误，可能会循环）
                if (runner == block || runner == imm_dom[runner]) break;
            }
        }
    }
}

void DomAnalyzer::clear()
{
    dom_tree.clear();
    dom_frontier.clear();
    imm_dom.clear();
}
