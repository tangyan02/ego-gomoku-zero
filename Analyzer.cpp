
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
            if (checkPointDirectShape(game, player, action, i, shape1)) {
                shape1Exist = true;
                continue;
            }
            if (checkPointDirectShape(game, player, action, i, shape2)) {
                shape2Exist = true;
                continue;
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
getThreeFourWinMoves(int player, Game &game, vector<Point> &myBasedMoves, vector<Point> &oppBasedMoves) {
    auto result = getTwoShapeMoves(player, game, myBasedMoves, SLEEPY_FOUR, ACTIVE_THREE);
    auto oppSleepFour = getSleepyFourMoves(3 - player, game, oppBasedMoves);
    auto oppActiveFour = getActiveFourMoves(3 - player, game, oppBasedMoves);
    if (oppSleepFour.empty() && oppActiveFour.empty()) {
        return result;
    }
    return {};
}

std::vector<Point>
getDoubleThreeWinMoves(int player, Game &game, vector<Point> &myBasedMoves, vector<Point> &oppBasedMoves) {
    auto oppActiveFourMoves = getActiveFourMoves(3 - player, game, oppBasedMoves);
    auto oppSleepyFourMoves = getSleepyFourMoves(3 - player, game, oppBasedMoves);
    auto doubleThreeMoves = getTwoShapeMoves(player, game, myBasedMoves, ACTIVE_THREE, ACTIVE_THREE);
    if (oppActiveFourMoves.empty() && oppSleepyFourMoves.empty() && !doubleThreeMoves.empty()) {
        return doubleThreeMoves;
    }
    return {};
}

std::vector<Point>
getDoubleFourWinMoves(int player, Game &game, vector<Point> &myBasedMoves) {
    return getTwoShapeMoves(player, game, myBasedMoves, SLEEPY_FOUR, SLEEPY_FOUR);
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
       vector<Point> *attackPoints, vector<Point> *defencePoints, vector<Point> *allAttackPoints) {
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

            if (activeMoves.empty()) {
                //没有活4就冲4
                auto fourMoves = getTwoShapeMoves(currentPlayer, game, nearMoves, SLEEPY_FOUR, SLEEPY_THREE);
                auto fourMoreMoves = getShapeMoves(currentPlayer, game, nearMoves, SLEEPY_FOUR_MORE);
                moves.insert(moves.end(), fourMoves.begin(), fourMoves.end());
                moves.insert(moves.end(), fourMoreMoves.begin(), fourMoreMoves.end());
            } else {
                //活4
                moves.insert(moves.end(), activeMoves.begin(), activeMoves.end());
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
                                level + 1, attackPoints, defencePoints, allAttackPoints);
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
            if (level > 0) {
                game.board[item.x][item.y] = 0;
                return std::make_pair(true, winMoves);
            }
        }
        game.board[item.x][item.y] = 0;
    }

    return std::make_pair(finalResult, winMoves);
}

std::pair<int, std::vector<Point>>
dfsVCTIter(int checkPlayer, int currentPlayer, Game &game) {
    for (int level = 1; level <= 2; level += 1) {
        auto result = dfsVCT(checkPlayer, currentPlayer, game,
                             Point(), Point(), Point(),
                             false, 0, 0, level, level * 4, 0);
        if (result.first) {
            return make_pair(level, result.second);
        }
    }
    return std::make_pair(0, std::vector<Point>());
}

