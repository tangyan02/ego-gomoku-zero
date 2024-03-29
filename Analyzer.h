
#ifndef EGO_GOMOKU_ZERO_ANALYZER_H
#define EGO_GOMOKU_ZERO_ANALYZER_H

#include "Game.h"
#include "Utils.h"
#include <algorithm>
#include "Shape.h"

using namespace std;

tuple<bool, vector<Point>, string> selectActions(Game &game);

vector<Point> getWinningMoves(int player, Game &game, vector<Point> &basedMoves);

vector<Point> getActiveFourMoves(int player, Game &game, vector<Point> &basedMoves);

vector<Point> getSleepyFourMoves(int player, Game &game, vector<Point> &basedMoves);

vector<Point> getActiveThreeMoves(int player, Game &game, vector<Point> &basedMoves);

vector<Point> getSleepyThreeMoves(int player, Game &game, vector<Point> &basedMoves);

vector<Point> getActiveTwoMoves(int player, Game &game, vector<Point> &basedMoves);

vector<Point> getSleepyTwoMoves(int player, Game &game, vector<Point> &basedMoves);

pair<bool, vector<Point>>
dfsVCF(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, int level = 0,
       vector<Point> *defencePoints = nullptr);

vector<Point> getThreeDefenceMoves(Game &game, vector<Point> &basedMoves);

vector<Point> getNearByEmptyPoints(Point action, Game &game);

vector<Point> getLineEmptyPoints(Point action, Game &game, int direct);

#endif //EGO_GOMOKU_ZERO_ANALYZER_H
