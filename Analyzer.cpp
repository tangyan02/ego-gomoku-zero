
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

vector<Point> getThreeDefenceMoves(Game &game, std::vector<Point> &basedMoves) {
    //自己的冲4点，和对手的活4。再从对手眠4里面选.如果下了之后活4活眠4没有了，则视为防御点
    std::vector<Point> defenceMoves;
    auto otherActiveFourMoves = getActiveFourMoves(game.getOtherPlayer(), game, basedMoves);
    if (otherActiveFourMoves.size() >= 1) {
        //对手的冲4点
        auto otherSleepyFourMoves = getSleepyFourMoves(game.getOtherPlayer(), game, basedMoves);
        for (const auto &item: otherSleepyFourMoves) {
            game.board[item.x][item.y] = game.currentPlayer;
            auto nextOtherActiveFourMoves = getActiveFourMoves(game.getOtherPlayer(), game, otherActiveFourMoves);
            if (nextOtherActiveFourMoves.empty()) {
                defenceMoves.emplace_back(item);
            }
            game.board[item.x][item.y] = 0;
        }

        //对手活4点
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());

        //自己的眠4点
        auto sleepyFourMoves = getSleepyFourMoves(game.currentPlayer, game, basedMoves);
        defenceMoves.insert(defenceMoves.end(), sleepyFourMoves.begin(), sleepyFourMoves.end());
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


std::pair<bool, std::vector<Point>>
dfsVCT(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, Point attackPoint,
       bool fourMode, int &nodeRecord, int level, int threeCount, int maxThreeCount, long long timeout,
       bool realPlay) {

    nodeRecord++;

    if (timeout > 0) {
        if (getSystemTime() > timeout) {
            return std::make_pair(false, std::vector<Point>());
        }
    }

    //使用有限点长3，防止检索范围爆炸
    if (threeCount >= maxThreeCount) {
//        cout << "判定转换长4" << endl;
        fourMode = true;
    }
//    std::cout << "===" << std::endl;
//    std::cout << "in four " << fourMode << std::endl;
//    game.printBoard();
//    std::cout << "===" << std::endl;
    std::vector <Point> moves;
    bool attack = checkPlayer == currentPlayer;
    bool attackMove = true;

    std::vector <Point> nearMoves;
    if (lastLastMove.isNull()) {
        nearMoves = game.getEmptyPoints();
    } else {
        if (attack) {
            nearMoves = getNearByEmptyPoints(lastLastMove, game);
        } else {
            nearMoves = getNearByEmptyPoints(lastMove, game);
        }
    }

    // attackPoint废弃，不再计算防守端的进攻
//    auto attackNearMoves = getNearByEmptyPoints(attackPoint, game);
//    nearMoves.insert(nearMoves.end(), attackNearMoves.begin(), attackNearMoves.end());
//    nearMoves = removeDuplicates(nearMoves);

    if (attack) {
        auto oppNearMoves = getNearByEmptyPoints(lastMove, game);

        auto oppWinMoves = getWinningMoves(3 - currentPlayer, game, oppNearMoves);
        auto activeMoves = getActiveFourMoves(currentPlayer, game, nearMoves);
        auto sleepMoves = getSleepyFourMoves(currentPlayer, game, nearMoves);

        //如果对方有2个胜利点，则失败
        if (oppWinMoves.size() > 1) {
            return std::make_pair(false, std::vector<Point>());
        }

        //如果对手有一个胜利点，则必下胜利点
        if (oppWinMoves.size() == 1) {
            auto oppWinMove = oppWinMoves[0];
            if (existPoints(activeMoves, oppWinMove) ||
                existPoints(sleepMoves, oppWinMove)) {
                attackMove = true;
            } else {
                attackMove = false;
            }

            moves.emplace_back(oppWinMove);
//            cout << "find opp win moves " << oppWinMove.x << " " << oppWinMove.y << endl;
//            cout << "moves count " << moves.size() << endl;
//            cout << "activeMoves ";
//            printVector(activeMoves);
//            cout << "sleepMoves ";
//            printVector(sleepMoves);
        }

        //对方没有胜利点，正常连击
        if (oppWinMoves.empty()) {
            //我方有活4点直接下
            if (!activeMoves.empty()) {
//                cout << "发现我方活4点 " << endl;
                return std::make_pair(true, activeMoves);
            }

            unordered_set <Point, PointHash, PointEqual> attackHistory;
            if (!fourMode) {
                //注意，此处要更新attackHistory
                auto oppVCFMoves = dfsVCF(3 - currentPlayer, 3 - currentPlayer,
                                          game, Point(), Point(), 0);
                //对方有VCF点，则我方仅做长4
                if (oppVCFMoves.first) {
//                    cout << "发现对手VCF" << endl;
                    fourMode = true;
                }
            }
//            cout << "attackHistory";
//            vector <Point> attackHistoryList = {attackHistory.begin(), attackHistory.end()};
//            printVector(attackHistoryList);

            //活三模式，对于长三点，需要屏蔽对手连4点
            //如果已经有过一次活3，则长4也需要屏蔽对手连4点
            for (const auto &item: attackHistory) {
                game.board[item.x][item.y] = 3;
            }

            //活三模式考虑活三点
            if (!fourMode) {
                auto threeActiveMoves = getActiveThreeMoves(currentPlayer, game, nearMoves);
                moves.insert(moves.end(), threeActiveMoves.begin(), threeActiveMoves.end());

//                cout << "nearMoves";
//                vector <Point> nearMovesList = {nearMoves.begin(), nearMoves.end()};
//                printVector(nearMovesList);
//
//                cout << "活三点:";
//                vector <Point> threeActiveMovesList = {threeActiveMoves.begin(), threeActiveMoves.end()};
//                printVector(threeActiveMovesList);

            }
//            moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());
            if (threeCount >= 1) {
                auto sleepMovesInBaned = getSleepyFourMoves(currentPlayer, game, nearMoves);
                moves.insert(moves.end(), sleepMovesInBaned.begin(), sleepMovesInBaned.end());
            } else {
                moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());
            }

            for (const auto &item: attackHistory) {
                game.board[item.x][item.y] = 0;
            }
        }
    } else {

        auto oppWinMoves = getWinningMoves(checkPlayer, game, nearMoves);
        //防守眠4
        if (oppWinMoves.size() == 1) {
            moves.emplace_back(oppWinMoves[0]);
        }
        //认输
        if (oppWinMoves.size() > 1) {
            return std::make_pair(true, oppWinMoves);
        }
        if (!fourMode) {
            if (oppWinMoves.empty()) {
                //防活3
                auto threeDefenceMoves = getThreeDefenceMoves(game, nearMoves);

                //如果无需防御，则连击终止
                if (threeDefenceMoves.empty()) {
                    return std::make_pair(false, std::vector<Point>());
                }

                moves.insert(moves.end(), threeDefenceMoves.begin(), threeDefenceMoves.end());
//                //长4
//                auto sleepMoves = getSleepyFourMoves(currentPlayer, game, nearMoves);
//                moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());
            }
        }
    }

    if (moves.empty()) {
//        cout<<"没有可移动的点了"<<endl;
        return std::make_pair(false, std::vector<Point>());
    }

    if (!attack && moves.size() > 1) {
        threeCount++;
    }

    //去重
    moves = removeDuplicates(moves);

    bool finalResult = false;
    if (!attack) {
        finalResult = true;
    }
    std::vector <Point> winMoves;
    for (const auto &item: moves) {
        game.board[item.x][item.y] = currentPlayer;
        auto nextAttackMove = attack && attackMove ? item : attackPoint;
        auto dfsResult = dfsVCT(checkPlayer, 3 - currentPlayer, game, item, lastMove, nextAttackMove,
                                fourMode, nodeRecord, level + 1, threeCount, maxThreeCount, timeout, realPlay);

        if (attack) {
            if (dfsResult.first) {
                finalResult = true;
                winMoves.emplace_back(item);
                if (level > 0 || realPlay) {
                    game.board[item.x][item.y] = 0;
                    return std::make_pair(true, winMoves);
                }
            }
        } else {
            //防守时，默认为true，发现一个失败则为false
            if (!dfsResult.first) {
                game.board[item.x][item.y] = 0;
                return std::make_pair(false, std::vector<Point>());
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

    //我方VCF点
    auto myVCFMoves = game.getMyVCFMoves();
    if (!myVCFMoves.empty()) {
        return make_tuple(true, myVCFMoves, " VCF! ");
    }

    //防御活4点
    auto threeDefenceMoves = getThreeDefenceMoves(game, emptyPoints);
    if (!threeDefenceMoves.empty()) {
        return make_tuple(false, threeDefenceMoves, "  defence 3 ");
    }

    return make_tuple(false, emptyPoints, "");
}