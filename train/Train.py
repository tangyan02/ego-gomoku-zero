import torch
import torch.nn as nn
from torch.utils.data import DataLoader

from SampleSet import SampleSet
# 定义训练数据集类
from Utils import getTimeStr
from ConfigReader import ConfigReader


def _get_new_channel_lr_mult():
    """读 application.conf 中的 newChannelLrMult；缺省 1.0（不放大）。
    每次训练 episode 调用一次 init() 重读，支持运行时热调。"""
    try:
        ConfigReader.init()
        if 'newChannelLrMult' in ConfigReader.config:
            return float(ConfigReader.get('newChannelLrMult'))
    except Exception:
        pass
    return 1.0


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
    n_batches = len(dataloader)

    # 新通道（ch4/ch5）梯度放大系数：从 conv1.weight 的 [in_ch] 维度切片放大
    new_ch_lr_mult = _get_new_channel_lr_mult()
    conv1_weight = None
    if new_ch_lr_mult != 1.0:
        for name, p in network.named_parameters():
            if name == 'conv1.weight' or name.endswith('.conv1.weight'):
                if p.ndim == 4 and p.shape[1] >= 6:
                    conv1_weight = p
                    break
        if conv1_weight is not None and i_episode % 10 == 0:
            print(f"  [train] newChannelLrMult={new_ch_lr_mult} on conv1.weight ch4/5")

    for batch_idx, batch_data in enumerate(dataloader):
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

        # 在裁剪之后单独放大新通道梯度：ch4/ch5 等效 lr × new_ch_lr_mult
        if conv1_weight is not None and conv1_weight.grad is not None:
            with torch.no_grad():
                conv1_weight.grad[:, 4:6].mul_(new_ch_lr_mult)

        optimizer.step()

        running_loss += loss.item()
        running_value_loss += value_loss.item()
        running_policy_loss += policy_loss.item()

        # 进度输出（覆盖式）
        if (batch_idx + 1) % 10 == 0 or batch_idx == n_batches - 1:
            cur_loss = running_loss / (batch_idx + 1)
            print(f"\r  [train] batch {batch_idx+1}/{n_batches} loss={cur_loss:.4f}", end="", flush=True)

    print()  # 换行
    loss_avg = running_loss / n_batches
    value_loss_avg = running_value_loss / n_batches
    policy_loss_avg = running_policy_loss / n_batches
    print(getTimeStr() + f"episode {i_episode} Loss: {loss_avg:.4f} (value: {value_loss_avg:.4f}, policy: {policy_loss_avg:.4f})")
    return loss_avg, value_loss_avg, policy_loss_avg

