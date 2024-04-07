
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


vector<Point> getDoubleThreeDefenceMoves(Game &game, std::vector<Point> &basedMoves) {
    std::vector<Point> defenceMoves;
    bool existDoubleThree = false;
    for (const auto &item: basedMoves) {
        auto action = item;
        //考虑到连续冲4可能打断双3，这种情况不需要特别防守
        for (const auto &atkPoint: game.myAllAttackMoves) {
            game.board[atkPoint.x][atkPoint.y] = game.currentPlayer;
        }
        int count = countPointShape(game, game.getOtherPlayer(), action, ACTIVE_THREE);
        for (const auto &atkPoint: game.myAllAttackMoves) {
            game.board[atkPoint.x][atkPoint.y] = 0;
        }
        if (count >= 2) {
            existDoubleThree = true;

            //能消除双3的点，则为可选点
            for (int k = 0; k < 4; k++) {
                auto lineMoves = getLineEmptyPoints(item, game, k);
                for (const auto &lineMove: lineMoves) {
                    //剔除中心点，因为形状check时不考虑中心点
                    if(lineMove.x == item.x && lineMove.y == lineMove.x){
                        continue;
                    }
                    game.board[lineMove.x][lineMove.y] = game.currentPlayer;
                    int countAfter = countPointShape(game, game.getOtherPlayer(), action, ACTIVE_THREE);
                    if (countAfter <= 1) {
                        defenceMoves.emplace_back(lineMove);
                    }
                    game.board[lineMove.x][lineMove.y] = 0;
                }
            }

            defenceMoves.emplace_back(item);
        }

    }

    //如果存在双3
    if (existDoubleThree) {
        //那么和活3，眠4，眠3也作为可选点
        auto mySleepFourMoves = getSleepyFourMoves(game.currentPlayer, game, basedMoves);
        auto myActiveThreeMoves = getActiveThreeMoves(game.currentPlayer, game, basedMoves);
        auto mySleepThreeMoves = getSleepyThreeMoves(game.currentPlayer, game, basedMoves);
        defenceMoves.insert(defenceMoves.end(), mySleepFourMoves.begin(), mySleepFourMoves.end());
        defenceMoves.insert(defenceMoves.end(), myActiveThreeMoves.begin(), myActiveThreeMoves.end());
        defenceMoves.insert(defenceMoves.end(), mySleepThreeMoves.begin(), mySleepThreeMoves.end());

        //假设VCF进攻点都下了，长5，活4,眠4,活3
        for (const auto &item: game.myAllAttackMoves) {
            game.board[item.x][item.y] = game.currentPlayer;
        }

        //计算新的可选点，但要在原来的范围内
        auto nearsNew = game.getEmptyPoints();
        vector<Point> nearsInRange;
        for (const auto &item: nearsNew) {
            if (existPoints(basedMoves, item)) {
                nearsInRange.emplace_back(item);
            }
        }

        auto myFiveMoves_more = getWinningMoves(game.currentPlayer, game, nearsInRange);
        auto myActiveFourMoves_more = getActiveFourMoves(game.currentPlayer, game, nearsInRange);
        auto myActiveThreeMoves_more = getActiveThreeMoves(game.currentPlayer, game, nearsInRange);
        auto mySleepFourMoves_more = getSleepyFourMoves(game.currentPlayer, game, nearsInRange);
        auto mySleepThreeMoves_more = getSleepyThreeMoves(game.currentPlayer, game, nearsInRange);

        defenceMoves.insert(defenceMoves.end(), myFiveMoves_more.begin(), myFiveMoves_more.end());
        defenceMoves.insert(defenceMoves.end(), myActiveFourMoves_more.begin(), myActiveFourMoves_more.end());
        defenceMoves.insert(defenceMoves.end(), myActiveThreeMoves_more.begin(), myActiveThreeMoves_more.end());
        defenceMoves.insert(defenceMoves.end(), mySleepFourMoves_more.begin(), mySleepFourMoves_more.end());
        defenceMoves.insert(defenceMoves.end(), mySleepThreeMoves_more.begin(), mySleepThreeMoves_more.end());

        for (const auto &item: game.myAllAttackMoves) {
            game.board[item.x][item.y] = 0;
        }
    }

    return removeDuplicates(defenceMoves);
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
                moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());
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

/**
 * 返回两个值，第一个值代表返回值是否是必胜点
 */
tuple<bool, vector<Point>, string> selectActions(Game &game) {
    int range = 2;
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
    auto threeDefenceMoves = getThreeDefenceMoves(game, emptyPoints);
    if (!threeDefenceMoves.empty()) {
        return make_tuple(false, threeDefenceMoves, "  defence 3 ");
    }

    //防御对方VCF点
    auto vcfDefenceMoves = getVCFDefenceMoves(game, emptyPoints);
    if (!vcfDefenceMoves.empty()) {
        return make_tuple(false, vcfDefenceMoves, "  defence VCF ");
    }

    //防御对方双3点
    //要考虑的情况有点多，由于担心计算有漏洞。还会去掉了
//    auto doubleThreeDefenceMoves = getDoubleThreeDefenceMoves(game, emptyPoints);
//    if (!doubleThreeDefenceMoves.empty()) {
//        return make_tuple(false, doubleThreeDefenceMoves, "  defence double 3 ");
//    }

    return make_tuple(false, emptyPoints, "");
}