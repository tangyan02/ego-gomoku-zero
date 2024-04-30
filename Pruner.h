

#ifndef EGO_GOMOKU_ZERO_PRUNER_H
#define EGO_GOMOKU_ZERO_PRUNER_H

#include "MCTS.h"
#include "Analyzer.h"
#include "array"

void pruning(Node &node, Game &game, int timeLimit);

#endif //EGO_GOMOKU_ZERO_PRUNER_H
