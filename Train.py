import time
import logging

import Network
import torch
import torch.nn as nn
import torch.optim as optim
from torch.multiprocessing import Pool
from torch.utils.data import DataLoader

from Network import PolicyValueNetwork
from ReplayBuffer import ReplayBuffer
from SelfPlay import self_play
# 定义训练数据集类
from Utils import getDevice, dirPreBuild, getTimeStr


def train(replay_buffer, network, device, lr, num_epochs, batch_size):
    # 创建数据加载器
    dataloader = DataLoader(replay_buffer, batch_size=batch_size, shuffle=True)
    # 定义损失函数
    criterion = nn.MSELoss()
    # 定义优化器
    optimizer = optim.Adam(network.parameters(), lr)
    # 训练循环
    for epoch in range(num_epochs):
        running_loss = 0.0
        for batch_data in dataloader:
            states = batch_data[0].float().to(device)
            mcts_probs = batch_data[1].float().to(device)
            values = batch_data[2].float().to(device)

            optimizer.zero_grad()

            # 前向传播
            predicted_values, predicted_action_logits = network(states)

            # 计算值和策略的损失
            value_loss = criterion(predicted_values, values)

            # 计算交叉熵损失
            policy_loss = -torch.mean(torch.sum(mcts_probs * predicted_action_logits, 1))

            # 总损失
            loss = value_loss + policy_loss

            # 反向传播和优化
            loss.backward()
            optimizer.step()

            running_loss += loss.item()

        logging.info(getTimeStr() + f"Epoch {epoch + 1}/{num_epochs}, Loss: {running_loss / len(dataloader)}")


logging.basicConfig(filename='output.log', level=logging.INFO)

if __name__ == '__main__':
    torch.multiprocessing.set_start_method('spawn')
    # 配置日志记录器
    dirPreBuild()

    num_games = 10
    concurrent_size = 5

    # num_games = 1
    num_simulations = 400
    lr = 0.001
    num_epochs = 25
    batch_size = 128
    episode = 10000
    replay_buffer_size = 20000
    start_train_size = 10000
    # start_train_size = 100000
    temperature = 1
    exploration_factor = 3

    device = getDevice()
    replay_buffer = ReplayBuffer(replay_buffer_size)

    pool = Pool(processes=concurrent_size)

    for i_episode in range(1, episode + 1):
        start_time = time.time()
        # 使用多进程进行计算
        sub_num_games = num_games // concurrent_size
        params = [(device, sub_num_games, num_simulations, temperature, exploration_factor)
                  for _ in range(concurrent_size)]
        training_data_list = pool.starmap(self_play, params)

        training_data = []
        for item in training_data_list:
            training_data += item

        end_time = time.time()
        logging.info(getTimeStr() + f"通过{num_games}次对局，获得样本共计{len(training_data)}，用时{end_time - start_time}s")

        replay_buffer.add_samples(training_data)

        if replay_buffer.size() >= start_train_size:
            network = Network.get_network()

            start_time = time.time()
            train(replay_buffer, network, device, lr, num_epochs, batch_size)
            end_time = time.time()
            logging.info(getTimeStr() + f"训练完毕，用时{end_time - start_time}")

            if i_episode % 100 == 0:
                Network.save_network(network, f"model/net_{i_episode}.mdl")
                logging.info(getTimeStr() + f"模型已保存 episode:{i_episode}")
            Network.save_network(network)
            logging.info(getTimeStr() + f"最新模型已保存 episode:{i_episode}")
