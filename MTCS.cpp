#include <iostream>
#include <cmath>
#include <unordered_map>
#include <vector>
#include "Game.cpp"

class MonteCarloTree
{
public:
    MonteCarloTree(torch::nn::Module network, torch::Device device, float exploration_factor = 5)
        : network(network), root(nullptr), device(device), exploration_factor(exploration_factor) {}

    void simulate(Game game)
    {
        if (game.isGameOver())
            return;

        Node *node = root;
        while (!node->isLeaf())
        {
            std::pair<int, Node *> result = node->selectChild(exploration_factor);
            int action = result.first;
            node = result.second;
            Point pointAction = game.getPointFormIndex(action);
            game.makeMove(pointAction);
        }

        std::pair<float, std::vector<float>> result = evaluate_state(game.getState());
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

    void search(Game game, Node *node, int num_simulations)
    {
        root = node;

        for (int i = 0; i < num_simulations; i++)
            simulate(game);
    }

    std::pair<float, std::vector<float>> evaluate_state(torch::Tensor state)
    {
        torch::Tensor state_tensor = state.to(device).clone();
        std::pair<torch::Tensor, torch::Tensor> result = network->forward(state_tensor);
        torch::Tensor value = result.first;
        torch::Tensor policy = result.second;

        float valueFloat = value[0][0].item<float>();
        std::vector<float> prior_prob = torch::exp(policy).cpu().data().flatten().tolist<float>();
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
        std::vector<std::pair<Action, int>> action_visits;
        for (auto &item : node->children)
        {
            action_visits.push_back(std::make_pair(item.first, item.second->visits));
        }

        std::vector<Action> actions;
        std::vector<int> visits;
        for (auto &item : action_visits)
        {
            actions.push_back(item.first);
            visits.push_back(item.second);
        }

        torch::Tensor visits_tensor = torch::from_blob(visits.data(), {visits.size()}).clone();
        std::vector<float> action_probs = torch::softmax(1.0 / temperature * torch::log(visits_tensor + 1e-10), 0).tolist<float>();

        std::vector<float> probs(game.board_size * game.board_size, 0);
        for (int i = 0; i < actions.size(); i++)
        {
            if (visits[i] == 1)
                action_probs[i] = 0;
            probs[game.get_action_index(actions[i])] = action_probs[i];
        }

        std::vector<int> range(game.board_size * game.board_size);
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
    torch::nn::Module network;
    Node *root;
    torch::Device device;
    float exploration_factor;
};

class Node
{
public:
    Node *parent;
    int visits;
    double value_sum;
    double prior_prob;
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
            double ucb_value = (child.second->value_sum / child.second->visits) +
                               exploration_factor * child.second->prior_prob *
                                   std::sqrt(total_visits) / (1 + child.second->visits);
            ucb_values.push_back(ucb_value);
        }
        int selected_action = -1;
        double max_ucb = std::numeric_limits<double>::lowest();
        for (int i = 0; i < ucb_values.size(); i++)
        {
            if (ucb_values[i] > max_ucb)
            {
                max_ucb = ucb_values[i];
                selected_action = i;
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