std::pair<bool, std::vector<Point>>
dfsVCT(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, Point attackPoint,
       bool fourMode, int level, int threeCount, int maxThreeCount, int maxLevel, long long timeOutTime) {
    if (timeOutTime > 0) {
        if (getSystemTime() > timeOutTime) {
            return std::make_pair(false, std::vector<Point>());
        }
    }

    if (level > maxLevel) {
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

    std::vector<Point> nearMoves;
    std::vector<Point> nearMoves3;
    if (lastLastMove.isNull()) {
        nearMoves = game.getEmptyPoints();
        nearMoves3 = game.getEmptyPoints();
    } else {
        if (attack) {
            nearMoves = getNearByEmptyPoints(lastLastMove, game, 4);
            nearMoves3 = getNearByEmptyPoints(lastLastMove, game, 3);
            std::vector<Point> moves3;
        } else {
//            nearMoves = getNearByEmptyPoints(lastMove, game, 4);
//            nearMoves3 = getNearByEmptyPoints(lastMove, game, 3)

            nearMoves = game.getNearEmptyPoints();
            nearMoves3 = game.getNearEmptyPoints();
        }
    }

    // 其实还是要考虑的，防守端计算会让局面更完整
    auto attackNearMoves = getNearByEmptyPoints(attackPoint, game, 4);
    auto attackNearMoves3 = getNearByEmptyPoints(attackPoint, game, 3);
    nearMoves.insert(nearMoves.end(), attackNearMoves.begin(), attackNearMoves.end());
    nearMoves3.insert(nearMoves3.end(), attackNearMoves.begin(), attackNearMoves.end());

    nearMoves = removeDuplicates(nearMoves);
    nearMoves3 = removeDuplicates(nearMoves3);

    if (attack) {
        auto oppNearMoves1 = game.getNearEmptyPoints(1);
        auto oppNearMoves2 = game.getNearEmptyPoints(2);

        auto oppWinMoves = getWinningMoves(3 - currentPlayer, game, oppNearMoves1);
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

            //如果有冲4活3也直接下
            auto threeFourMoves = getThreeFourWinMoves(currentPlayer, game, nearMoves, oppNearMoves2);
            if (!threeFourMoves.empty()) {
                return std::make_pair(true, threeFourMoves);
            }

            //如果有双冲4也直接下
            auto doubleFourMoves = getDoubleFourWinMoves(currentPlayer, game, nearMoves);
            if (!doubleFourMoves.empty()) {
                return std::make_pair(true, doubleFourMoves);
            }

            //如果有双3，对手没有冲4，直接胜利
            auto doubleThreeMoves = getDoubleThreeWinMoves(currentPlayer, game, nearMoves, oppNearMoves2);
            if (!doubleThreeMoves.empty()) {
                return std::make_pair(true, doubleThreeMoves);
            }

            //活3模式的情形
            if (!fourMode) {
                //活3接活3，活3接长4
                auto threeActiveMoves = getTwoShapeMoves(currentPlayer, game, nearMoves3, ACTIVE_THREE, ACTIVE_TWO);
                auto threeActiveMoves2 = getTwoShapeMoves(currentPlayer, game, nearMoves3, ACTIVE_THREE, SLEEPY_THREE);

                //如果对手有活4，则看防守点是否是长3点
                auto allEmptyMoves = game.getNearEmptyPoints(2);
                auto myThreeDefenceMoves = getThreeDefenceMoves(currentPlayer, game, allEmptyMoves);

//                cout << "threeActiveMoves ";
//                printVector(threeActiveMoves);
                if (myThreeDefenceMoves.empty()) {
                    moves.insert(moves.end(), threeActiveMoves.begin(), threeActiveMoves.end());
                    moves.insert(moves.end(), threeActiveMoves2.begin(), threeActiveMoves2.end());
                    threeCount++;
                } else {
                    //如果防御3点刚好也是活3点，则执行
//                    cout << "发现对手活3" << endl;
                    for (const auto &myThreeDefenceMove: myThreeDefenceMoves) {
                        if (existPoints(threeActiveMoves, myThreeDefenceMove)) {
                            moves.emplace_back(myThreeDefenceMove);
                        }
                        if (existPoints(threeActiveMoves2, myThreeDefenceMove)) {
                            moves.emplace_back(myThreeDefenceMove);
                        }
                    }
                }

                //长4接活3
                auto fourMoves = getTwoShapeMoves(currentPlayer, game, nearMoves, SLEEPY_FOUR, ACTIVE_TWO);
                moves.insert(moves.end(), fourMoves.begin(), fourMoves.end());
            }

            //长4的情形
            auto fourMoves = getTwoShapeMoves(currentPlayer, game, nearMoves, SLEEPY_FOUR, SLEEPY_THREE);
            auto fourMoreMoves = getShapeMoves(currentPlayer, game, nearMoves, SLEEPY_FOUR_MORE);
            moves.insert(moves.end(), fourMoves.begin(), fourMoves.end());
            moves.insert(moves.end(), fourMoreMoves.begin(), fourMoreMoves.end());
        }
    } else {

//        cout << "正在计算防御点" << endl;
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
//                cout << "正在计算防活3" << endl;
                auto threeDefenceMoves = getThreeDefenceMoves(currentPlayer, game, nearMoves);
                moves.insert(moves.end(), threeDefenceMoves.begin(), threeDefenceMoves.end());
            }
        }
    }

    if (moves.empty()) {
//        cout <<"attack "<< attack << "没有可移动的点了" << endl;
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
        auto dfsResult = dfsVCT(checkPlayer, 3 - currentPlayer, game, item, lastMove, nextAttackMove,
                                fourMode, level + 1, threeCount, maxThreeCount, maxLevel, timeOutTime);

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
    int range = 3;
    if (game.historyMoves.size() <= 3) {
        range = 4;
    }

    auto emptyPoints = game.getNearEmptyPoints(range);
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
    auto threeDefenceMoves = getThreeDefenceMoves(game.currentPlayer, game, emptyPoints);
    if (!threeDefenceMoves.empty()) {
        return make_tuple(false, threeDefenceMoves, "  defence 3 ");
    }

    //防御对方VCF点
    auto vcfDefenceMoves = getVCFDefenceMoves(game, emptyPoints);
    if (!vcfDefenceMoves.empty()) {
        return make_tuple(false, vcfDefenceMoves, "  defence VCF ");
    }

    //VCT点
    auto vctMoves = game.getMyVCTMoves();
    if (!vctMoves.empty()) {
        return make_tuple(true, vctMoves, " VCT! " + to_string(game.myVctLevel));
    }

    string msg;
    if (!game.getOppVCTMoves().empty()) {
        msg = " Opp VCT! " + to_string(game.oppVctLevel);
    }

    return make_tuple(false, emptyPoints, msg);
}