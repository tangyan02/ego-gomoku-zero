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


Game::Game(int boardSize) {
    currentPlayer = 1;
    this->boardSize = boardSize;
    for (int i = 0; i < boardSize; i++)
        for (int j = 0; j < boardSize; j++)
            board[i][j] = 0;
}

int Game::getOtherPlayer() {
    return 3 - currentPlayer;
}

int Game::getActionIndex(Point &p) {
    return p.x * boardSize + p.y;
}

Point Game::getPointFromIndex(int actionIndex) {
    return {actionIndex / boardSize, actionIndex % boardSize};
}


std::vector<Point> Game::getEmptyPoints() {
    std::vector<Point> emptyPoints;
    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (board[row][col] == 0) {
                emptyPoints.emplace_back(row, col);
            }
        }
    }
    return emptyPoints;
}

torch::Tensor Game::getState() {
    torch::Tensor tensor = torch::zeros({6, boardSize, boardSize});
    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (board[row][col] == currentPlayer) {
                tensor[0][row][col] = 1;
                tensor[2][row][col] = 1;
                tensor[4][row][col] = 1;
            } else if (board[row][col] == getOtherPlayer()) {
                tensor[1][row][col] = 1;
                tensor[3][row][col] = 1;
                tensor[5][row][col] = 1;
            }
        }
    }
    if (lastAction.x >= 0 && lastAction.y >= 0) {
        tensor[2][lastAction.x][lastAction.y] = 0;
        tensor[3][lastAction.x][lastAction.y] = 0;
        tensor[4][lastAction.x][lastAction.y] = 0;
        tensor[5][lastAction.x][lastAction.y] = 0;
    }
    if (lastLastAction.x >= 0 && lastLastAction.y >= 0) {
        tensor[4][lastLastAction.x][lastLastAction.y] = 0;
        tensor[5][lastLastAction.x][lastLastAction.y] = 0;
    }
    return tensor;
}

bool Game::isGameOver() {
    if (lastAction.x >= 0 && lastAction.y >= 0) {
        if (checkWin(lastAction.x, lastAction.y, getOtherPlayer())) {
            return true;
        }
    }
    return getEmptyPoints().empty();
}

void Game::printBoard() {
    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            switch (board[i][j]) {
                case NONE_P:
                    std::cout << ".";
                    break;
                case BLACK:
                    std::cout << "X";
                    break;
                case WHITE:
                    std::cout << "O";
                    break;
                default:
                    std::cout << "#";
            }
            std::cout << " ";
        }
        std::cout << std::endl;
    }
}

bool Game::makeMove(Point p) {
    int row = p.x, col = p.y;
    if (row < 0 || row >= boardSize || col < 0 || col >= boardSize || board[row][col] != NONE_P) {
        cout << "move失败!" << endl;
        return false;
    }

    board[row][col] = currentPlayer;
    currentPlayer = (currentPlayer == BLACK) ? WHITE : BLACK;

    lastLastAction.x = lastAction.x;
    lastLastAction.y = lastAction.y;
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
    while (j < boardSize && board[i][j] == player) {
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
    while (i < boardSize && board[i][j] == player) {
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
    while (i < boardSize && j < boardSize && board[i][j] == player) {
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
    while (i >= 0 && j < boardSize && board[i][j] == player) {
        count++;
        i--;
        j++;
    }
    i = row + 1, j = col - 1;
    while (i < boardSize && j >= 0 && board[i][j] == player) {
        count++;
        i++;
        j--;
    }
    if (count >= CONNECT) {
        return true;
    }

    return false;
}
