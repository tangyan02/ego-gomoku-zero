#ifndef EGO_GOMOKU_ZERO_GUMBEL_MCTS_H
#define EGO_GOMOKU_ZERO_GUMBEL_MCTS_H

#include "Game.h"
#include "Model.h"
#include "MCTS.h"
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <unordered_map>

/**
 * Gumbel MCTS: Policy Improvement by Planning with Gumbel
 * 只用于自对弈数据生成，评估和实战仍用传统 MCTS。
 *
 * 核心区别：
 * 1. 根节点用 Gumbel 噪声 + Sequential Halving 选择动作
 * 2. 内部节点用确定性 Q 值选择（无 PUCT 探索项）
 * 3. Policy target 基于 improved policy（prior * exp(σ(Q))），不是访问次数
 */

struct GumbelResult {
    Point action;                    // 选择的动作
    std::vector<float> policyTarget; // 改进后的 policy target (boardSize*boardSize)
    float rootQ;                     // root 节点的 Q 值
};

class GumbelMCTS {
public:
    GumbelMCTS(Model* model, int boardSize, int numSimulations = 16,
               int maxConsidered = 8, float cScale = 1.0f);

    /**
     * 执行一步 Gumbel MCTS 搜索
     * @param game 当前局面
     * @return GumbelResult 包含选择的动作和 policy target
     */
    GumbelResult search(Game& game);

    // Transposition Table（和传统 MCTS 共享结构）
    std::unordered_map<uint64_t, TTEntry> transpositionTable;
    void clearTranspositionTable() { transpositionTable.clear(); }

private:
    Model* model;
    int boardSize;
    int numSimulations;   // 总模拟预算（默认 16）
    int maxConsidered;    // Sequential Halving 初始候选数（默认 8）
    float cScale;         // σ 函数的缩放系数
    std::mt19937 rng{std::random_device{}()};

    // Gumbel(0, 1) 采样
    double sampleGumbel();

    // σ 函数：将 Q 值转换到 logits 可比较的尺度
    float sigma(float q, int totalVisits);

    // 评估一个叶子节点，返回 (value, policy_priors)
    std::pair<float, std::vector<float>> evaluateLeaf(Game& game);

    // 内部节点确定性选择：选 σ(Q) + logit 最大的子节点
    Node* selectInterior(Node* node);

    // 单次模拟：从 root 到叶子，展开并回传
    void simulate(Game& game, Node* root, const std::vector<Point>& candidates);

    // 计算改进后的 policy target
    std::vector<float> computeImprovedPolicy(Node* root, Game& game,
                                              const std::vector<Point>& candidates,
                                              const std::vector<float>& logits);
};

#endif // EGO_GOMOKU_ZERO_GUMBEL_MCTS_H
