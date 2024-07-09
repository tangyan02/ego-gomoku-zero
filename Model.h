

#ifndef EGO_GOMOKU_ZERO_MODEL_H
#define EGO_GOMOKU_ZERO_MODEL_H

#include <vector>
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <numeric>
#include <codecvt>
#include <locale>
#include <future>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <stdexcept>
#include <chrono>

using namespace std;

class Model {
public:

    Model();
    ~Model();

    void init(std::string modelPath, int modelBatchSize);

    future<std::pair<float, std::vector<float>>> enqueueData(std::vector<std::vector<std::vector<float>>> data);

    std::pair<float, std::vector<float>> evaluate_state(std::vector<std::vector<std::vector<float>>> &data);
    std::vector<std::pair<float, std::vector<float>>> evaluate_state_batch(const vector<std::vector<std::vector<std::vector<float>>>>& batchData);

private:
    void batchInference();

    Ort::Env *env;
    Ort::Session *session;
    Ort::SessionOptions *sessionOptions;
    Ort::MemoryInfo memoryInfo;

    using DataPromisePair = std::pair<std::vector<std::vector<std::vector<float>>>, std::promise<std::pair<float, std::vector<float>>>>;
    std::queue<DataPromisePair> dataQueue;
    std::thread batchThread;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    int modelBatchSize;
};


#endif //EGO_GOMOKU_ZERO_MODEL_H
