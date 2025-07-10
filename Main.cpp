#define DOCTEST_CONFIG_IMPLEMENT

#include "ConfigReader.h"
#include "test/TestAssistant.cpp"
#include "SelfPlay.h"
#include "Pisqpipe.h"
#include "Shape.h"
#include "Model.h"
#include "Bridge.h"

using namespace std;

void selfPlay(int argc, char *argv[]) {
    int boardSize = stoi(ConfigReader::get("boardSize"));
    int numGames = stoi(ConfigReader::get("numGames"));
    int numSimulation = stoi(ConfigReader::get("numSimulation"));
    float temperatureDefault = stof(ConfigReader::get("temperatureDefault"));
    float explorationFactor = stof(ConfigReader::get("explorationFactor"));
    int numProcesses = stoi(ConfigReader::get("numProcesses"));

    std::vector<std::thread> threads; // 存储线程的容器

    Context* context = new Context(numGames);
    // 创建n个线程并将函数作为入口点
    for (int i = 0; i < numProcesses; ++i) {
        threads.emplace_back(recordSelfPlay,
                             boardSize,
                             context,
                             numSimulation,
                             temperatureDefault,
                             explorationFactor,
                             i);
    }

    // 等待所有线程执行完毕
    for (auto &thread: threads) {
        thread.join();
    };
}

int main(int argc, char *argv[]) {
    initShape();
    //    printShape();

    #ifdef _WIN32
//    piskvork();
//    return 0;

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

    return startTest(argc, argv);
}
