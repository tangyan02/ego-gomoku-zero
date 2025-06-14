
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

void Model::init(string modelPath, string coreType) {
    this->modelBatchSize = 1;
   // 初始化环境
    env = new Ort::Env(ORT_LOGGING_LEVEL_ERROR, "ModelInference");
    // env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "ModelInference");
    //env = new Ort::Env(ORT_LOGGING_LEVEL_VERBOSE, "onnxruntime"); // 启用详细日志
    // 初始化会话选项并添加模型
    sessionOptions = new Ort::SessionOptions();
    sessionOptions->SetIntraOpNumThreads(1);
    sessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    // 判断是否有GPU
    auto providers = Ort::GetAvailableProviders();
    //看看有没有GPU支持列表
    //auto tensorRtAvailable = std::find(providers.begin(), providers.end(), "TensorrtExecutionProvider");
    //if ((tensorRtAvailable != providers.end()))//找到cuda列表
    //{
    //    std::cout << "found providers:" << std::endl;
    //    for (auto provider: providers)
    //        std::cout << provider << std::endl;
    //    std::cout << "use: TensorrtExecutionProvider" << std::endl;
    //    OrtTensorRTProviderOptions tensorRtProviderOptions;
    //    sessionOptions->AppendExecutionProvider_TensorRT(tensorRtProviderOptions);
    //}
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
        if ((cudaAvailable != providers.end())) //找到cuda列表
        {

            //memoryInfo = Ort::MemoryInfo("Cuda", OrtAllocatorType::OrtArenaAllocator, 0, OrtMemTypeDefault);
            //memoryInfo = &Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

            //std::cout << "found providers:" << std::endl;
            //for (auto provider : providers)
            //std::cout << provider << std::endl;
            //std::cout << "use: CUDAExecutionProvider" << std::endl;

            // CUDA 执行提供器配置
            OrtCUDAProviderOptions cuda_options;
            //cuda_options.device_id = 0;
            //cuda_options.arena_extend_strategy = 1;               // kSameAsRequested
            //cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchHeuristic;
            //cuda_options.do_copy_in_default_stream = false;       // 保持异步拷贝
            //cuda_options.gpu_mem_limit = 0;                       // 自动管理显存
            //cuda_options.has_user_compute_stream = 0;


            sessionOptions->AppendExecutionProvider_CUDA(cuda_options);

            //sessionOptions->AddConfigEntry("cuda.deterministic_compute", "1"); // 确定性计算
            // 会话优化配置
            //sessionOptions->SetIntraOpNumThreads(1);
            //sessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_ALL);
            //sessionOptions->DisableMemPattern();                  // 禁用内存模式
            //sessionOptions->AddConfigEntry("disable_cpu_mem_buffer", "1");
            //sessionOptions->AddConfigEntry("optimization.enable_mixed_precision", "1");
        }
        //cout << "cuda init finish" << endl;
    }


#ifdef _WIN32
    // 创建会话
    session = new Ort::Session(*env, ConvertStringToWString(modelPath).c_str(), *sessionOptions);
#endif //_WIN32

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
    session = new Ort::Session(*env, modelPath.c_str(), *sessionOptions);
#endif // __unix__
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