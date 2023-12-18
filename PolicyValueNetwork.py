import torch.nn as nn

class PolicyValueNetwork(nn.Module):
    def __init__(self):
        super(PolicyValueNetwork, self).__init__()
        self.conv1 = nn.Conv2d(2, 32, kernel_size=(3, 3), stride=(1, 1), padding=1)
        self.conv2 = nn.Conv2d(32, 64, kernel_size=(3, 3), stride=(1, 1), padding=1)
        self.fc1 = nn.Linear(64 * 6 * 6, 256)
        self.fc_value = nn.Linear(256, 1)
        self.fc_policy = nn.Linear(256, 36)  # 输出层大小为36，对应6x6个可能的动作
        self.relu = nn.ReLU()
        self.tanh = nn.Tanh()
        self.softmax = nn.Softmax(dim=1)

    def forward(self, x):
        if len(x.shape) == 3:
            x = x.unsqueeze(0)  # 在第一个维度上添加批量维度为1

        x = self.relu(self.conv1(x))
        x = self.relu(self.conv2(x))
        x = x.view(x.size(0), -1)  # 调整输入张量的形状
        x = self.fc1(x)
        x = self.relu(x)
        value = self.tanh(self.fc_value(x))
        policy = self.softmax(self.fc_policy(x))
        return value, policy