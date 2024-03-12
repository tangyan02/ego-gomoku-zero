
#include "Model.h"

Model::Model() : memoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {
}

void Model::init(string modelPath) {
    // 初始化环境
    env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "ModelInference");

    // 初始化会话选项并添加模型
    sessionOptions = new Ort::SessionOptions();
    sessionOptions->SetIntraOpNumThreads(1);

    // 创建会话
    session = new Ort::Session(*env, modelPath.c_str(), *sessionOptions);
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

    std::vector<int64_t> input_tensor_shape = {dim1, dim2, dim3};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memoryInfo, flattened_data.data(),
                                                              flattened_data.size(), input_tensor_shape.data(),
                                                              input_tensor_shape.size());

    // 设置输入
    std::vector<const char *> input_node_names = {"input"};
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(std::move(input_tensor));

    // 设置输出
    std::vector<const char *> output_node_names = {"value", "act"};

    // 进行推理
    auto output_tensors = session->Run(Ort::RunOptions{nullptr}, input_node_names.data(), input_tensors.data(),
                                       input_tensors.size(), output_node_names.data(), output_node_names.size());

    // 获取输出张量
    float *value = output_tensors[0].GetTensorMutableData<float>();

    float *output2 = output_tensors[1].GetTensorMutableData<float>();
    std::vector<int64_t> output2_shape = output_tensors[1].GetTensorTypeAndShapeInfo().GetShape();
    int output2_size = std::accumulate(output2_shape.begin(), output2_shape.end(), 1, std::multiplies<int64_t>());

    vector<float> prior_prob;
    for (int i = 0; i < output2_size; i++) {
        prior_prob.emplace_back(exp(output2[i]));
    }

    return std::make_pair(*value, prior_prob);

}
