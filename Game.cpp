#include "Game.h"
#include "Analyzer.h"

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
    int range = 4;
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
    if (!historyMoves.empty()) {
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
    torch::Tensor tensor = torch::zeros({18, boardSize, boardSize});

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

    //每一种棋形判断
    Game game = *this;
    auto moves = getEmptyPoints();
    for (const auto &move: moves) {
        Point action = move;
        for (int direct = 0; direct < 4; direct++) {
            int x = action.x;
            int y = action.y;
            if (checkPointDirectShape(game, game.currentPlayer, action, direct, LONG_FIVE)) {
                tensor[2][x][y] += 1;
            }
            if (checkPointDirectShape(game, 3 - game.currentPlayer, action, direct, LONG_FIVE)) {
                tensor[3][x][y] += 1;
            }
            if (checkPointDirectShape(game, game.currentPlayer, action, direct, ACTIVE_FOUR)) {
                tensor[4][x][y] += 1;
            }
            if (checkPointDirectShape(game, 3 - game.currentPlayer, action, direct, ACTIVE_FOUR)) {
                tensor[5][x][y] += 1;
            }
            if (checkPointDirectShape(game, game.currentPlayer, action, direct, SLEEPY_FOUR)) {
                tensor[6][x][y] += 1;
            }
            if (checkPointDirectShape(game, 3 - game.currentPlayer, action, direct, SLEEPY_FOUR)) {
                tensor[7][x][y] += 1;
            }
            if (checkPointDirectShape(game, game.currentPlayer, action, direct, ACTIVE_THREE)) {
                tensor[8][x][y] += 1;
            }
            if (checkPointDirectShape(game, 3 - game.currentPlayer, action, direct, ACTIVE_THREE)) {
                tensor[9][x][y] += 1;
            }
            if (checkPointDirectShape(game, game.currentPlayer, action, direct, SLEEPY_THREE)) {
                tensor[10][x][y] += 1;
            }
            if (checkPointDirectShape(game, 3 - game.currentPlayer, action, direct, SLEEPY_THREE)) {
                tensor[11][x][y] += 1;
            }
            if (checkPointDirectShape(game, game.currentPlayer, action, direct, ACTIVE_TWO)) {
                tensor[12][x][y] += 1;
            }
            if (checkPointDirectShape(game, 3 - game.currentPlayer, action, direct, ACTIVE_TWO)) {
                tensor[13][x][y] += 1;
            }
            if (checkPointDirectShape(game, game.currentPlayer, action, direct, SLEEPY_TWO)) {
                tensor[14][x][y] += 1;
            }
            if (checkPointDirectShape(game, 3 - game.currentPlayer, action, direct, SLEEPY_TWO)) {
                tensor[15][x][y] += 1;
            }
        }
    }

    //VCF点
    auto myVCFMoves = getMyVCFMoves();
    if (!myVCFMoves.empty()) {
        for (const auto &item: myVCFMoves) {
            tensor[16][item.x][item.y] += 1;
        }
    }

    auto oppVCFMoves = getOppVCFMoves();
    if (!oppVCFMoves.empty()) {
        for (const auto &item: oppVCFMoves) {
            tensor[17][item.x][item.y] += 1;
        }
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
        cout << "move false! " << row << " " << col << " " << board[row][col] << endl;
        return false;
    }

    board[row][col] = currentPlayer;
    currentPlayer = (currentPlayer == BLACK) ? WHITE : BLACK;

    lastLastAction.x = lastAction.x;
    lastLastAction.y = lastAction.y;
    lastAction.x = row;
    lastAction.y = col;

    historyMoves.emplace_back(p);

    myVcfDone = false;
    oppVcfDone = false;
    myVcfMoves.clear();
    oppVcfMoves.clear();

    return true;
}

bool Game::checkWin(int row, int col, int player) {
    Game game = *this;
    Point action = Point(row, col);
    for (int i = 0; i < 4; i++) {
        if (checkPointDirectShape(game, player, action, i, LONG_FIVE)) {
            return true;
        }
    }
    return false;
}

vector<Point> Game::getMyVCFMoves() {
    if (myVcfDone) {
        return myVcfMoves;
    }
    Game game = *this;
    auto myVCF = dfsVCF(currentPlayer, currentPlayer, game, Point(), Point());
    myVcfMoves = myVCF.second;
    myVcfDone = true;
    return myVcfMoves;
}

vector<Point> Game::getOppVCFMoves() {
    if (oppVcfDone) {
        return oppVcfMoves;
    }
    Game game = *this;
    auto oppVCF = dfsVCF(getOtherPlayer(), getOtherPlayer(), game, Point(), Point());
    oppVcfMoves = oppVCF.second;
    oppVcfDone = true;
    return oppVcfMoves;
}