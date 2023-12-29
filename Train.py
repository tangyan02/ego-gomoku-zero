import os

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader

from PolicyValueNetwork import PolicyValueNetwork
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

        print(getTimeStr(), f"Epoch {epoch + 1}/{num_epochs}, Loss: {running_loss / len(dataloader)}")


dirPreBuild()

num_games = 10
# num_games = 1
num_simulations = 400
lr = 0.001
num_epochs = 100
batch_size = 32
episode = 10000
replay_buffer_size = 40000
start_train_size = 10000
# start_train_size = 0
temperature = 1
exploration_factor = 3

device = getDevice()
network = PolicyValueNetwork()
if os.path.exists(f"model/net_latest.mdl"):
    network.load_state_dict(torch.load(f"model/net_latest.mdl", map_location=torch.device(device)))
else:
    # 将所有权重初始化为 0
    for param in network.parameters():
        param.data.fill_(0)

network.to(device)

replay_buffer = ReplayBuffer(replay_buffer_size)

for i_episode in range(1, episode + 1):
    training_data = self_play(network, device, num_games, num_simulations, temperature, exploration_factor)

    replay_buffer.add_samples(training_data)

    if replay_buffer.size() >= start_train_size:
        train(replay_buffer, network, device, lr, num_epochs, batch_size)

        if i_episode % 100 == 0:
            torch.save(network.state_dict(), f"model/net_{i_episode}.mdl")
            print(getTimeStr(), f"模型已保存 episode:{i_episode}")
        torch.save(network.state_dict(), f"model/net_latest.mdl")
        print(getTimeStr(), f"最新模型已保存 episode:{i_episode}")
