#include "SelfPlay.h"
#include "AnalyzerTest.h"

using namespace std;

void selfPlay(int argc, char *argv[]) {
    std::string shard;
    if (argc > 1) {
        std::string firstArg = argv[1];
        std::cout << "当前分片：" << firstArg << std::endl;
        shard = "_" + firstArg;
    }

    int numGames = 1;
    int sumSimulations = 200;
    float temperatureDefault = 1;
    float explorationFactor = 3;
    int boardSize = 15;

    recordSelfPlay(boardSize, numGames, sumSimulations, temperatureDefault,
                   explorationFactor, shard);
}

void test() {
    typedef bool (*FunctionPtr)();
    FunctionPtr functions[] = {
            testGetWinnerMove,
            testGetActiveFourMoves,
            testGetActiveFourMoves2,
            testGetActiveFourMoves3,
            testGetSleepyFourMoves,
            testGetSleepyFourMoves2,
            testGetThreeDefenceMoves,
            testGetThreeDefenceMoves2
    };

    int total = sizeof(functions) / sizeof(functions[0]);
    int succeedCount = 0;
    for (const auto &func: functions) {
        bool result = func();
        if (result) {
            succeedCount += 1;
            cout << "成功" << endl;
        } else {
            cout << "失败" << endl;
        }
    }
    cout << "总计样例个数 " << total << endl;
    if (total == succeedCount) {
        cout << "全部成功" << endl;
    } else {
        cout << "存在失败样例 " << total - succeedCount << " 个" << endl;
    }

}

int main(int argc, char *argv[]) {
    selfPlay(argc, argv);
//    test();
    return 0;
}