import numpy as np
import math

import torch
import torch.nn.functional as F


class MonteCarloTree:
    def __init__(self, value_network, device, exploration_factor=5):
        self.value_network = value_network
        self.root = None
        self.device = device
        self.exploration_factor = exploration_factor

    def simulate(self, game):
        node = self.root
        while not node.is_leaf():
            action, node = node.select_child(self.exploration_factor)
            game.make_move(action)

        if game.is_game_over():
            winner = game.check_winner()
            if winner == game.current_player:
                value = 1
            elif winner == game.get_other_player():
                value = -1
            else:
                value, prior_prob = self.evaluate_state(game.get_state())
        else:
            value, prior_prob = self.evaluate_state(game.get_state())
            node.expand(game, prior_prob)

        self.backpropagate(node, -value)

    def search(self, game, node, num_simulations):
        self.root = node

        for _ in range(num_simulations):
            self.simulate(game.copy())

    def evaluate_state(self, state):
        state_tensor = torch.from_numpy(state).unsqueeze(0).float().to(self.device)  # 将状态转换为张量
        value, policy = self.value_network(state_tensor)  # 使用策略价值网络评估状态

        value = value[0][0].cpu().item()  # 将值转换为标量
        prior_prob = np.exp(policy.data.cpu().numpy().flatten())
        return value, prior_prob

    def backpropagate(self, node, value):
        while node is not None:
            node.update(value)
            node = node.parent
            value = -value

    def get_action_probabilities(self, game, temperature=1.0):
        node = self.root
        action_visits = [(action, child.visits) for action, child in node.children.items()]
        actions, visits = zip(*action_visits)
        visits_tensor = torch.tensor(visits, dtype=torch.float)
        action_probs = F.softmax(1.0 / temperature * torch.log(visits_tensor + 1e-10), dim=0).tolist()

        probs = [0] * game.board_size * game.board_size
        for i in range(len(actions)):
            probs[game.get_action_index(actions[i])] = action_probs[i]

        return np.array(range(game.board_size * game.board_size)), np.array(probs)

    def apply_temperature(self, action_probabilities, temperature):
        if temperature == 1:
            return action_probabilities
        action_probabilities = np.power(action_probabilities, 1 / temperature)
        action_probabilities /= np.sum(action_probabilities)
        return action_probabilities


class Node:
    def __init__(self, parent=None):
        self.parent = parent
        self.children = {}
        self.visits = 0
        self.value_sum = 0
        self.prior_prob = 0

    def is_leaf(self):
        return len(self.children) == 0

    def select_child(self, exploration_factor):
        total_visits = self.visits
        ucb_values = [
            ((child.value_sum / child.visits) if child.visits > 0 else 0) +
            exploration_factor * child.prior_prob * math.sqrt(total_visits) /
            (1 + child.visits) for i, child in
            enumerate(self.children.values())]
        selected_action = None
        max_ucb = float('-inf')
        for i, action in enumerate(self.children):
            if ucb_values[i] > max_ucb:
                max_ucb = ucb_values[i]
                selected_action = action
        return selected_action, self.children[selected_action]

    def expand(self, game, prior_probs):
        actions = game.get_valid_actions()
        for i, action in enumerate(actions):
            child = Node(parent=self)
            child.prior_prob = prior_probs[game.get_action_index(action)]
            self.children[action] = child

    def update(self, value):
        self.visits += 1
        self.value_sum += value
