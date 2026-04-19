
#ifndef EGO_GOMOKU_ZERO_ONNXBACKEND_H
#define EGO_GOMOKU_ZERO_ONNXBACKEND_H

#include "IModel.h"

#ifdef _WIN32
#include <codecvt>
#include <locale>
#endif

#include <onnxruntime_cxx_api.h>

class ONNXBackend : public IModel {
public:
    ONNXBackend();
    ~ONNXBackend() override;

    void init(const std::string& modelPath, const std::string& coreType) override;

    std::pair<float, std::vector<float>>
    evaluate_state(std::vector<std::vector<std::vector<float>>>& data) override;

    std::pair<float, std::vector<float>>
    evaluate_state(const float* data, int channels, int height, int width) override;

    std::vector<std::pair<float, std::vector<float>>>
    evaluate_state_batch(const std::vector<std::vector<std::vector<std::vector<float>>>>& batchData) override;

    std::future<std::pair<float, std::vector<float>>>
    enqueueData(std::vector<std::vector<std::vector<float>>> data) override;

private:
    void batchInference();

    Ort::Env* env;
    Ort::Session* session;
    Ort::SessionOptions* sessionOptions;
    Ort::MemoryInfo memoryInfo;

    using DataPromisePair = std::pair<std::vector<std::vector<std::vector<float>>>,
                                       std::promise<std::pair<float, std::vector<float>>>>;
    std::queue<DataPromisePair> dataQueue;
    std::thread batchThread;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    int modelBatchSize;
};

#endif //EGO_GOMOKU_ZERO_ONNXBACKEND_H
