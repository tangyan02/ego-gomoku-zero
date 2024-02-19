
#ifndef EGO_GOMOKU_ZERO_ANALYZER_H
#define EGO_GOMOKU_ZERO_ANALYZER_H

#include "Game.h"
#include "Utils.h"
#include <algorithm>

using namespace std;

tuple<bool, vector<Point>, string> selectActions(Game &game, bool vctMode = false, int timeLimit = 3000);

std::vector<Point> getWinningMoves(int player, Game &game, std::vector<Point> &basedMoves);

std::vector<Point> getActiveFourMoves(int player, Game &game, std::vector<Point> &basedMoves);

std::vector<Point> getSleepyFourMoves(int player, Game &game, std::vector<Point> &basedMoves);

std::vector<Point> getThreeDefenceMoves(int player, Game &game);

std::pair<bool, std::vector<Point>>
dfsVCF(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, int level = 0);

std::pair<bool, std::vector<Point>>
dfsVCT(int checkPlayer, int currentPlayer, Game &game, Point lastMove, Point lastLastMove, Point attackPoint,
       bool fourMode, int &nodeRecord, int level = 0, int threeCount = 0, int maxThreeCount = 5, long long timeout = 0);

tuple<bool, vector<Point>, int> dfsVCTIter(int player, Game &game, int timeLimit = 0);

std::vector<Point> getVCFDefenceMoves(int player, Game &game);

std::vector<Point> getVCTDefenceMoves(int player, Game &game, int &levelResult, int timeLimit = 0);

std::vector<Point> getNearByEmptyPoints(Point action, Game &game);

#endif //EGO_GOMOKU_ZERO_ANALYZER_H
