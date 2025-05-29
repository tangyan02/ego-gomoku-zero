#define DOCTEST_CONFIG_IMPLEMENT

#include "ConfigReader.h"
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
    // Parse parameters with defaults if not found
    int boardSize = stoi(ConfigReader::get("boardSize"));
    int numGames = stoi(ConfigReader::get("numGames"));
    int numSimulation = stoi(ConfigReader::get("numSimulation"));
    float temperatureDefault = stof(ConfigReader::get("temperatureDefault"));
    float explorationFactor = stof(ConfigReader::get("explorationFactor"));
    string modelPath = ConfigReader::get("modelPath");
    int numProcesses = stoi(ConfigReader::get("numProcesses"));
    string coreType = ConfigReader::get("coreType");

    int mctsThreadSize = 1;

    Model* model = new Model();
    model->init(modelPath, coreType);

    std::vector<std::thread> threads; // 存储线程的容器

    recordSelfPlay(boardSize, numGames, numSimulation, mctsThreadSize,
                             temperatureDefault, explorationFactor, "", model
        );
    // // 创建n个线程并将函数作为入口点
    // for (int i = 0; i < partNum; ++i)
    // {
    //     auto part = shard + "_" + std::to_string(i);
    //     threads.emplace_back(recordSelfPlay, boardSize, numGames, numSimulation, mctsThreadSize,
    //                          temperatureDefault, explorationFactor, part, model);
    // }
    //
    // // 等待所有线程执行完毕
    // for (auto& thread : threads)
    // {
    //     thread.join();
    // }

    delete model;
}

int main(int argc, char* argv[])
{
    initShape();
    //    printShape();
    auto mode = ConfigReader::get("mode");
    if (mode == "train")
    {
        selfPlay(argc, argv);
        return 0;
    }
    //    piskvork();
    return startTest(argc, argv);
}
