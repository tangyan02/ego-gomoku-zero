#ifndef EGO_GOMOKU_ZERO_SELFPLAY_H
#define EGO_GOMOKU_ZERO_SELFPLAY_H

#include <iostream>
#include "MCTS.h"
#include "Model.h"
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <iomanip>
#include "Pruner.h"

void printGame(Game &game, int action, std::vector<float> &action_probs, float temperature);

void recordSelfPlay(
        int boardSize,
        int numGames,
        int numSimulations,
        float temperatureDefault,
        float explorationFactor,
        const std::string& shard);

Game randomGame(Game &game);

#endif //EGO_GOMOKU_ZERO_SELFPLAY_H
