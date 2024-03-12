
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

class Node {
public:
    Node *parent;
    int visits;
    double value_sum;
    double prior_prob;
    double ucb{};
    std::unordered_map<int, Node *> children;
    string selectInfo;

    Node(Node *parent = nullptr);

    bool isLeaf();

    std::pair<int, Node *> selectChild(double exploration_factor);

    void expand(Game &game, std::vector<Point> &actions, const std::vector<float> &prior_probs);

    void update(double value);
};

class MonteCarloTree {
public:
    MonteCarloTree(Model *model, float exploration_factor = 5);

    void simulate(Game game);

    void search(Game &game, Node *node, int num_simulations);

    void backpropagate(Node *node, float value);

    std::pair<std::vector<int>, std::vector<float>> get_action_probabilities(Game game);

    std::vector<float> apply_temperature(std::vector<float> action_probabilities, float temperature);

    void release(Node *node);

private:
    Node *root;
    Model *model;
    float exploration_factor;
};

#endif //EGO_GOMOKU_ZERO_MCTS_H
