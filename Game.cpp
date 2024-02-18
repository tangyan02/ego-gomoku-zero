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

bool Point::isNull() {
    return x == -1 && y == -1;
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

std::vector<Point> Game::getNearEmptyPoints() {
    std::unordered_set<Point, PointHash, PointEqual> nearbyPointsSet;
    int range = 3;
    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (board[row][col] != 0) { // 非空点
                // 检查附近的点
                for (int dx = -range; dx <= range; dx++) {
                    for (int dy = -range; dy <= range; dy++) {
                        int newRow = row + dx;
                        int newCol = col + dy;
                        // 检查新点是否在棋盘内并且是非空的
                        if (newRow >= 0 && newRow < boardSize && newCol >= 0 && newCol < boardSize) {
                            if (board[newRow][newCol] == 0) {
                                nearbyPointsSet.insert(Point(newRow, newCol));
                            }
                        }
                    }
                }
            }
        }
    }

    return {nearbyPointsSet.begin(), nearbyPointsSet.end()};
}

std::vector<Point> Game::getEmptyPoints() {
    if(!historyMoves.empty()) {
        return getNearEmptyPoints();
    }
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
    torch::Tensor tensor = torch::zeros({16, boardSize, boardSize});

    //当前局面
    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (board[row][col] == currentPlayer) {
                tensor[0][row][col] = 1;
            } else if (board[row][col] == getOtherPlayer()) {
                tensor[1][row][col] = 1;
            }
        }
    }

    // 构造最近7步的局面
    int numMoves = 7;
    if (historyMoves.size() < numMoves) {
        numMoves = historyMoves.size();
    }
    int k = historyMoves.size() - 1;
    int kPlayer = currentPlayer;
    int tensorIndex = 2;
    for (int i = 0; i < numMoves; i++, k--, tensorIndex += 2) {
        auto p = historyMoves[k];
        kPlayer = 3 - kPlayer;
        board[p.x][p.y] = 0;

        for (int row = 0; row < boardSize; row++) {
            for (int col = 0; col < boardSize; col++) {
                if (board[row][col] == currentPlayer) {
                    tensor[tensorIndex][row][col] = 1;
                } else if (board[row][col] == getOtherPlayer()) {
                    tensor[tensorIndex + 1][row][col] = 1;
                }
            }
        }
    }

    k += 1;
    for (int i = k; i < historyMoves.size(); i++) {
        auto p = historyMoves[i];
        board[p.x][p.y] = kPlayer;
        kPlayer = 3 - kPlayer;
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

void Game::printBoard(const std::string &part) {
    for (int i = 0; i < boardSize; i++) {
        std::cout << part;
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

    historyMoves.emplace_back(p);

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
