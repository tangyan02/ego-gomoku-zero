import os

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch import optim

from Utils import getDevice, getTimeStr


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
        self.input_channels = 16
        # self.input_channels = 2
        self.residual_channels = 128
        super(PolicyValueNetwork, self).__init__()

        # common layers
        self.conv1 = nn.Conv2d(self.input_channels, self.residual_channels, kernel_size=(3, 3), padding=1)

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
        self.act_conv1 = nn.Conv2d(self.residual_channels, 4, kernel_size=(1, 1), bias=False)
        self.act_bn1 = nn.BatchNorm2d(4)
        self.act_fc1 = nn.Linear(4 * self.board_size * self.board_size,
                                 self.board_size * self.board_size)
        # state value layers
        self.val_conv1 = nn.Conv2d(self.residual_channels, 2, kernel_size=(1, 1), bias=False)
        self.val_bn1 = nn.BatchNorm2d(2)
        self.val_fc1 = nn.Linear(2 * self.board_size * self.board_size, 64)
        self.val_fc2 = nn.Linear(64, 1)

    def forward(self, state_input):
        if state_input.dim() == 3:
            state_input = torch.unsqueeze(state_input, dim=0)

        # common layers
        x = F.relu(self.conv1(state_input))
        x = self.residual_blocks(x)

        # action policy layers
        x_act = self.act_conv1(x)
        x_act = self.act_bn1(x_act)
        x_act = F.relu(x_act)
        x_act = x_act.view(-1, 4 * self.board_size * self.board_size)
        x_act = F.log_softmax(self.act_fc1(x_act), dim=1)

        # state value layers
        x_val = self.val_conv1(x)
        x_val = self.val_bn1(x_val)
        x_val = F.relu(x_val)
        x_val = x_val.view(-1, 2 * self.board_size * self.board_size)
        x_val = F.relu(self.val_fc1(x_val))
        x_val = torch.tanh(self.val_fc2(x_val))

        return x_val, x_act


def get_network(device, lr):
    network = PolicyValueNetwork()
    network.to(device)  # 将网络移动到设备

    # 定义优化器
    optimizer = optim.Adam(network.parameters(), lr)

    if os.path.exists(f"model/checkpoint.pth"):
        checkpoint = torch.load("model/checkpoint.pth")
        network.load_state_dict(checkpoint['model_state_dict'])

        # 重新定义优化器，确保优化器的状态在正确的设备上
        optimizer = optim.Adam(network.parameters(), lr)
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])

    return network, optimizer


def save_network(network, optimizer, subfix=""):
    path = f"model/checkpoint{subfix}.pth"
    torch.save({
        'model_state_dict': network.state_dict(),
        'optimizer_state_dict': optimizer.state_dict(),
    }, path)

    torch.jit.save(torch.jit.script(network), "model/model_latest.pt")
