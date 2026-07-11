#define DOCTEST_CONFIG_IMPLEMENT

#include "ConfigReader.h"
#include "test/TestAssistant.cpp"
#include "SelfPlay.h"
#include "Pisqpipe.h"
#include "Shape.h"
#include "Model.h"
#include "Bridge.h"
#include "Evaluate.h"
#include "GenerateOpenings.h"
#include "BenchSelectActions.h"
#include <memory>

using namespace std;

void selfPlay(int argc, char *argv[]) {
    int boardSize = stoi(ConfigReader::get("boardSize"));
    int numGames = stoi(ConfigReader::get("numGames"));
    int numSimulation = stoi(ConfigReader::get("numSimulation"));
    float explorationFactor = stof(ConfigReader::get("explorationFactor"));

    // 环境变量覆盖 numGames（Python 端按进程数分摊后传入）
    if (getenv("NUM_GAMES")) numGames = atoi(getenv("NUM_GAMES"));

    string modelPath = ConfigReader::get("modelPath");
    string coreType = ConfigReader::get("coreType");
    auto sharedModel = std::make_unique<Model>();
    sharedModel->init(modelPath, coreType);

    // 从环境变量读取 shard ID（Python 端启动多个 C++ 进程时传入）
    int shard = 0;
    if (getenv("SHARD_ID")) shard = atoi(getenv("SHARD_ID"));

    auto context = std::make_unique<Context>(numGames);
    // 单进程单线程自对弈（多进程并行由 Python 端启动多个 C++ 进程实现）
    recordSelfPlay(boardSize, context.get(), numSimulation, explorationFactor, shard, sharedModel.get());
}

int main(int argc, char *argv[]) {
    initShape();
    //    printShape();

    #ifdef _WIN32
    piskvork();
    return 0;

    #endif

    auto mode = ConfigReader::get("mode");
    if (mode == "train") {
        selfPlay(argc, argv);
        return 0;
    }

    if (mode == "predict") {
        Bridge bridge;
        bridge.startGame();
        return 0;
    }

    if (mode == "evaluate") {
        runEvaluate();
        return 0;
    }

    if (mode == "generate_openings") {
        runGenerateOpenings();
        return 0;
    }

    if (mode == "vct_label") {
        std::cout << "[Main] vct_label mode has been removed" << std::endl;
        return 0;
    }

    if (mode == "bench_select_actions") {
        runBenchSelectActions();
        return 0;
    }


    return startTest(argc, argv);
}
