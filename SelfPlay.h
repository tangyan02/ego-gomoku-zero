#ifndef EGO_GOMOKU_ZERO_SELFPLAY_H
#define EGO_GOMOKU_ZERO_SELFPLAY_H

#include <torch/torch.h>
#include <iostream>
#include "MCTS.h"
#include "Network.h"
#include <random>
#include "Utils.h"
#include <iostream>
#include <fstream>

void printGame(Game &game, int action, std::vector<float> &action_probs, float temperature);

void recordSelfPlay(
        int numGames,
        int numSimulations,
        float temperatureDefault,
        float explorationFactor,
        const std::string& shard);

#endif //EGO_GOMOKU_ZERO_SELFPLAY_H
