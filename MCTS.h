//
// Created by 唐雁 on 2024/1/15.
//

#ifndef EGO_GOMOKU_ZERO_MCTS_H
#define EGO_GOMOKU_ZERO_MCTS_H

#include <utility>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <limits>
#include "Game.h"
#include "Network.h"

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
    MonteCarloTree(std::__1::shared_ptr<PolicyValueNetwork> network, torch::Device device,
                   float exploration_factor = 5);

    void simulate(Game game);

    void search(Game &game, Node *node, int num_simulations);

    std::pair<float, std::vector<float>> evaluate_state(torch::Tensor &state);

    void backpropagate(Node *node, float value);

    std::pair<std::vector<int>, std::vector<float>> get_action_probabilities(Game game, float temperature = 1.0);

    std::vector<float> apply_temperature(std::vector<float> action_probabilities, float temperature);

private:
    std::__1::shared_ptr<PolicyValueNetwork> network;
    Node *root;
    torch::Device device;
    float exploration_factor;
};

#endif //EGO_GOMOKU_ZERO_MCTS_H