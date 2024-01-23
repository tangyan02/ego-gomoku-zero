
#include "Analyzer.h"

std::vector<Point> getNearByEmptyPoints(Point action, Game &game) {
    std::vector<Point> empty_points;
    if (action.x != -1 && action.y != -1) {
        int last_row = action.x;
        int last_col = action.y;
        for (int dx = -CONNECT; dx <= CONNECT; dx++) {
            for (int dy = -CONNECT; dy <= CONNECT; dy++) {
                int row = last_row + dx;
                int col = last_col + dy;
                if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && game.board[row][col] == 0) {
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

std::vector<Point> selectActions(Game &game) {
    auto emptyPoints = game.getEmptyPoints();
    auto roundPoints = getTwoRoundPoints(game.lastAction, game.lastLastAction, game);
    //我方长5
    auto currentWinnerMoves = getWinningMoves(game.currentPlayer, game, roundPoints);
    if (!currentWinnerMoves.empty()) {
        return currentWinnerMoves;
    }
    //防止对手长5
    auto otherWinnerMoves = getWinningMoves(game.getOtherPlayer(), game, roundPoints);
    if (!otherWinnerMoves.empty()) {
        return otherWinnerMoves;
    }
    //我方活4
    auto activeFourMoves = getActiveFourMoves(game.currentPlayer, game, roundPoints);
    if (!otherWinnerMoves.empty()) {
        return otherWinnerMoves;
    }

    return emptyPoints;
}