import os

import torch
import torch.nn as nn
import torch.nn.functional as F

from Utils import getDevice


class PolicyValueNetwork(nn.Module):
    def __init__(self):
        self.board_size = 11
        self.input_channels = 3
        super(PolicyValueNetwork, self).__init__()

        # common layers
        self.conv1 = nn.Conv2d(self.input_channels, 32, kernel_size=(3, 3), padding=1)
        self.conv2 = nn.Conv2d(32, 64, kernel_size=(3, 3), padding=1)
        self.conv3 = nn.Conv2d(64, 128, kernel_size=(3, 3), padding=1)
        # action policy layers
        self.act_conv1 = nn.Conv2d(128, 4, kernel_size=(1, 1))
        self.act_fc1 = nn.Linear(4 * self.board_size * self.board_size,
                                 self.board_size * self.board_size)
        # state value layers
        self.val_conv1 = nn.Conv2d(128, 2, kernel_size=(1, 1))
        self.val_fc1 = nn.Linear(2 * self.board_size * self.board_size, 64)
        self.val_fc2 = nn.Linear(64, 1)

    def forward(self, state_input):
        # common layers
        x = F.relu(self.conv1(state_input))
        x = F.relu(self.conv2(x))
        x = F.relu(self.conv3(x))
        # action policy layers
        x_act = F.relu(self.act_conv1(x))
        x_act = x_act.view(-1, 4 * self.board_size * self.board_size)
        x_act = F.log_softmax(self.act_fc1(x_act))
        # state value layers
        x_val = F.relu(self.val_conv1(x))
        x_val = x_val.view(-1, 2 * self.board_size * self.board_size)
        x_val = F.relu(self.val_fc1(x_val))
        x_val = torch.tanh(self.val_fc2(x_val))

        return x_val, x_act


def get_network():
    network = PolicyValueNetwork()
    if os.path.exists(f"model/net_latest.mdl"):
        network.load_state_dict(torch.load(f"model/net_latest.mdl", map_location=torch.device(getDevice())))
    return network


def save_network(network, path=f"model/net_latest.mdl"):
    torch.save(network.state_dict(), path)
