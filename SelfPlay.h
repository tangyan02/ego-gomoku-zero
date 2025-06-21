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
#include <cstdlib>
#include <ctime>
#include <atomic>

class Context
{
public:
    std::atomic<int> counter = atomic(0);
    int max;

    explicit Context(int max)
    {
        this->max = max;
    }
};

void printGame(Game& game, int action, std::vector<float>& action_probs, float temperature);

void recordSelfPlay(
    int boardSize,
    Context *context,
    int numSimulation,
    float temperatureDefault,
    float explorationFactor,
    int shard);

Game randomGame(Game& game);

#endif //EGO_GOMOKU_ZERO_SELFPLAY_H
