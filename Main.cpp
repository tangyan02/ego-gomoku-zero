#define DOCTEST_CONFIG_IMPLEMENT

#include "test/TestAssistant.cpp"
#include "SelfPlay.h"
#include "Pisqpipe.h"
#include "Shape.h"
#include "Model.h"

using namespace std;

void selfPlay(int argc, char* argv[])
{
    string shard;
    int partNum = 1;
    if (argc > 1)
    {
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
    int modelBatchSize = 1;
    int mctsThreadSize = 1;

    Model* model = new Model();
    model->init("model/agent_model.onnx", modelBatchSize);

    std::vector<std::thread> threads; // 存储线程的容器
    // 创建n个线程并将函数作为入口点
    for (int i = 0; i < partNum; ++i)
    {
        auto part = shard + "_" + std::to_string(i);
        threads.emplace_back(recordSelfPlay, boardSize, numGames, sumSimulations, mctsThreadSize,
                             temperatureDefault, explorationFactor, part, model);
    }

    // 等待所有线程执行完毕
    for (auto& thread : threads)
    {
        thread.join();
    }

    delete model;
}

int main(int argc, char* argv[])
{
    initShape();
    //    printShape();
    // selfPlay(argc, argv);
    //    test();
    //    piskvork();
    return startTest(argc, argv);
}
