#include "SelfPlay.h"

using namespace std;

int main(int argc, char *argv[]) {
    std::string shard;
    if (argc > 1) {
        std::string firstArg = argv[1];
        std::cout << "当前分片：" << firstArg << std::endl;
        shard = "_" + firstArg;
    }

    int numGames = 1;
    int sumSimulations = 800;
    float temperatureDefault = 1;
    float explorationFactor = 3;

    recordSelfPlay(numGames, sumSimulations, temperatureDefault,
                             explorationFactor, shard);

    return 0;
}