"""
自动生成平衡开局库
使用当前 best 模型的 value head 评估局面，保留 value ≈ 0 的开局。
"""

import os
import random
import numpy as np
import onnxruntime as ort
import Logger


def generate_balanced_openings(
    model_path: str,
    output_path: str = "openings/openings.txt",
    board_size: int = 20,
    num_openings: int = 200,
    num_moves_range: tuple = (3, 8),
    value_threshold: float = 0.15,
    max_attempts: int = 5000,
    near_center_range: int = 6,
):
    """
    生成平衡开局库。
    
    Args:
        model_path: ONNX 模型路径（通常是 model_best.onnx）
        output_path: 输出开局文件路径
        board_size: 棋盘大小
        num_openings: 目标生成开局数量
        num_moves_range: 每个开局的步数范围 (min, max)
        value_threshold: value 判定为平衡的阈值 (|value| < threshold)
        max_attempts: 最大尝试次数
        near_center_range: 开局落子范围（距中心的距离）
    """
    if not os.path.exists(model_path):
        Logger.infoD(f"[Openings] 模型不存在: {model_path}，跳过开局生成")
        return False

    # 加载 ONNX 模型
    sess = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])

    center = board_size // 2
    balanced = []
    attempts = 0

    while len(balanced) < num_openings and attempts < max_attempts:
        attempts += 1

        # 随机步数
        num_moves = random.randint(num_moves_range[0], num_moves_range[1])

        # 在中心附近随机生成落子序列
        board = np.zeros((board_size, board_size), dtype=int)
        moves = []
        valid = True

        for step in range(num_moves):
            # 在中心附近随机选空位
            candidates = []
            for r in range(max(0, center - near_center_range), min(board_size, center + near_center_range + 1)):
                for c in range(max(0, center - near_center_range), min(board_size, center + near_center_range + 1)):
                    if board[r][c] == 0:
                        candidates.append((r, c))

            if not candidates:
                valid = False
                break

            r, c = random.choice(candidates)
            player = 1 if step % 2 == 0 else 2  # 1=黑, 2=白
            board[r][c] = player
            moves.append((r, c))

        if not valid or len(moves) < num_moves_range[0]:
            continue

        # 构造 state tensor [4, board_size, board_size]
        # 当前玩家视角：下一步该谁走
        current_player = 1 if len(moves) % 2 == 0 else 2
        other_player = 3 - current_player
        state = np.zeros((1, 4, board_size, board_size), dtype=np.float32)
        for r in range(board_size):
            for c in range(board_size):
                if board[r][c] == current_player:
                    state[0, 0, r, c] = 1.0
                elif board[r][c] == other_player:
                    state[0, 1, r, c] = 1.0
        # 通道 2/3: 最近落子
        if len(moves) >= 1:
            lr, lc = moves[-1]
            state[0, 2, lr, lc] = 1.0
        if len(moves) >= 2:
            llr, llc = moves[-2]
            state[0, 3, llr, llc] = 1.0

        # 推理
        value, _ = sess.run(None, {'input': state})
        v = float(value[0][0])

        if abs(v) < value_threshold:
            # 转为相对中心的坐标格式
            relative_moves = [(r - center, c - center) for r, c in moves]
            balanced.append(relative_moves)

    if not balanced:
        Logger.infoD(f"[Openings] 生成失败：{max_attempts} 次尝试中没有找到平衡开局")
        return False

    # 写入文件
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    # 备份原文件
    if os.path.exists(output_path):
        backup_path = output_path + ".bak"
        import shutil
        shutil.copy2(output_path, backup_path)

    with open(output_path, 'w') as f:
        for moves in balanced:
            parts = []
            for r, c in moves:
                parts.append(f"{r},{c}")
            f.write(",".join(parts) + "\n")

    Logger.infoD(
        f"[Openings] 生成 {len(balanced)} 个平衡开局（尝试 {attempts} 次，通过率 {len(balanced)/attempts*100:.1f}%），"
        f"阈值 |value|<{value_threshold}",
        "openings.log"
    )
    return True


if __name__ == "__main__":
    # 独立运行测试
    generate_balanced_openings(
        model_path="model/model_best.onnx",
        output_path="openings/openings.txt",
        num_openings=200,
    )
