#include <torch/torch.h>
#include <iostream>
#include "MTCS.cpp"

int main()
{
    auto network = getNetwork();
    MonteCarloTree mct = MonteCarloTree(network, getDevice());
    Game game;
    Node node;
    mct.search(game, &node, 800);
    std::pair<std::vector<int>, std::vector<float>> result = mct.get_action_probabilities(game);
    std::cout << result;
}