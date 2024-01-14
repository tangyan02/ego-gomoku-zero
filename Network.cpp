#include <torch/torch.h>
#include <torch/nn.h>
#include <torch/optim.h>

class PolicyValueNetwork : public torch::nn::Module
{
public:
    PolicyValueNetwork() : board_size(15), input_channels(3)
    {
        // common layers
        conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(input_channels, 32, 7).padding(3)));
        conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(32, 64, 5).padding(2)));
        conv3 = register_module("conv3", torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 128, 3).padding(1)));
        // action policy layers
        act_conv1 = register_module("act_conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(128, 4, 1)));
        act_fc1 = register_module("act_fc1", torch::nn::Linear(4 * board_size * board_size, board_size * board_size));
        // state value layers
        val_conv1 = register_module("val_conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(128, 2, 1)));
        val_fc1 = register_module("val_fc1", torch::nn::Linear(2 * board_size * board_size, 64));
        val_fc2 = register_module("val_fc2", torch::nn::Linear(64, 1));
    }

    std::pair<torch::Tensor, torch::Tensor> forward(torch::Tensor state_input)
    {
        // common layers
        torch::Tensor x = torch::relu(conv1->forward(state_input));
        x = torch::relu(conv2->forward(x));
        x = torch::relu(conv3->forward(x));
        // action policy layers
        torch::Tensor x_act = torch::relu(act_conv1->forward(x));
        x_act = x_act.view({-1, 4 * board_size * board_size});
        x_act = torch::log_softmax(act_fc1->forward(x_act), /*dim=*/1);
        // state value layers
        torch::Tensor x_val = torch::relu(val_conv1->forward(x));
        x_val = x_val.view({-1, 2 * board_size * board_size});
        x_val = torch::relu(val_fc1->forward(x_val));
        x_val = torch::tanh(val_fc2->forward(x_val));

        return std::make_pair(x_val, x_act);
    }

private:
    int board_size;
    int input_channels;
    torch::nn::Conv2d conv1{nullptr}, conv2{nullptr}, conv3{nullptr};
    torch::nn::Conv2d act_conv1{nullptr};
    torch::nn::Linear act_fc1{nullptr};
    torch::nn::Conv2d val_conv1{nullptr};
    torch::nn::Linear val_fc1{nullptr}, val_fc2{nullptr};
};

torch::Device getDevice()
{
    if (torch::cuda::is_available())
    {
        return torch::kCUDA;
    }
    else
    {
        return torch::kCPU;
    }
}

std::__1::shared_ptr<PolicyValueNetwork> getNetwork(torch::Device device = getDevice())
{
    auto network = std::make_shared<PolicyValueNetwork>();

    std::string path = "../model/net_latest.mdl";
    if (std::filesystem::exists(path))
    {
        torch::load(network, path);
        network->to(device);
    }
    return network;
}

void saveNetwork(std::__1::shared_ptr<PolicyValueNetwork> network, const std::string &path = "../model/net_latest.mdl")
{
    torch::save(network, path);
}