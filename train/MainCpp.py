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


def read_persistent_int(filepath, default=0):
    """从文件读取持久化的整数值"""
    try:
        with open(filepath, 'r') as f:
            return int(f.read().strip())
    except (FileNotFoundError, ValueError):
        return default


def write_persistent_int(value, filepath):
    """将整数值持久化到文件"""
    with open(filepath, 'w') as f:
        f.write(str(value))


def save_checkpoint(total_games):
    """保存带对局计数编号的检查点模型（从 best 模型保存）"""
    src = "model/model_best.onnx"
    if not os.path.exists(src):
        src = "model/model_latest.onnx"  # 兼容首次启动
    dst = f"model/checkpoint_g{total_games}.onnx"
    if os.path.exists(src) and not os.path.exists(dst):
        shutil.copy2(src, dst)
        Logger.infoD(f"检查点已保存: {dst}")
        # 同时复制 .pt 文件（libtorch 后端需要）
        src_pt = src.replace('.onnx', '.pt')
        dst_pt = dst.replace('.onnx', '.pt')
        if os.path.exists(src_pt):
            shutil.copy2(src_pt, dst_pt)
            Logger.infoD(f"检查点已保存: {dst_pt}")
    elif os.path.exists(dst):
        Logger.infoD(f"检查点已存在，跳过: {dst}")
    return dst


