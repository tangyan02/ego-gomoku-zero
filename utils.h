//
// Created by 唐雁 on 2024/1/17.
//

#ifndef EGO_GOMOKU_ZERO_UTILS_H
#define EGO_GOMOKU_ZERO_UTILS_H

#include <torch/torch.h>
#include <torch/nn.h>
#include <torch/optim.h>
#include <fstream>

torch::Device getDevice();

bool fileExists(const std::string &filePath);

#endif //EGO_GOMOKU_ZERO_UTILS_H
