
#ifndef EGO_GOMOKU_ZERO_ANALYZER_H
#define EGO_GOMOKU_ZERO_ANALYZER_H

#include "Game.h"

std::vector<Point> selectActions(Game &game);

std::vector<Point> getWinningMoves(int player, Game &game, std::vector<Point> &basedMoves);

std::vector<Point> getActiveFourMoves(int player, Game &game, std::vector<Point> &basedMoves);

#endif //EGO_GOMOKU_ZERO_ANALYZER_H
