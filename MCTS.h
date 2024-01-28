
#ifndef EGO_GOMOKU_ZERO_MCTS_H
#define EGO_GOMOKU_ZERO_MCTS_H

#include <utility>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <limits>
#include "Game.h"
#include <torch/script.h>
#include "Analyzer.h"

class Node {
public:
    Node *parent;
    int visits;
    double value_sum;
    double prior_prob;
    double ucb{};
    std::unordered_map<int, Node *> children;

    Node(Node *parent = nullptr);

    bool isLeaf();

    std::pair<int, Node *> selectChild(double exploration_factor);

    void expand(Game &game, const std::vector<float> &prior_probs);

    void update(double value);
};

class MonteCarloTree {
public:
    MonteCarloTree(torch::jit::Module *network, torch::Device device,
                   float exploration_factor = 5);

    void simulate(Game game);

    void search(Game &game, Node *node, int num_simulations);

    std::pair<float, std::vector<float>> evaluate_state(torch::Tensor &state);

    void backpropagate(Node *node, float value);

    std::pair<std::vector<int>, std::vector<float>> get_action_probabilities(Game game, float temperature = 1.0);

    std::vector<float> apply_temperature(std::vector<float> action_probabilities, float temperature);

    void release(Node *node);

private:
    torch::jit::Module *network;
    Node *root;
    torch::Device device;
    float exploration_factor;
};


torch::jit::Module getNetwork(torch::Device device, std::string path);

#endif //EGO_GOMOKU_ZERO_MCTS_H
