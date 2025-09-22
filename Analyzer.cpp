
#include "Analyzer.h"
#include <atomic>
#include <chrono>

#include "ConfigReader.h"

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

// 创建一个函数来查找特定的点
bool existPoints(const std::vector<Point> &moves, const Point &target) {
    for (const auto &item: moves) {
        if (item.x == target.x && item.y == target.y) {
            return true;
        }
    }
    return false;
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

std::vector<Point> getNearByEmptyPoints(Point action, Game &game, int range) {
    std::vector<Point> empty_points;
    empty_points.reserve(200);
    if (!action.isNull()) {
        int last_row = action.x;
        int last_col = action.y;
        for (int i = 0; i < 8; i++) {
            int x = dx[i];
            int y = dy[i];
            for (int k = 1; k <= range; k++) {
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

vector<Point> getTwoShapeMoves(int player, Game &game, std::vector<Point> &basedMoves, Shape shape1, Shape shape2) {
    std::vector<Point> result;
    for (const auto &point: basedMoves) {
        if (game.board[point.x][point.y] != 0) {
            continue;
        }
        bool shape1Exist = false;
        bool shape2Exist = false;
        for (int i = 0; i < 4; i++) {
            auto action = point;
            if (!shape1Exist) {
                if (checkPointDirectShape(game, player, action, i, shape1)) {
                    shape1Exist = true;
                    continue;
                }
            }
            if (!shape2Exist) {
                if (checkPointDirectShape(game, player, action, i, shape2)) {
                    shape2Exist = true;
                    continue;
                }
            }
        }
        if (shape1Exist && shape2Exist) {
            result.emplace_back(point);
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

std::vector<Point>
getQuickWinMoves(int player, Game &game, vector<Point> &myBasedMoves) {
    //对方没有长5，我方有双4的情况
    auto doubleFourMoves = getTwoShapeMoves(player, game, myBasedMoves, SLEEPY_FOUR, SLEEPY_FOUR);
    if (!doubleFourMoves.empty()) {
        return doubleFourMoves;
    }

    //对方没有长4，我方有34或者33的情况
    auto rangeMove2 = game.getNearEmptyPoints(2);
    auto oppActiveFourMoves = getActiveFourMoves(3 - player, game, rangeMove2);
    auto oppSleepyFourMoves = getSleepyFourMoves(3 - player, game, rangeMove2);
    if (!oppActiveFourMoves.empty() || !oppSleepyFourMoves.empty()) {
        return {};
    }

    //由于对方防守形成不了长4，因此我方下一步不会被动防守，可利用另一个活3来取胜。
    auto threeFourMoves = getTwoShapeMoves(player, game, myBasedMoves, SLEEPY_FOUR, ACTIVE_THREE);
    if (!threeFourMoves.empty()) {
        return threeFourMoves;
    }

    auto doubleThreeMoves = getTwoShapeMoves(player, game, myBasedMoves, ACTIVE_THREE, ACTIVE_THREE);
    if (!doubleThreeMoves.empty()) {
        return doubleThreeMoves;
    }

    return {};
}

vector<Point> getVCFDefenceMoves(Game &game, std::vector<Point> &basedMoves) {
    //如果对手有VCF点，考虑对手的进攻点，和我防防守点形成进攻点
    auto otherVCFMoves = game.getOppVCFMoves();
    std::vector<Point> defenceMoves;
    if (!otherVCFMoves.empty()) {

        //记录所有VCF进攻点和防守点
        vector<Point> vcfPoints;
        for (const auto &item: game.oppVcfAttackMoves) {
            if (existPoints(basedMoves, item)) {
                vcfPoints.emplace_back(item);
            }
        }
        for (const auto &item: game.oppVcfDefenceMoves) {
            if (existPoints(basedMoves, item)) {
                vcfPoints.emplace_back(item);
            }
        }
        defenceMoves.insert(defenceMoves.end(),
                            vcfPoints.begin(),
                            vcfPoints.end());

        //假设防御点都下了，再找冲4和长5点
        for (const auto &item: game.oppVcfDefenceMoves) {
            game.board[item.x][item.y] = game.currentPlayer;
        }

//        game.printBoard();

        //计算新的可选点，但要在原来的范围内
        auto nearsNew = game.getEmptyPoints();
        vector<Point> nearsInRange;
        for (const auto &item: nearsNew) {
            if (existPoints(basedMoves, item)) {
                nearsInRange.emplace_back(item);
            }
        }

        auto mySleepFourMoves_more = getSleepyFourMoves(game.currentPlayer, game, nearsInRange);
        auto myActiveFourMoves_more = getActiveFourMoves(game.currentPlayer, game, nearsInRange);
        auto myFiveMoves_more = getWinningMoves(game.currentPlayer, game, nearsInRange);


        defenceMoves.insert(defenceMoves.end(), myActiveFourMoves_more.begin(), myActiveFourMoves_more.end());
        defenceMoves.insert(defenceMoves.end(), mySleepFourMoves_more.begin(), mySleepFourMoves_more.end());
        defenceMoves.insert(defenceMoves.end(), myFiveMoves_more.begin(), myFiveMoves_more.end());


        for (const auto &item: game.oppVcfDefenceMoves) {
            game.board[item.x][item.y] = 0;
        }
    }
    return removeDuplicates(defenceMoves);
}

vector<Point> getThreeDefenceMoves(int player, Game &game, std::vector<Point> &basedMoves, bool onlyDefence) {
    //自己的冲4点，和对手的活4。再从对手眠4里面选.如果下了之后活4活眠4没有了，则视为防御点
    std::vector<Point> defenceMoves;
    auto otherActiveFourMoves = getActiveFourMoves(3 - player, game, basedMoves);
    if (otherActiveFourMoves.size() >= 1) {
        //对手的冲4点
        auto otherSleepyFourMoves = getSleepyFourMoves(3 - player, game, basedMoves);
        for (const auto &item: otherSleepyFourMoves) {
            game.board[item.x][item.y] = player;
            auto nextOtherActiveFourMoves = getActiveFourMoves(3 - player, game, otherActiveFourMoves);
            if (nextOtherActiveFourMoves.empty()) {
                defenceMoves.emplace_back(item);
            }
            game.board[item.x][item.y] = 0;
        }

        //对手活4点
        defenceMoves.insert(defenceMoves.end(), otherActiveFourMoves.begin(), otherActiveFourMoves.end());

        //自己的眠4点
        if (!onlyDefence) {
            auto sleepyFourMoves = getSleepyFourMoves(player, game, basedMoves);
            defenceMoves.insert(defenceMoves.end(), sleepyFourMoves.begin(), sleepyFourMoves.end());
        }
    }
    return removeDuplicates(defenceMoves);
}

std::pair<bool, std::vector<Point>>
dfsVCF(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, int level,
       vector<Point> *attackPoints, vector<Point> *defencePoints, vector<Point> *allAttackPoints,
       bool checkDoubleThree) {
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
        if (oppNearMoves.empty()) {
            oppNearMoves = game.getEmptyPoints();
        }
//        printVector(oppNearMoves);

        auto winMoves = getWinningMoves(currentPlayer, game, nearMoves);
        auto oppWinMoves = getWinningMoves(3 - currentPlayer, game, oppNearMoves);
        auto activeMoves = getActiveFourMoves(currentPlayer, game, nearMoves);
        auto sleepMoves = getSleepyFourMoves(currentPlayer, game, nearMoves);

        //我方胜利点
        if (!winMoves.empty()) {
            return std::make_pair(true, winMoves);
        }

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

            if (activeMoves.empty()) {
                auto fourMoves = getSleepyFourMoves(currentPlayer, game, nearMoves);
                moves.insert(moves.end(), fourMoves.begin(), fourMoves.end());
            } else {
                //活4
                return std::make_pair(true, activeMoves);
            }

            if (moves.empty()) {
                return std::make_pair(false, std::vector<Point>());
            }
        }

        //记录所有进攻点
        if (allAttackPoints != nullptr) {
            allAttackPoints->insert(allAttackPoints->end(), moves.begin(), moves.end());
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
            //记录防守
            if (defencePoints != nullptr) {
                defencePoints->insert(defencePoints->end(), oppWinMoves.begin(), oppWinMoves.end());
            }
            return std::make_pair(true, oppWinMoves);
        }
    }

    moves = removeDuplicates(moves);

    bool finalResult = false;
    std::vector<Point> winMoves;
    for (const auto &item: moves) {
        game.board[item.x][item.y] = currentPlayer;
        auto dfsResult = dfsVCF(checkPlayer, 3 - currentPlayer,
                                game, item, lastMove,
                                level + 1, attackPoints, defencePoints, allAttackPoints,
                                checkDoubleThree);
        if (dfsResult.first) {
            if (attack) {
                //记录进攻点
                if (attackPoints != nullptr) {
                    attackPoints->emplace_back(item);
                }
            } else {
                //记录防守
                if (defencePoints != nullptr) {
                    defencePoints->emplace_back(item);
                }
            }
            finalResult = true;
            winMoves.emplace_back(item);
            game.board[item.x][item.y] = 0;
            return std::make_pair(true, winMoves);
        }
        game.board[item.x][item.y] = 0;
    }

    return std::make_pair(finalResult, winMoves);
}


std::pair<int, std::vector<Point>> dfsVCTIter(int currentPlayer, Game game, atomic<bool>& running) {
    int maxLevel = 30;
    int level = 2;
    for (; level <= maxLevel; level += 2) {
        // cout << level << endl;
        auto result = dfsVCT(currentPlayer, currentPlayer, game, running,
                             Point(), Point(), Point(),
                             false, 0, 0, 99, level);
        if (result.first) {
            return make_pair(level, result.second);
        }
//        cout << "level=" << to_string(level) << endl;
    }
    // cout << "dfsVCTIter finish" << endl;
    return std::make_pair(level, std::vector<Point>());
}

std::pair<int, std::vector<Point>> dfsVCTIter(int currentPlayer, Game* game, atomic<bool>& running)
{
    if (ConfigReader::get("useVct") != "true")
    {
        int level = 0;
        vector<Point> moves;
        return tie(level, moves);
    }
    return dfsVCTIter(currentPlayer, *game, running);
}

std::pair<bool, std::vector<Point>>
dfsVCT(int checkPlayer, int currentPlayer, Game &game, atomic<bool>& running, Point lastMove, Point lastLastMove, Point attackPoint,
       bool fourMode, int level, int threeCount, int maxThreeCount, int maxLevel) {
    if (!running.load()) {
        return make_pair(false, std::vector<Point>());
    }
//    cout << "level " << level << endl;
    if (level > maxLevel) {
//        cout << "超出层数" << endl;
        return std::make_pair(false, std::vector<Point>());
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

    std::vector<Point> nearMoves4;
    std::vector<Point> nearMoves3;
    if (lastLastMove.isNull()) {
        nearMoves4 = game.getEmptyPoints();
        nearMoves3 = game.getEmptyPoints();
    } else {
        if (attack) {
            nearMoves4 = getNearByEmptyPoints(lastLastMove, game, 4);
            nearMoves3 = getNearByEmptyPoints(lastLastMove, game, 3);
            std::vector<Point> moves3;
        } else {
            nearMoves4 = getNearByEmptyPoints(lastMove, game, 4);
            nearMoves3 = getNearByEmptyPoints(lastMove, game, 3);

//            nearMoves4 = game.getNearEmptyPoints();
//            nearMoves3 = game.getNearEmptyPoints();
        }
    }

    // 其实还是要考虑的，防守端计算会让局面更完整
    auto attackNearMoves4 = getNearByEmptyPoints(attackPoint, game, 4);
    auto attackNearMoves3 = getNearByEmptyPoints(attackPoint, game, 3);
    nearMoves4.insert(nearMoves4.end(), attackNearMoves4.begin(), attackNearMoves4.end());
    nearMoves3.insert(nearMoves3.end(), attackNearMoves3.begin(), attackNearMoves3.end());

    nearMoves4 = removeDuplicates(nearMoves4);
    nearMoves3 = removeDuplicates(nearMoves3);

    if (attack) {
        vector<Point> oppNearMoves = getNearByEmptyPoints(lastMove, game, 4);
        if (oppNearMoves.empty()) {
            oppNearMoves = game.getEmptyPoints();
        }

        auto winMoves = getWinningMoves(currentPlayer, game, nearMoves4);
        auto oppWinMoves = getWinningMoves(3 - currentPlayer, game, oppNearMoves);
        auto activeMoves = getActiveFourMoves(currentPlayer, game, nearMoves4);
        auto sleepMoves = getSleepyFourMoves(currentPlayer, game, nearMoves4);

        //我方胜利点
        if (!winMoves.empty()) {
            return std::make_pair(true, winMoves);
        }

        //如果对方有2个胜利点，则失败
        if (oppWinMoves.size() > 1) {
//            cout << "对方2个胜利点" << endl;
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

            //快速胜利
//            auto quickWinMove = getQuickWinMoves(currentPlayer, game, nearMoves4);
//            if (!quickWinMove.empty()) {
////                cout << "发现快速胜利" << endl;
//                return std::make_pair(true, quickWinMove);
//            }

            //活3模式的情形
            if (!fourMode) {
                //活3接活3，活3接长4
                auto activeThree = getActiveThreeMoves(currentPlayer, game, nearMoves3);
                moves.insert(moves.end(), activeThree.begin(), activeThree.end());
            }

            //长4的情形
            auto fourMoves = getSleepyFourMoves(currentPlayer, game, nearMoves4);
            moves.insert(moves.end(), fourMoves.begin(), fourMoves.end());
        }
    } else {

//        cout << "正在计算防御点" << endl;
        auto oppWinMoves = getWinningMoves(checkPlayer, game, nearMoves4);
        //防守眠4
        if (oppWinMoves.size() == 1) {
            moves.emplace_back(oppWinMoves[0]);
        }
        //认输
        if (oppWinMoves.size() > 1) {
            return std::make_pair(true, oppWinMoves);
        }

        if (oppWinMoves.empty()) {
            auto allMoves = game.getEmptyPoints();
            //如果有活4，防守成功
            auto activeMoves = getActiveFourMoves(currentPlayer, game, allMoves);
            if (!activeMoves.empty()) {
//                cout << "防守方有活4" << endl;
                return std::make_pair(false, vector<Point>());
            }

            //如果有VCF则防守成功
            auto myVCMoves = dfsVCF(currentPlayer, currentPlayer, game, Point(), Point());
            if (myVCMoves.first) {
//                cout << "防守方有VCF" << endl;
                return std::make_pair(false, vector<Point>());
            }

            if (!fourMode) {
                if (oppWinMoves.empty()) {
                    //防活3
//                cout << "正在计算防活3" << endl;
                    auto threeDefenceMoves = getThreeDefenceMoves(currentPlayer, game, nearMoves4);
                    moves.insert(moves.end(), threeDefenceMoves.begin(), threeDefenceMoves.end());
                }
            }
        }
    }

    if (moves.empty()) {
//        cout << "attack " << attack << "没有可移动的点了" << endl;
        return std::make_pair(false, std::vector<Point>());
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
        auto dfsResult = dfsVCT(checkPlayer, 3 - currentPlayer, game,running,
            item, lastMove, nextAttackMove,fourMode, level + 1, threeCount, maxThreeCount, maxLevel);

        if (attack) {
            if (dfsResult.first) {
                finalResult = true;
                winMoves.emplace_back(item);

                game.board[item.x][item.y] = 0;
                return std::make_pair(true, winMoves);
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

    vector<Point> emptyPoints;
    if(game.lastAction == Point()) {
        emptyPoints = game.getAllEmptyPoints();
    } else {
        int range = 2;
        // if (game.historyMoves.size() <= 6) {
        //     range = 3;
        // }
        // if (game.historyMoves.size() <= 3) {
        //     range = 4;
        // }
        emptyPoints = game.getNearEmptyPoints(range);
    }

    //我方长5
    auto currentWinnerMoves = getWinningMoves(game.currentPlayer, game, emptyPoints);
    if (!currentWinnerMoves.empty()) {
        return make_tuple(true, currentWinnerMoves, " win move");
    }

    //防止对手长5
    auto otherWinnerMoves = getWinningMoves(game.getOtherPlayer(), game, emptyPoints);
    if (!otherWinnerMoves.empty()) {
        return make_tuple(false, otherWinnerMoves, " defence 5");
    }

    //我方活4
    auto activeFourMoves = getActiveFourMoves(game.currentPlayer, game, emptyPoints);
    if (!activeFourMoves.empty()) {
        return make_tuple(true, activeFourMoves, " active 4");
    }

    //快速胜利
    // auto quickWinMove = getQuickWinMoves(game.currentPlayer, game, emptyPoints);
    // if (!quickWinMove.empty()) {
    //     return make_tuple(true, quickWinMove, " quick win");
    // }

    //我方VCF点
    auto myVCFMoves = game.getMyVCFMoves();
    if (!myVCFMoves.empty()) {
        return make_tuple(true, myVCFMoves, " VCF!");
    }

    //防御活4点
    auto threeDefenceMoves = getThreeDefenceMoves(game.currentPlayer, game, emptyPoints);
    if (!threeDefenceMoves.empty()) {
        return make_tuple(false, threeDefenceMoves, " defence 3");
    }

    string msg;

//    //对方VCF点判断
//    auto otherVCFMoves = game.getOppVCFMoves();
//    if (!otherVCFMoves.empty()) {
//        msg += " Opp VCF!";
//    }

    return make_tuple(false, emptyPoints, msg);
}