import json
import os
import random
import shutil
import subprocess
import time
from collections import deque

import numpy as np

import Bridge
import Logger as Logger
from Network import get_model, save_model
from Train import train
from Utils import getDevice, dirPreBuild

import sys

from ConfigReader import ConfigReader

sys.stdout = sys.__stdout__
sys.stdout.reconfigure(line_buffering=True)  # 强制行缓冲


class ReplayBuffer:
    """经验回放池，保留最近 max_size 条训练数据"""

    def __init__(self, max_size=500000):
        self.buffer = deque(maxlen=max_size)

    def add(self, data_list):
        self.buffer.extend(data_list)

    def sample(self, sample_size):
        if sample_size >= len(self.buffer):
            return list(self.buffer)
        return random.sample(list(self.buffer), sample_size)

    def __len__(self):
        return len(self.buffer)


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


def update_count(k, filepath="model/count.txt"):
    try:
        with open(filepath, 'r') as f:
            count = int(f.read())
    except FileNotFoundError:
        count = 0

    count += k

    with open(filepath, 'w') as f:
        f.write(str(count))

    Logger.infoD(f"更新对局计数，当前完成对局 {count}")
    return count


def save_checkpoint(episode):
    """保存带编号的检查点 ONNX 模型"""
    src = "model/model_latest.onnx"
    dst = f"model/checkpoint_ep{episode}.onnx"
    if os.path.exists(src):
        shutil.copy2(src, dst)
        Logger.infoD(f"检查点已保存: {dst}")
    return dst


def find_latest_checkpoint(current_episode, eval_interval):
    """找到上一个检查点模型"""
    prev_ep = current_episode - eval_interval
    while prev_ep >= 0:
        path = f"model/checkpoint_ep{prev_ep}.onnx"
        if os.path.exists(path):
            return path, prev_ep
        prev_ep -= eval_interval
    return None, 0


