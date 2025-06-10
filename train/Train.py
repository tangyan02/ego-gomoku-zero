import torch
import torch.nn as nn
from torch.utils.data import DataLoader

from SampleSet import SampleSet
# 定义训练数据集类
from Utils import getTimeStr


def train(extended_data, network, device, optimizer, batch_size, i_episode):
    # 创建数据加载器
    sample_set = SampleSet(extended_data)
    dataloader = DataLoader(sample_set, batch_size=batch_size, shuffle=True)
    # 定义损失函数
    criterion = nn.MSELoss()
    # 训练循环
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

    loss_avg = running_loss / len(dataloader)
    print(getTimeStr() + f"episode {i_episode} Loss: {loss_avg}")
    return loss_avg

