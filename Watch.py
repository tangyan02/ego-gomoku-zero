import numpy as np
import torch

from Game import FourInARowGame
from MTCS import MonteCarloTree
from PolicyValueNetwork import PolicyValueNetwork
from Utils import getTimeStr, dirPreBuild, getDevice


def self_play(mcts, num_games, num_simulations):
    for _ in range(num_games):
        game = FourInARowGame()  # 初始化四子连珠游戏

        while not game.is_game_over():
            mcts.search(game, num_simulations)  # 执行蒙特卡洛树搜索

            # 获取动作概率
            actions, action_probs = mcts.get_action_probabilities(game.get_state())
            max_prob_index = np.argmax(action_probs)
            action = actions[max_prob_index]
            game.make_move(action)  # 执行动作

            game.print_board()
            line = ""
            for i in range(game.board_size * game.board_size):
                line += str(round(action_probs[i], 2)) + " "
                if (i + 1) % game.board_size == 0:
                    print(line)
                    line = ""
            print(getTimeStr(), f"action is {action}")

        winner = game.check_winner()
        # 为每个状态添加胜利者信息
        print(getTimeStr(), f"winner is {winner}")

dirPreBuild()

num_games = 1
num_simulations = 2000

device = getDevice()
network = PolicyValueNetwork()
network.load_state_dict(torch.load(f"model/net_latest.mdl", map_location=torch.device(device)))
network.to(device)

while True:
    mcts = MonteCarloTree(network, device)
    self_play(mcts, num_games, num_simulations)
