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
        self.buffer = []
        self.max_size = max_size

    def add(self, data_list):
        self.buffer.extend(data_list)
        # 超出容量时只保留最新的 max_size 条
        if len(self.buffer) > self.max_size:
            self.buffer = self.buffer[-self.max_size:]

    def sample(self, sample_size, recent_ratio=0.7):
        """采样训练数据，recent_ratio 比例来自最近 1/3 数据，其余来自全池"""
        buf_len = len(self.buffer)
        if sample_size >= buf_len:
            return list(self.buffer)

        recent_count = int(sample_size * recent_ratio)
        old_count = sample_size - recent_count

        # 最近 1/3 的数据
        recent_boundary = max(buf_len * 2 // 3, buf_len - sample_size)
        recent_pool_size = buf_len - recent_boundary
        recent_count = min(recent_count, recent_pool_size)
        old_count = sample_size - recent_count

        recent_indices = random.sample(range(recent_boundary, buf_len), recent_count)
        old_indices = random.sample(range(recent_boundary), min(old_count, recent_boundary))
        
        indices = recent_indices + old_indices
        return [self.buffer[i] for i in indices]

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
    """保存带对局计数编号的检查点模型（从 latest 模型保存）"""
    src = "model/model_latest.onnx"
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

    # 防御性检查：如果 wins+losses+draws==0，说明 evaluate 没真正跑（可能是模型加载失败、通道不匹配等）
    total = result["wins"] + result["losses"] + result["draws"]
    if total == 0:
        Logger.infoD(f"⚠️ Evaluate 异常：wins+losses+draws=0，疑似模型加载或对弈失败")
        result["valid"] = False
    else:
        result["valid"] = True

    return result


def run_generate_openings(cpp_path, model_path, threshold=1.0, max_attempts=80000):
    """调用 C++ generate_openings 模式生成平衡开局库

    threshold: 实际通过阈值 = threshold * 0.2（C++ 端 thresholds[3] 设计）。
               threshold=1.0 → |v| < 0.2（默认放宽，2ch 网络早期通过率提高）
               threshold=0.5 → |v| < 0.1（严格平衡）
               threshold=5.0 → |v| < 1.0（g0 全随机网络兜底）
    """
    model_path = os.path.abspath(model_path)
    cpp_dir = os.path.dirname(os.path.abspath(cpp_path))
    conf_path = os.path.join(cpp_dir, "application.conf")

    # 备份原配置
    backup_conf = conf_path + ".bak"
    if os.path.exists(conf_path):
        shutil.copy2(conf_path, backup_conf)

    with open(conf_path, 'w') as f:
        f.write(f"mode=generate_openings\n")
        f.write(f"coreType={ConfigReader.get('coreType')}\n")  # 跟随训练配置（macOS 用 apple CoreML 加速）
        f.write(f"boardSize={ConfigReader.get('boardSize')}\n")
        f.write(f"modelPath={model_path}\n")
        f.write(f"genOpenings_trainCount=300\n")
        f.write(f"genOpenings_evalCount=50\n")
        f.write(f"genOpenings_minMoves=1\n")
        f.write(f"genOpenings_maxMoves=4\n")
        f.write(f"genOpenings_threshold={threshold}\n")
        f.write(f"genOpenings_maxAttempts={max_attempts}\n")
        f.write(f"genOpenings_nearCenter=9\n")

    env = os.environ.copy()
    env['DYLD_LIBRARY_PATH'] = os.path.join(os.path.dirname(os.path.abspath(cpp_path)), '..', 'onnxruntime', 'lib')
    process = subprocess.Popen([os.path.abspath(cpp_path)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               cwd=cpp_dir, env=env)
    openings_output = []
    for line in process.stdout:
        decoded = line.decode()
        print(decoded, end='')
        if '[Openings]' in decoded:
            openings_output.append(decoded.strip())
    process.wait()

    # 恢复原配置
    if os.path.exists(backup_conf):
        shutil.copy2(backup_conf, conf_path)
        os.remove(backup_conf)

    # 把生成的开局文件复制到 train 目录和自对弈目录
    src_train = os.path.join(cpp_dir, "openings", "openings_train.txt")
    src_eval = os.path.join(cpp_dir, "openings", "openings_eval.txt")

    # 统计生成结果并写日志
    train_count = sum(1 for _ in open(src_train)) if os.path.exists(src_train) and os.path.getsize(src_train) > 0 else 0
    eval_count = sum(1 for _ in open(src_eval)) if os.path.exists(src_eval) and os.path.getsize(src_eval) > 0 else 0
    Logger.infoD(f"开局生成完成: train={train_count}, eval={eval_count}", "openings.log")
    # 把 C++ 端的 [Openings] 所有详细日志逐行落到 openings.log（含尝试次数 / 通过率 / 各档 / 步数桶细分）
    for line in openings_output:
        Logger.infoD(line, "openings.log")

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


def run_vct_labeling(cpp_path):
    """[已移除] VCT 离线标注机制已废弃"""
    return True


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
    # g0 阶段网络全随机，value 噪声偏离 0 较大，用宽阈值 + 少尝试快速通过
    Logger.infoD("启动时生成平衡开局库...")
    if total_games_count < 5000:
        Logger.infoD(f"g{total_games_count} 阶段网络未成熟，开局生成放宽阈值")
        run_generate_openings(cppPathEval, latest_path, threshold=5.0, max_attempts=10000)
    else:
        run_generate_openings(cppPathEval, latest_path)

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
        total_value_loss = 0.0
        total_policy_loss = 0.0
        for epoch in range(train_epochs):
            loss, v_loss, p_loss = train(sampled_data, model, device, optimizer, batch_size, i_episode)
            total_loss += loss
            total_value_loss += v_loss
            total_policy_loss += p_loss
        avg_loss = total_loss / train_epochs
        avg_value_loss = total_value_loss / train_epochs
        avg_policy_loss = total_policy_loss / train_epochs
        Logger.infoD(f"episode {i_episode} 训练 {train_epochs} epochs, 平均 loss: {avg_loss:.4f} (value: {avg_value_loss:.4f}, policy: {avg_policy_loss:.4f})")

        save_model(model, optimizer)
        Logger.infoD(f"最新模型已保存 episode:{i_episode}")

        # 隐式备份：latest → backup（防止写入中途崩溃丢失模型）
        backup_onnx = "model/model_backup.onnx"
        backup_pt = "model/model_backup.pt"
        shutil.copy2(latest_path, backup_onnx)
        if os.path.exists(latest_path.replace('.onnx', '.pt')):
            shutil.copy2(latest_path.replace('.onnx', '.pt'), backup_pt)

        # 先更新计数，再检查是否触发评估
        total_games_count = update_count(numGames)

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

                # 无效评估（wins+losses+draws=0）→ 跳过记录与 best 更新，避免污染历史
                eval_valid = eval_result.get("valid", True)
                if not eval_valid:
                    Logger.infoD(f"⚠️ 跳过本次 Elo 评估记录（结果无效）", "elo.log")

                if eval_valid:
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

                    # Elo 上升时更新 best 模型（best = 最后一个 Elo 没下降的 checkpoint）
                    if eval_result["elo_diff"] >= 0:
                        current_onnx = f"model/checkpoint_g{current_eval_point}.onnx"
                        current_pt = f"model/checkpoint_g{current_eval_point}.pt"
                        if os.path.exists(current_onnx):
                            shutil.copy2(current_onnx, best_path)
                            best_pt = best_path.replace('.onnx', '.pt')
                            if os.path.exists(current_pt):
                                shutil.copy2(current_pt, best_pt)
                            Logger.infoD(f"✅ best 模型已更新为 g{current_eval_point}", "elo.log")

                    # 安全阀：连续 N 次 Elo 评估为负（胜率 < 50%），回滚到最后一个正 Elo 的 checkpoint
                    # 每次判定前重读 application.conf，支持运行时热调阈值
                    ConfigReader.init()
                    # 开关：eloRollbackEnabled=false 时跳过自动回退（默认 false，2026-05-31 改为默认关闭）
                    rollback_enabled = (
                        ConfigReader.get('eloRollbackEnabled').lower() == 'true'
                        if 'eloRollbackEnabled' in ConfigReader.config else False
                    )
                    if rollback_enabled:
                        rollback_threshold = int(
                            ConfigReader.get('eloRollbackThreshold')
                            if 'eloRollbackThreshold' in ConfigReader.config else 6
                        )
                        consecutive_decline = 0
                        rollback_target = None
                        for entry in reversed(elo_history):
                            if entry["elo_diff"] < 0:
                                consecutive_decline += 1
                            else:
                                rollback_target = entry["total_games"]
                                break
                        if consecutive_decline >= rollback_threshold and rollback_target is not None:
                            rollback_onnx = f"model/checkpoint_g{rollback_target}.onnx"
                            rollback_pt = f"model/checkpoint_g{rollback_target}.pt"
                            rollback_pth = f"model/checkpoint_g{rollback_target}.pth"
                            if os.path.exists(rollback_onnx):
                                Logger.infoD(
                                    f"⚠️ 安全阀触发：连续 {consecutive_decline} 次 Elo 下降，"
                                    f"回滚到 g{rollback_target}",
                                    "elo.log"
                                )
                                # 写入回退标记，前端据此标黄
                                rollback_info = {
                                    "rollback": True,
                                    "rollback_target": rollback_target
                                }
                                Logger.infoD(json.dumps(rollback_info), "elo.log")
                                shutil.copy2(rollback_onnx, latest_path)
                                shutil.copy2(rollback_onnx, best_path)
                                if os.path.exists(rollback_pt):
                                    shutil.copy2(rollback_pt, latest_path.replace('.onnx', '.pt'))
                                    shutil.copy2(rollback_pt, best_path.replace('.onnx', '.pt'))
                                if os.path.exists(rollback_pth):
                                    shutil.copy2(rollback_pth, "model/checkpoint.pth")
                                    # 重新加载模型权重
                                    model, optimizer = get_model(device, lr, wd)
                                    Logger.infoD(f"模型已回滚到 g{rollback_target}，训练继续")
                                # 同步回调 count.txt 到 rollback_target，避免后续 checkpoint 编号与权重训练历史脱节
                                # （之前的 bug：回滚权重但 count 继续增加，导致 g26100/g26200... 实际是从回滚点训出来的版本）
                                with open("model/count.txt", "w") as f:
                                    f.write(str(rollback_target))
                                total_games_count = rollback_target
                                # 删除 rollback_target 之后的 stale checkpoint，避免误用
                                import glob as _glob
                                for stale in _glob.glob("model/checkpoint_g*.onnx"):
                                    try:
                                        g = int(stale.replace("model/checkpoint_g", "").replace(".onnx", ""))
                                        if g > rollback_target:
                                            os.remove(stale)
                                            pt = stale.replace(".onnx", ".pt")
                                            if os.path.exists(pt):
                                                os.remove(pt)
                                    except Exception:
                                        pass
                                Logger.infoD(f"已删除 g>{rollback_target} 的 stale checkpoint，count.txt 已回调")
                                elo_history.clear()  # 清空历史，重新开始追踪

        # 开局库自动刷新：每 openings_refresh_interval 局用 best 模型重新生成平衡开局
        current_openings_point = (total_games_count // openings_refresh_interval) * openings_refresh_interval
        if current_openings_point > last_openings_refresh and current_openings_point > 0:
            last_openings_refresh = current_openings_point
            Logger.infoD(f"开始刷新开局库（g{total_games_count}）...")
            if total_games_count < 5000:
                run_generate_openings(cppPathEval, latest_path, threshold=5.0, max_attempts=10000)
            else:
                run_generate_openings(cppPathEval, latest_path)

        Logger.infoD(f"episode {i_episode} 完成")

        # 记录迭代信息
        episodeInfo = {
            "i_episode": i_episode,
            "loss": avg_loss,
            "value_loss": avg_value_loss,
            "policy_loss": avg_policy_loss,
            "record_count": len(extended_data),
            "buffer_size": len(replay_buffer),
            "sample_size": len(sampled_data),
            "train_epochs": train_epochs,
            "total_games_count": total_games_count,
            "speed": speed
        }
        Logger.infoD(json.dumps(episodeInfo), "episode.log")

