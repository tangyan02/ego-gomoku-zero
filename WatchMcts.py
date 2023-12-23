import os

import numpy as np
import pygame

# 初始化界面
import torch

from Game import FourInARowGame
from MTCS import MonteCarloTree
from PolicyValueNetwork import PolicyValueNetwork
from Utils import getDevice


# 创建消息框
def show_message_box(message, width, height):
    # 创建文字对象
    text = font.render(message, True, BLACK)
    # 获取文字对象的矩形
    text_rect = text.get_rect()
    # 设置文字矩形的位置为窗口中央
    text_rect.center = (width // 2, height // 2)
    # 绘制背景矩形
    pygame.draw.rect(screen, WHITE, (text_rect.x - 10, text_rect.y - 10, text_rect.width + 20, text_rect.height + 20))
    # 在屏幕上绘制文字
    screen.blit(text, text_rect)


def getProbs(mtsc, game):
    actions, prior_prob = mtsc.get_action_probabilities()
    print(prior_prob)
    prior_probs = prior_prob.view().reshape(game.board_size, game.board_size)
    max_index = np.argmax(prior_probs)
    max_x = max_index // game.board_size
    max_y = max_index % game.board_size
    return prior_probs, (max_x, max_y)


pygame.init()
width, height = 400, 400
screen = pygame.display.set_mode((width, height))
pygame.display.set_caption("连珠")

# 定义颜色
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
RED = (255, 0, 0)
YELLOW = (128, 128, 0)

# 定义棋盘大小和格子大小
game = FourInARowGame()  # 初始化四子连珠游戏
margin = width / game.board_size / 2  # 留边大小
grid_size = (width - 2 * margin) // (game.board_size - 1)  # 格子大小
stone_radius = grid_size // 2  # 棋子半径为格子大小的1/2

font = pygame.font.Font(None, 24)

device = getDevice()
network = PolicyValueNetwork()
network.to(device)
mtsc = MonteCarloTree(network, device)
if os.path.exists(f"model/net_latest.mdl"):
    network.load_state_dict(torch.load(f"model/net_latest.mdl", map_location=torch.device(device)))

# 游戏主循环
running = True

while running:
    mtsc.search(game.copy(), 100, False)
    prior_probs, action = getProbs(mtsc, game)
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.MOUSEBUTTONDOWN:
            # 获取鼠标点击位置
            x, y = event.pos
            # 计算在棋盘上的位置
            row = round((y - margin) / grid_size)
            col = round((x - margin) / grid_size)
            # 在棋盘上落子
            if game.board[row][col] == 0:
                game.make_move((row, col))

    # 绘制棋盘线条
    screen.fill(WHITE)
    for i in range(game.board_size):
        pygame.draw.line(screen, BLACK, (margin, i * grid_size + margin), (width - margin, i * grid_size + margin))
        pygame.draw.line(screen, BLACK, (i * grid_size + margin, margin), (i * grid_size + margin, height - margin))

    # 绘制棋子
    for row in range(game.board_size):
        for col in range(game.board_size):
            if game.board[row][col] == 1:
                pygame.draw.circle(screen, BLACK, (col * grid_size + margin, row * grid_size + margin), stone_radius)
            elif game.board[row][col] == 2:
                pygame.draw.circle(screen, WHITE, (col * grid_size + margin, row * grid_size + margin), stone_radius)
                pygame.draw.circle(screen, BLACK, (col * grid_size + margin, row * grid_size + margin), stone_radius, 1)

    # 绘制概率
    for row in range(game.board_size):
        for col in range(game.board_size):
            # if game.board[row][col] == 0:
            # 创建文字对象
            color = RED
            if row == action[0] and col == action[1]:
                color = YELLOW
            text = font.render(str(round(prior_probs[row][col], 2)), True, color)
            # 获取文字对象的矩形
            text_rect = text.get_rect()
            # 设置文字矩形的位置
            text_rect.center = (col * grid_size + margin, row * grid_size + margin)
            # 在屏幕上绘制文字
            screen.blit(text, text_rect)

    if game.check_winner() != 0:
        show_message_box(f"winner is player {game.get_other_player()}", width, height)
    pygame.display.flip()

# 退出游戏
pygame.quit()
