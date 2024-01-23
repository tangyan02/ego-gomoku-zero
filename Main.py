import concurrent.futures
import subprocess
import time

import numpy as np
from torch import optim
from torch.optim import lr_scheduler

from Network import get_network, save_network
from Train import train
from Utils import getDevice, getTimeStr, dirPreBuild


def run_program(shard):
    # 执行可执行程序，传入参数shard
    process = subprocess.Popen(['./build/ego-gomoku-zero', str(shard)], stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)

    # 持续打印输出
    for line in process.stdout:
        print(line.decode(), end='')

    # 等待命令执行完成
    process.wait()

    # 检查命令的返回码
    if process.returncode == 0:
        print(f"Command execution successful for shard {shard}.")
    else:
        print(f"Command execution failed with return code {process.returncode} for shard {shard}.")


def selfPlayInCpp(shard_num, worker_num):
    with concurrent.futures.ThreadPoolExecutor(max_workers=worker_num) as executor:
        # 提交任务给线程池
        futures = [executor.submit(run_program, shard) for shard in range(shard_num)]
        # 等待所有任务完成
        concurrent.futures.wait(futures)


def getFileData(shard_num):
    training_data = []
    for shard in range(shard_num):
        f = open(f"record/data_{shard}.txt", "r")
        count = int(f.readline())
        for i in range(count):
            state_shape = f.readline().strip().split(" ")
            k, x, y = int(state_shape[0]), int(state_shape[1]), int(state_shape[2])
            state = np.zeros((k, x, y), dtype=float)
            for r in range(k):
                for i in range(x):
                    arr = f.readline().strip().split(" ")
                    for j in range(len(arr)):
                        state[r][i][j] = float(arr[j])
            f.readline()
            props_line = f.readline()
            props = [float(x) for x in props_line.strip().split(" ")]
            f.readline()
            values_line = f.readline()
            values = [float(x) for x in values_line.strip().split(" ")]

            training_data.append((state, np.array(props), np.array(values)))

    return training_data


def get_extended_data(play_data):
    extend_data = []
    for state, mcts_porb, value in play_data:
        for i in [1, 2, 3, 4]:
            # rotate counterclockwise
            equi_state = np.array([np.rot90(s, i) for s in state])
            board_size = state.shape[1]
            equi_mcts_prob = np.rot90(mcts_porb.reshape(board_size, board_size), i)
            extend_data.append((equi_state, equi_mcts_prob.flatten(), value))
            # flip horizontally
            equi_state = np.array([np.fliplr(s) for s in equi_state])
            equi_mcts_prob = np.fliplr(equi_mcts_prob)
            extend_data.append((equi_state,
                                equi_mcts_prob.flatten(),
                                value))
    return extend_data


dirPreBuild()

lr = 0.01
num_epochs = 5
batch_size = 128
episode = 100000
shard_num = 10
worker_num = 4

network = get_network()
save_network(network)

device = getDevice()
# 定义优化器
optimizer = optim.SGD(network.parameters(), lr)

# 定义学习率调度器
scheduler = lr_scheduler.OneCycleLR(optimizer, lr, total_steps=episode)

for i_episode in range(1, episode + 1):

    start_time = time.time()

    selfPlayInCpp(shard_num, worker_num)

    end_time = time.time()
    print(getTimeStr() + f"自我对弈完毕，用时 {end_time - start_time} s")

    training_data = getFileData(shard_num)
    extended_data = get_extended_data(training_data)
    print(getTimeStr() + f"完成扩展自我对弈数据，条数 " + str(len(extended_data)))

    # 输出当前的学习率
    current_lr = optimizer.param_groups[0]['lr']
    scheduler.step()
    print(f"Current learning rate: {current_lr}")

    train(extended_data, network, device, lr, num_epochs, batch_size)

    if i_episode % 100 == 0:
        save_network(network, f"model/net_{i_episode}.mdl")
        print(getTimeStr() + f"模型已保存 episode:{i_episode}")
    save_network(network)
    print(getTimeStr() + f"最新模型已保存 episode:{i_episode}")
