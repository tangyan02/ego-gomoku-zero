import numpy as np

from Game import FourInARowGame
from Utils import getTimeStr


def self_play(mcts, num_games, num_simulations):
    training_data = []

    for _ in range(num_games):
        game = FourInARowGame()  # 初始化四子连珠游戏
        game_data = []

        while not game.is_game_over():
            mcts.search(game, num_simulations)  # 执行蒙特卡洛树搜索

            # 获取动作概率
            actions, action_probs = mcts.get_action_probabilities(game.get_state())

            # 归一化概率分布
            action_probs_normalized = action_probs / np.sum(action_probs)
            # 根据归一化后的概率分布选择动作的索引
            action_index = np.random.choice(np.arange(len(actions)), p=action_probs_normalized)
            # 根据索引获取对应的动作
            action = actions[action_index]

            # 保存当前状态和动作概率
            state = game.get_state()
            game_data.append((state, action_probs))

            game.make_move(action)  # 执行动作

            print(getTimeStr(), "\n", game.get_board())
            print(getTimeStr(), f"action is {action}")

        winner = game.check_winner()
        # 为每个状态添加胜利者信息
        for state, action_probs in game_data:
            value = 1 if winner == 1 else -1 if winner == 2 else 0
            training_data.append((state, action_probs, value))
        print(getTimeStr(), "\n", game.get_board())
        print(getTimeStr(), f"winner is {winner}")

    return training_data
