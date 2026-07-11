import subprocess

import numpy as np

from ConfigReader import ConfigReader


def getFileData(shard_num):
    training_data = []
    for shard in range(shard_num):
        try:
            f = open(f"record/data_{shard}.txt", "r")
            first_line = f.readline().strip()
            if not first_line:
                print(f"Warning: record/data_{shard}.txt is empty, skipping")
                f.close()
                continue
            count = int(first_line)
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
            f.close()
        except Exception as e:
            print(f"Warning: failed to read record/data_{shard}.txt: {e}, skipping")

    return training_data


def run_program(cppPath, num_shards=1, num_games=None):
    """启动 num_shards 个 C++ 子进程并行自对弈，各自写独立日志文件
    
    num_games: 总对局数（按进程数平均分摊）。None 时不覆盖，C++ 自行读配置。
    """
    import os, platform

    base_env = os.environ.copy()
    lib_dir = os.path.join(os.path.dirname(os.path.abspath(cppPath)), '..', 'onnxruntime', 'lib')
    lib_key = 'DYLD_LIBRARY_PATH' if platform.system() == 'Darwin' else 'LD_LIBRARY_PATH'
    extra_paths = lib_dir
    if platform.system() == 'Linux':
        extra_paths += ':/usr/local/cuda/lib64'
    base_env[lib_key] = extra_paths + ':' + base_env.get(lib_key, '')

    os.makedirs("log", exist_ok=True)
    os.makedirs("record", exist_ok=True)

    # 清空跨进程共享计数器（每轮自对弈开始时重置）
    with open("record/game_counter.lock", "w") as f:
        f.write("0")

    processes = []
    log_files = []
    for shard in range(num_shards):
        env = base_env.copy()
        env["SHARD_ID"] = str(shard)
        if num_games is not None:
            env["NUM_GAMES"] = str(num_games)  # 传总数，C++ 端动态抢
        lf = open(f"log/selfplay_{shard}.log", "w")
        p = subprocess.Popen([cppPath], stdout=lf, stderr=subprocess.STDOUT, env=env)
        processes.append(p)
        log_files.append(lf)

    # 等待全部完成
    return_codes = []
    for p in processes:
        p.wait()
        return_codes.append(p.returncode)
    for lf in log_files:
        lf.close()

    # 检查返回码
    all_success = all(rc == 0 for rc in return_codes)
    if all_success:
        print(f"All {num_shards} self-play processes completed successfully.")
    else:
        for i, rc in enumerate(return_codes):
            if rc != 0:
                print(f"Self-play process {i} failed with return code {rc}")

    # 返回第一个非零返回码，或 0 表示全部成功
    for rc in return_codes:
        if rc != 0:
            return rc
    return 0


if __name__ == '__main__':
    run_program(0)
    result = getFileData(1)
    print(len(result))
