#include "SelfPlay.h"
#include "AnalyzerTest.h"
#include "GameTest.h"
#include "Pisqpipe.h"
#include "Console.h"
#include "Shape.h"
#include "ShapeTest.h"
#include <onnxruntime_cxx_api.h>

using namespace std;

void selfPlay(int argc, char *argv[]) {
    string shard;
    int partNum = 1;
    if (argc > 1) {
        string firstArg = argv[1];
        partNum = std::stoi(argv[2]);;
        cout << "current shard " << firstArg << endl;
        shard = "_" + firstArg;
    }

    int numGames = 1;
    int sumSimulations = 800;
    float temperatureDefault = 1;
    float explorationFactor = 3;
    int boardSize = 20;

    std::vector<std::thread> threads; // 存储线程的容器
    // 创建n个线程并将函数作为入口点
    for (int i = 0; i < partNum; ++i) {
        auto part = shard + "_" + std::to_string(i);
        threads.emplace_back(recordSelfPlay, boardSize, numGames, sumSimulations, temperatureDefault,
                             explorationFactor, part);
    }

    // 等待所有线程执行完毕
    for (auto &thread: threads) {
        thread.join();
    }
}

void test() {
    typedef bool (*FunctionPtr)();
    FunctionPtr functions[] = {
            testGetWinnerMove,
            testGetActiveFourMoves,
            testGetActiveFourMoves2,
            testGetActiveFourMoves3,
            testGetActiveFourMoves4,
            testGetSleepyFourMoves,
            testGetSleepyFourMoves2,
            testGetActiveThreeMoves,
            testGetActiveThreeMoves2,
            testGetActiveThreeMoves3,
            testGetActiveThreeMoves4,
            testGetActiveFourMoves5,
            testGetSleepyThreeMoves,
            testGetSleepyThreeMoves2,
            testGetActiveTwoMoves,
            testGetSleepyTwoMoves,
            testDfsVCF,
            testDfsVCF2,
            testDfsVCF3,
            testGetNearByEmptyPoints,
            testGetNearEmptyPoints,
            testGetLineEmptyPoints,
            testGetState,
            testGetKeysInGame,
            testGetThreeDefenceMoves,
            testGetThreeDefenceMoves2,
            testDfsVCT,
            testDfsVCT2,
            testDfsVCT3,
            testDfsVCT4,
            testDfsVCT5,
            testDfsVCT6,
            testDfsVCT7
    };

    int total = sizeof(functions) / sizeof(functions[0]);
    int succeedCount = 0;
    for (const auto &func: functions) {
        bool result = func();
        if (result) {
            succeedCount += 1;
            cout << "succeed" << endl;
        } else {
            cout << "fail" << endl;
        }
    }
    cout << "total cases count " << total << endl;
    if (total == succeedCount) {
        cout << "all succeed" << endl;
    } else {
        cout << "fail case exist " << total - succeedCount << endl;
    }
}

void onnxTest() {
    // 初始化环境
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ModelInference");

    // 初始化会话选项并添加模型
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);

    const char *model_path = "model/model_latest.onnx";

    Ort::Session session(env, model_path, session_options);
    // 创建输入张量
    std::vector<float> input_tensor_values(1 * 18 * 20 * 20); // 填充你的实际数据
    std::vector<int64_t> input_tensor_shape = {1, 18, 20, 20};
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_tensor_shape.data(), input_tensor_shape.size());

    // 设置输入
    std::vector<const char*> input_node_names = {"input"};
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(std::move(input_tensor));

    // 设置输出
    std::vector<const char*> output_node_names = {"output"};

    // 进行推理
    auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_node_names.data(), input_tensors.data(), input_tensors.size(), output_node_names.data(), output_node_names.size());

    // 获取输出张量
    float* floatarr = output_tensors.front().GetTensorMutableData<float>();
    std::cout << "Output: " << floatarr[0] << std::endl;

}

int main(int argc, char *argv[]) {
    onnxTest();
//    initShape();
//    selfPlay(argc, argv);
//    test();
//    piskvork();
//    startConsole(true);
    return 0;
}