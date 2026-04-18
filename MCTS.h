
#ifndef EGO_GOMOKU_ZERO_MCTS_H
#define EGO_GOMOKU_ZERO_MCTS_H

#include <utility>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <limits>
#include "Game.h"
#include "Analyzer.h"
#include "Model.h"
#include <atomic>
#include <random>
#include <thread>

class Node {
public:
    Node *parent;
    Point move;
    int visits;
    double value_sum;
    double prior_prob;
    double ucb;
    int virtual_loss;  // Virtual Loss 计数，用于批推理时避免路径重复
    unordered_map<Point, Node *, PointHash> children;
    string selectInfo;

    Node(Node *parent = nullptr);

    bool isLeaf();

    Node *selectChild(double exploration_factor);

    void expand(Game &game, vector<Point> &actions, const vector<float> &prior_probs);

    void update(double value);

    void release();

    // Virtual Loss 辅助方法
    void addVirtualLoss();
    void removeVirtualLoss();
};

// Transposition Table 缓存项：局面的评估结果
struct TTEntry {
    float value;
    std::vector<float> priors;
};

class MonteCarloTree {
public:
    MonteCarloTree(Model *model, float exploration_factor = 5, bool useNoice = false);

    void simulate(Game game);

    void search(Game &game, Node *node, int num_simulations);

    // 批量搜索：使用 Virtual Loss + 批推理加速 (默认 batch_size=8)
    void searchBatched(Game &game, Node *node, int num_simulations, int batch_size = 8);

    void backpropagate(Node *node, float value);

    pair<vector<Point>, vector<float> > get_action_probabilities(float temperature);

    pair<vector<Point>, vector<float>> get_action_probabilities();

    Point get_max_visit_move();

    static void add_dirichlet_noise(
        std::vector<float> &priors, // 原始概率
        double epsilon, // 混合系数
        double alpha, // Dirichlet参数
        std::mt19937 &rng
    );

    // Transposition Table：缓存局面评估结果，避免重复推理
    std::unordered_map<uint64_t, TTEntry> transpositionTable;
    void clearTranspositionTable() { transpositionTable.clear(); }

    Node *root;
private:
    // 走一条路径到叶子，沿途加 virtual loss
    struct LeafInfo {
        Node *leaf;
        Game game;
        float immediate_value;
        bool needs_eval;
    };
    LeafInfo selectLeafWithVirtualLoss(Game game);

    Model *model;
    float exploration_factor;
    bool useNoice = false;
    std::mt19937 rng{std::random_device{}()};

    static std::vector<double> sample_dirichlet(int size, double alpha, std::mt19937 &rng);

};

#endif //EGO_GOMOKU_ZERO_MCTS_H
