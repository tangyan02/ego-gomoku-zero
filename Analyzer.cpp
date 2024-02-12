
#include "Analyzer.h"

static int dx[8] = {0, 0, 1, -1, 1, 1, -1, -1};
static int dy[8] = {1, -1, 0, 0, 1, -1, 1, -1};

void printVector(vector<Point> &a) {
    for (const auto &item: a) {
        cout << "(" << item.x << "," << item.y << ") ";
    }
    cout << endl;
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

struct PointHash {
    std::size_t operator()(const Point &p) const {
        return std::hash<int>()(p.x) ^ std::hash<int>()(p.y);
    }
};

bool operator==(const Point &p1, const Point &p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

std::vector<Point> removeDuplicates(const std::vector<Point> &points) {
    std::unordered_set<Point, PointHash> uniquePoints(points.begin(), points.end());
    return {uniquePoints.begin(), uniquePoints.end()};
}

std::vector<Point> getTwoRoundPoints(Point one, Point two, Game &game) {
    std::vector<Point> empty_points;
    std::vector<Point> empty_points_last;
    if (one.x >= 0 && one.y >= 0) {
        empty_points = getNearByEmptyPoints(one, game);
    }
    if (two.x >= 0 && two.y >= 0) {
        empty_points_last = getNearByEmptyPoints(two, game);
    }
    empty_points.insert(empty_points.end(), empty_points_last.begin(), empty_points_last.end());
    auto unique_points = removeDuplicates(empty_points);
    return unique_points;
}

std::vector<Point> getWinningMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    std::vector<Point> winning_moves;
    for (const auto &point: basedMoves) {
        int row = point.x;
        int col = point.y;
        if (game.board[row][col] != 0) {
            continue;
        }
        game.board[row][col] = player;
        if (game.checkWin(row, col, player)) {
            winning_moves.emplace_back(row, col);
        }
        game.board[row][col] = 0;
    }
    return winning_moves;
}


std::vector<Point> getActiveThreeMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    std::vector<Point> result;
    for (const auto &point: basedMoves) {
        int row = point.x;
        int col = point.y;
        if (game.board[row][col] != 0) {
            continue;
        }
        game.board[row][col] = player;
        auto nearByEmptyPoints = getNearByEmptyPoints(point, game);
        auto winMoves = getActiveFourMoves(player, game, nearByEmptyPoints);
        if (winMoves.size() >= 1) {
            result.emplace_back(point);
        }
        game.board[row][col] = 0;
    }
    return result;
}

std::vector<Point> getActiveFourMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    std::vector<Point> result;
    for (const auto &point: basedMoves) {
        int row = point.x;
        int col = point.y;
        if (game.board[row][col] != 0) {
            continue;
        }
        game.board[row][col] = player;
        auto nearByEmptyPoints = getNearByEmptyPoints(point, game);
        auto winMoves = getWinningMoves(player, game, nearByEmptyPoints);
        if (winMoves.size() >= 2) {
            result.emplace_back(point);
        }
        game.board[row][col] = 0;
    }
    return result;
}

std::vector<Point> getSleepyFourMoves(int player, Game &game, std::vector<Point> &basedMoves) {
    std::vector<Point> result;
    for (const auto &point: basedMoves) {
        int row = point.x;
        int col = point.y;
        game.board[row][col] = player;
        auto nearByEmptyPoints = getNearByEmptyPoints(point, game);
        auto winMoves = getWinningMoves(player, game, nearByEmptyPoints);
        if (winMoves.size() == 1) {
            result.emplace_back(point);
        }
        game.board[row][col] = 0;
    }
    return result;
}

std::vector<Point> getThreeDefenceMoves(int player, Game &game) {
    //如果对方有2个活4点，阻止活4点
    //如果对方只有1个活4点，则阻止活4点和眠4点,加上自己的所有眠4点
    std::vector<Point> defenceMoves;
    auto allMoves = game.getEmptyPoints();
    auto otherActiveFourMoves = getActiveFourMoves(3 - player, game, allMoves);
    if (!otherActiveFourMoves.empty()) {
        if (otherActiveFourMoves.size() >= 2) {
            return otherActiveFourMoves;
        }
        auto otherSleepyFourMoves = getSleepyFourMoves(3 - player, game, allMoves);
        auto sleepyFourMoves = getSleepyFourMoves(player, game, allMoves);
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), otherSleepyFourMoves.begin(), otherSleepyFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), sleepyFourMoves.begin(), sleepyFourMoves.end());
    }
    return removeDuplicates(defenceMoves);
}


