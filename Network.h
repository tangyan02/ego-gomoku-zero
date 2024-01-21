#ifndef EGO_GOMOKU_ZERO_NETWORK_H
#define EGO_GOMOKU_ZERO_NETWORK_H

#include <torch/torch.h>
#include <torch/nn.h>
#include <torch/optim.h>
#include <torch/script.h>
#include <fstream>
#include "Utils.h"

using namespace std;

torch::jit::Module getNetwork(torch::Device device);

#endif //EGO_GOMOKU_ZERO_NETWORK_H
