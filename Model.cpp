
#include "Model.h"


static torch::jit::Module getNetwork(torch::Device device, std::string path = "model/model_latest.pt") {
    auto model = torch::jit::load(path, device);
    model.eval();
    torch::NoGradGuard no_grad;
    return model;
}

static torch::Device getDevice() {
    if (torch::cuda::is_available()) {
        return torch::kCUDA;
    } else {
        return torch::kCPU;
    }
}

Model::Model() : device(torch::kCPU) {
    device = getDevice();
}

void Model::init(string model_path) {
    network = getNetwork(device, model_path);
}

std::pair<float, std::vector<float>> Model::evaluate_state(vector<vector<vector<float>>> &data) {

    // 获取数据的维度
    int dim1 = data.size();
    int dim2 = data[0].size();
    int dim3 = data[0][0].size();

    // 将数据转换为一维数组
    std::vector<float> flattened_data(dim1 * dim2 * dim3);
    int index = 0;
    for (int i = 0; i < dim1; i++) {
        for (int j = 0; j < dim2; j++) {
            for (int k = 0; k < dim3; k++) {
                flattened_data[index++] = data[i][j][k];
            }
        }
    }

    // 创建张量并从一维数组中加载数据
    torch::Tensor tensor = torch::from_blob(flattened_data.data(), {dim1, dim2, dim3}).clone();

    torch::Tensor state_tensor = tensor.to(device).clone();
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(state_tensor);

    auto outputs = network.forward(inputs).toTuple();
    torch::Tensor value = outputs->elements()[0].toTensor();
    torch::Tensor policy = outputs->elements()[1].toTensor();

    auto valueFloat = value[0][0].cpu().item<float>();

    // 将张量转换为 CPU 上的张量
    torch::Tensor cpu_policy = torch::exp(policy).cpu();

    // 打平并转换为 std::vector<float>
    std::vector<float> prior_prob(cpu_policy.data<float>(), cpu_policy.data<float>() + cpu_policy.numel());

    return std::make_pair(valueFloat, prior_prob);

}
