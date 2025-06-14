#include "MCTS.h"

#include <random>
//#include <__random/random_device.h>

using namespace std;

Node::Node(Node *parent) : parent(parent), visits(0), value_sum(0), prior_prob(0), ucb(0) {
}

bool Node::isLeaf() {
    return children.empty();
}

Node *Node::selectChild(double exploration_factor) {
    int total_visits = visits;
    vector<double> ucb_values;
    for (const auto &child: children) {
        double q = 0;
        if (child.second->visits > 0) {
            q = child.second->value_sum / child.second->visits;
        }
        double ucb_value = q + exploration_factor * child.second->prior_prob *
                           sqrt(total_visits) / (1 + child.second->visits);
        child.second->ucb = ucb_value;
    }
    Node *selected;
    double max_ucb = numeric_limits<double>::lowest();
    for (const auto &child: children) {
        if (child.second->ucb > max_ucb) {
            max_ucb = child.second->ucb;
            selected = child.second;
        }
    }
    return selected;
}

void Node::expand(Game &game, vector<Point> &moves, const vector<float> &prior_probs) {
    //特殊处理跳过的情况
    vector<float> probs_arr;
    if (moves.size() > 1) {
        for (auto &move: moves) {
            int moveIndex = game.getActionIndex(move);
            probs_arr.emplace_back(prior_probs[moveIndex]);
        }
    } else {
        probs_arr.emplace_back(1);
    }

    // 计算概率总和
    float sum_probs = 0.0;
    for (auto &prob: probs_arr) {
        sum_probs += prob;
    }

    for (int i = 0; i < moves.size(); i++) {
        auto move = moves[i];
        auto prob = probs_arr[i];
        Node *child = new Node(this);
        child->move = move;
        child->parent = this;

        // 归一化处理
        if (sum_probs != 0) {
            child->prior_prob = prob / sum_probs;
        } else {
            child->prior_prob = prob;
        }

        children[move] = child;
    }
}

void Node::update(double value) {
    visits++;
    value_sum += value;
}

MonteCarloTree::MonteCarloTree(Model *model, float exploration_factor, bool useNoice)
    : root(nullptr), model(model), exploration_factor(exploration_factor), useNoice(useNoice) {
}


void MonteCarloTree::simulate(Game game) {
    if (game.isGameOver()) {
        return;
    }

    Node *node = root;
    while (!node->isLeaf()) {
        auto result = node->selectChild(exploration_factor);
        game.makeMove(result->move);
        node = result;
    }

    float value;

    if (game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
        value = -1;
    } else {
        auto [win, moves, selectInfo] = selectActions(game);
        node->selectInfo = selectInfo;

        auto state = game.getState();
        auto [eva_value, probs] = model->evaluate_state(state);
        value = eva_value;
        if (win) {
            value = 1;
        }

        if (useNoice && node == root) {
            std::random_device rd;
            std::mt19937 rng(rd());
            add_dirichlet_noise(probs, 0.25, 0.03, rng);
        }
        node->expand(game, moves, probs);
    }

    backpropagate(node, -value);
}

void MonteCarloTree::search(Game &game, Node *node, int num_simulations) {
    root = node;

    // 简单的模拟和更新逻辑
    for (int i = 0; i < num_simulations; i++) {
        //        cout << "开始模拟，次数 " << i << endl;
        simulate(game);
    }
}

void MonteCarloTree::backpropagate(Node *node, float value) {
    while (node != nullptr) {
        node->update(value);
        node = node->parent;
        value = -value;
    }
}

pair<vector<Point>, vector<float> > MonteCarloTree::get_action_probabilities(float temperature) {

    Node *node = root;
    vector<pair<Point, int> > action_visits;
    for (auto &item: node->children) {
        action_visits.emplace_back(item.first, item.second->visits);
    }

    vector<Point> moves;
    vector<int> visits;
    for (auto &item: action_visits) {
        moves.push_back(item.first);
        visits.push_back(item.second);
    }

    vector<float> action_probs(moves.size(), 0.0f);

    // 防止温度过小导致溢出
    if (temperature < 1e-3) {
        // 只选最大访问数的动作
        int max_visit = -1;
        size_t max_idx = 0;
        for (size_t i = 0; i < visits.size(); ++i) {
            if (visits[i] > max_visit) {
                max_visit = visits[i];
                max_idx = i;
            }
        }
        if (!visits.empty()) {
            action_probs[max_idx] = 1.0f;
        }
    } else {
        // 用log技巧防止溢出
        std::vector<float> log_probs(moves.size(), 0.0f);
        float max_log = -std::numeric_limits<float>::infinity();
        for (size_t i = 0; i < visits.size(); ++i) {
            if (visits[i] > 0) {
                log_probs[i] = std::log(static_cast<float>(visits[i])) / temperature;
                if (log_probs[i] > max_log) max_log = log_probs[i];
            } else {
                log_probs[i] = -std::numeric_limits<float>::infinity();
            }
        }
        // log-sum-exp
        float sum_exp = 0.0f;
        for (size_t i = 0; i < log_probs.size(); ++i) {
            if (log_probs[i] > -std::numeric_limits<float>::infinity()) {
                action_probs[i] = std::exp(log_probs[i] - max_log);
                sum_exp += action_probs[i];
            } else {
                action_probs[i] = 0.0f;
            }
        }
        if (sum_exp > 0.0f) {
            for (auto &prob: action_probs) {
                prob /= sum_exp;
            }
        }
    }

    return make_pair(moves, action_probs);
}


pair<vector<Point>, vector<float> > MonteCarloTree::get_action_probabilities() {
    Node *node = root;
    vector<pair<Point, int> > action_visits;
    for (auto &item: node->children) {
        action_visits.emplace_back(item.first, item.second->visits);
    }

    vector<Point> moves;
    vector<int> visits;
    for (auto &item: action_visits) {
        moves.push_back(item.first);
        visits.push_back(item.second);
    }

    // 计算总和
    int sum = 0;
    for (int visit: visits) {
        sum += visit;
    }

    // 归一化为概率分布
    vector<float> action_probs;
    for (const int visit: visits) {
        float prob = static_cast<float>(visit) / static_cast<float>(sum);
        action_probs.push_back(prob);
    }

    return make_pair(moves, action_probs);
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
std::vector<double> MonteCarloTree::sample_dirichlet(int size, double alpha, std::mt19937 &rng) {
    std::gamma_distribution gamma_dist(alpha, 1.0);
    std::vector<double> samples(size);
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        samples[i] = gamma_dist(rng);
        sum += samples[i];
    }
    // 归一化
    for (int i = 0; i < size; ++i) {
        samples[i] /= sum;
    }
    return samples;
}

void MonteCarloTree::add_dirichlet_noise(std::vector<float> &priors, double epsilon, double alpha, std::mt19937 &rng) {
    int size = priors.size();
    std::vector<double> noise = sample_dirichlet(size, alpha, rng);
    for (int i = 0; i < size; ++i) {
        priors[i] = float((1 - epsilon) * priors[i] + epsilon * noise[i]);
    }
    // 可选：归一化，防止数值误差
    double sum = std::accumulate(priors.begin(), priors.end(), 0.0);
    for (int i = 0; i < size; ++i) {
        priors[i] /= float(sum);
    }
}