#include "Game.h"
#include "Analyzer.h"

using namespace std;

std::vector<Point> removeDuplicates(const std::vector<Point> &points) {
    std::array<std::array<bool, 20>, 20> pointExists = {{{false}}}; // 初始化为 false
    std::vector<Point> uniquePoints;
    for (const auto &point: points) {
        int row = point.x;
        int col = point.y;

        if (!pointExists[row][col]) {
            pointExists[row][col] = true;
            uniquePoints.emplace_back(point);
        }
    }

    return uniquePoints;
}

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
    this->emptyCount = boardSize * boardSize;
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

std::vector<Point> Game::getNearEmptyPoints(int range) {
    std::array<std::array<bool, 20>, 20> nearbyPointsArray = {false}; // 初始化为 false

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
                                nearbyPointsArray[newRow][newCol] = true;
                            }
                        }
                    }
                }
            }
        }
    }

    std::vector<Point> nearbyPoints;
    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (nearbyPointsArray[row][col]) {
                nearbyPoints.push_back(Point(row, col));
            }
        }
    }

    return nearbyPoints;
}

std::vector<Point> Game::getAllEmptyPoints() {
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

std::vector<Point> Game::getEmptyPoints() {
    if (!historyMoves.empty()) {
        return getNearEmptyPoints();
    }
    return getAllEmptyPoints();
}


vector<vector<vector<float>>> Game::getState() {

    vector<vector<vector<float>>> data(INPUT_CHANNELS, vector<vector<float>>(boardSize, vector<float>(boardSize, 0.0f)));

    int otherPlayer = getOtherPlayer();
    //当前局面
    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (board[row][col] == currentPlayer) {
                data[0][row][col] = 1;
            } else if (board[row][col] == otherPlayer) {
                data[1][row][col] = 1;
            }
        }
    }

    // 通道2: 最近一步落子位置
    if (lastAction.x >= 0 && lastAction.y >= 0) {
        data[2][lastAction.x][lastAction.y] = 1;
    }

    // 通道3: 最近两步落子位置
    if (lastLastAction.x >= 0 && lastLastAction.y >= 0) {
        data[3][lastLastAction.x][lastLastAction.y] = 1;
    }

    return data;
}

void Game::getState(float* buffer, int channels) const {
    const int planeSize = boardSize * boardSize;
    memset(buffer, 0, channels * planeSize * sizeof(float));

    int otherPlayer = 3 - currentPlayer;
    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            int cell = board[row][col];
            int idx = row * boardSize + col;
            if (cell == currentPlayer) {
                buffer[idx] = 1.0f;
            } else if (cell == otherPlayer) {
                buffer[planeSize + idx] = 1.0f;
            }
        }
    }

    // 通道2: 最近一步落子位置
    if (lastAction.x >= 0 && lastAction.y >= 0) {
        buffer[2 * planeSize + lastAction.x * boardSize + lastAction.y] = 1.0f;
    }

    // 通道3: 最近两步落子位置
    if (lastLastAction.x >= 0 && lastLastAction.y >= 0) {
        buffer[3 * planeSize + lastLastAction.x * boardSize + lastLastAction.y] = 1.0f;
    }
}

bool Game::isGameOver() {
    if (lastAction.x >= 0 && lastAction.y >= 0) {
        if (checkWin(lastAction.x, lastAction.y, getOtherPlayer())) {
            return true;
        }
    }
    return emptyCount == 0;
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
                case FLAG1:
                    std::cout << "#";
                    break;
                case FLAG2:
                    std::cout << "*";
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
        cout << "move false! " << row << "," << col << ". board value = " << board[row][col] << endl;
        return false;
    }

    board[row][col] = currentPlayer;
    currentPlayer = (currentPlayer == BLACK) ? WHITE : BLACK;
    emptyCount--;

    lastLastAction.x = lastAction.x;
    lastLastAction.y = lastAction.y;
    lastAction.x = row;
    lastAction.y = col;

    historyMoves.emplace_back(p);

    myVcfDone = false;
    oppVcfDone = false;
    myVcfMoves.clear();
    myAllAttackMoves.clear();
    oppVcfMoves.clear();
    oppVcfAttackMoves.clear();
    oppVcfDefenceMoves.clear();

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
    myAllAttackMoves.clear();
    auto myVCF = dfsVCF(currentPlayer, currentPlayer,
                        game, Point(), Point(), 0,
                        nullptr, nullptr, &myAllAttackMoves);
    myAllAttackMoves = removeDuplicates(myAllAttackMoves);
    myVcfMoves = myVCF.second;
    myVcfDone = true;
    return myVcfMoves;
}

vector<Point> Game::getOppVCFMoves() {
    if (oppVcfDone) {
        return oppVcfMoves;
    }
    Game game = *this;
    oppVcfDefenceMoves.clear();
    oppVcfAttackMoves.clear();
    auto oppVCF = dfsVCF(getOtherPlayer(), getOtherPlayer(),
                         game, Point(), Point(), 0,
                         &oppVcfAttackMoves, &oppVcfDefenceMoves);
//    cout<<"oppVcfDefenceMoves "<<oppVcfDefenceMoves.size()<<endl;
    oppVcfDefenceMoves = removeDuplicates(oppVcfDefenceMoves);
    oppVcfAttackMoves = removeDuplicates(oppVcfAttackMoves);
    oppVcfMoves = oppVCF.second;
    oppVcfDone = true;
    return oppVcfMoves;
}
