#include "GumbelMCTS.h"
#include "Analyzer.h"
#include <numeric>
#include <cstring>
#include <iostream>

using namespace std;

GumbelMCTS::GumbelMCTS(Model* model, int boardSize, int numSimulations,
                       int maxConsidered, float cScale)
    : model(model), boardSize(boardSize), numSimulations(numSimulations),
      maxConsidered(maxConsidered), cScale(cScale) {}

double GumbelMCTS::sampleGumbel() {
    std::uniform_real_distribution<double> dist(1e-10, 1.0 - 1e-10);
    double u = dist(rng);
    return -log(-log(u));
}

float GumbelMCTS::sigma(float q, int totalVisits) {
    // σ(Q) 需要和 logits 在同一量级
    // log_softmax 典型范围 [-10, -3]，所以 σ 也应在这个量级
    // 使用 c * sqrt(N) * Q，随访问数增长逐渐信任 Q
    float c = 2.0f;
    return c * sqrt(max(1, totalVisits)) * q;
}

std::pair<float, std::vector<float>> GumbelMCTS::evaluateLeaf(Game& game) {
    uint64_t hash = game.zobristHash;
    auto it = transpositionTable.find(hash);
    if (it != transpositionTable.end()) {
        return {it->second.value, it->second.priors};
    }

    const int channels = INPUT_CHANNELS;
    float stateBuffer[channels * MAX_BOARD_SIZE * MAX_BOARD_SIZE];
    game.getState(stateBuffer, channels);
    auto [value, priors] = model->evaluate_state(stateBuffer, channels, boardSize, boardSize);
    transpositionTable[hash] = TTEntry{value, priors};
    return {value, priors};
}

Node* GumbelMCTS::selectInterior(Node* node) {
    // 内部节点：PUCT 式选择（和传统 MCTS 一致，保证搜索质量）
    double parentVisits = max(1, node->visits);
    double sqrtParent = sqrt(parentVisits);
    double parentQ = node->visits > 0 ? node->value_sum / node->visits : 0.0;

    // FPU
    double visitedPriorSum = 0.0;
    for (auto& [p, child] : node->children) {
        if (child->visits > 0) visitedPriorSum += child->prior_prob;
    }
    double fpuValue = parentQ - 0.2 * sqrt(visitedPriorSum);

    Node* best = nullptr;
    double bestScore = -1e18;
    float cPuct = 3.0f;

    for (auto& [point, child] : node->children) {
        double q = child->visits > 0 ? child->value_sum / child->visits : fpuValue;
        double exploration = cPuct * child->prior_prob * sqrtParent / (1 + child->visits);
        double score = q + exploration;
        if (score > bestScore) {
            bestScore = score;
            best = child;
        }
    }
    return best;
}

void GumbelMCTS::simulate(Game& game, Node* root, const std::vector<Point>& candidates, int rootActionIdx) {
    Node* node = root;
    Game simGame = game;

    // root 是叶子则先展开
    if (node->isLeaf()) {
        auto [value, priors] = evaluateLeaf(simGame);
        vector<Point> moves = candidates;
        node->expand(simGame, moves, priors);
        float v = -value;
        Node* cur = node;
        while (cur != nullptr) {
            cur->update(v);
            cur = cur->parent;
            v = -v;
        }
        return;
    }

    // 根节点：走指定的动作（Sequential Halving 分配的）
    Node* child = nullptr;
    if (rootActionIdx >= 0 && rootActionIdx < (int)candidates.size()) {
        auto it = node->children.find(candidates[rootActionIdx]);
        if (it != node->children.end()) child = it->second;
    }
    if (child == nullptr) {
        // fallback: 用内部选择
        child = selectInterior(node);
    }
    if (child == nullptr) return;
    simGame.makeMove(child->move);
    node = child;

    // 继续往下直到叶子
    while (!node->isLeaf()) {
        if (simGame.isGameOver()) break;
        Node* next = selectInterior(node);
        if (next == nullptr) break;
        simGame.makeMove(next->move);
        node = next;
    }

    // 叶子节点处理
    float value;
    if (simGame.isGameOver() ||
        (simGame.lastAction.x >= 0 && simGame.checkWin(simGame.lastAction.x, simGame.lastAction.y, simGame.getOtherPlayer()))) {
        value = -1.0f;  // 当前方视角：对方刚赢了
    } else if (node->isLeaf()) {
        auto [evalResult, priors] = evaluateLeaf(simGame);
        value = evalResult;
        auto [win, moves, selectInfo] = selectActions(simGame);
        if (win) {
            value = 1.0f;
        } else {
            node->expand(simGame, moves, priors);
        }
    } else {
        value = 0.0f;
    }

    // 回传（从 node 往上，交替取反）
    float v = -value;
    Node* cur = node;
    while (cur != nullptr) {
        cur->update(v);
        cur = cur->parent;
        v = -v;
    }
}

