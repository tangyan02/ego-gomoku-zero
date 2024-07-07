
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
#include <thread>

class Node {
public:
    Node *parent;
    int visits;
    double value_sum;
    double prior_prob;
    double ucb{};
    double virtual_loss = 0;

    std::mutex mtx; // 互斥锁
    std::mutex mtx_loss; // 互斥锁
    std::unordered_map<int, Node *> children;
    string selectInfo;

    Node(Node *parent = nullptr);

    bool isLeaf();

    std::pair<int, Node *> selectChild(double exploration_factor);

    void expand(Game &game, std::vector<Point> &actions, const std::vector<float> &prior_probs);

    void update(double value);

    void release();

    void add_virtual_loss(double loss);

    void remove_virtual_loss(double loss);

};

class MonteCarloTree {
public:
    MonteCarloTree(Model *model, float exploration_factor = 5);

    void simulate(Game game);

    void threadSimulate(Game game, int num_simulations);

    void search(Game &game, Node *node, int num_simulations, int threadNum = 1);

    void backpropagate(Node *node, float value, double virtual_loss);

    std::pair<std::vector<int>, std::vector<float>> get_action_probabilities(Game game);

    std::vector<float> apply_temperature(std::vector<float> action_probabilities, float temperature);

private:
    Node *root;
    Model *model;
    float exploration_factor;
};

#endif //EGO_GOMOKU_ZERO_MCTS_H
