#include "SelfPlay.h"
#include "AnalyzerTest.h"
#include "Pisqpipe.h"

using namespace std;

void selfPlay(int argc, char *argv[]) {
    string shard;
    if (argc > 1) {
        string firstArg = argv[1];
        cout << "current shard " << firstArg << endl;
        shard = "_" + firstArg;
    }

    int numGames = 1;
    int sumSimulations = 800;
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
            cout << "succeed" << endl;
        } else {
            cout << "faid" << endl;
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
    selfPlay(argc, argv);
    //test();
//    piskvork();
    return 0;
}