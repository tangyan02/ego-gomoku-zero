import subprocess
import time

import numpy as np

from Network import get_network, save_network
from ReplayBuffer import ReplayBuffer
from Train import train
from Utils import getDevice, getTimeStr, dirPreBuild


def selfPlayInCpp():
    # 执行可执行程序
    process = subprocess.Popen('./build/ego-gomoku-zero', stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    # 持续打印输出
    for line in process.stdout:
        print(line.decode(), end='')

    # 等待命令执行完成
    process.wait()

    # 检查命令的返回码
    if process.returncode == 0:
        print("Command execution successful.")
    else:
        print(f"Command execution failed with return code: {process.returncode}")


def getFileData():
    f = open("record/data.txt", "r")
    count = int(f.readline())
    training_data = []
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


def get_equi_data(play_data):
    """augment the data set by rotation and flipping
    play_data: [(state, mcts_prob, winner_z), ..., ...]
    """
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

lr = 0.001
num_epochs = 5
batch_size = 128
episode = 10000
replay_buffer_size = 12000

network = get_network()
save_network(network)

device = getDevice()

replay_buffer = ReplayBuffer(replay_buffer_size)
for i_episode in range(1, episode + 1):

    start_time = time.time()

    selfPlayInCpp()

    end_time = time.time()
    print(getTimeStr() + f"训练完毕，用时 {end_time - start_time} s")

    training_data = getFileData()
    equi_data = get_equi_data(training_data)
    print(getTimeStr() + "完成扩展训练数据，条数 " + str(len(equi_data)))
    replay_buffer.add_samples(equi_data)

    if replay_buffer.size() >= replay_buffer_size:
        train(replay_buffer, network, device, lr, num_epochs, batch_size)

        if i_episode % 100 == 0:
            save_network(network, f"model/net_{i_episode}.mdl")
            print(getTimeStr() + f"模型已保存 episode:{i_episode}")
        save_network(network)
        print(getTimeStr() + f"最新模型已保存 episode:{i_episode}")
