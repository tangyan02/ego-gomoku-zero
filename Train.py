import os

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, Dataset

from MTCS import MonteCarloTree
from PolicyValueNetwork import PolicyValueNetwork
from SelfPlay import self_play


def dirPreBuild():
    if not os.path.exists("model"):
        os.mkdir("model")


def getDevice():
    device = "cpu"
    if torch.cuda.is_available():
        device = "cuda"
    if hasattr(torch.backends, "mps"):
        if torch.backends.mps.is_available():
            device = "mps"
    print(device)
    return device


# 定义训练数据集类
class TrainingDataset(Dataset):
    def __init__(self, training_data):
        self.training_data = training_data

    def __getitem__(self, index):
        # 返回单个训练样本
        return self.training_data[index]

    def __len__(self):
        # 返回训练数据集大小
        return len(self.training_data)


def train(training_data, network, lr, num_epochs, batch_size):
    # 创建训练数据集实例
    training_dataset = TrainingDataset(training_data)
    # 创建数据加载器
    dataloader = DataLoader(training_dataset, batch_size=batch_size, shuffle=True)
    # 定义损失函数
    criterion = nn.MSELoss()
    # 定义优化器
    optimizer = optim.Adam(network.parameters(), lr)
    # 训练循环
    device = torch.device(getDevice())
    network.to(device)

    for epoch in range(num_epochs):
        running_loss = 0.0
        for batch_data in dataloader:
            states = batch_data[0].to(device)
            action_probs = batch_data[1].to(device)
            values = batch_data[2].to(device)

            optimizer.zero_grad()

            # 前向传播
            predicted_values, predicted_action_probs = network(states)

            # 计算值和策略的损失
            value_loss = criterion(predicted_values, values)
            policy_loss = criterion(predicted_action_probs, action_probs)

            # 总损失
            loss = value_loss + policy_loss

            # 反向传播和优化
            loss.backward()
            optimizer.step()

            running_loss += loss.item()

        print(f"Epoch {epoch + 1}/{num_epochs}, Loss: {running_loss / len(dataloader)}")


dirPreBuild()

num_games = 1
num_simulations = 100
lr = 0.001
num_epochs = 100
batch_size = 8
episode = 2

network = PolicyValueNetwork()
mcts = MonteCarloTree(network)

for i_episode in range(episode):
    training_data = self_play(mcts, num_games, num_simulations)
    print(training_data)
    train(training_data, network, lr, num_epochs, batch_size)
    torch.save(network.state_dict(), f"model/net_{i_episode}.mdl")
    print(f"模型已保存 episode:{i_episode}")
