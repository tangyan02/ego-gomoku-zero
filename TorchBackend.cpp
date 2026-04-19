
#include "TorchBackend.h"
#include <iostream>
#include <torch/mps.h>

TorchBackend::TorchBackend() = default;

TorchBackend::~TorchBackend() {
    if (batchThread.joinable()) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        batchThread.join();
    }
}

void TorchBackend::init(const std::string& modelPath, const std::string& coreType) {
    this->modelBatchSize = 1;

    // 选择设备
    if (coreType == "apple" || coreType == "mps") {
        if (torch::mps::is_available()) {
            device = torch::Device(torch::kMPS);
            std::cout << "[TorchBackend] Using MPS device" << std::endl;
        } else {
            std::cout << "[TorchBackend] MPS not available, falling back to CPU" << std::endl;
            device = torch::Device(torch::kCPU);
        }
    } else if (coreType == "cuda") {
        // CUDA not available in macOS CPU-only libtorch build
        std::cout << "[TorchBackend] CUDA not available in this build, using CPU" << std::endl;
        device = torch::Device(torch::kCPU);
    } else {
        device = torch::Device(torch::kCPU);
        std::cout << "[TorchBackend] Using CPU device" << std::endl;
    }

    // 加载 TorchScript 模型（自动将 .onnx 路径替换为 .pt）
    std::string torchModelPath = modelPath;
    if (torchModelPath.size() > 5 && torchModelPath.substr(torchModelPath.size() - 5) == ".onnx") {
        torchModelPath = torchModelPath.substr(0, torchModelPath.size() - 5) + ".pt";
    }
    // 也检查 .pth -> .pt
    if (torchModelPath.size() > 4 && torchModelPath.substr(torchModelPath.size() - 4) == ".pth") {
        torchModelPath = torchModelPath.substr(0, torchModelPath.size() - 4) + ".pt";
    }

    try {
        module = torch::jit::load(torchModelPath, device);
        module.eval();
        loaded = true;
        std::cout << "[TorchBackend] Model loaded from: " << torchModelPath << std::endl;
    } catch (const c10::Error& e) {
        std::cerr << "[TorchBackend] Error loading model: " << e.what() << std::endl;
        throw std::runtime_error("Failed to load TorchScript model: " + torchModelPath);
    }

    // 启动批推理线程
    batchThread = std::thread(&TorchBackend::batchInference, this);
}

std::vector<std::pair<float, std::vector<float>>>
TorchBackend::evaluate_state_batch(const std::vector<std::vector<std::vector<std::vector<float>>>>& batchData) {
    int batch_size = batchData.size();
    if (batch_size == 0) {
        return {};
    }

    int dim1 = batchData[0].size();
    int dim2 = batchData[0][0].size();
    int dim3 = batchData[0][0][0].size();

    // 拼接成连续 float 数组
    std::vector<float> flattened_data(batch_size * dim1 * dim2 * dim3);
    int index = 0;
    for (const auto& data : batchData) {
        for (int i = 0; i < dim1; i++) {
            for (int j = 0; j < dim2; j++) {
                for (int k = 0; k < dim3; k++) {
                    flattened_data[index++] = data[i][j][k];
                }
            }
        }
    }

    // 先在 CPU 上创建 tensor，再转移到目标设备（MPS 不支持 from_blob 直接创建）
    auto input_tensor = torch::from_blob(
        flattened_data.data(), {batch_size, dim1, dim2, dim3},
        torch::TensorOptions().dtype(torch::kFloat32)).clone().to(device);

    // 前向推理
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input_tensor);

    torch::NoGradGuard no_grad;
    auto output = module.forward(inputs).toTuple();

    auto value_tensor = output->elements()[0].toTensor();  // [batch, 1]
    auto act_tensor = output->elements()[1].toTensor();     // [batch, 400]

    // 拷回 CPU
    auto value_cpu = value_tensor.to(torch::kCPU);
    auto act_cpu = act_tensor.to(torch::kCPU);

    // 解析输出
    std::vector<std::pair<float, std::vector<float>>> results;
    results.reserve(batch_size);

    int act_size = act_cpu.size(1);
    for (int b = 0; b < batch_size; b++) {
        float value = value_cpu[b].item<float>();
        std::vector<float> prior_prob(act_size);
        for (int i = 0; i < act_size; i++) {
            prior_prob[i] = exp(act_cpu[b][i].item<float>());
        }
        results.emplace_back(value, prior_prob);
    }

    return results;
}

std::pair<float, std::vector<float>>
TorchBackend::evaluate_state(std::vector<std::vector<std::vector<float>>>& data) {
    std::vector<std::vector<std::vector<std::vector<float>>>> batchData;
    batchData.emplace_back(data);
    return evaluate_state_batch(batchData)[0];
}

std::pair<float, std::vector<float>>
TorchBackend::evaluate_state(const float* data, int channels, int height, int width) {
    int totalSize = channels * height * width;

    // 先在 CPU 上创建 tensor，再转移到目标设备
    auto input_tensor = torch::from_blob(
        const_cast<float*>(data), {1, channels, height, width},
        torch::TensorOptions().dtype(torch::kFloat32)).clone().to(device);

    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input_tensor);

    torch::NoGradGuard no_grad;
    auto output = module.forward(inputs).toTuple();

    auto value_tensor = output->elements()[0].toTensor();
    auto act_tensor = output->elements()[1].toTensor();

    auto value_cpu = value_tensor.to(torch::kCPU);
    auto act_cpu = act_tensor.to(torch::kCPU);

    float value = value_cpu[0].item<float>();
    int act_size = act_cpu.size(1);
    std::vector<float> prior_prob(act_size);
    for (int i = 0; i < act_size; i++) {
        prior_prob[i] = exp(act_cpu[0][i].item<float>());
    }

    return {value, prior_prob};
}

std::future<std::pair<float, std::vector<float>>>
TorchBackend::enqueueData(std::vector<std::vector<std::vector<float>>> data) {
    std::promise<std::pair<float, std::vector<float>>> promise;
    std::future<std::pair<float, std::vector<float>>> future = promise.get_future();
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        dataQueue.emplace(data, std::move(promise));
    }
    condition.notify_one();
    return future;
}

void TorchBackend::batchInference() {
    while (true) {
        std::vector<DataPromisePair> batchData;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait_for(lock, std::chrono::milliseconds(50), [this] {
                return dataQueue.size() >= modelBatchSize || stop;
            });

            if (stop && dataQueue.empty()) {
                return;
            }

            while (!dataQueue.empty() && batchData.size() <= modelBatchSize) {
                batchData.push_back(std::move(dataQueue.front()));
                dataQueue.pop();
            }
        }

        if (!batchData.empty()) {
            std::vector<std::vector<std::vector<std::vector<float>>>> allData;
            for (const auto& dataPromisePair : batchData) {
                allData.push_back(dataPromisePair.first);
            }

            auto results = evaluate_state_batch(allData);

            for (size_t i = 0; i < batchData.size(); ++i) {
                batchData[i].second.set_value(results[i]);
            }
        }
    }
}
