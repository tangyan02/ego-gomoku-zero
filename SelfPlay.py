import logging

import Network
import numpy as np

from Game import FourInARowGame
from MTCS import MonteCarloTree, Node
from Utils import getTimeStr


def print_game(game, action, action_probs):
    game.print_board()
    line = ""
    for i in range(game.board_size * game.board_size):
        line += str(round(action_probs[i], 3)) + " "
        if (i + 1) % game.board_size == 0:
            logging.info(line)
            line = ""
    if game.get_other_player() == 1:
        pic = "x"
    else:
        pic = "o"
    logging.info(getTimeStr() + f" {pic} action is {game.parse_action_from_index(action)}")


def get_equi_data(game, play_data):
    """augment the data set by rotation and flipping
    play_data: [(state, mcts_prob, winner_z), ..., ...]
    """
    extend_data = []
    for state, mcts_porb, value in play_data:
        for i in [1, 2, 3, 4]:
            # rotate counterclockwise
            equi_state = np.array([np.rot90(s, i) for s in state])
            equi_mcts_prob = np.rot90(mcts_porb.reshape(game.board_size, game.board_size), i)
            extend_data.append((equi_state, equi_mcts_prob.flatten(), value))
            # flip horizontally
            equi_state = np.array([np.fliplr(s) for s in equi_state])
            equi_mcts_prob = np.fliplr(equi_mcts_prob)
            extend_data.append((equi_state,
                                equi_mcts_prob.flatten(),
                                value))
    return extend_data


def get_noise_action(actions, action_probs_normalized):
    noise_eps = 0.1  # 噪声参数
    dirichlet_alpha = 0.9  # dirichlet系数

    # 根据带有噪声的概率分布选择动作
    action_probs_with_noise = (1 - noise_eps) * action_probs_normalized + noise_eps * np.random.dirichlet(
        dirichlet_alpha * np.ones(len(action_probs_normalized)))
    return np.random.choice(actions, p=action_probs_with_noise)


def self_play(device, num_games, num_simulations, temperature, exploration_factor):
    network = Network.get_network(device)
    training_data = []

    mcts = MonteCarloTree(network, device, exploration_factor)
    for _ in range(num_games):
        game = FourInARowGame()  # 初始化四子连珠游戏
        game_data = []

        while not game.is_game_over():
            mcts.search(game, Node(None), num_simulations)  # 执行蒙特卡洛树搜索

            # 获取动作概率
            actions, action_probs = mcts.get_action_probabilities(game)

            action_probs_temperature = mcts.apply_temperature(action_probs, temperature)

            # 归一化概率分布
            action_probs_normalized = action_probs_temperature / np.sum(action_probs_temperature)

            # 添加噪声
            action = get_noise_action(actions, action_probs_normalized)
            while not game.is_valid(game.parse_action_from_index(action)):
                action = get_noise_action(actions, action_probs_normalized)

            # action = np.random.choice(actions, p=action_probs_normalized)

            # 保存当前状态和动作概率
            state = game.get_state()
            record = (state, game.current_player, action_probs)

            # 执行动作
            game.make_move(game.parse_action_from_index(action))
            game_data.append(record)
            print_game(game, action, action_probs_normalized)

        winner = game.check_winner()
        # 为每个状态添加胜利者信息
        for state, player, mcts_probs in game_data:
            value = 1 if winner == player else -1 if winner == (3 - player) else 0
            # 将action_probs处理为概率值
            training_data.append((state, mcts_probs, np.array([value])))

        logging.info(getTimeStr() + f"winner is {winner}")

    extend_data = get_equi_data(FourInARowGame(), training_data)
    return extend_data
