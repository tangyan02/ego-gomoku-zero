
#include "Model.h"


#ifdef _WIN32
std::wstring ConvertStringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
#endif //_WIN32

Model::Model() : memoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)), stop(false) {
}

Model::~Model() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    if (batchThread.joinable()) {
        batchThread.join();
    }
}

void Model::init(string modelPath, int modelBatchSize) {
    // 初始化环境
    env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "ModelInference");
    this->modelBatchSize = modelBatchSize;

    // 初始化会话选项并添加模型
    sessionOptions = new Ort::SessionOptions();
    sessionOptions->SetIntraOpNumThreads(1);
    sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // 判断是否有GPU
    auto providers = Ort::GetAvailableProviders();

    auto cudaAvailable = std::find(providers.begin(), providers.end(), "CUDAExecutionProvider");
    if ((cudaAvailable != providers.end()))//找到cuda列表
    {
        std::cout << "found providers:" << std::endl;
        for (auto provider: providers)
            std::cout << provider << std::endl;
        std::cout << "use: CUDAExecutionProvider" << std::endl;
        OrtCUDAProviderOptions cudaProviderOptions;
        sessionOptions->AppendExecutionProvider_CUDA(cudaProviderOptions);
    }

#ifdef _WIN32
    // 创建会话
    session = new Ort::Session(*env, ConvertStringToWString(modelPath).c_str(), *sessionOptions);
#endif //_WIN32

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
    session = new Ort::Session(*env, modelPath.c_str(), *sessionOptions);
#endif // __unix__

    // 启动批量推理线程
    batchThread = std::thread(&Model::batchInference, this);
}

std::vector<std::pair<float, std::vector<float>>>
Model::evaluate_state_batch(const std::vector<std::vector<std::vector<std::vector<float>>>> &batchData) {
    int batch_size = batchData.size();
    if (batch_size == 0) {
        return {};
    }

    // 获取单个数据的维度
    int dim1 = batchData[0].size();
    int dim2 = batchData[0][0].size();
    int dim3 = batchData[0][0][0].size();

//    cout << "输入尺寸 " << batch_size << " " << dim1 << " " << dim2 << " " << dim3 << endl;
    // 将批量数据转换为一维数组
    std::vector<float> flattened_data(modelBatchSize * dim1 * dim2 * dim3);
    int index = 0;
    for (const auto &data: batchData) {
        for (int i = 0; i < dim1; i++) {
            for (int j = 0; j < dim2; j++) {
                for (int k = 0; k < dim3; k++) {
                    flattened_data[index++] = data[i][j][k];
                }
            }
        }
    }


    // 创建输入张量
    std::vector<int64_t> input_tensor_shape = {modelBatchSize, dim1, dim2, dim3};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memoryInfo, flattened_data.data(),
                                                              flattened_data.size(), input_tensor_shape.data(),
                                                              input_tensor_shape.size());

    // 设置输入
    std::vector<const char *> input_node_names = {"input"};
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(std::move(input_tensor));

    // 设置输出
    std::vector<const char *> output_node_names = {"value", "act"};

    // 进行一次推理
    auto output_tensors = session->Run(Ort::RunOptions{nullptr}, input_node_names.data(), input_tensors.data(),
                                       input_tensors.size(), output_node_names.data(), output_node_names.size());

    // 获取输出张量
    float *value_data = output_tensors[0].GetTensorMutableData<float>();
    float *act_data = output_tensors[1].GetTensorMutableData<float>();

    // 获取输出张量的形状
    std::vector<int64_t> act_shape = output_tensors[1].GetTensorTypeAndShapeInfo().GetShape();
    int act_size_per_batch = std::accumulate(act_shape.begin() + 1, act_shape.end(), 1, std::multiplies<int64_t>());

    // 解析输出
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

std::pair<float, std::vector<float>> Model::evaluate_state(vector<vector<vector<float>>> &data) {
    vector<vector<vector<vector<float>>>> batchData;
    batchData.emplace_back(data);
    return evaluate_state_batch(batchData)[0];
}

std::future<std::pair<float, std::vector<float>>>
Model::enqueueData(std::vector<std::vector<std::vector<float>>> data) {
    std::promise<std::pair<float, std::vector<float>>> promise;
    std::future<std::pair<float, std::vector<float>>> future = promise.get_future();
    {
        std::unique_lock<std::mutex> lock(queueMutex);
//        cout << "放入数据 尺寸：" << data.size() << " " << data.front().size() << " " << data.front().front().size()
//             << endl;
        dataQueue.emplace(data, std::move(promise));
    }
    condition.notify_one();
    return future;
}

void Model::batchInference() {
    while (true) {
        std::vector<DataPromisePair> batchData;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait_for(lock, std::chrono::milliseconds(1000), [this] {
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
            // 收集所有数据
//            cout << "当前处理批量大小 " << batchData.size() << endl;
            vector<std::vector<std::vector<std::vector<float>>>> allData;
            for (const auto &dataPromisePair: batchData) {
                allData.push_back(dataPromisePair.first);
            }

            // 批量推理
            auto results = evaluate_state_batch(allData);

            // 将结果分发给各个 promise
            for (size_t i = 0; i < batchData.size(); ++i) {
                batchData[i].second.set_value(results[i]);
            }
        }
    }
}