#include "SelfPlay.h"
#include "AnalyzerTest.h"
#include "GameTest.h"
#include "Pisqpipe.h"
#include "Shape.h"
#include "ShapeTest.h"
#include "Model.h"

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
    int modelBatchSize = 1;
    int mctsThreadSize = 1;

    Model *model = new Model();
    model->init("model/agent_model.onnx", modelBatchSize);

    std::vector<std::thread> threads; // 存储线程的容器
    // 创建n个线程并将函数作为入口点
    for (int i = 0; i < partNum; ++i) {
        auto part = shard + "_" + std::to_string(i);
        threads.emplace_back(recordSelfPlay, boardSize, numGames, sumSimulations, mctsThreadSize,
                             temperatureDefault, explorationFactor, part, model);
    }

    // 等待所有线程执行完毕
    for (auto &thread: threads) {
        thread.join();
    }

    delete model;
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
            testDfsVCF4,
            testGetNearByEmptyPoints,
            testGetNearEmptyPoints,
            testGetLineEmptyPoints,
            testGetState,
            testGetKeysInGame,
            testGetThreeDefenceMoves,
            testGetThreeDefenceMoves2,
            testSelectActions,
            testSelectActions2,
            testSelectActions3,
            testSelectActions4,
            testSelectActions5,
            testSelectActions6,
            testSelectActions7,
            testSelectActions8,
            testDfsVCT,
            testDfsVCT2,
            testDfsVCT3,
            testDfsVCT4,
            testDfsVCT5,
            testDfsVCT6,
            testDfsVCT7,
            testDfsVCT8,
            testDfsVCT9,
            testDfsVCT10,
            testDfsVCT11,
            testDfsVCT12
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

int main(int argc, char *argv[]) {
    initShape();
//    printShape();
    selfPlay(argc, argv);
//    test();
//    piskvork();
    return 0;
}