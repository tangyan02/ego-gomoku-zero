#include "SelfPlay.h"
#include "AnalyzerTest.h"
#include "GameTest.h"
#include "Pisqpipe.h"
#include "Console.h"

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
            testGetActiveThreeMoves,
            testGetSleepyFourMoves,
            testGetSleepyFourMoves2,
            testGetThreeDefenceMoves,
            testGetThreeDefenceMoves2,
            testGetState,
            testGetState2,
            testGetState3,
            testDfsVCF,
            testDfsVCF2,
            testDfsVCF3,
            testGetVCFDefenceMoves,
            testGetVCFDefenceMoves2,
            testGetVCFDefenceMoves3,
            testGetVCFDefenceMoves4,
            testGetVCFDefenceMoves5,
            testGetNearByEmptyPoints,
            testGetNearEmptyPoints,
            testDfsVCT,
            testDfsVCT2,
            testDfsVCT3,
            testDfsVCT4,
            testDfsVCT5,
            testDfsVCT6,
            testDfsVCTIter,
            testDfsVCTDefenceIter,
            testDfsVCTDefenceIter2,
            testDfsVCTDefenceIter3
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
//    selfPlay(argc, argv);
    test();
//    piskvork();
//    startConsole(true);
    return 0;
}