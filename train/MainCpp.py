import json
import random
import time
from collections import deque

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


class ReplayBuffer:
    """经验回放池，保留最近 max_size 条训练数据"""

    def __init__(self, max_size=500000):
        self.buffer = deque(maxlen=max_size)

    def add(self, data_list):
        self.buffer.extend(data_list)

    def sample(self, sample_size):
        if sample_size >= len(self.buffer):
            return list(self.buffer)
        return random.sample(list(self.buffer), sample_size)

    def __len__(self):
        return len(self.buffer)


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
    wd = float(ConfigReader.get('wd'))
    episode = int(ConfigReader.get('episode'))
    batch_size = int(ConfigReader.get('batchSize'))
    numGames = int(ConfigReader.get('numGames'))
    cppPath = ConfigReader.get("cppPath")
    train_epochs = int(ConfigReader.get('trainEpochs') if 'trainEpochs' in ConfigReader.config else 3)
    replay_buffer_size = int(ConfigReader.get('replayBufferSize') if 'replayBufferSize' in ConfigReader.config else 500000)

    total_games_count = update_count(0)

    # 模型初始化
    device = getDevice()
    model, optimizer = get_model(device, lr, wd)

    save_model(model, optimizer)

    # 经验回放池
    replay_buffer = ReplayBuffer(max_size=replay_buffer_size)
    Logger.infoD(f"经验回放池已初始化，容量 {replay_buffer_size}")

    for i_episode in range(1, episode + 1):

        start_time = time.time()

        Bridge.run_program(cppPath)

        training_data = Bridge.getFileData(num_processes)

        end_time = time.time()
        Logger.infoD(f"自我对弈完毕，用时 {end_time - start_time} s")

        extended_data = get_extended_data(training_data)
        speed = round(len(extended_data) / (end_time - start_time), 1)
        Logger.infoD(f"完成扩展自我对弈数据，条数 " + str(len(extended_data)) + " , " + str(
            speed) + " 条/s")

        # 加入经验回放池
        replay_buffer.add(extended_data)
        Logger.infoD(f"回放池大小: {len(replay_buffer)}")

        # 从回放池中采样训练数据
        sample_size = min(len(replay_buffer), max(len(extended_data) * 2, batch_size * 20))
        sampled_data = replay_buffer.sample(sample_size)
        Logger.infoD(f"本轮采样 {len(sampled_data)} 条数据进行训练")

        # 多 epoch 训练
        total_loss = 0.0
        for epoch in range(train_epochs):
            loss = train(sampled_data, model, device, optimizer, batch_size, i_episode)
            total_loss += loss
        avg_loss = total_loss / train_epochs
        Logger.infoD(f"episode {i_episode} 训练 {train_epochs} epochs, 平均 loss: {avg_loss:.4f}")

        save_model(model, optimizer)
        Logger.infoD(f"最新模型已保存 episode:{i_episode}")

        Logger.infoD(f"episode {i_episode} 完成")

        # 更新计数
        total_games_count = update_count(numGames)

        # 记录迭代信息
        episodeInfo = {
            "i_episode": i_episode,
            "loss": avg_loss,
            "record_count": len(extended_data),
            "buffer_size": len(replay_buffer),
            "sample_size": len(sampled_data),
            "train_epochs": train_epochs,
            "total_games_count": total_games_count,
            "speed": speed
        }
        Logger.infoD(json.dumps(episodeInfo), "episode.log")

