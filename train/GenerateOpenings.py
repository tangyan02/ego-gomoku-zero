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
    train_output_path: str = "openings/openings_train.txt",
    eval_output_path: str = "openings/openings_eval.txt",
    board_size: int = 20,
    num_train_openings: int = 300,
    num_eval_openings: int = 50,
    num_moves_range: tuple = (3, 8),
    value_threshold: float = 0.4,
    max_attempts: int = 15000,
    near_center_range: int = 6,
):
    """
    生成平衡开局库。
    
    Args:
        model_path: ONNX 模型路径（通常是 model_best.onnx）
        train_output_path: 自对弈用开局文件
        eval_output_path: 评估用开局文件（与训练不重叠）
        board_size: 棋盘大小
        num_train_openings: 自对弈开局数量
        num_eval_openings: 评估开局数量
        num_moves_range: 每个开局的步数范围 (min, max)
        value_threshold: value 判定为平衡的阈值 (|value| < threshold)
        max_attempts: 最大尝试次数
        near_center_range: 开局落子范围（距中心的距离）
    """
    num_openings = num_train_openings + num_eval_openings

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

        # 双视角评估：当前玩家和对方各评一次，双方都认为接近 0 才算平衡
        current_player = 1 if len(moves) % 2 == 0 else 2
        other_player = 3 - current_player

        # 视角1：当前行棋方
        state1 = np.zeros((1, 4, board_size, board_size), dtype=np.float32)
        for r in range(board_size):
            for c in range(board_size):
                if board[r][c] == current_player:
                    state1[0, 0, r, c] = 1.0
                elif board[r][c] == other_player:
                    state1[0, 1, r, c] = 1.0
        if len(moves) >= 1:
            lr, lc = moves[-1]
            state1[0, 2, lr, lc] = 1.0
        if len(moves) >= 2:
            llr, llc = moves[-2]
            state1[0, 3, llr, llc] = 1.0

        # 视角2：对方视角（交换通道0和1）
        state2 = np.zeros((1, 4, board_size, board_size), dtype=np.float32)
        for r in range(board_size):
            for c in range(board_size):
                if board[r][c] == other_player:
                    state2[0, 0, r, c] = 1.0
                elif board[r][c] == current_player:
                    state2[0, 1, r, c] = 1.0
        if len(moves) >= 1:
            state2[0, 2, lr, lc] = 1.0
        if len(moves) >= 2:
            state2[0, 3, llr, llc] = 1.0

        # 双视角推理
        value1, _ = sess.run(None, {'input': state1})
        value2, _ = sess.run(None, {'input': state2})
        v1 = float(value1[0][0])  # 当前行棋方视角
        v2 = float(value2[0][0])  # 对方视角

        # 平衡判定：双视角绝对值之和 < 阈值
        # 如果真平衡，双方都接近 0，之和很小
        # 如果一方觉得优（v1=0.3），另一方也觉得自己劣（v2=-0.3），和=0 但不平衡
        # 所以用绝对值之和，要求双方各自都觉得接近均势
        balance_score = abs(v1) + abs(v2)
        if balance_score < value_threshold * 2:
            # 转为相对中心的坐标格式
            relative_moves = [(r - center, c - center) for r, c in moves]
            balanced.append(relative_moves)

    if not balanced:
        Logger.infoD(f"[Openings] 生成失败：{max_attempts} 次尝试中没有找到平衡开局")
        return False

    # 分割为训练集和评估集（不重叠）
    random.shuffle(balanced)
    train_openings = balanced[:num_train_openings]
    eval_openings = balanced[num_train_openings:num_train_openings + num_eval_openings]

    def write_openings(path, openings):
        os.makedirs(os.path.dirname(path) if os.path.dirname(path) else '.', exist_ok=True)
        with open(path, 'w') as f:
            for moves in openings:
                parts = [f"{r},{c}" for r, c in moves]
                f.write(",".join(parts) + "\n")

    write_openings(train_output_path, train_openings)
    write_openings(eval_output_path, eval_openings)

    Logger.infoD(
        f"[Openings] 生成 {len(train_openings)} 训练 + {len(eval_openings)} 评估开局"
        f"（尝试 {attempts} 次，通过率 {len(balanced)/attempts*100:.1f}%），"
        f"阈值 |value|<{value_threshold}",
        "openings.log"
    )
    return True


if __name__ == "__main__":
    # 独立运行测试
    generate_balanced_openings(
        model_path="model/model_best.onnx",
    )
