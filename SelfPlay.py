import numpy as np

from Game import FourInARowGame
from MTCS import MonteCarloTree
from Utils import getTimeStr


def print_game(game, action, action_probs):
    game.print_board()
    line = ""
    for i in range(game.board_size * game.board_size):
        line += str(round(action_probs[i], 2)) + " "
        if (i + 1) % game.board_size == 0:
            print(line)
            line = ""
    print(getTimeStr(), f"action is {action}")


def self_play(network, device, num_games, num_simulations):
    training_data = []

    mcts = MonteCarloTree(network, device)
    for _ in range(num_games):
        game = FourInARowGame()  # 初始化四子连珠游戏
        game_data = []

        while not game.is_game_over():
            mcts.search(game, num_simulations)  # 执行蒙特卡洛树搜索

            # 获取动作概率
            actions, action_probs = mcts.get_action_probabilities(game)

            # 归一化概率分布
            action_probs_normalized = action_probs / np.sum(action_probs)

            # 添加噪声
            noise_eps = 0.75  # 噪声参数
            dirichlet_alpha = 0.3  # dirichlet系数
            action_probs_with_noise = (1 - noise_eps) * action_probs_normalized + noise_eps * np.random.dirichlet(
                dirichlet_alpha * np.ones(len(action_probs_normalized)))

            # 根据带有噪声的概率分布选择动作
            action = np.random.choice(actions, p=action_probs_with_noise)

            # 保存当前状态和动作概率
            state = game.get_state()
            game_data.append((state, action_probs_normalized))
            game.make_move(game.parse_action_from_index(action))  # 执行动作

            print_game(game, action, action_probs_with_noise)

        winner = game.check_winner()
        # 为每个状态添加胜利者信息
        for state, mcts_probs in game_data:
            value = 1 if winner == game.current_player else -1 if winner == game.get_other_player() else 0
            # 将action_probs处理为概率值
            training_data.append((state, mcts_probs, value))
        print(getTimeStr(), f"winner is {winner}")

    return training_data
