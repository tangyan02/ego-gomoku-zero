
#include "Analyzer.h"

static int dx[8] = {0, 0, 1, -1, 1, 1, -1, -1};
static int dy[8] = {1, -1, 0, 0, 1, -1, 1, -1};

static int ddx[4] = {1, 1, 1, 0};
static int ddy[4] = {1, 0, -1, 1};

void printVector(vector<Point> &a) {
    for (const auto &item: a) {
        cout << "(" << item.x << "," << item.y << ") ";
    }
    cout << endl;
}

std::vector<Point> getLineEmptyPoints(Point action, Game &game, int direct) {
    std::vector<Point> empty_points;
    if (!action.isNull()) {
        int x = action.x;
        int y = action.y;
        for (int k = -4; k <= 4; k++) {
            int tx = x + k * ddx[direct];
            int ty = y + k * ddy[direct];
            if (tx >= 0 && tx < game.boardSize && ty >= 0 && ty < game.boardSize &&
                game.board[tx][ty] == 0) {
                empty_points.emplace_back(tx, ty);
            }
        }
    }
    return empty_points;
}

std::vector<Point> getNearByEmptyPoints(Point action, Game &game) {
    std::vector<Point> empty_points;
    if (!action.isNull()) {
        int last_row = action.x;
        int last_col = action.y;
        for (int i = 0; i < 8; i++) {
            int x = dx[i];
            int y = dy[i];
            for (int k = 1; k <= 5; k++) {
                int row = last_row + x * k;
                int col = last_col + y * k;
                if (row >= 0 && row < game.boardSize && col >= 0 && col < game.boardSize &&
                    game.board[row][col] == 0) {
                    empty_points.emplace_back(row, col);
                }
            }
        }
    }
    return empty_points;
}

std::vector<Point> removeDuplicates(const std::vector<Point> &points) {
    std::unordered_set<Point, PointHash, PointEqual> uniquePoints(points.begin(), points.end());
    return {uniquePoints.begin(), uniquePoints.end()};
}


vector<Point> getShapeMoves(int player, Game &game, std::vector<Point> &basedMoves, Shape shape) {
    std::vector<Point> result;
    for (const auto &point: basedMoves) {
        if (game.board[point.x][point.y] != 0) {
            continue;
        }
        for (int i = 0; i < 4; i++) {
            auto action = point;
            if (checkPointDirectShape(game, player, action, i, shape)) {
                result.emplace_back(point);
                break;
            }
        }
    }
    return result;
}

std::vector<Point>
getWinningMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    return getShapeMoves(player, game, basedMoves, LONG_FIVE);
}

std::vector<Point>
getActiveFourMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    return getShapeMoves(player, game, basedMoves, ACTIVE_FOUR);
}

std::vector<Point>
getSleepyFourMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    return getShapeMoves(player, game, basedMoves, SLEEPY_FOUR);
}

std::vector<Point>
getActiveThreeMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    return getShapeMoves(player, game, basedMoves, ACTIVE_THREE);

}

std::vector<Point>
getSleepyThreeMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    return getShapeMoves(player, game, basedMoves, SLEEPY_THREE);
}

std::vector<Point>
getActiveTwoMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    return getShapeMoves(player, game, basedMoves, ACTIVE_TWO);
}

std::vector<Point>
getSleepyTwoMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    return getShapeMoves(player, game, basedMoves, SLEEPY_TWO);
}

vector<Point> getThreeDefenceMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    //两个活4点，必须堵一个
    auto otherActiveFourMoves = getActiveFourMoves(3 - player, game, basedMoves);
    if (otherActiveFourMoves.size() >= 2) {
        return otherActiveFourMoves;
    }

    //一个活4点,阻止活4或阻止眠4,或自己眠4
    std::vector<Point> defenceMoves;
    if (otherActiveFourMoves.size() == 1) {
        auto nearMoves = getNearByEmptyPoints(otherActiveFourMoves[0], game);
        auto otherSleepyFourMoves = getSleepyFourMoves(3 - player, game, nearMoves);
        auto sleepyFourMoves = getSleepyFourMoves(player, game, basedMoves);
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), otherSleepyFourMoves.begin(), otherSleepyFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), sleepyFourMoves.begin(), sleepyFourMoves.end());
    }
    return removeDuplicates(defenceMoves);
}

std::vector<Point> getVCFDefenceMoves(Game &game) {
    //如果有2个VCF点，则堵任意一个
    //如果只有一个VCF点，则类似防3处理，阻止活4点和眠4点,加上自己的所有眠4点,在加上阻止活3点
    std::vector<Point> defenceMoves;
    auto allMoves = game.getEmptyPoints();
    auto oppVCFMoves = game.getOppVCFMoves();
    if (!oppVCFMoves.empty()) {
        if (oppVCFMoves.size() >= 2) {
            return oppVCFMoves;
        }

        auto otherActiveFourMoves = getActiveFourMoves(game.getOtherPlayer(), game, allMoves);
        if (otherActiveFourMoves.size() >= 2) {
            return otherActiveFourMoves;
        }

        auto otherSleepyFourMoves = getSleepyFourMoves(game.getOtherPlayer(), game, allMoves);
        auto sleepyFourMoves = getSleepyFourMoves(game.currentPlayer, game, allMoves);
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), otherSleepyFourMoves.begin(), otherSleepyFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), sleepyFourMoves.begin(), sleepyFourMoves.end());

        if (otherActiveFourMoves.empty()) {
            auto otherActiveThreeMoves = getActiveThreeMoves(game.getOtherPlayer(), game, allMoves);
            defenceMoves.insert(defenceMoves.end(), otherActiveThreeMoves.begin(), otherActiveThreeMoves.end());
        }
    }
    return removeDuplicates(defenceMoves);
}


