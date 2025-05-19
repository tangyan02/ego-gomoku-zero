
#include "MCTS.h"

using namespace std;

Node::Node(Node *parent) : parent(parent), visits(0), value_sum(0), prior_prob(0), ucb(0) {}

bool Node::isLeaf() {
    std::lock_guard<std::mutex> lock(mtx); // 自动加锁和解锁
    return children.empty();
}

std::pair<int, Node *> Node::selectChild(double exploration_factor) {
    int total_visits = visits;
    std::vector<double> ucb_values;
    for (const auto &child: children) {
        double q = 0;
        if (child.second->visits > 0) {
            q = child.second->value_sum / (child.second->visits + child.second->virtual_loss);
        }
        double ucb_value = q + exploration_factor * child.second->prior_prob *
                               std::sqrt(total_visits) / (1 + child.second->visits + child.second->virtual_loss);
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
    if (selected_action == -1) {
        //cout << "当前子节点数量" << children.size() << endl;
        for (const auto &item: children) {
            cout << "idx = " << item.first << " visit = " << item.second->visits << " virtualLoss = "
                 << item.second->virtual_loss << " ubc = " << item.second->ucb
                 << " prior_prob = " << item.second->prior_prob << endl;
        }
    }
    return std::make_pair(selected_action, children[selected_action]);
}

void Node::expand(Game &game, std::vector<Point> &actions, const std::vector<float> &prior_probs) {
    std::lock_guard<std::mutex> lock(mtx); // 自动加锁和解锁
    if (!children.empty()) {
//        cout << "发现expand相同的节点" << endl;
        return;
    }
    // 计算概率总和
    float sum_probs = 0.0;
    for (auto &prob: prior_probs) {
        sum_probs += prob;
    }

    for (auto &action: actions) {
        Node *child = new Node(this);
        int actionIndex = game.getActionIndex(action);

        // 归一化处理
        if (sum_probs != 0) {
            child->prior_prob = prior_probs[actionIndex] / sum_probs;
        } else {
            child->prior_prob = prior_probs[actionIndex];
        }

        children[actionIndex] = child;
    }
}

void Node::update(double value) {
    std::lock_guard<std::mutex> lock(mtx); // 自动加锁和解锁
    visits++;
    value_sum += value;
}

void Node::add_virtual_loss(double loss) {
    std::lock_guard<std::mutex> lock(mtx_loss); // 自动加锁和解锁
    virtual_loss += loss;
}

void Node::remove_virtual_loss(double loss) {
    std::lock_guard<std::mutex> lock(mtx_loss); // 自动加锁和解锁
    virtual_loss -= loss;
}

MonteCarloTree::MonteCarloTree(Model *model, float exploration_factor)
        : root(nullptr), model(model), exploration_factor(exploration_factor) {
}


void MonteCarloTree::simulate(Game game) {
    if (game.isGameOver()) {
        return;
    }

    double virtual_loss = 1;

    Node *node = root;
    while (!node->isLeaf()) {
        std::pair<int, Node *> result = node->selectChild(exploration_factor);
        int action = result.first;
        // cout << action << endl;
        node = result.second;
        node->add_virtual_loss(virtual_loss);
        Point pointAction = game.getPointFromIndex(action);
        game.makeMove(pointAction);
        // game.printBoard();
    }

    float value;

    if (game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
        value = -1;
    } else {
        auto actions = selectActions(game);
        node->selectInfo = get<2>(actions);

        auto state = game.getState();
//        auto future = model->enqueueData(state);
//        auto result = future.get();
        auto result = model->evaluate_state(state);
        value = result.first;
        if (get<0>(actions)) {
            value = 1;
        }
        std::vector<float> priorProb = result.second;
        node->expand(game, get<1>(actions), priorProb);
    }

    backpropagate(node, value, virtual_loss);
}

void MonteCarloTree::search(Game &game, Node *node, int num_simulations, int threadNum) {
    root = node;

    std::vector<std::thread> threads;
    for (int i = 0; i < threadNum; ++i) {
        threads.emplace_back(&MonteCarloTree::threadSimulate, this, game, (int) (num_simulations / (float) threadNum));
    }
    for (auto &thread: threads) {
        thread.join();
    }
}

void MonteCarloTree::threadSimulate(Game game, int num_simulations) {
    // 简单的模拟和更新逻辑
    for (int i = 0; i < num_simulations; i++) {
//        cout << "开始模拟，次数 " << i << endl;
        simulate(game);
    }
}

void MonteCarloTree::backpropagate(Node *node, float value, double virtual_loss) {
    while (node != nullptr) {
        node->update(value);
        if (node->parent != root) {
            node->remove_virtual_loss(virtual_loss);
        }
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

void Node::release() {
    for (const auto &item: this->children) {
        item.second->release();
    }
    if (this->parent != nullptr) {
        delete this;
    }
}