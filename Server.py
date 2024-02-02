import concurrent.futures
import subprocess

import numpy as np
from flask import Flask, request, Response
import pickle

# 创建一个服务
app = Flask(__name__)


def getFileData(shard_num, part_num):
    training_data = []
    for shard in range(shard_num):
        for part in range(part_num):
            f = open(f"record/data_{shard}_{part}.txt", "r")
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


def run_program(shard, part_num):
    # 执行可执行程序，传入参数shard
    process = subprocess.Popen(['./build/ego-gomoku-zero', str(shard), str(part_num)], stdout=subprocess.PIPE,
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


def selfPlayInCpp(shard_num, part_num, worker_num):
    with concurrent.futures.ThreadPoolExecutor(max_workers=worker_num) as executor:
        # 提交任务给线程池
        futures = [executor.submit(run_program, shard, part_num) for shard in range(shard_num)]
        # 等待所有任务完成
        concurrent.futures.wait(futures)


@app.route(rule='/play', methods=['GET'])
def play():
    shard_num = int(request.args.get("shard_num"))
    part_num = int(request.args.get("part_num"))
    worker_num = int(request.args.get("worker_num"))
    print(f"请求参数: shard_num:{shard_num} part_num:{part_num} worker_num:{worker_num}")
    selfPlayInCpp(shard_num, part_num, worker_num)
    print(f"本轮自我对弈结束")

    training_data = getFileData(shard_num, part_num)

    data_pickle = pickle.dumps(training_data)
    return Response(data_pickle, mimetype='application/octet-stream')


@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' in request.files:
        file = request.files['file']
        file.save('model/agent_model.pt')
        return 'File uploaded successfully'
    return 'No file part in the request'


if __name__ == '__main__':
    # 启动服务 指定主机和端口
    app.run(host='0.0.0.0', port=8888, threaded=True)
    print('server is running...')
