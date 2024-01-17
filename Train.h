#ifndef EGO_GOMOKU_ZERO_TRAIN_H
#define EGO_GOMOKU_ZERO_TRAIN_H

#include <iostream>
#include <vector>
#include <torch/torch.h>
#include <torch/nn.h>
#include <torch/optim.h>
#include <torch/data/dataloader.h>
#include <torch/data/datasets.h>
#include <torch/data/example.h>
#include <torch/types.h>
#include <torch/utils.h>

#include "Network.h"
#include "ReplayBuffer.h"
#include "SelfPlay.h"

using namespace torch;
using namespace torch::nn;
using namespace torch::optim;
using namespace torch::data;
using namespace torch::data::datasets;
using namespace torch::data::samplers;
using namespace std;


void train(ReplayBuffer& replay_buffer, const std::__1::shared_ptr<PolicyValueNetwork>& network, Device device, float lr, int num_epochs, int batch_size);

#endif //EGO_GOMOKU_ZERO_TRAIN_H