def run_evaluate(cpp_path, model_path1, model_path2, eval_games, eval_simulation):
    """调用 C++ evaluate 模式对弈，解析输出结果"""
    # 模型路径转绝对路径
    model_path1 = os.path.abspath(model_path1)
    model_path2 = os.path.abspath(model_path2)

    # 写临时配置到 C++ 可执行文件目录
    cpp_dir = os.path.dirname(os.path.abspath(cpp_path))
    conf_path = os.path.join(cpp_dir, "application.conf")

    # 备份原配置
    backup_conf = conf_path + ".bak"
    if os.path.exists(conf_path):
        shutil.copy2(conf_path, backup_conf)

    with open(conf_path, 'w') as f:
        f.write(f"mode=evaluate\n")
        f.write(f"coreType={ConfigReader.get('coreType')}\n")
        f.write(f"boardSize={ConfigReader.get('boardSize')}\n")
        f.write(f"explorationFactor={ConfigReader.get('explorationFactor')}\n")
        f.write(f"evalModelPath1={model_path1}\n")
        f.write(f"evalModelPath2={model_path2}\n")
        f.write(f"evalGames={eval_games}\n")
        f.write(f"evalSimulation={eval_simulation}\n")

    # 运行 evaluate
    process = subprocess.Popen([cpp_path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output_lines = []
    for line in process.stdout:
        decoded = line.decode()
        print(decoded, end='')
        output_lines.append(decoded)
    process.wait()

    # 恢复训练模式配置
    if os.path.exists(backup_conf):
        shutil.copy2(backup_conf, conf_path)
        os.remove(backup_conf)

    # 解析结果
    result = {"wins": 0, "losses": 0, "draws": 0, "win_rate": 0.0, "elo_diff": 0.0}
    for line in output_lines:
        if "Win rate:" in line:
            result["win_rate"] = float(line.split("Win rate:")[1].strip().replace("%", "")) / 100
        elif "Elo diff:" in line:
            elo_str = line.split("Elo diff:")[1].strip()
            result["elo_diff"] = float(elo_str)
        elif "Wins:" in line and "Losses:" in line:
            parts = line.split("|")
            for part in parts:
                part = part.strip()
                if part.startswith("Wins:"):
                    result["wins"] = int(part.split(":")[1].strip())
                elif part.startswith("Losses:"):
                    result["losses"] = int(part.split(":")[1].strip())
                elif part.startswith("Draws:"):
                    result["draws"] = int(part.split(":")[1].strip())

    return result


if __name__ == "__main__":

    dirPreBuild()

    ConfigReader.init()

    board_size = int(ConfigReader.get('boardSize'))
    num_processes = int(ConfigReader.get('numProcesses'))
    lr = float(ConfigReader.get('lr'))
    wd = float(ConfigReader.get('wd'))
    episode = int(ConfigReader.get('episode'))
    batch_size = int(ConfigReader.get('batchSize'))
    numGames = int(ConfigReader.get('numGames'))
    cppPath = ConfigReader.get("cppPath")
    train_epochs = int(ConfigReader.get('trainEpochs') if 'trainEpochs' in ConfigReader.config else 3)
    replay_buffer_size = int(
        ConfigReader.get('replayBufferSize') if 'replayBufferSize' in ConfigReader.config else 500000)
    eval_interval = int(ConfigReader.get('evalInterval') if 'evalInterval' in ConfigReader.config else 50)
    eval_games = int(ConfigReader.get('evalGames') if 'evalGames' in ConfigReader.config else 40)
    eval_simulation = int(ConfigReader.get('evalSimulation') if 'evalSimulation' in ConfigReader.config else 100)

    total_games_count = update_count(0)

    # 模型初始化
    device = getDevice()
    model, optimizer = get_model(device, lr, wd)

    save_model(model, optimizer)

    # 保存初始检查点作为基线
    save_checkpoint(0)

    # Elo 追踪
    elo_history = []

    # 经验回放池
    replay_buffer = ReplayBuffer(max_size=replay_buffer_size)
    Logger.infoD(f"经验回放池已初始化，容量 {replay_buffer_size}")

    for i_episode in range(1, episode + 1):

        start_time = time.time()

        Bridge.run_program(cppPath)

        training_data = Bridge.getFileData(num_processes)

        end_time = time.time()
        Logger.infoD(f"自我对弈完毕，用时 {end_time - start_time} s")

        extended_data = get_extended_data(training_data)
        speed = round(len(extended_data) / (end_time - start_time), 1)
        Logger.infoD(f"完成扩展自我对弈数据，条数 " + str(len(extended_data)) + " , " + str(
            speed) + " 条/s")

        # 加入经验回放池
        replay_buffer.add(extended_data)
        Logger.infoD(f"回放池大小: {len(replay_buffer)}")

        # 从回放池中采样训练数据
        sample_size = min(len(replay_buffer), max(len(extended_data) * 2, batch_size * 20))
        sampled_data = replay_buffer.sample(sample_size)
        Logger.infoD(f"本轮采样 {len(sampled_data)} 条数据进行训练")

        # 多 epoch 训练
        total_loss = 0.0
        for epoch in range(train_epochs):
            loss = train(sampled_data, model, device, optimizer, batch_size, i_episode)
            total_loss += loss
        avg_loss = total_loss / train_epochs
        Logger.infoD(f"episode {i_episode} 训练 {train_epochs} epochs, 平均 loss: {avg_loss:.4f}")

        save_model(model, optimizer)
        Logger.infoD(f"最新模型已保存 episode:{i_episode}")

        # Elo 评估
        if i_episode % eval_interval == 0:
            save_checkpoint(i_episode)
            baseline_path, baseline_ep = find_latest_checkpoint(i_episode, eval_interval)
            if baseline_path:
                Logger.infoD(f"开始 Elo 评估: ep{i_episode} vs ep{baseline_ep}")
                eval_result = run_evaluate(
                    cppPath,
                    f"model/checkpoint_ep{i_episode}.onnx",
                    baseline_path,
                    eval_games,
                    eval_simulation
                )
                elo_history.append({
                    "episode": i_episode,
                    "vs_episode": baseline_ep,
                    "elo_diff": eval_result["elo_diff"],
                    "win_rate": eval_result["win_rate"],
                    "wins": eval_result["wins"],
                    "losses": eval_result["losses"],
                    "draws": eval_result["draws"]
                })
                Logger.infoD(
                    f"Elo 评估完成: ep{i_episode} vs ep{baseline_ep} → "
                    f"胜率 {eval_result['win_rate'] * 100:.1f}%, Elo {eval_result['elo_diff']:+.0f}",
                    "elo.log"
                )
                Logger.infoD(json.dumps(elo_history[-1]), "elo.log")

        Logger.infoD(f"episode {i_episode} 完成")

        # 更新计数
        total_games_count = update_count(numGames)

        # 记录迭代信息
        episodeInfo = {
            "i_episode": i_episode,
            "loss": avg_loss,
            "record_count": len(extended_data),
            "buffer_size": len(replay_buffer),
            "sample_size": len(sampled_data),
            "train_epochs": train_epochs,
            "total_games_count": total_games_count,
            "speed": speed
        }
        Logger.infoD(json.dumps(episodeInfo), "episode.log")

