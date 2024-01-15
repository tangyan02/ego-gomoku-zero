#include <iostream>
#include <vector>
#include <torch/torch.h>
#include "Game.h"

using namespace std;

Point::Point() {
    this->x = -1;
    this->y = -1;
}

Point::Point(int x, int y) {
    this->x = x;
    this->y = y;
}


Game::Game() {
    currentPlayer = 1;
    boardSize = BOARD_SIZE;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board[i][j] = 0;
}

int Game::getOtherPlayer() {
    return 3 - currentPlayer;
}

int Game::getActionIndex(Point &p) {
    return p.x * BOARD_SIZE + p.y;
}

Point Game::getPointFromIndex(int actionIndex) {
    return Point(actionIndex / BOARD_SIZE, actionIndex % BOARD_SIZE);
}


std::vector<Point> Game::getEmptyPoints() {
    std::vector<Point> emptyPoints;
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (board[row][col] == 0) {
                emptyPoints.push_back(Point(row, col));
            }
        }
    }
    return emptyPoints;
}

torch::Tensor Game::getState() {
    torch::Tensor tensor = torch::zeros({3, BOARD_SIZE, BOARD_SIZE});
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (board[row][col] == 1) {
                tensor[0][row][col] = 1;
            } else if (board[row][col] == 2) {
                tensor[1][row][col] = 1;
            }
        }
    }
    if (lastAction.x >= 0 && lastAction.y >= 0) {
        tensor[2][lastAction.x][lastAction.y] = 1;
    }
    return tensor;
}

bool Game::isGameOver() {
    if (lastAction.x >= 0 && lastAction.y >= 0) {
        if (checkWin(lastAction.x, lastAction.y, getOtherPlayer())) {
            return true;
        }
    }
    return getEmptyPoints().size() == 0;
}

void Game::printBoard() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            switch (board[i][j]) {
                case NONE:
                    std::cout << ".";
                    break;
                case BLACK:
                    std::cout << "X";
                    break;
                case WHITE:
                    std::cout << "O";
                    break;
            }
            std::cout << " ";
        }
        std::cout << std::endl;
    }
}

bool Game::makeMove(Point p) {
    int row = p.x, col = p.y;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE || board[row][col] != NONE) {
        cout << "move失败!" << endl;
        return false;
    }

    board[row][col] = currentPlayer;
    currentPlayer = (currentPlayer == BLACK) ? WHITE : BLACK;

    lastAction.x = row;
    lastAction.y = col;

    return true;
}

bool Game::checkWin(int row, int col, int player) {
    // 水平方向
    int count = 1;
    int i = row, j = col - 1;
    while (j >= 0 && board[i][j] == player) {
        count++;
        j--;
    }
    j = col + 1;
    while (j < BOARD_SIZE && board[i][j] == player) {
        count++;
        j++;
    }
    if (count >= CONNECT) {
        return true;
    }

    // 垂直方向
    count = 1, i = row - 1;
    j = col;
    while (i >= 0 && board[i][j] == player) {
        count++;
        i--;
    }
    i = row + 1;
    while (i < BOARD_SIZE && board[i][j] == player) {
        count++;
        i++;
    }
    if (count >= CONNECT) {
        return true;
    }

    // 左上到右下斜线方向
    count = 1;
    i = row - 1, j = col - 1;
    while (i >= 0 && j >= 0 && board[i][j] == player) {
        count++;
        i--;
        j--;
    }
    i = row + 1, j = col + 1;
    while (i < BOARD_SIZE && j < BOARD_SIZE && board[i][j] == player) {
        count++;
        i++;
        j++;
    }
    if (count >= CONNECT) {
        return true;
    }

    // 右上到左下斜线方向
    count = 1;
    i = row - 1, j = col + 1;
    while (i >= 0 && j < BOARD_SIZE && board[i][j] == player) {
        count++;
        i--;
        j++;
    }
    i = row + 1, j = col - 1;
    while (i < BOARD_SIZE && j >= 0 && board[i][j] == player) {
        count++;
        i++;
        j--;
    }
    if (count >= CONNECT) {
        return true;
    }

    return false;
}
