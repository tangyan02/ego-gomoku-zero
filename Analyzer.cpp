
#include "Analyzer.h"

static int dx[8] = {0, 0, 1, -1, 1, 1, -1, -1};
static int dy[8] = {1, -1, 0, 0, 1, -1, 1, -1};

std::vector<Point> getNearByEmptyPoints(Point action, Game &game) {
    std::vector <Point> empty_points;
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

std::pair<bool, std::vector<Point>>
dfsVCF(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, int level) {
    if (level > 30) {
        cout << " level error " << level << endl;
        game.printBoard();
    }
//    std::cout << "===" << std::endl;
//    game.printBoard();
//    std::cout << "===" << std::endl;
    std::vector<Point> moves;
    bool attack = checkPlayer == currentPlayer;

    std::vector<Point> nearMoves;
    if (lastLastMove.isNull()) {
        nearMoves = game.getEmptyPoints();
    } else {
        if(attack) {
            nearMoves = getNearByEmptyPoints(lastLastMove, game);
        }else{
            nearMoves = getNearByEmptyPoints(lastMove, game);
        }
    }


    if (attack) {
        auto oppNearMoves = getNearByEmptyPoints(lastMove, game);
        auto oppWinMoves = getWinningMoves(3 - currentPlayer, game, oppNearMoves);
        if (!oppWinMoves.empty()) {
            return std::make_pair(false, std::vector<Point>());
        }

        auto activeMoves = getActiveFourMoves(currentPlayer, game, nearMoves);
        if (!activeMoves.empty()) {
            return std::make_pair(true, activeMoves);
        }

        auto sleepMoves = getSleepyFourMoves(currentPlayer, game, nearMoves);
        moves.insert(moves.end(), sleepMoves.begin(), sleepMoves.end());

        if (moves.empty()) {
            return std::make_pair(false, std::vector<Point>());
        }
    } else {
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

std::vector<Point> selectActions(Game &game) {
    auto emptyPoints = game.getEmptyPoints();
    auto nearPoints = getNearByEmptyPoints(game.lastAction, game);
    auto roundPoints = getTwoRoundPoints(game.lastAction, game.lastLastAction, game);
    //我方长5
    auto currentWinnerMoves = getWinningMoves(game.currentPlayer, game, roundPoints);
    if (!currentWinnerMoves.empty()) {
        return currentWinnerMoves;
    }
    //防止对手长5
    auto otherWinnerMoves = getWinningMoves(game.getOtherPlayer(), game, nearPoints);
    if (!otherWinnerMoves.empty()) {
        return otherWinnerMoves;
    }
    //我方活4
    auto activeFourMoves = getActiveFourMoves(game.currentPlayer, game, roundPoints);
    if (!activeFourMoves.empty()) {
        return activeFourMoves;
    }
    //我方VCF点
    auto vcfResult = dfsVCF(game.currentPlayer, game.currentPlayer, game, Point(), Point());
    if (vcfResult.first) {
        return vcfResult.second;
    }
    //防对方VCF点
    auto VCFDefenceMoves = getVCFDefenceMoves(game.currentPlayer, game);
    if (!VCFDefenceMoves.empty()) {
        return VCFDefenceMoves;
    }

    //防活3
//    auto threeDefenceMoves = getThreeDefenceMoves(game.currentPlayer, game, nearPoints);
//    if (!threeDefenceMoves.empty()) {
//        return threeDefenceMoves;
//    }

    return emptyPoints;
}