#include <iostream>
#include <cmath>
#include <unordered_map>
#include <vector>
#include "Game.cpp"
#include "Network.cpp"

using namespace std;

class Node
{
public:
    Node *parent;
    int visits;
    double value_sum;
    double prior_prob;
    double ucb;
    std::unordered_map<int, Node *> children;

    Node(Node *parent = nullptr) : parent(parent), visits(0), value_sum(0), prior_prob(0) {}

    bool isLeaf()
    {
        return children.empty();
    }

    std::pair<int, Node *> selectChild(double exploration_factor)
    {
        int total_visits = visits;
        std::vector<double> ucb_values;
        for (const auto &child : children)
        {

            double q = 0;
            if (child.second->visits > 0)
            {
                q = child.second->value_sum / child.second->visits;
            }
            double ucb_value = q + exploration_factor * child.second->prior_prob *
                                       std::sqrt(total_visits) / (1 + child.second->visits);
            child.second->ucb = ucb_value;
        }
        int selected_action = -1;
        double max_ucb = std::numeric_limits<double>::lowest();
        for (const auto &child : children)
        {
            if (child.second->ucb > max_ucb)
            {
                max_ucb = child.second->ucb;
                selected_action = child.first;
            }
        }
        return std::make_pair(selected_action, children[selected_action]);
    }

    void expand(Game &game, const std::vector<float> &prior_probs)
    {
        std::vector<Point> actions = game.getEmptyPoints();
        for (int i = 0; i < actions.size(); i++)
        {
            Node *child = new Node(this);
            int actionIndex = game.getActionIndex(actions[i]);
            child->prior_prob = prior_probs[actionIndex];
            children[actionIndex] = child;
        }
    }

    void update(double value)
    {
        visits++;
        value_sum += value;
    }
};

class MonteCarloTree
{
public:
    MonteCarloTree(std::__1::shared_ptr<PolicyValueNetwork> network, torch::Device device, float exploration_factor = 5)
        : network(network), root(nullptr), device(device), exploration_factor(exploration_factor) {}

    void simulate(Game game)
    {
        if (game.isGameOver())
        {
            return;
        }

        Node *node = root;
        while (!node->isLeaf())
        {
            std::pair<int, Node *> result = node->selectChild(exploration_factor);
            int action = result.first;
            // cout << action << endl;
            node = result.second;
            Point pointAction = game.getPointFormIndex(action);
            game.makeMove(pointAction);
            // game.printBoard();
        }

        std::pair<float, std::vector<float>>
            result = evaluate_state(game.getState());
        float value = result.first;
        std::vector<float> priorProb = result.second;
        if (game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer()))
        {
            value = -1;
        }
        else
        {
            node->expand(game, priorProb);
        }

        backpropagate(node, -value);
    }

    void search(Game &game, Node *node, int num_simulations)
    {
        root = node;

        for (int i = 0; i < num_simulations; i++)
        {
            // cout << "开始模拟，次数 " << i << endl;
            simulate(game);
        }
    }

    std::pair<float, std::vector<float>> evaluate_state(torch::Tensor state)
    {
        torch::Tensor state_tensor = state.to(device).clone();
        std::pair<torch::Tensor, torch::Tensor> result = network->forward(state_tensor);

        torch::Tensor value = result.first;
        torch::Tensor policy = result.second;

        float valueFloat = value[0][0].item<float>();

        // 将张量转换为 CPU 上的张量
        torch::Tensor cpu_policy = torch::exp(policy).cpu();

        // 打平并转换为 std::vector<float>
        std::vector<float> prior_prob(cpu_policy.data<float>(), cpu_policy.data<float>() + cpu_policy.numel());

        return std::make_pair(valueFloat, prior_prob);
    }

    void backpropagate(Node *node, float value)
    {
        while (node != nullptr)
        {
            node->update(value);
            node = node->parent;
            value = -value;
        }
    }

    std::pair<std::vector<int>, std::vector<float>> get_action_probabilities(Game game, float temperature = 1.0)
    {
        Node *node = root;
        std::vector<std::pair<int, int>> action_visits;
        for (auto &item : node->children)
        {
            action_visits.push_back(std::make_pair(item.first, item.second->visits));
        }

        std::vector<int> actions;
        std::vector<int> visits;
        for (auto &item : action_visits)
        {
            actions.push_back(item.first);
            visits.push_back(item.second);
        }

        // 计算总和
        float sum = 0.0;
        for (int visit : visits)
        {
            sum += visit;
        }

        // 归一化为概率分布
        std::vector<float> action_probs;
        for (int visit : visits)
        {
            float prob = static_cast<float>(visit) / sum;
            action_probs.push_back(prob);
        }

        std::vector<float> probs(game.boardSize * game.boardSize, 0);
        for (int i = 0; i < actions.size(); i++)
        {
            // if (visits[i] == 1)
            //     action_probs[i] = 0;
            probs[actions[i]] = action_probs[i];
        }

        std::vector<int> range(game.boardSize * game.boardSize);
        std::iota(range.begin(), range.end(), 0);
        return std::make_pair(range, probs);
    }

    std::vector<float> apply_temperature(std::vector<float> action_probabilities, float temperature)
    {
        if (temperature == 1)
            return action_probabilities;
        for (float &prob : action_probabilities)
        {
            prob = std::pow(prob, 1 / temperature);
        }
        float sum = std::accumulate(action_probabilities.begin(), action_probabilities.end(), 0.0f);
        for (float &prob : action_probabilities)
        {
            prob /= sum;
        }
        return action_probabilities;
    }

private:
    std::__1::shared_ptr<PolicyValueNetwork> network;
    Node *root;
    torch::Device device;
    float exploration_factor;
};