// 创建一个函数来查找特定的点
bool existPoints(const std::vector<Point> &moves, const Point &target) {
    for (const auto &item: moves) {
        if (item.x == target.x && item.y == target.y) {
            return true;
        }
    }
    return false;
}

std::pair<bool, std::vector<Point>>
dfsVCF(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, int level) {
//    std::cout << "===" << std::endl;
//    game.printBoard();
//    std::cout << "===" << std::endl;
    std::vector<Point> moves;
    bool attack = checkPlayer == currentPlayer;

    std::vector<Point> nearMoves;
    if (lastLastMove.isNull()) {
        nearMoves = game.getEmptyPoints();
    } else {
        if (attack) {
            nearMoves = getNearByEmptyPoints(lastLastMove, game);
        } else {
            nearMoves = getNearByEmptyPoints(lastMove, game);
        }
    }

    if (attack) {
        auto oppNearMoves = getNearByEmptyPoints(lastMove, game);

        auto oppWinMoves = getWinningMoves(3 - currentPlayer, game, oppNearMoves);
        auto activeMoves = getActiveFourMoves(currentPlayer, game, nearMoves);
        auto sleepMoves = getSleepyFourMoves(currentPlayer, game, nearMoves);
        //如果对方有2个胜利点，则失败
        if (oppWinMoves.size() > 1) {
            return std::make_pair(false, std::vector<Point>());
        }

        //如果有一个胜利点，则看是不是和我方连击点重合，重合则返回
        if (oppWinMoves.size() == 1) {
            auto oppWinMove = oppWinMoves[0];
            if (existPoints(activeMoves, oppWinMove) || existPoints(sleepMoves, oppWinMove)) {
                moves.emplace_back(oppWinMove);
            }
//            cout << "find opp win moves " << oppWinMove.x << " " << oppWinMove.y << endl;
//            cout << "moves count " << moves.size() << endl;
        }

        //对方没有胜利点，正常连击
        if (oppWinMoves.empty()) {
            if (!activeMoves.empty()) {
                return std::make_pair(true, activeMoves);
            }

            moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());
            if (moves.empty()) {
                return std::make_pair(false, std::vector<Point>());
            }
        }
    } else {
        //防守
        auto oppWinMoves = getWinningMoves(checkPlayer, game, nearMoves);
        if (oppWinMoves.empty()) {
            return std::make_pair(false, std::vector<Point>());
        }
        if (oppWinMoves.size() == 1) {
            moves.emplace_back(oppWinMoves[0]);
        }
        if (oppWinMoves.size() > 1) {
            return std::make_pair(true, oppWinMoves);
        }
    }

    bool finalResult = false;
    std::vector<Point> winMoves;
    for (const auto &item: moves) {
        game.board[item.x][item.y] = currentPlayer;
        auto dfsResult = dfsVCF(checkPlayer, 3 - currentPlayer, game, item, lastMove, level + 1);
        if (dfsResult.first) {
            finalResult = true;
            winMoves.emplace_back(item);
            if (level > 0) {
                game.board[item.x][item.y] = 0;
                return std::make_pair(true, winMoves);
            }
        }
        game.board[item.x][item.y] = 0;
    }

    return std::make_pair(finalResult, winMoves);
}

/**
 * 返回两个值，第一个值代表返回值是否是必胜点
 */
tuple<bool, vector<Point>, string> selectActions(Game &game) {
    auto emptyPoints = game.getEmptyPoints();
    //我方长5
    auto currentWinnerMoves = getWinningMoves(game.currentPlayer, game, emptyPoints);
    if (!currentWinnerMoves.empty()) {
        return make_tuple(true, currentWinnerMoves, " win move  ");
    }
    //防止对手长5
    auto otherWinnerMoves = getWinningMoves(game.getOtherPlayer(), game, emptyPoints);
    if (!otherWinnerMoves.empty()) {
        return make_tuple(false, otherWinnerMoves, " defence 5");
    }
    //我方活4
    auto activeFourMoves = getActiveFourMoves(game.currentPlayer, game, emptyPoints);
    if (!activeFourMoves.empty()) {
        return make_tuple(true, activeFourMoves, "  active 4 ");
    }

    //防御活4点
    auto threeDefenceMoves = getThreeDefenceMoves(game.currentPlayer, game, emptyPoints);
    if (!threeDefenceMoves.empty()) {
        return make_tuple(true, threeDefenceMoves, "  defence 3 ");
    }

    //我方VCF点
    auto myVCFMoves = game.getMyVCFMoves();
    if (!myVCFMoves.empty()) {
        return make_tuple(true, myVCFMoves, " VCF! ");
    }

    //防对方VCF点
    auto VCFDefenceMoves = getVCFDefenceMoves(game);
    if (!VCFDefenceMoves.empty()) {
        return make_tuple(false, VCFDefenceMoves, " defence VCF ");
    }

    return make_tuple(false, emptyPoints, "");
}