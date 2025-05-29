import subprocess

import numpy as np

from ConfigReader import ConfigReader


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


def run_program(cppPath):
    # 执行可执行程序，传入参数shard
    process = subprocess.Popen([cppPath], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    # 持续打印输出
    for line in process.stdout:
        print(line.decode(), end='')

    # 等待命令执行完成
    process.wait()

    # 检查命令的返回码
    if process.returncode == 0:
        print(f"Command execution successful.")
    else:
        print(f"Command execution failed with return code {process.returncode}")


if __name__ == '__main__':
    run_program(0)
    result = getFileData(1)
    print(len(result))
