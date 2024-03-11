#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <numeric>

using namespace std;

void onnxTest() {
    // 初始化环境
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ModelInference");

    // 初始化会话选项并添加模型
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);

    const char *model_path = "model/model_latest.onnx";

    Ort::Session session(env, model_path, session_options);
    // 创建输入张量
    std::vector<float> input_tensor_values(18 * 20 * 20, 0.0f); // 填充你的实际数据
    std::vector<int64_t> input_tensor_shape = {18, 20, 20};
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(),
                                                              input_tensor_values.size(), input_tensor_shape.data(),
                                                              input_tensor_shape.size());

    // 设置输入
    std::vector<const char *> input_node_names = {"input"};
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(std::move(input_tensor));

    // 设置输出
    std::vector<const char *> output_node_names = {"value", "act"};

    // 进行推理
    auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_node_names.data(), input_tensors.data(),
                                      input_tensors.size(), output_node_names.data(), output_node_names.size());

    // 获取输出张量
    float *output1 = output_tensors[0].GetTensorMutableData<float>();
    std::cout << "Output 1: " << output1[0] << std::endl;

    float *output2 = output_tensors[1].GetTensorMutableData<float>();
    std::vector<int64_t> output2_shape = output_tensors[1].GetTensorTypeAndShapeInfo().GetShape();
    int output2_size = std::accumulate(output2_shape.begin(), output2_shape.end(), 1, std::multiplies<int64_t>());

    for (int i = 0; i < output2_size; i++) {
        std::cout << "Output 2 element " << i << ": " << exp(output2[i]) << std::endl;
    }
}

int main(int argc, char *argv[]) {
    onnxTest();
    return 0;
}