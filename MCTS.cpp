
#include "MCTS.h"

using namespace std;

Node::Node(Node *parent) : parent(parent), visits(0), value_sum(0), prior_prob(0), ucb(0) {}

bool Node::isLeaf() {
    return children.empty();
}

std::pair<int, Node *> Node::selectChild(double exploration_factor) {
    int total_visits = visits;
    std::vector<double> ucb_values;
    for (const auto &child: children) {
        double q = 0;
        if (child.second->visits > 0) {
            q = child.second->value_sum / child.second->visits;
        }
        double ucb_value = q + exploration_factor * child.second->prior_prob *
                               std::sqrt(total_visits) / (1 + child.second->visits);
        child.second->ucb = ucb_value;
    }
    int selected_action = -1;
    double max_ucb = std::numeric_limits<double>::lowest();
    for (const auto &child: children) {
        if (child.second->ucb > max_ucb) {
            max_ucb = child.second->ucb;
            selected_action = child.first;
        }
    }
    return std::make_pair(selected_action, children[selected_action]);
}

void Node::expand(Game &game, std::vector<Point> &actions, const std::vector<float> &prior_probs) {
    for (auto &action: actions) {
        Node *child = new Node(this);
        int actionIndex = game.getActionIndex(action);
        child->prior_prob = prior_probs[actionIndex];
        children[actionIndex] = child;
    }
}

void Node::update(double value) {
    visits++;
    value_sum += value;
}

MonteCarloTree::MonteCarloTree(torch::jit::Module *network, torch::Device device,
                               float exploration_factor)
        : network(network), root(nullptr), device(device), exploration_factor(exploration_factor) {
}

void MonteCarloTree::simulate(Game game) {
    if (game.isGameOver()) {
        return;
    }

    Node *node = root;
    while (!node->isLeaf()) {
        std::pair<int, Node *> result = node->selectChild(exploration_factor);
        int action = result.first;
        // cout << action << endl;
        node = result.second;
        Point pointAction = game.getPointFromIndex(action);
        game.makeMove(pointAction);
        // game.printBoard();
    }

    float value;

    if (game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
        value = -1;
    } else {
        bool useVct = false;
        if (node->parent == nullptr) {
            useVct = true;
        }
        auto actions = selectActions(game, useVct);
        if (actions.first && node->parent != nullptr) {
            value = 1;
        } else {
            auto state = game.getState();
            std::pair<float, std::vector<float>>
                    result = evaluate_state(state);
            value = result.first;
            std::vector<float> priorProb = result.second;
            node->expand(game, actions.second, priorProb);
        }
    }

    backpropagate(node, -value);
}

void MonteCarloTree::search(Game &game, Node *node, int num_simulations) {
    root = node;

    for (int i = 0; i < num_simulations; i++) {
        // cout << "开始模拟，次数 " << i << endl;
        simulate(game);
    }
}

std::pair<float, std::vector<float>> MonteCarloTree::evaluate_state(torch::Tensor &state) {
    torch::Tensor state_tensor = state.to(device).clone();
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(state_tensor);

    auto outputs = network->forward(inputs).toTuple();
    torch::Tensor value = outputs->elements()[0].toTensor();
    torch::Tensor policy = outputs->elements()[1].toTensor();

    auto valueFloat = value[0][0].cpu().item<float>();

    // 将张量转换为 CPU 上的张量
    torch::Tensor cpu_policy = torch::exp(policy).cpu();

    // 打平并转换为 std::vector<float>
    std::vector<float> prior_prob(cpu_policy.data<float>(), cpu_policy.data<float>() + cpu_policy.numel());

    return std::make_pair(valueFloat, prior_prob);
}

void MonteCarloTree::backpropagate(Node *node, float value) {
    while (node != nullptr) {
        node->update(value);
        node = node->parent;
        value = -value;
    }
}

std::pair<std::vector<int>, std::vector<float>>
MonteCarloTree::get_action_probabilities(Game game) {
    Node *node = root;
    std::vector<std::pair<int, int>> action_visits;
    for (auto &item: node->children) {
        action_visits.emplace_back(item.first, item.second->visits);
    }

    std::vector<int> actions;
    std::vector<int> visits;
    for (auto &item: action_visits) {
        actions.push_back(item.first);
        visits.push_back(item.second);
    }

    // 计算总和
    float sum = 0.0;
    for (int visit: visits) {
        sum += visit;
    }

    // 归一化为概率分布
    std::vector<float> action_probs;
    for (int visit: visits) {
        float prob = static_cast<float>(visit) / sum;
        action_probs.push_back(prob);
    }

    std::vector<float> probs(game.boardSize * game.boardSize, 0);
    for (int i = 0; i < actions.size(); i++) {
        //劣势的情况下会有大量节点只访问一次，并影响了原有的概率，自我对战时不考虑选点
        if (visits[i] == 1)
            action_probs[i] = 0;
        probs[actions[i]] = action_probs[i];
    }

    std::vector<int> range(game.boardSize * game.boardSize);
    std::iota(range.begin(), range.end(), 0);
    return std::make_pair(range, probs);
}

std::vector<float> MonteCarloTree::apply_temperature(std::vector<float> action_probabilities, float temperature) {
    if (temperature == 1)
        return action_probabilities;
    for (float &prob: action_probabilities) {
        prob = std::pow(prob, 1 / temperature);
    }
    float sum = std::accumulate(action_probabilities.begin(), action_probabilities.end(), 0.0f);
    for (float &prob: action_probabilities) {
        prob /= sum;
    }
    return action_probabilities;
}

void MonteCarloTree::release(Node *node) {
    if (!node->children.empty()) {
        for (const auto &item: node->children) {
            release(item.second);
        }
    }
    if (node->parent != nullptr) {
        delete node;
    }
}