std::vector<Point> getThreeDefenceMovesAtOnlyDefence(int player, Game &game, std::vector<Point> &basedMoves) {
    //如果对方有2个活4点，阻止活4点
    //如果对方只有1个活4点，则阻止活4点和眠4点
    std::vector<Point> defenceMoves;
    auto otherActiveFourMoves = getActiveFourMoves(3 - player, game, basedMoves);
    if (!otherActiveFourMoves.empty()) {
        if (otherActiveFourMoves.size() >= 2) {
            return otherActiveFourMoves;
        }
        auto otherSleepyFourMoves = getSleepyFourMoves(3 - player, game, basedMoves);
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), otherSleepyFourMoves.begin(), otherSleepyFourMoves.end());
    }
    return removeDuplicates(defenceMoves);
}

std::vector<Point> getVCFDefenceMoves(int player, Game &game) {
    //如果有2个VCF点，则堵任意一个
    //如果只有一个VCF点，则类似防3处理，阻止活4点和眠4点,加上自己的所有眠4点,在加上阻止活3点
    std::vector<Point> defenceMoves;
    auto allMoves = game.getEmptyPoints();
    auto vcfResult = dfsVCF(3 - player, 3 - player, game, Point(), Point());
    if (vcfResult.first) {
        if (vcfResult.second.size() >= 2) {
            return vcfResult.second;
        }

        auto otherActiveFourMoves = getActiveFourMoves(3 - player, game, allMoves);
        if (otherActiveFourMoves.size() >= 2) {
            return otherActiveFourMoves;
        }

        auto otherSleepyFourMoves = getSleepyFourMoves(3 - player, game, allMoves);
        auto sleepyFourMoves = getSleepyFourMoves(player, game, allMoves);
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), otherSleepyFourMoves.begin(), otherSleepyFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), sleepyFourMoves.begin(), sleepyFourMoves.end());

        if (otherActiveFourMoves.empty()) {
            auto otherActiveThreeMoves = getActiveThreeMoves(3 - player, game, allMoves);
            defenceMoves.insert(defenceMoves.end(), otherActiveThreeMoves.begin(), otherActiveThreeMoves.end());
        }
    }
    return removeDuplicates(defenceMoves);
}


