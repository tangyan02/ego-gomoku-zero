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
    running_value_loss = 0.0
    running_policy_loss = 0.0
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

        # 梯度裁剪，防止梯度爆炸
        torch.nn.utils.clip_grad_norm_(network.parameters(), max_norm=1.0)

        optimizer.step()

        running_loss += loss.item()
        running_value_loss += value_loss.item()
        running_policy_loss += policy_loss.item()

    n_batches = len(dataloader)
    loss_avg = running_loss / n_batches
    value_loss_avg = running_value_loss / n_batches
    policy_loss_avg = running_policy_loss / n_batches
    print(getTimeStr() + f"episode {i_episode} Loss: {loss_avg:.4f} (value: {value_loss_avg:.4f}, policy: {policy_loss_avg:.4f})")
    return loss_avg

