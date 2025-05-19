import onnx
import torch
from onnxsim import simplify
import onnxoptimizer

import Network

lr = 0.001
network, optimizer = Network.get_network("cpu", lr)
network.eval()

# 获取模型的权重参数
state_dict = network.state_dict()

# 遍历权重参数并查看精度
for name, param in state_dict.items():
    print("Parameter:", name)
    print("Data Type:", param.dtype)
    print("Data Shape:", param.shape)
    print("Data:", param)
    print()