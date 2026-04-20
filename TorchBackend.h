
#ifndef EGO_GOMOKU_ZERO_TORCHBACKEND_H
#define EGO_GOMOKU_ZERO_TORCHBACKEND_H

#include "IModel.h"
#include <torch/script.h>

class TorchBackend : public IModel {
public:
    TorchBackend();
    ~TorchBackend() override;

    void init(const std::string& modelPath, const std::string& coreType) override;

    std::pair<float, std::vector<float>>
    evaluate_state(std::vector<std::vector<std::vector<float>>>& data) override;

    std::pair<float, std::vector<float>>
    evaluate_state(const float* data, int channels, int height, int width) override;

    std::vector<std::pair<float, std::vector<float>>>
    evaluate_state_batch(const std::vector<std::vector<std::vector<std::vector<float>>>>& batchData) override;

    std::vector<std::pair<float, std::vector<float>>>
    evaluate_state_batch_flat(const float* data, int batch_size, int channels, int height, int width) override;

    std::future<std::pair<float, std::vector<float>>>
    enqueueData(std::vector<std::vector<std::vector<float>>> data) override;

private:
    void batchInference();

    torch::jit::script::Module module;
    torch::Device device{torch::kCPU};
    bool loaded{false};
    std::mutex inferenceMutex;  // 保护 MPS 推理的互斥锁

    using DataPromisePair = std::pair<std::vector<std::vector<std::vector<float>>>,
                                       std::promise<std::pair<float, std::vector<float>>>>;
    std::queue<DataPromisePair> dataQueue;
    std::thread batchThread;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop{false};
    int modelBatchSize{1};
};

#endif //EGO_GOMOKU_ZERO_TORCHBACKEND_H
