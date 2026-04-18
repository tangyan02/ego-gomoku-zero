import os

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.optim import AdamW


# 定义一个Residual block
class ResidualBlock(nn.Module):
    def __init__(self, channels):
        super(ResidualBlock, self).__init__()

        self.conv1 = nn.Conv2d(channels, channels, kernel_size=3, stride=1, padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(channels)
        self.relu = nn.ReLU(inplace=True)
        self.conv2 = nn.Conv2d(channels, channels, kernel_size=3, stride=1, padding=1, bias=False)
        self.bn2 = nn.BatchNorm2d(channels)

    def forward(self, x):
        residual = x

        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu(out)

        out = self.conv2(out)
        out = self.bn2(out)

        out += residual
        out = self.relu(out)

        return out


class PolicyValueNetwork(nn.Module):
    def __init__(self):
        self.board_size = 20
        self.input_channels = 4  # 己方棋子, 对方棋子, 最近一步, 最近两步
        self.residual_channels = 128
        super(PolicyValueNetwork, self).__init__()

        # common layers
        self.conv1 = nn.Conv2d(self.input_channels, self.residual_channels, kernel_size=(3, 3), padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(self.residual_channels)

        self.residual_blocks = nn.Sequential(
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels),
            ResidualBlock(self.residual_channels)
        )

        # action policy layers
        self.act_channels = 8
        self.act_conv1 = nn.Conv2d(self.residual_channels, self.act_channels, kernel_size=(1, 1), bias=False)
        self.act_bn1 = nn.BatchNorm2d(self.act_channels)
        self.act_fc1 = nn.Linear(self.act_channels * self.board_size * self.board_size,
                                 self.board_size * self.board_size)
        # state value layers
        self.val_channels = 8
        self.val_fc1_dim = 128
        self.val_conv1 = nn.Conv2d(self.residual_channels, self.val_channels, kernel_size=(1, 1), bias=False)
        self.val_bn1 = nn.BatchNorm2d(self.val_channels)
        self.val_fc1 = nn.Linear(self.val_channels * self.board_size * self.board_size, self.val_fc1_dim)
        self.val_fc2 = nn.Linear(self.val_fc1_dim, 1)

    def forward(self, state_input):
        if state_input.dim() == 3:
            state_input = torch.unsqueeze(state_input, dim=0)

        # common layers
        x = F.relu(self.bn1(self.conv1(state_input)))
        x = self.residual_blocks(x)

        # action policy layers
        x_act = self.act_conv1(x)
        x_act = self.act_bn1(x_act)
        x_act = F.relu(x_act)
        x_act = x_act.view(-1, self.act_channels * self.board_size * self.board_size)
        x_act = F.log_softmax(self.act_fc1(x_act), dim=1)

        # state value layers
        x_val = self.val_conv1(x)
        x_val = self.val_bn1(x_val)
        x_val = F.relu(x_val)
        x_val = x_val.view(-1, self.val_channels * self.board_size * self.board_size)
        x_val = F.relu(self.val_fc1(x_val))
        x_val = torch.tanh(self.val_fc2(x_val))

        return x_val, x_act


def get_model(device, lr, wd):
    network = PolicyValueNetwork()
    network.to(device)  # 将网络移动到设备

    # 定义优化器
    optimizer = AdamW(network.parameters(), lr=lr, weight_decay=wd)

    if os.path.exists(f"model/checkpoint.pth"):
        checkpoint = torch.load("model/checkpoint.pth", device)
        saved_state = checkpoint['model_state_dict']
        model_state = network.state_dict()

        # 兼容旧权重：跳过形状不匹配的层（新增/改变的 BN 和 Value 头参数）
        compatible_state = {}
        skipped_keys = []
        for key in model_state:
            if key in saved_state and saved_state[key].shape == model_state[key].shape:
                compatible_state[key] = saved_state[key]
            else:
                skipped_keys.append(key)

        network.load_state_dict(compatible_state, strict=False)

        if skipped_keys:
            print(f"[Network] 权重迁移：继承 {len(compatible_state)}/{len(model_state)} 个参数，"
                  f"以下 {len(skipped_keys)} 个参数使用随机初始化：")
            for k in skipped_keys:
                print(f"  - {k} (new shape: {model_state[k].shape})")
        else:
            print(f"[Network] 权重完全匹配，已加载全部 {len(compatible_state)} 个参数")

        # 重新定义优化器（旧优化器状态与新参数形状不匹配，不再加载）
        optimizer = AdamW(network.parameters(), lr=lr, weight_decay=wd)

        # 仅在权重完全匹配时恢复优化器状态
        if not skipped_keys:
            try:
                optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
            except Exception as e:
                print(f"[Network] 优化器状态加载失败，使用新优化器: {e}")

    return network, optimizer


def save_model(network, optimizer, onnx_path='model/model_latest.onnx'):
    path = f"model/checkpoint.pth"
    torch.save({
        'model_state_dict': network.state_dict(),
        'optimizer_state_dict': optimizer.state_dict(),
    }, path)

    torch.jit.save(torch.jit.script(network), "model/model_latest.pt")

    # 导出 FP32 ONNX（CoreML provider 对 FP32 加速效果最好，实测快 2.6x）
    # 注：FP16 会导致 CoreML fallback 到 CPU，反而变慢，故不使用
    network.eval()
    example = torch.randn(1, network.input_channels, 20, 20, requires_grad=True,
                          device=next(network.parameters()).device)

    torch.onnx.export(network,
                      (example),
                      onnx_path,
                      input_names=['input'],
                      output_names=['value', "act"],
                      dynamic_axes={
                          'input': {0: 'batch'},
                          'value': {0: 'batch'},
                          'act': {0: 'batch'},
                      },
                      opset_version=17,
                      verbose=False)

    network.train()
