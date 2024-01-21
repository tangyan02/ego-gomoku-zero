#include "Network.h"

torch::jit::Module getNetwork(torch::Device device) {
    std::string path = "model/net_latest.mdl.pt";
    auto model = torch::jit::load(path);
    std::cout << "模型" << path << "已加载" << endl;
    return model;
}