import pygame

# 初始化界面
pygame.init()
width, height = 400, 400
screen = pygame.display.set_mode((width, height))
pygame.display.set_caption("连珠")

# 定义颜色
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)

# 定义棋盘大小和格子大小
board_size = 6
margin = 40  # 留边大小
grid_size = (width - 2 * margin) // (board_size - 1)  # 格子大小
stone_radius = grid_size // 2  # 棋子半径为格子大小的1/2

# 初始化棋盘
board = [[0] * board_size for _ in range(board_size)]

# 游戏主循环
running = True
player = 1  # 当前玩家，1代表黑子，2代表白子
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.MOUSEBUTTONDOWN:
            # 获取鼠标点击位置
            x, y = event.pos
            # 判断是否在边距内
            if margin < x < width - margin and margin < y < height - margin:
                # 计算在棋盘上的位置
                row = round((y - margin) / grid_size)
                col = round((x - margin) / grid_size)
                # 在棋盘上落子
                if board[row][col] == 0:
                    board[row][col] = player
                    player = 2 if player == 1 else 1  # 切换到下一个玩家

    # 绘制棋盘线条
    screen.fill(WHITE)
    for i in range(board_size):
        pygame.draw.line(screen, BLACK, (margin, i * grid_size + margin), (width - margin, i * grid_size + margin))
        pygame.draw.line(screen, BLACK, (i * grid_size + margin, margin), (i * grid_size + margin, height - margin))

    # 绘制棋子
    for row in range(board_size):
        for col in range(board_size):
            if board[row][col] == 1:
                pygame.draw.circle(screen, BLACK, (col * grid_size + margin, row * grid_size + margin), stone_radius)
            elif board[row][col] == 2:
                pygame.draw.circle(screen, WHITE, (col * grid_size + margin, row * grid_size + margin), stone_radius)
                pygame.draw.circle(screen, BLACK, (col * grid_size + margin, row * grid_size + margin), stone_radius, 1)

    pygame.display.flip()

# 退出游戏
pygame.quit()