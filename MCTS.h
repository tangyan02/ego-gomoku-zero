
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

    Node *root;
private:
    // 走一条路径到叶子，沿途加 virtual loss；返回 (leaf, game_at_leaf, immediate_value, is_terminal)
    // immediate_value: 如果叶子已是终局或已经被扩展过（路径撞到同一叶子），返回立即值；否则返回 NAN
    struct LeafInfo {
        Node *leaf;
        Game game;
        float immediate_value;  // NaN 表示需要网络评估；有值表示直接回传
        bool needs_eval;        // true 表示需要加入 batch 推理队列
    };
    LeafInfo selectLeafWithVirtualLoss(Game game);

    Model *model;
    float exploration_factor;
    bool useNoice = false;
    std::mt19937 rng{std::random_device{}()};  // 类成员 RNG，避免每次 simulate 重建

    static std::vector<double> sample_dirichlet(int size, double alpha, std::mt19937 &rng);

};

#endif //EGO_GOMOKU_ZERO_MCTS_H
