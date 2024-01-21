#include "SelfPlay.h"

using namespace std;

int main() {
    int numGames = 10;
    int concurrent = 10;
    int sumSimulations = 800;
    float temperatureDefault = 1;
    float explorationFactor = 3;

    recordConcurrentSelfPlay(numGames ,sumSimulations, temperatureDefault, explorationFactor, concurrent);

    return 0;
}