import numpy as np


class FourInARowGame:
    def __init__(self, board_size=6, connect=4):
        self.board_size = board_size
        self.connect = connect
        self.board = np.zeros((board_size, board_size), dtype=int)
        self.board_last = np.zeros((board_size, board_size), dtype=int)
        self.current_player = 1

    def get_other_player(self):
        return 3 - self.current_player

    def get_action_index(self, action):
        return action[0] * self.board_size + action[1]

    def parse_action_from_index(self, action_idx):
        return (action_idx // self.board_size, action_idx % self.board_size)

    def exchange_color(self):
        self.board = np.where(self.board == 0, 0, 3 - self.board)
        self.current_player = 3 - self.current_player

    def get_valid_actions(self):
        valid_actions = []
        for row in range(self.board_size):
            for col in range(self.board_size):
                if self.board[row][col] == 0:
                    valid_actions.append((row, col))

        return valid_actions

    def make_move(self, action):
        row, col = action
        if self.board[row][col] == 0:
            self.board_last = self.board.copy()
            self.board[row][col] = self.current_player
            self.current_player = 3 - self.current_player  # 切换玩家

    def check_winner(self):
        # 检查行
        for row in range(self.board_size):
            for col in range(self.board_size - self.connect + 1):
                if self.board[row][col] != 0:
                    if np.all(self.board[row][col:col + self.connect] == self.board[row][col]):
                        return self.board[row][col]

        # 检查列
        for col in range(self.board_size):
            for row in range(self.board_size - self.connect + 1):
                if self.board[row][col] != 0:
                    if np.all(self.board[row:row + self.connect, col] == self.board[row][col]):
                        return self.board[row][col]

        # 检查左上到右下的对角线
        for row in range(self.board_size - self.connect + 1):
            for col in range(self.board_size - self.connect + 1):
                if self.board[row][col] != 0:
                    if np.all(np.diag(self.board[row:row + self.connect, col:col + self.connect]) == self.board[row][
                        col]):
                        return self.board[row][col]

        # 检查右上到左下的对角线
        for row in range(self.board_size - self.connect + 1):
            for col in range(self.connect - 1, self.board_size):
                if self.board[row][col] != 0:
                    if np.all(np.diag(np.fliplr(self.board[row:row + self.connect, col - self.connect + 1:col + 1])) ==
                              self.board[row][col]):
                        return self.board[row][col]

        return 0  # 没有胜者

    def is_game_over(self):
        return len(self.get_valid_actions()) == 0 or self.check_winner() != 0

    def get_board(self):
        return self.board.copy()

    def get_state(self):
        board1 = np.where(self.board == 1, 1, 0)
        board2 = np.where(self.board == 2, 1, 0)
        board3 = np.where(self.board_last == 1, 1, 0)
        board4 = np.where(self.board_last == 2, 1, 0)
        current_player = np.full_like(self.board, fill_value=1 if self.current_player == 1 else 0)
        return np.stack([board1, board2, board3, board4, current_player], axis=0)

    def print_board(self):
        for row in range(self.board_size):
            print("|", end="")
            for col in range(self.board_size):
                if self.board[row][col] == 0:
                    print(" ", end="|")
                else:
                    print(self.board[row][col], end="|")
            print()

    def copy(self):
        new_game = FourInARowGame(self.board_size, self.connect)
        new_game.board = self.board.copy()
        new_game.current_player = self.current_player
        new_game.board_last = self.board_last
        return new_game
