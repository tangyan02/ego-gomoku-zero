
#ifndef EGO_GOMOKU_ZERO_ANALYZER_H
#define EGO_GOMOKU_ZERO_ANALYZER_H

#include "Game.h"
#include <algorithm>

using namespace std;

pair<bool, vector<Point>> selectActions(Game &game, bool vctMode = false);

std::vector<Point> getWinningMoves(int player, Game &game, std::vector<Point> &basedMoves);

std::vector<Point> getActiveFourMoves(int player, Game &game, std::vector<Point> &basedMoves);

std::vector<Point> getSleepyFourMoves(int player, Game &game, std::vector<Point> &basedMoves);

std::vector<Point> getThreeDefenceMoves(int player, Game &game);

std::pair<bool, std::vector<Point>>
dfsVCF(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, int level = 0);

std::pair<bool, std::vector<Point>>
dfsVCT(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, bool fourMode,
       int level = 0);

std::vector<Point> getVCFDefenceMoves(int player, Game &game);

std::vector<Point> getNearByEmptyPoints(Point action, Game &game);

#endif //EGO_GOMOKU_ZERO_ANALYZER_H