std::vector<float> GumbelMCTS::computeImprovedPolicy(
    Node* root, Game& game,
    const std::vector<Point>& candidates,
    const std::vector<float>& logits) {

    // π_improved(a) ∝ prior(a) * exp(σ(Q̂(a)))
    // 等价于 log π_improved(a) = logit(a) + σ(Q̂(a))
    float parentQ = root->visits > 0 ? (float)(root->value_sum / root->visits) : 0.0f;

    std::vector<float> scores(candidates.size());
    float maxScore = -1e18f;
    for (int i = 0; i < (int)candidates.size(); i++) {
        float logit = logits[candidates[i].x * boardSize + candidates[i].y];
        float q = parentQ;  // 默认用父节点 Q
        auto it = root->children.find(candidates[i]);
        if (it != root->children.end() && it->second->visits > 0) {
            q = (float)(it->second->value_sum / it->second->visits);
        }
        scores[i] = logit + sigma(q, root->visits);
        if (scores[i] > maxScore) maxScore = scores[i];
    }

    // softmax
    std::vector<float> policyTarget(boardSize * boardSize, 0.0f);
    float sumExp = 0.0f;
    for (int i = 0; i < (int)candidates.size(); i++) {
        float e = exp(scores[i] - maxScore);
        policyTarget[candidates[i].x * boardSize + candidates[i].y] = e;
        sumExp += e;
    }
    for (auto& v : policyTarget) {
        v /= sumExp;
    }
    return policyTarget;
}

GumbelResult GumbelMCTS::search(Game& game) {
    // 1. 获取候选动作和 prior logits
    auto [win, candidates, selectInfo] = selectActions(game);

    // 如果必胜，直接返回
    if (win && !candidates.empty()) {
        std::vector<float> policyTarget(boardSize * boardSize, 0.0f);
        // 均分给所有胜利点
        for (auto& p : candidates) {
            policyTarget[p.x * boardSize + p.y] = 1.0f / candidates.size();
        }
        // 随机选一个胜利点
        std::uniform_int_distribution<int> dist(0, candidates.size() - 1);
        Point action = candidates[dist(rng)];
        return GumbelResult{action, policyTarget, 1.0f};
    }

    // 2. 评估 root 获得 prior logits
    auto [rootValue, priors] = evaluateLeaf(game);

    // 3. 构建 root 节点并展开
    Node root_node;
    root_node.expand(game, candidates, priors);

    // 4. 为每个候选动作生成 Gumbel 噪声
    int numCands = candidates.size();
    std::vector<double> gumbels(numCands);
    std::vector<double> scores(numCands);
    for (int i = 0; i < numCands; i++) {
        gumbels[i] = sampleGumbel();
        float logit = priors[candidates[i].x * boardSize + candidates[i].y];
        // 使用 log(prior) 而非 raw logit（Network 输出的是 log_softmax）
        scores[i] = (double)logit + gumbels[i];
    }

    // 5. Sequential Halving
    int m = min(maxConsidered, numCands);
    // 按初始 score 取 top-m
    std::vector<int> alive(numCands);
    std::iota(alive.begin(), alive.end(), 0);
    std::partial_sort(alive.begin(), alive.begin() + m, alive.end(),
                      [&](int a, int b) { return scores[a] > scores[b]; });
    alive.resize(m);

    int phases = max(1, (int)floor(log2(m)));
    int simsUsed = 0;

    for (int phase = 0; phase < phases && alive.size() > 1; phase++) {
        int simsPerAction = max(1, numSimulations / ((int)alive.size() * phases));

        // 对每个存活动作分配模拟
        for (int idx : alive) {
            for (int s = 0; s < simsPerAction && simsUsed < numSimulations; s++) {
                simulate(game, &root_node, candidates, idx);
                simsUsed++;
            }
        }

        // 更新 score 加入 Q 值修正
        for (int idx : alive) {
            auto it = root_node.children.find(candidates[idx]);
            if (it != root_node.children.end() && it->second->visits > 0) {
                float q = (float)(it->second->value_sum / it->second->visits);
                float logit = priors[candidates[idx].x * boardSize + candidates[idx].y];
                scores[idx] = (double)logit + gumbels[idx] + sigma(q, root_node.visits);
            }
        }

        // 淘汰一半（按 score 排序，保留 top half）
        if (phase < phases - 1) {
            int keep = max(1, (int)alive.size() / 2);
            std::partial_sort(alive.begin(), alive.begin() + keep, alive.end(),
                              [&](int a, int b) { return scores[a] > scores[b]; });
            alive.resize(keep);
        }
    }

    // 剩余预算分配给最终存活的最佳动作
    int bestAlive = alive[0];
    for (int idx : alive) {
        if (scores[idx] > scores[bestAlive]) bestAlive = idx;
    }
    while (simsUsed < numSimulations) {
        simulate(game, &root_node, candidates, bestAlive);
        simsUsed++;
    }

    // 6. 选择最终动作（score 最高的）
    int bestIdx = alive[0];
    for (int idx : alive) {
        auto it = root_node.children.find(candidates[idx]);
        if (it != root_node.children.end() && it->second->visits > 0) {
            float q = (float)(it->second->value_sum / it->second->visits);
            float logit = priors[candidates[idx].x * boardSize + candidates[idx].y];
            scores[idx] = (double)logit + gumbels[idx] + sigma(q, root_node.visits);
        }
        if (scores[idx] > scores[bestIdx]) bestIdx = idx;
    }
    Point action = candidates[bestIdx];

    // 7. 计算改进后的 policy target
    std::vector<float> policyTarget = computeImprovedPolicy(&root_node, game, candidates, priors);

    // 8. 计算 root Q
    float rootQ = root_node.visits > 0 ? (float)(root_node.value_sum / root_node.visits) : 0.0f;

    // 释放树
    root_node.release();

    return GumbelResult{action, policyTarget, rootQ};
}
