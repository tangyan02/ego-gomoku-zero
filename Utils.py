import os
import time

import torch


def getTimeStr():
    return time.strftime("[%Y-%m-%d %H:%M:%S] ", time.localtime())

def dirPreBuild():
    if not os.path.exists("model"):
        os.mkdir("model")

    if not os.path.exists("record"):
        os.mkdir("record")

def getDevice():
    device = "cpu"
    if torch.cuda.is_available():
        device = "cuda"
    if hasattr(torch.backends, "mps"):
        if torch.backends.mps.is_available():
            device = "mps"
    print(device)
    return device