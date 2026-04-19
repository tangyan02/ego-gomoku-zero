
#include "ONNXBackend.h"
#include <numeric>

#ifdef _WIN32
static std::wstring ConvertStringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
#endif //_WIN32

ONNXBackend::ONNXBackend()
    : memoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      env(nullptr), session(nullptr), sessionOptions(nullptr), stop(false), modelBatchSize(1) {
}

ONNXBackend::~ONNXBackend() {
    if (batchThread.joinable()) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        batchThread.join();
    }

    delete session;
    delete sessionOptions;
    delete env;
}

void ONNXBackend::init(const std::string& modelPath, const std::string& coreType) {
    this->modelBatchSize = 1;
    env = new Ort::Env(ORT_LOGGING_LEVEL_ERROR, "ModelInference");
    sessionOptions = new Ort::SessionOptions();
    sessionOptions->SetIntraOpNumThreads(1);
    sessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_ALL);

    auto providers = Ort::GetAvailableProviders();

    if (coreType == "apple") {
        std::unordered_map<std::string, std::string> provider_options;
        provider_options["ModelFormat"] = "MLProgram";
        provider_options["MLComputeUnits"] = "ALL";
        provider_options["RequireStaticInputShapes"] = "0";
        provider_options["EnableOnSubgraphs"] = "0";
        sessionOptions->AppendExecutionProvider("CoreML", provider_options);
    }

#ifdef _WIN32
    if (coreType == "tensorRT") {
        int device_id = 0;
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_Tensorrt(*sessionOptions, device_id));
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_CUDA(*sessionOptions, device_id));
    }
#endif //_WIN32

    if (coreType == "cuda") {
        auto cudaAvailable = std::find(providers.begin(), providers.end(), "CUDAExecutionProvider");
        if (cudaAvailable != providers.end()) {
            OrtCUDAProviderOptions cuda_options;
            sessionOptions->AppendExecutionProvider_CUDA(cuda_options);
        }
    }

#ifdef _WIN32
    session = new Ort::Session(*env, ConvertStringToWString(modelPath).c_str(), *sessionOptions);
#else
    session = new Ort::Session(*env, modelPath.c_str(), *sessionOptions);
#endif
}

std::vector<std::pair<float, std::vector<float>>>
ONNXBackend::evaluate_state_batch(const std::vector<std::vector<std::vector<std::vector<float>>>>& batchData) {
    int batch_size = batchData.size();
    if (batch_size == 0) {
        return {};
    }

    int dim1 = batchData[0].size();
    int dim2 = batchData[0][0].size();
    int dim3 = batchData[0][0][0].size();

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

    std::vector<int64_t> input_tensor_shape = {batch_size, dim1, dim2, dim3};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memoryInfo, flattened_data.data(), flattened_data.size(),
        input_tensor_shape.data(), input_tensor_shape.size());

    std::vector<const char*> input_node_names = {"input"};
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(std::move(input_tensor));

    std::vector<const char*> output_node_names = {"value", "act"};

    auto output_tensors = session->Run(
        Ort::RunOptions{nullptr}, input_node_names.data(), input_tensors.data(),
        input_tensors.size(), output_node_names.data(), output_node_names.size());

    float* value_data = output_tensors[0].GetTensorMutableData<float>();
    float* act_data = output_tensors[1].GetTensorMutableData<float>();

    std::vector<int64_t> act_shape = output_tensors[1].GetTensorTypeAndShapeInfo().GetShape();
    int act_size_per_batch = std::accumulate(act_shape.begin() + 1, act_shape.end(), 1, std::multiplies<int64_t>());

    std::vector<std::pair<float, std::vector<float>>> results;
    results.reserve(batch_size);

    for (int b = 0; b < batch_size; b++) {
        float value = value_data[b];
        std::vector<float> prior_prob;
        for (int i = 0; i < act_size_per_batch; i++) {
            prior_prob.emplace_back(exp(act_data[b * act_size_per_batch + i]));
        }
        results.emplace_back(value, prior_prob);
    }

    return results;
}

std::pair<float, std::vector<float>>
ONNXBackend::evaluate_state(std::vector<std::vector<std::vector<float>>>& data) {
    std::vector<std::vector<std::vector<std::vector<float>>>> batchData;
    batchData.emplace_back(data);
    return evaluate_state_batch(batchData)[0];
}

std::pair<float, std::vector<float>>
ONNXBackend::evaluate_state(const float* data, int channels, int height, int width) {
    int totalSize = channels * height * width;

    std::vector<int64_t> input_tensor_shape = {1, channels, height, width};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memoryInfo, const_cast<float*>(data), totalSize,
        input_tensor_shape.data(), input_tensor_shape.size());

    std::vector<const char*> input_node_names = {"input"};
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(std::move(input_tensor));

    std::vector<const char*> output_node_names = {"value", "act"};

    auto output_tensors = session->Run(
        Ort::RunOptions{nullptr}, input_node_names.data(), input_tensors.data(),
        input_tensors.size(), output_node_names.data(), output_node_names.size());

    float value = output_tensors[0].GetTensorMutableData<float>()[0];
    float* act_data = output_tensors[1].GetTensorMutableData<float>();

    std::vector<int64_t> act_shape = output_tensors[1].GetTensorTypeAndShapeInfo().GetShape();
    int act_size = std::accumulate(act_shape.begin() + 1, act_shape.end(), 1, std::multiplies<int64_t>());

    std::vector<float> prior_prob(act_size);
    for (int i = 0; i < act_size; i++) {
        prior_prob[i] = exp(act_data[i]);
    }

    return {value, prior_prob};
}

std::future<std::pair<float, std::vector<float>>>
ONNXBackend::enqueueData(std::vector<std::vector<std::vector<float>>> data) {
    std::promise<std::pair<float, std::vector<float>>> promise;
    std::future<std::pair<float, std::vector<float>>> future = promise.get_future();
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        dataQueue.emplace(data, std::move(promise));
    }
    condition.notify_one();
    return future;
}

void ONNXBackend::batchInference() {
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
