import os

import torch
import torch.nn as nn
import torch.nn.functional as F

from Utils import getDevice


class ResidualBlock(nn.Module):
    def __init__(self, channels):
        super(ResidualBlock, self).__init__()
        self.conv1 = nn.Conv2d(channels, channels, kernel_size=(3, 3), padding=1)
        self.conv2 = nn.Conv2d(channels, channels, kernel_size=(3, 3), padding=1)

    def forward(self, x):
        residual = x
        out = F.relu(self.conv1(x))
        out = self.conv2(out)
        out += residual
        out = F.relu(out)
        return out


class PolicyValueNetwork(nn.Module):
    def __init__(self):
        self.board_size = 15
        self.input_channels = 4
        self.filters = 128
        super(PolicyValueNetwork, self).__init__()

        # common layers
        self.conv1 = nn.Conv2d(self.input_channels, self.filters, kernel_size=(3, 3), padding=1)

        self.residual_blocks = nn.Sequential(
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters),
            ResidualBlock(self.filters)
        )

        # action policy layers
        self.act_conv1 = nn.Conv2d(self.filters, 4, kernel_size=(1, 1))
        self.act_fc1 = nn.Linear(4 * self.board_size * self.board_size,
                                 self.board_size * self.board_size)
        # state value layers
        self.val_conv1 = nn.Conv2d(self.filters, 2, kernel_size=(1, 1))
        self.val_fc1 = nn.Linear(2 * self.board_size * self.board_size, 64)
        self.val_fc2 = nn.Linear(64, 1)

    def forward(self, state_input):
        if state_input.dim() == 3:
            state_input = torch.unsqueeze(state_input, dim=0)

        # common layers
        x = F.relu(self.conv1(state_input))
        x = self.residual_blocks(x)

        # action policy layers
        x_act = F.relu(self.act_conv1(x))
        x_act = x_act.view(-1, 4 * self.board_size * self.board_size)
        x_act = F.log_softmax(self.act_fc1(x_act), dim=1)
        # state value layers
        x_val = F.relu(self.val_conv1(x))
        x_val = x_val.view(-1, 2 * self.board_size * self.board_size)
        x_val = F.relu(self.val_fc1(x_val))
        x_val = torch.tanh(self.val_fc2(x_val))

        return x_val, x_act


def get_network(device=getDevice()):
    network = PolicyValueNetwork()
    if os.path.exists(f"../model/net_latest.mdl"):
        network.load_state_dict(torch.load(f"../model/net_latest.mdl", map_location=torch.device(device)))
    network.to(device)
    return network


def save_network(network, path=f"model/net_latest.mdl"):
    torch.save(network.state_dict(), path)
    torch.jit.save(torch.jit.script(network), path + ".pt")
