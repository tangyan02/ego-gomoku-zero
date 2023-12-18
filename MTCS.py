import numpy as np
import math

import torch
import torch.nn.functional as F

from Utils import getDevice


class MonteCarloTree:
    def __init__(self, value_network, device):
        self.value_network = value_network
        self.root = None
        self.node_dict = {}
        self.device = device

    def search(self, game, num_simulations):
        self.root = Node(game, None, self.node_dict)

        for _ in range(num_simulations):
            node = self.root
            while not node.is_leaf():
                action, node = node.select_child()

            if node.game.is_game_over():
                winner = node.game.check_winner()
                if winner == node.game.current_player:
                    value = -1
                elif winner == node.game.get_other_player():
                    value = 1
                else:
                    value, prior_prob = self.evaluate_state(node.game.get_state())
            else:
                value, prior_prob = self.evaluate_state(node.game.get_state())
                node.expand(prior_prob)

            self.backpropagate(node, value)

    def evaluate_state(self, state):
        state_tensor = torch.from_numpy(state).unsqueeze(0).float().to(self.device)  # 将状态转换为张量
        value, policy = self.value_network(state_tensor)  # 使用策略价值网络评估状态

        value = value.cpu().item()  # 将值转换为标量
        prior_prob = policy.cpu().squeeze().detach().numpy()  # 将概率转换为NumPy数组
        return value, prior_prob

    def backpropagate(self, node, value):
        while node is not None:
            node.update(value)
            node = node.parent
            value = -value

    def get_action_probabilities(self, state, temperature=1.0):
        node = self.node_dict[str(state)]
        action_visits = [(action, child.visits) for action, child in node.children.items()]
        actions, visits = zip(*action_visits)
        visits_tensor = torch.tensor(visits, dtype=torch.float)
        action_probs = F.softmax(1.0 / temperature * torch.log(visits_tensor + 1e-10), dim=0).tolist()

        acts = []
        for x in range(node.game.board_size):
            for y in range(node.game.board_size):
                acts.append((x, y))
        probs = [0] * node.game.board_size * node.game.board_size
        for i in range(len(actions)):
            probs[node.game.get_action_index(actions[i])] = action_probs[i]

        return np.array(acts), np.array(probs)

    def apply_temperature(self, action_probabilities, temperature):
        action_probabilities = np.power(action_probabilities, 1 / temperature)
        action_probabilities /= np.sum(action_probabilities)
        return action_probabilities


class Node:
    def __init__(self, game, parent=None, node_dict=None):
        self.game = game
        self.parent = parent
        self.children = {}
        self.visits = 0
        self.value_sum = 0
        self.prior_prob = 0
        self.node_dict = node_dict
        self.node_dict[str(game.get_state())] = self

    def is_leaf(self):
        return len(self.children) == 0

    def select_child(self, exploration_factor=1.4):
        total_visits = self.visits
        ucb_values = [
            ((child.value_sum / child.visits) if child.visits > 0 else 0) +
            exploration_factor * child.prior_prob * math.sqrt(total_visits) /
            (1 + child.visits) for i, child in
            enumerate(self.children.values())]
        selected_action = None
        max_ucb = float('-inf')
        for i, child in enumerate(self.children):
            if ucb_values[i] > max_ucb:
                max_ucb = ucb_values[i]
                selected_action = child
        return selected_action, self.children[selected_action]

    def expand(self, prior_probs):
        actions = self.game.get_valid_actions()
        for i, action in enumerate(actions):
            new_game = self.game.copy()
            new_game.make_move(action)
            new_node = Node(new_game, parent=self, node_dict=self.node_dict)
            new_node.prior_prob = prior_probs[self.game.get_action_index(action)]
            self.children[action] = new_node

    def update(self, value):
        self.visits += 1
        self.value_sum += value
