

#include "SelfPlayTest.h"

bool testRandomGame() {
    cout << "testRandomGame" << endl;
    Game game(20);

    auto device = torch::kCPU;
    auto network = getNetwork(device, "model/model_latest.pt");
    MonteCarloTree mcts = MonteCarloTree(&network, device, 1);
    game = randomGame(game, mcts);

    game.printBoard();
    return true;
}