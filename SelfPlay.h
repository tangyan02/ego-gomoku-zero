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
#include <memory>

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

void printGame(Game& game, Point action, float rate,
               const std::string &prefix, const std::string selectInfo);

void recordSelfPlay(
    int boardSize,
    Context *context,
    int numSimulation,
    float explorationFactor,
    int shard,
    Model* sharedModel);

Game randomGame(Game& game, const std::string& prefix = "");

#endif //EGO_GOMOKU_ZERO_SELFPLAY_H
