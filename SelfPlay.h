//
// Created by 唐雁 on 2024/1/16.
//

#ifndef EGO_GOMOKU_ZERO_SELFPLAY_H
#define EGO_GOMOKU_ZERO_SELFPLAY_H

#include <torch/torch.h>
#include <iostream>
#include "MCTS.h"
#include "Network.h"
#include <random>

void printGame(Game &game, int action, std::vector<float> &action_probs, float temperature);

std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> selfPlay(
        torch::Device device,
        int numGames,
        int numSimulations,
        float temperatureDefault,
        float explorationFactor);

std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> extendData(
        int boardSize, std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> &play_data);

#endif //EGO_GOMOKU_ZERO_SELFPLAY_H
