
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

std::vector<Point> removeDuplicates(const std::vector<Point> &points) {
    std::unordered_set<Point, PointHash, PointEqual> uniquePoints(points.begin(), points.end());
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
        if (game.board[row][col] != 0) {
            continue;
        }
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
dfsVCF(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, int level,
       unordered_set<Point, PointHash, PointEqual> *attackHistory) {
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
                if (attackHistory != nullptr) {
                    attackHistory->insert(oppWinMove);
                }
                moves.emplace_back(oppWinMove);
            }
//            cout << "find opp win moves " << oppWinMove.x << " " << oppWinMove.y << endl;
//            cout << "moves count " << moves.size() << endl;
        }

        //对方没有胜利点，正常连击
        if (oppWinMoves.empty()) {
            if (!activeMoves.empty()) {
                if (attackHistory != nullptr) {
                    attackHistory->insert(activeMoves.begin(), activeMoves.end());
                }
                return std::make_pair(true, activeMoves);
            }

            moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());
            if (attackHistory != nullptr) {
                attackHistory->insert(sleepMoves.begin(), sleepMoves.end());
            }
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
    std::vector<Point> moves;
    bool attack = checkPlayer == currentPlayer;
    bool attackMove = true;

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

            unordered_set<Point, PointHash, PointEqual> attackHistory;
            if (!fourMode) {
//                auto oppVCFMoves = dfsVCF(3 - currentPlayer, 3 - currentPlayer,
//                                          game, Point(), game.lastAction, 0, &attackHistory);
                auto oppVCFMoves = dfsVCF(3 - currentPlayer, 3 - currentPlayer,
                                          game, Point(), Point(), 0, &attackHistory);
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
            }

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
                auto threeDefenceMoves = getThreeDefenceMovesAtOnlyDefence
                        (currentPlayer, game, nearMoves);

                //如果无需防御，则连击终止
                if (threeDefenceMoves.empty()) {
                    return std::make_pair(false, std::vector<Point>());
                }

                moves.insert(moves.end(), threeDefenceMoves.begin(), threeDefenceMoves.end());

            }
        }
    }

    if (moves.empty()) {
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
    std::vector<Point> winMoves;
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


tuple<bool, vector<Point>, int> dfsVCTIter(int player, Game &game, int timeLimit, bool realPlay) {
    long long startTime = getSystemTime();
    long long timeout = getSystemTime() + static_cast<long long>(timeLimit);
    int threeCount = 1;
    int lastNodeRecord = 0;
    for (; threeCount <= 20; threeCount++) {
//        cout << "threeCount=" << threeCount << " startTime=" << startTime << " timeout=" << timeout << " timeLimit="
//             << timeLimit << endl;
        int nodeRecord = 0;
        auto dfsResult = dfsVCT(player, player, game, Point(), Point(), Point(),
                                false, nodeRecord, 0, 0, threeCount, timeout,
                                realPlay);
//        cout << "nodeRecord=" << nodeRecord << endl;
        if (lastNodeRecord == nodeRecord || (getSystemTime() > timeout)) {
            break;
        }
        lastNodeRecord = nodeRecord;
        if (dfsResult.first) {
//            cout << "time cost " << getSystemTime() - startTime << endl;
            return make_tuple(dfsResult.first, dfsResult.second, threeCount);
        }
    }
//    cout << " time cost " << getSystemTime() - startTime << endl;
    return make_tuple(false, std::vector<Point>(), threeCount);
}

std::vector<Point> getAllDefenceMoves(int player, Game &game) {
    auto allMoves = game.getEmptyPoints();
    std::vector<Point> defenceMoves;

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
    return removeDuplicates(defenceMoves);
}


tuple<bool, vector<Point>, int> dfsVCTDefenceIter(int player, Game &game, int timeLimit) {
    int threeCount = 1;
    auto defencePoints = getAllDefenceMoves(player, game);
    int lastNodeRecord = 0;
    std::unordered_set<Point, PointHash, PointEqual> banPoints;
    std::unordered_set<Point, PointHash, PointEqual> lastBanPoints;

    bool needDefence = false;
    for (; threeCount <= 20; threeCount++) {
        lastBanPoints = banPoints;
//        cout << "threeCount=" << threeCount << " timeLimit=" << timeLimit << endl;
        int nodeRecord = 0;

        if (!needDefence) {
            long long timeout = getSystemTime() + static_cast<long long>(timeLimit);
            long long startTime = getSystemTime();
            auto dfsResult = dfsVCT(3 - player, 3 - player, game,
                                    Point(), Point(), Point(),
                                    false, nodeRecord, 0, 0, threeCount, timeout,
                                    false);
            auto timeCost = getSystemTime() - startTime;
            timeLimit -= timeCost;
            if (dfsResult.first) {
                needDefence = true;
//                cout << "需要防守" << endl;
//                cout << "threeCount=" << threeCount << " timeLimit=" << timeLimit << endl;
            }
        }

        if (needDefence) {
            for (const auto &item: defencePoints) {
                if (banPoints.find(item) != banPoints.end()) {
                    continue;
                }
                game.board[item.x][item.y] = player;

                long long timeout = getSystemTime() + static_cast<long long>(timeLimit);
                long long startTime = getSystemTime();
                auto dfsResult = dfsVCT(3 - player, 3 - player, game,
                                        item, game.lastAction, Point(),
                                        false, nodeRecord, 0, 0, threeCount, timeout,
                                        false);
                auto timeCost = getSystemTime() - startTime;
                timeLimit -= timeCost;

                if (dfsResult.first) {
                    banPoints.insert(item);
                }
                game.board[item.x][item.y] = 0;
            }
//            cout << "ban points:";
//            vector<Point> banPointList = {banPoints.begin(), banPoints.end()};
//            printVector(banPointList);
//            cout << "nodeRecord=" << nodeRecord << endl;

            if (lastNodeRecord == nodeRecord || timeLimit <= 0) {
                break;
            }
            lastNodeRecord = nodeRecord;
        }
        if (banPoints.size() == defencePoints.size()) {
//            cout << "已经全部被禁,使用上个迭代的banpoint" << endl;
            banPoints = lastBanPoints;
            break;
        }
    }
    vector<Point> result;
    for (const auto &item: defencePoints) {
        if (banPoints.find(item) == banPoints.end()) {
            result.emplace_back(item);
        }
    }

    if (result.empty()) {
        result = defencePoints;
    }
    return make_tuple(needDefence, result, threeCount);
}

std::vector<Point> getVCTDefenceMoves(int player, Game &game, int &dfsThreeCount, int timeLimit, bool realPlay) {
    //如果有2个以上VCT点，则堵任意一个
    //如果只有一个VCT点，阻止活4点和眠4点,加上自己的所有眠4点,在加上阻止活3点,加上自己活3点
    std::vector<Point> defenceMoves;
    auto allMoves = game.getEmptyPoints();
    auto vctResult = dfsVCTIter(3 - player, game, timeLimit, realPlay);
    dfsThreeCount = get<2>(vctResult);
    if (get<0>(vctResult)) {
        if (get<1>(vctResult).size() >= 2) {
            return get<1>(vctResult);
        }

        defenceMoves = getAllDefenceMoves(player, game);
    }
    return defenceMoves;
}

/**
 * 返回两个值，第一个值代表返回值是否是必胜点
 */
tuple<bool, vector<Point>, string> selectActions(Game &game, bool vctMode, int timeLimit, bool realPlay) {
    auto emptyPoints = game.getEmptyPoints();
    auto nearPoints = getNearByEmptyPoints(game.lastAction, game);
    auto roundPoints = getTwoRoundPoints(game.lastAction, game.lastLastAction, game);
    //我方长5
    auto currentWinnerMoves = getWinningMoves(game.currentPlayer, game, roundPoints);
    if (!currentWinnerMoves.empty()) {
        return make_tuple(true, currentWinnerMoves, " win move  ");
    }
    //防止对手长5
    auto otherWinnerMoves = getWinningMoves(game.getOtherPlayer(), game, nearPoints);
    if (!otherWinnerMoves.empty()) {
        return make_tuple(false, otherWinnerMoves, " defence 5");
    }
    //我方活4
    auto activeFourMoves = getActiveFourMoves(game.currentPlayer, game, roundPoints);
    if (!activeFourMoves.empty()) {
        return make_tuple(true, activeFourMoves, "  active 4 ");
    }
    //对方有两个以上活4
    auto otherActiveFourMoves = getActiveFourMoves(3 - game.currentPlayer, game, roundPoints);
    if (otherActiveFourMoves.size() > 1) {
        return make_tuple(true, otherActiveFourMoves, " defence active 4 ");
    }

    if (!vctMode) {
        //我方VCF点
        auto vcfResult = dfsVCF(game.currentPlayer, game.currentPlayer, game, Point(), Point());
        if (vcfResult.first) {
            return make_tuple(true, vcfResult.second, " VCF! ");
        }

        //防对方VCF点
        auto VCFDefenceMoves = getVCFDefenceMoves(game.currentPlayer, game);
        if (!VCFDefenceMoves.empty()) {
            return make_tuple(false, VCFDefenceMoves, " defence VCF ");
        }
    } else {
        int maxThreeCount = 1;
        int halfTimeLimit = timeLimit / 2;

        //我方VCT点
        auto vctResult = dfsVCTIter(game.currentPlayer, game, halfTimeLimit, realPlay);
        if (get<0>(vctResult)) {
            return make_tuple(true, get<1>(vctResult), " VCT! threeCount=" + to_string(get<2>(vctResult)));
        }

        //防对方VCT点
        auto vctDefenceResult = dfsVCTDefenceIter(game.currentPlayer, game, halfTimeLimit);
        if (get<0>(vctDefenceResult)) {
            return make_tuple(true, get<1>(vctDefenceResult),
                              " defence VCT threeCount=" + to_string(get<2>(vctDefenceResult)));
        }
    }

    return make_tuple(false, emptyPoints, "");
}