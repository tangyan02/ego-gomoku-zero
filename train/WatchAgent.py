import os
import subprocess
from time import sleep

import numpy as np

from GameUI import GameUI
from train.ConfigReader import ConfigReader

board_size = 20

hint = [0, True, True]

ConfigReader.init()
cppPath = ConfigReader.get("cppPath")
visitCount = 0

# 启动C++子进程
print(cppPath)
proc = subprocess.Popen(
    [cppPath],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    text=True
)


def callInCpp(text):
    proc.stdin.write(f"{text}\n")
    proc.stdin.flush()  # 确保数据发送
    return proc.stdout.readline().strip()


def handle_predict():
    line = callInCpp("PREDICT 2")
    probs_arr = line.strip().split(" ")
    probs = []
    for str in probs_arr:
        items = str.split(",")
        x = int(items[0])
        y = int(items[1])
        prob = float(items[2])
        probs.append((x, y, prob))
    return probs


def handle_move(x, y):
    print(f"move {x},{y}")
    callInCpp(f"MOVE {x},{y}")


def handle_end_check():
    line = callInCpp(f"END_CHECK")
    return line.strip() == "true"


def handle_get_moves():
    line = callInCpp(f"GET_MOVES")
    arr = line.strip().split(" ")
    moves = []
    for str in arr:
        items = str.split(",")
        x = int(items[0])
        y = int(items[1])
        moves.append((x, y))
    return moves


def handle_current_player():
    line = callInCpp("CURRENT_PLAYER")
    return int(line.strip())


def handle_board():
    line = callInCpp("BOARD")
    arr = line.strip().split(" ")
    k = 0

    board = np.zeros((board_size, board_size), dtype=np.uint8)
    for i in range(board_size):
        for j in range(board_size):
            board[i][j] = int(arr[k])
            k += 1

    return board


def handle_winner():
    line = callInCpp("WINNER_CHECK")
    arr = line.strip().split(" ")
    winner = int(arr[0])
    return winner


def handle_rollback():
    callInCpp("ROLLBACK")


if __name__ == '__main__':
    os.environ["SDL_RENDER_DRIVER"] = "opengl"
    gameUi = GameUI(board_size=board_size)
    board = np.zeros((board_size, board_size), dtype=np.uint8)
    while True:

        if handle_end_check():
            winner = handle_winner()
            gameUi.render(board, f"胜利玩家 {winner} ")
            sleep(10000)

        moves = handle_get_moves()
        if len(moves) == 1 and moves[0][0] == -1:
            handle_move(moves[0][0], moves[0][1])
            continue

        while True:
            if gameUi.rollback:
                handle_rollback()
                gameUi.rollback = False
                visitCount = 0
                break

            if gameUi.next_move is not None:
                if board[gameUi.next_move[0]][gameUi.next_move[1]] == 0:
                    handle_move(gameUi.next_move[0], gameUi.next_move[1])
                gameUi.next_move = None
                visitCount = 0
                break

            probs = None
            if visitCount < 10000:
                if hint[handle_current_player()]:
                    visitCount += 1
                    probs = handle_predict()

            if gameUi.auto:
                max_move = max(probs, key=lambda x: x[2])
                handle_move(max_move[0], max_move[1])
                gameUi.auto = False
                visitCount = 0
                break

            board = handle_board()
            gameUi.render(board,
                          f"模拟次数: {visitCount}",
                          probs)