def find_latest_checkpoint(current_games, eval_games_interval):
    """找到上一个检查点模型"""
    prev_games = current_games - eval_games_interval
    while prev_games >= 0:
        path = f"model/checkpoint_g{prev_games}.onnx"
        if os.path.exists(path):
            return path, prev_games
        prev_games -= eval_games_interval
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

    # 运行 evaluate（在 C++ 目录下执行，确保读到 evaluate 配置）
    env = os.environ.copy()
    env['DYLD_LIBRARY_PATH'] = os.path.join(os.path.dirname(os.path.abspath(cpp_path)), '..', 'onnxruntime', 'lib')
    process = subprocess.Popen([os.path.abspath(cpp_path)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               cwd=cpp_dir, env=env)
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
                if "Wins:" in part:
                    result["wins"] = int(part.split("Wins:")[1].strip())
                elif "Losses:" in part:
                    result["losses"] = int(part.split("Losses:")[1].strip())
                elif "Draws:" in part:
                    result["draws"] = int(part.split("Draws:")[1].strip())

    return result


def run_generate_openings(cpp_path, model_path):
    """调用 C++ generate_openings 模式生成平衡开局库"""
    model_path = os.path.abspath(model_path)
    cpp_dir = os.path.dirname(os.path.abspath(cpp_path))
    conf_path = os.path.join(cpp_dir, "application.conf")

    # 备份原配置
    backup_conf = conf_path + ".bak"
    if os.path.exists(conf_path):
        shutil.copy2(conf_path, backup_conf)

    with open(conf_path, 'w') as f:
        f.write(f"mode=generate_openings\n")
        f.write(f"coreType=cpu\n")  # 开局生成用 CPU 即可，无需 GPU/MPS
        f.write(f"boardSize={ConfigReader.get('boardSize')}\n")
        f.write(f"modelPath={model_path}\n")
        f.write(f"genOpenings_trainCount=300\n")
        f.write(f"genOpenings_evalCount=50\n")
        f.write(f"genOpenings_minMoves=1\n")
        f.write(f"genOpenings_maxMoves=8\n")
        f.write(f"genOpenings_threshold=0.4\n")
        f.write(f"genOpenings_maxAttempts=15000\n")
        f.write(f"genOpenings_nearCenter=6\n")

    env = os.environ.copy()
    env['DYLD_LIBRARY_PATH'] = os.path.join(os.path.dirname(os.path.abspath(cpp_path)), '..', 'onnxruntime', 'lib')
    process = subprocess.Popen([os.path.abspath(cpp_path)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               cwd=cpp_dir, env=env)
    for line in process.stdout:
        decoded = line.decode()
        print(decoded, end='')
    process.wait()

    # 恢复原配置
    if os.path.exists(backup_conf):
        shutil.copy2(backup_conf, conf_path)
        os.remove(backup_conf)

    # 把生成的开局文件复制到 train 目录和自对弈目录
    src_train = os.path.join(cpp_dir, "openings", "openings_train.txt")
    src_eval = os.path.join(cpp_dir, "openings", "openings_eval.txt")

    # 目标目录列表：train/openings + 自对弈 C++ 目录（如果与 eval 不同）
    dst_dirs = ["openings"]
    cpp_path_train = ConfigReader.get("cppPath")
    train_cpp_dir = os.path.dirname(os.path.abspath(cpp_path_train))
    if os.path.normpath(train_cpp_dir) != os.path.normpath(cpp_dir):
        dst_dirs.append(os.path.join(train_cpp_dir, "openings"))

    for dst_dir in dst_dirs:
        os.makedirs(dst_dir, exist_ok=True)
        if os.path.exists(src_train):
            shutil.copy2(src_train, os.path.join(dst_dir, "openings_train.txt"))
        if os.path.exists(src_eval):
            shutil.copy2(src_eval, os.path.join(dst_dir, "openings_eval.txt"))

    return process.returncode == 0


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
    cppPathEval = ConfigReader.get("cppPathEval") if 'cppPathEval' in ConfigReader.config else cppPath
    train_epochs = int(ConfigReader.get('trainEpochs') if 'trainEpochs' in ConfigReader.config else 3)
    replay_buffer_size = int(
        ConfigReader.get('replayBufferSize') if 'replayBufferSize' in ConfigReader.config else 500000)
    eval_interval = int(ConfigReader.get('evalInterval') if 'evalInterval' in ConfigReader.config else 5000)
    eval_games = int(ConfigReader.get('evalGames') if 'evalGames' in ConfigReader.config else 40)
    eval_simulation = int(ConfigReader.get('evalSimulation') if 'evalSimulation' in ConfigReader.config else 100)
    arena_interval = int(ConfigReader.get('arenaInterval') if 'arenaInterval' in ConfigReader.config else 500)
    arena_games = int(ConfigReader.get('arenaGames') if 'arenaGames' in ConfigReader.config else 40)
    arena_simulation = int(ConfigReader.get('arenaSimulation') if 'arenaSimulation' in ConfigReader.config else 100)
    arena_threshold = float(ConfigReader.get('arenaWinRateThreshold') if 'arenaWinRateThreshold' in ConfigReader.config else 0.55)
    arena_max_skip = int(ConfigReader.get('arenaMaxSkip') if 'arenaMaxSkip' in ConfigReader.config else 10)

    total_games_count = update_count(0)

    # 模型初始化
    device = getDevice()
    model, optimizer = get_model(device, lr, wd)

    save_model(model, optimizer)

    # 初始化 best 模型：首次启动时复制 latest 为 best
    best_path = "model/model_best.onnx"
    latest_path = "model/model_latest.onnx"
    best_pt = "model/model_best.pt"
    latest_pt = "model/model_latest.pt"
    if not os.path.exists(best_path):
        shutil.copy2(latest_path, best_path)
        Logger.infoD("首次启动：model_best.onnx 从 model_latest.onnx 复制而来")
    if not os.path.exists(best_pt) and os.path.exists(latest_pt):
        shutil.copy2(latest_pt, best_pt)
        Logger.infoD("首次启动：model_best.pt 从 model_latest.pt 复制而来")

    # Arena 状态（arena_skip_count 持久化，防止进程重启丢失）
    arena_skip_count = read_persistent_int("model/arena_skip_count.txt", 0)
    last_arena_games = (total_games_count // arena_interval) * arena_interval  # 上一次 arena 的对局数

    # 保存初始检查点作为基线（仅首次训练时）
    save_checkpoint(total_games_count)

    # Elo 追踪
    elo_history = []
    last_eval_games = (total_games_count // eval_interval) * eval_interval  # 上一次评估的对局数

    # 开局库自动生成（每 2000 局刷新一次）
    openings_refresh_interval = 2000
    last_openings_refresh = (total_games_count // openings_refresh_interval) * openings_refresh_interval

    # 经验回放池
    replay_buffer = ReplayBuffer(max_size=replay_buffer_size)
    Logger.infoD(f"经验回放池已初始化，容量 {replay_buffer_size}")

    # 启动时立即生成平衡开局（确保自对弈第一局就用生成开局）
    Logger.infoD("启动时生成平衡开局库...")
    run_generate_openings(cppPathEval, best_path)

    for i_episode in range(1, episode + 1):

        start_time = time.time()

        retcode = Bridge.run_program(cppPath)
        if retcode != 0:
            Logger.infoD(f"C++ 自对弈进程异常退出 (code {retcode})，重试本 episode")
            continue

        training_data = Bridge.getFileData(num_processes)
        if not training_data:
            Logger.infoD("自对弈数据为空，跳过本 episode")
            continue

        end_time = time.time()
        Logger.infoD(f"自我对弈完毕，用时 {end_time - start_time} s")

        extended_data = get_extended_data(training_data)
        speed = round(len(extended_data) / (end_time - start_time), 1)
        Logger.infoD(f"完成扩展自我对弈数据，条数 " + str(len(extended_data)) + " , " + str(
            speed) + " 条/s")

        # 加入经验回放池
        replay_buffer.add(extended_data)
        Logger.infoD(f"回放池大小: {len(replay_buffer)}")

        # 从回放池中采样训练数据（固定 batch_size * 80 条，平衡数据利用率与训练耗时）
        sample_size = min(len(replay_buffer), batch_size * 80)
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

        # 先更新计数，再检查是否触发评估
        total_games_count = update_count(numGames)

        # Arena 门槛准入：基于对局计数触发（每 arena_interval 局触发一次）
        current_arena_point = (total_games_count // arena_interval) * arena_interval
        if current_arena_point > last_arena_games and current_arena_point > 0:
            last_arena_games = current_arena_point
            Logger.infoD(f"开始 Arena 评估: latest(g{total_games_count}) vs best (连续跳过: {arena_skip_count}, 全开局模式)")
            arena_result = run_evaluate(
                cppPathEval,
                latest_path,
                best_path,
                -1,
                arena_simulation
            )
            arena_win_rate = arena_result['win_rate']
            force_accept = arena_skip_count >= arena_max_skip
            accepted = arena_win_rate >= arena_threshold or force_accept

            reason = "通过阈值" if arena_win_rate >= arena_threshold else ("强制接受(连续跳过达上限)" if force_accept else "未通过")
            Logger.infoD(
                f"Arena [g{total_games_count}] 结果: 胜率 {arena_win_rate * 100:.1f}% ({arena_result['wins']}-{arena_result['losses']}-{arena_result['draws']}), "
                f"Elo {arena_result['elo_diff']:+.0f}, {reason}",
                "arena.log"
            )

            if accepted:
                shutil.copy2(latest_path, best_path)
                # 同时复制 .pt 文件（libtorch 后端需要）
                latest_pt = latest_path.replace('.onnx', '.pt')
                best_pt = best_path.replace('.onnx', '.pt')
                if os.path.exists(latest_pt):
                    shutil.copy2(latest_pt, best_pt)
                Logger.infoD(f"✅ g{total_games_count} 已升格为 best (胜率 {arena_win_rate * 100:.1f}%)", "arena.log")
                arena_skip_count = 0
                write_persistent_int(arena_skip_count, "model/arena_skip_count.txt")
                # 保存 pth 快照，方便回退
                pth_snapshot = f"model/checkpoint_g{total_games_count}.pth"
                shutil.copy2("model/checkpoint.pth", pth_snapshot)
                Logger.infoD(f"已保存权重快照: {pth_snapshot}")
            else:
                arena_skip_count += 1
                write_persistent_int(arena_skip_count, "model/arena_skip_count.txt")
                Logger.infoD(f"❌ 新模型被拒绝 (胜率 {arena_win_rate * 100:.1f}% < {arena_threshold * 100:.0f}%)", "arena.log")

        # Elo 评估（基于全局对局计数）
        current_eval_point = (total_games_count // eval_interval) * eval_interval
        if current_eval_point > last_eval_games and current_eval_point > 0:
            last_eval_games = current_eval_point
            save_checkpoint(current_eval_point)
            baseline_path, baseline_games = find_latest_checkpoint(current_eval_point, eval_interval)
            if baseline_path:
                current_path = f"model/checkpoint_g{current_eval_point}.onnx"
                Logger.infoD(f"开始 Elo 评估: g{current_eval_point} vs g{baseline_games} (全开局模式)")
                eval_result = run_evaluate(
                    cppPathEval,
                    current_path,
                    baseline_path,
                    -1,
                    eval_simulation
                )
                elo_history.append({
                    "total_games": current_eval_point,
                    "vs_games": baseline_games,
                    "elo_diff": eval_result["elo_diff"],
                    "win_rate": eval_result["win_rate"],
                    "wins": eval_result["wins"],
                    "losses": eval_result["losses"],
                    "draws": eval_result["draws"]
                })
                Logger.infoD(
                    f"Elo 评估完成: g{current_eval_point} vs g{baseline_games} → "
                    f"胜率 {eval_result['win_rate'] * 100:.1f}%, Elo {eval_result['elo_diff']:+.0f}",
                    "elo.log"
                )
                Logger.infoD(json.dumps(elo_history[-1]), "elo.log")

        # 开局库自动刷新：每 openings_refresh_interval 局用 best 模型重新生成平衡开局
        current_openings_point = (total_games_count // openings_refresh_interval) * openings_refresh_interval
        if current_openings_point > last_openings_refresh and current_openings_point > 0:
            last_openings_refresh = current_openings_point
            Logger.infoD(f"开始刷新开局库（g{total_games_count}）...")
            run_generate_openings(cppPathEval, best_path)

        Logger.infoD(f"episode {i_episode} 完成")

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

