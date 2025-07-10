import threading

import pygame

BOARD_COLOR = (204, 153, 102)
LINE_COLOR = (0, 0, 0)
BLACK_STONE_COLOR = (0, 0, 0)
WHITE_STONE_COLOR = (255, 255, 255)
TEXT_COLOR = (0, 0, 0)  # 文字颜色


class GameUI:

    def __init__(self, board_size):
        self.board_size = board_size

        # 初始化 pygame
        pygame.init()
        # 计算窗口大小
        self.window_size = 1000
        self.cell_size = self.window_size // self.board_size
        # 创建窗口
        self.screen = pygame.display.set_mode((self.window_size, self.window_size))
        # 状态栏高度
        self.status_bar_height = 50

        # 操作区域高度
        self.operation_bar_height = 50

        # 创建窗口，包含状态栏高度
        self.screen = pygame.display.set_mode((self.window_size,
                                               self.window_size + self.status_bar_height + self.operation_bar_height * 2))
        try:
            # 尝试加载黑体字体
            self.font = pygame.font.Font('font/msyhbd.ttc', 20)
        except FileNotFoundError:
            # 如果找不到字体文件，使用默认字体
            self.font = pygame.font.Font(None, 20)

        self.next_move = None
        self.rollback = False
        self.auto = False

        pygame.display.set_caption("棋盘")

    def render_task(self, board, info_text="", probability_list=None):
        # 填充背景色
        self.screen.fill(BOARD_COLOR)

        # 计算偏移量，使棋盘从中间开始
        offset_x = (self.window_size - (self.board_size - 1) * self.cell_size) // 2
        offset_y = (self.window_size - (self.board_size - 1) * self.cell_size) // 2

        # 绘制棋盘网格
        for i in range(self.board_size):
            start_x = offset_x + i * self.cell_size
            start_y = offset_y + i * self.cell_size
            pygame.draw.line(self.screen, LINE_COLOR, (start_x, offset_y),
                             (start_x, offset_y + (self.board_size - 1) * self.cell_size), 2)
            pygame.draw.line(self.screen, LINE_COLOR, (offset_x, start_y),
                             (offset_x + (self.board_size - 1) * self.cell_size, start_y), 2)

        # 如果是9路棋盘，绘制特定小黑点
        if self.board_size == 9:
            dot_coordinates = [(2, 2), (2, 6), (6, 2), (6, 6), (4, 4)]
            for x, y in dot_coordinates:
                dot_x = offset_x + x * self.cell_size
                dot_y = offset_y + y * self.cell_size
                pygame.draw.circle(self.screen, BLACK_STONE_COLOR, (dot_x, dot_y), 7)

        # 绘制棋子
        for i in range(self.board_size):
            for j in range(self.board_size):
                if board[i][j] == 1:  # 黑子
                    # 调整坐标到网格交叉点
                    y = offset_x + i * self.cell_size
                    x = offset_y + j * self.cell_size
                    pygame.draw.circle(self.screen, BLACK_STONE_COLOR, (x, y), self.cell_size // 2 - 2)
                elif board[i][j] == 2:  # 白子
                    # 调整坐标到网格交叉点
                    y = offset_x + i * self.cell_size
                    x = offset_y + j * self.cell_size
                    pygame.draw.circle(self.screen, WHITE_STONE_COLOR, (x, y), self.cell_size // 2 - 2)

        # 绘制概率值
        if probability_list:
            for x, y, prob in probability_list:
                # 调整坐标到网格交叉点
                prob_y = offset_x + x * self.cell_size
                prob_x = offset_y + y * self.cell_size
                prob_text = f"{prob:.2f}"
                # 根据概率值调整颜色，概率越小越黄
                blue_value = int(255 * prob)
                yellow_value = 255 - blue_value
                color = (yellow_value, yellow_value, blue_value)
                prob_surface = self.font.render(prob_text, True, color)
                prob_rect = prob_surface.get_rect(center=(prob_x, prob_y))
                self.screen.blit(prob_surface, prob_rect)

        # 绘制操作区域
        pygame.draw.rect(self.screen, (180, 180, 180),
                         (0, self.window_size, self.window_size, self.operation_bar_height))

        text_surface = self.font.render("自动落子", True, TEXT_COLOR)
        text_rect = text_surface.get_rect(
            center=(self.window_size // 2
                    , self.window_size + self.operation_bar_height // 2))
        self.screen.blit(text_surface, text_rect)

        pygame.draw.rect(self.screen, (200, 200, 200),
                         (0, self.window_size + self.operation_bar_height, self.window_size, self.operation_bar_height))

        text_surface = self.font.render("回滚", True, TEXT_COLOR)
        text_rect = text_surface.get_rect(
            center=(self.window_size // 2
                    , self.window_size + self.operation_bar_height + self.operation_bar_height // 2))
        self.screen.blit(text_surface, text_rect)

        # 绘制状态栏背景
        pygame.draw.rect(self.screen, (220, 220, 220),
                         (0
                          , self.window_size + self.operation_bar_height * 2
                          , self.window_size
                          , self.status_bar_height))

        # 绘制文字信息栏
        text_surface = self.font.render(info_text, True, TEXT_COLOR)
        text_rect = text_surface.get_rect(
            center=(self.window_size // 2,
                    self.window_size + self.operation_bar_height * 2 + self.status_bar_height // 2))
        self.screen.blit(text_surface, text_rect)

        # 更新显示
        pygame.display.flip()

    def render(self, board, text="", probability_list=None):
        for event in pygame.event.get():  # pygame 退出
            if event.type == pygame.QUIT:
                return
            # 处理鼠标点击事件
            if event.type == pygame.MOUSEBUTTONDOWN:
                # 获取鼠标点击的位置
                mouse_x, mouse_y = event.pos
                # 计算偏移量，使棋盘从中间开始
                offset_x = (self.window_size - (self.board_size - 1) * self.cell_size) // 2
                offset_y = (self.window_size - (self.board_size - 1) * self.cell_size) // 2
                # 判断点击位置是否在棋盘范围内
                if offset_x <= mouse_x <= offset_x + (
                        self.board_size - 1) * self.cell_size and offset_y <= mouse_y <= offset_y + (
                        self.board_size - 1) * self.cell_size:
                    # 计算点击的格子坐标
                    # 找到离点击位置最近的网格交叉点
                    x = round((mouse_x - offset_x) / self.cell_size)
                    y = round((mouse_y - offset_y) / self.cell_size)
                    # 确保坐标在合法范围内
                    x = max(0, min(x, self.board_size - 1))
                    y = max(0, min(y, self.board_size - 1))
                    self.next_move = (y, x)

                if self.window_size < mouse_y < self.window_size + self.operation_bar_height:
                    self.auto = True

                if self.window_size + self.operation_bar_height < mouse_y < self.window_size + self.operation_bar_height * 2:
                    self.rollback = True

        threading.Thread(target=self.render_task, args=(board, text, probability_list)).start()


# 以下是使用示例
if __name__ == "__main__":
    board_size = 9
    game_ui = GameUI(board_size)
    board = [[0] * board_size for _ in range(board_size)]
    probability_list = [(2, 3, 0.2), (4, 5, 0.8)]  # 示例概率数组
    game_ui.render(board, "测试", probability_list)

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
    pygame.quit()
