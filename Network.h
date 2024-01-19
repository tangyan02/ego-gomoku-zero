#ifndef EGO_GOMOKU_ZERO_NETWORK_H
#define EGO_GOMOKU_ZERO_NETWORK_H

#include <torch/torch.h>
#include <torch/nn.h>
#include <torch/optim.h>
#include <fstream>
#include "utils.h"

using namespace std;

class PolicyValueNetwork : public torch::nn::Module {

public:
    PolicyValueNetwork();

    std::pair<torch::Tensor, torch::Tensor> forward(torch::Tensor &state_input);

private:
    int board_size;
    int input_channels;
    torch::nn::Conv2d conv1{nullptr}, conv2{nullptr}, conv3{nullptr};
    torch::nn::Conv2d act_conv1{nullptr};
    torch::nn::Linear act_fc1{nullptr};
    torch::nn::Conv2d val_conv1{nullptr};
    torch::nn::Linear val_fc1{nullptr}, val_fc2{nullptr};
};

std::shared_ptr<PolicyValueNetwork> getNetwork(torch::Device device = getDevice());

void saveNetwork(std::shared_ptr<PolicyValueNetwork> &network, const std::string &path = "model/net_latest.mdl");

#endif //EGO_GOMOKU_ZERO_NETWORK_H