std::vector<Point> getVCTDefenceMoves(int player, Game &game) {
    //如果有2个以上VCT点，则堵任意一个
    //如果只有一个VCT点，阻止活4点和眠4点,加上自己的所有眠4点,在加上阻止活3点,加上自己活3点
    std::vector<Point> defenceMoves;
    auto allMoves = game.getEmptyPoints();
    auto vctResult = dfsVCT(3 - player, 3 - player, game, Point(), Point(), false);
    if (vctResult.first) {
        if (vctResult.second.size() >= 2) {
            return vctResult.second;
        }

        auto otherActiveFourMoves = getActiveFourMoves(3 - player, game, allMoves);
        if (otherActiveFourMoves.size() >= 2) {
            return otherActiveFourMoves;
        }

        auto otherSleepyFourMoves = getSleepyFourMoves(3 - player, game, allMoves);
        auto sleepyFourMoves = getSleepyFourMoves(player, game, allMoves);
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), otherSleepyFourMoves.begin(), otherSleepyFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), sleepyFourMoves.begin(), sleepyFourMoves.end());

        if (otherActiveFourMoves.empty()) {
            auto otherActiveThreeMoves = getActiveThreeMoves(3 - player, game, allMoves);
            defenceMoves.insert(defenceMoves.end(), otherActiveThreeMoves.begin(), otherActiveThreeMoves.end());

            auto activeThreeMoves = getActiveThreeMoves(player, game, allMoves);
            defenceMoves.insert(defenceMoves.end(), activeThreeMoves.begin(), activeThreeMoves.end());
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
//    if (level > 30) {
//        cout << " level error " << level << endl;
//        game.printBoard();
//    }
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
dfsVCT(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, bool fourMode, int level,
       int threeCount) {
//    if (level > 30) {
//        cout << " level error " << level << endl;
//        game.printBoard();
//    }
    //使用有限点长3，防止检索范围爆炸
    if (threeCount <= 0) {
        fourMode = true;
    }
//    std::cout << "===" << std::endl;
//    std::cout << "in four" << fourMode << std::endl;
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

        //如果有一个胜利点，则看是不是和我方4连击点重合，重合则返回
        if (oppWinMoves.size() == 1) {
            auto oppWinMove = oppWinMoves[0];
            if (existPoints(activeMoves, oppWinMove) ||
                existPoints(sleepMoves, oppWinMove)) {
                moves.emplace_back(oppWinMove);
            }
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

            if (!fourMode) {
                auto oppVCFMoves = dfsVCF(3 - currentPlayer, 3 - currentPlayer, game, lastMove, lastMove);
                //对方有VCF点，则我方仅做长4
                if (oppVCFMoves.first) {
//                    cout << "发现对手VCF" << endl;
                    fourMode = true;
                }
            }

            if (!fourMode) {
                //一种保守下法
                //对方没有活4点，但是有眠4点，且眠4点，则把对手眠4点设置成障碍，再找活三点和眠4点
                auto oppSleepyFourMoves = getSleepyFourMoves(3 - currentPlayer, game, nearMoves);
                //设置障碍
                for (const auto &item: oppSleepyFourMoves) {
                    game.board[item.x][item.y] = 3;
                }

                auto threeActiveMoves = getActiveThreeMoves(currentPlayer, game, nearMoves);
                moves.insert(moves.end(), threeActiveMoves.begin(), threeActiveMoves.end());

                //恢复障碍
                for (const auto &item: oppSleepyFourMoves) {
                    game.board[item.x][item.y] = 0;
                }
            }

            if (fourMode) {
                moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());
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
                auto threeDefenceMoves = getThreeDefenceMovesAtOnlyDefence
                        (currentPlayer, game, nearMoves);
                moves.insert(moves.end(), threeDefenceMoves.begin(), threeDefenceMoves.end());
            }
        }
    }

    if (moves.empty()) {
        return std::make_pair(false, std::vector<Point>());
    }

    if (!attack && moves.size() > 1) {
        threeCount--;
    }

    //去重
    moves = removeDuplicates(moves);

    bool finalResult = false;
    if (!attack) {
        finalResult = true;
    }
    std::vector<Point> winMoves;
    for (const auto &item: moves) {
        game.board[item.x][item.y] = currentPlayer;
        auto dfsResult = dfsVCT(checkPlayer, 3 - currentPlayer, game, item, lastMove, fourMode, level + 1, threeCount);

        if (attack) {
            if (dfsResult.first) {
                finalResult = true;
                winMoves.emplace_back(item);
                if (level > 0) {
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
pair<bool, vector<Point>> selectActions(Game &game, bool vctMode) {
    auto emptyPoints = game.getEmptyPoints();
    auto nearPoints = getNearByEmptyPoints(game.lastAction, game);
    auto roundPoints = getTwoRoundPoints(game.lastAction, game.lastLastAction, game);
    //我方长5
    auto currentWinnerMoves = getWinningMoves(game.currentPlayer, game, roundPoints);
    if (!currentWinnerMoves.empty()) {
        return make_pair(true, currentWinnerMoves);
    }
    //防止对手长5
    auto otherWinnerMoves = getWinningMoves(game.getOtherPlayer(), game, nearPoints);
    if (!otherWinnerMoves.empty()) {
        return make_pair(false, otherWinnerMoves);
    }
    //我方活4
    auto activeFourMoves = getActiveFourMoves(game.currentPlayer, game, roundPoints);
    if (!activeFourMoves.empty()) {
        return make_pair(true, activeFourMoves);
    }

    if (!vctMode) {
        //我方VCF点
        auto vcfResult = dfsVCF(game.currentPlayer, game.currentPlayer, game, Point(), Point());
        if (vcfResult.first) {
            return make_pair(true, vcfResult.second);
        }

        //防对方VCF点
        auto VCFDefenceMoves = getVCFDefenceMoves(game.currentPlayer, game);
        if (!VCFDefenceMoves.empty()) {
            return make_pair(false, VCFDefenceMoves);
        }
    } else {
        //我方VCT点
        auto vctResult = dfsVCT(game.currentPlayer, game.currentPlayer, game, Point(), Point(), false);
        if (vctResult.first) {
            return make_pair(true, vctResult.second);
        }

        //防对方VCT点
        auto VCTDefenceMoves = getVCTDefenceMoves(game.currentPlayer, game);
        if (!VCTDefenceMoves.empty()) {
            return make_pair(false, VCTDefenceMoves);
        }
    }

    return make_pair(false, emptyPoints);
}