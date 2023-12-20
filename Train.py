import os

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, Dataset

from PolicyValueNetwork import PolicyValueNetwork
from SelfPlay import self_play
# 定义训练数据集类
from Utils import getDevice, dirPreBuild, getTimeStr


class TrainingDataset(Dataset):
    def __init__(self, training_data):
        self.training_data = training_data

    def __getitem__(self, index):
        # 返回单个训练样本
        return self.training_data[index]

    def __len__(self):
        # 返回训练数据集大小
        return len(self.training_data)


def train(training_data, network, device, lr, num_epochs, batch_size):
    training_dataset = TrainingDataset(training_data)
    # 创建数据加载器
    dataloader = DataLoader(training_dataset, batch_size=batch_size, shuffle=True)
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

num_games = 20
num_simulations = 200
lr = 0.001
num_epochs = 200
batch_size = 32
episode = 10000

device = getDevice()
network = PolicyValueNetwork()
if os.path.exists(f"model/net_latest.mdl"):
    network.load_state_dict(torch.load(f"model/net_latest.mdl", map_location=torch.device(device)))
network.to(device)

for i_episode in range(1, episode + 1):
    training_data = self_play(network, device, num_games, num_simulations)
    train(training_data, network, device, lr, num_epochs, batch_size)

    if i_episode % 100 == 0:
        torch.save(network.state_dict(), f"model/net_{i_episode}.mdl")
        print(getTimeStr(), f"模型已保存 episode:{i_episode}")
    torch.save(network.state_dict(), f"model/net_latest.mdl")
    print(getTimeStr(), f"最新模型已保存 episode:{i_episode}")
