import json
import time

import numpy as np

import Bridge
import Logger as Logger
from Network import get_model, save_model
from Train import train
from Utils import getDevice, dirPreBuild

import sys

from ConfigReader import ConfigReader

sys.stdout = sys.__stdout__
sys.stdout.reconfigure(line_buffering=True)  # 强制行缓冲


def get_extended_data(play_data):
    extend_data = []
    for state, mcts_porb, value in play_data:
        for i in [1, 2, 3, 4]:
            # rotate counterclockwise
            equi_state = np.array([np.rot90(s, i) for s in state])
            board_size = state.shape[1]
            equi_mcts_prob = np.rot90(mcts_porb.reshape(board_size, board_size), i)
            extend_data.append((equi_state, equi_mcts_prob.flatten(), value))
            # flip horizontally
            equi_state = np.array([np.fliplr(s) for s in equi_state])
            equi_mcts_prob = np.fliplr(equi_mcts_prob)
            extend_data.append((equi_state,
                                equi_mcts_prob.flatten(),
                                value))
    return extend_data


def update_count(k, filepath="model/count.txt"):
    try:
        with open(filepath, 'r') as f:
            count = int(f.read())
    except FileNotFoundError:
        count = 0

    count += k

    with open(filepath, 'w') as f:
        f.write(str(count))

    Logger.infoD(f"更新对局计数，当前完成对局 {count}")
    return count


if __name__ == "__main__":

    dirPreBuild()

    ConfigReader.init()

    board_size = int(ConfigReader.get('boardSize'))
    num_processes = int(ConfigReader.get('numProcesses'))
    lr = float(ConfigReader.get('lr'))
    episode = int(ConfigReader.get('episode'))
    batch_size = int(ConfigReader.get('batchSize'))
    numGames = int(ConfigReader.get('numGames'))
    cppPath = ConfigReader.get("cppPath")

    total_games_count = update_count(0)

    # 模型初始化
    device = getDevice()
    model, optimizer = get_model(device, lr)

    save_model(model, optimizer, board_size)

    for i_episode in range(1, episode + 1):

        start_time = time.time()

        Bridge.run_program(cppPath)

        training_data = Bridge.getFileData(num_processes)

        end_time = time.time()
        Logger.infoD(f"自我对弈完毕，用时 {end_time - start_time} s")

        extended_data = get_extended_data(training_data)
        Logger.infoD(f"完成扩展自我对弈数据，条数 " + str(len(extended_data)) + " , " + str(
            round(len(extended_data) / (end_time - start_time), 1)) + " 条/s")

        loss = train(extended_data, model, device, optimizer, batch_size, i_episode)

        save_model(model, optimizer, board_size)
        Logger.infoD(f"最新模型已保存 episode:{i_episode}")

        Logger.infoD(f"episode {i_episode} 完成")

        # 更新计数
        count = num_processes * numGames
        total_games_count = update_count(count)

        # 记录迭代信息
        episodeInfo = {
            "i_episode" : i_episode,
            "loss": loss,
            "total_games_count" : total_games_count
        }
        Logger.infoD(json.dumps(episodeInfo), "episode.log")

