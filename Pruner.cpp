
#include "Pruner.h"

void pruning(Node &node, Game &game, int timeLimit) {

    if (!node.children.empty()) {
        long long timeout = timeLimit + getSystemTime();
        int maxLevel = 20;
        int nodeLast = 0;
        array<bool, 400> loss;
        array<bool, 400> finished;
        Point winMove;

        for (int level = 4; level <= maxLevel; level += 4) {
            auto result = dfsVCT(game.currentPlayer, game.currentPlayer, game,
                                 Point(), Point(), Point(),
                                 false, 0, 0, 99, level, timeout);
            if (result.first) {
                winMove = result.second[0];
                break;
            }

            for (const auto &item: node.children) {
                int actionIndex = item.first;
                if (!finished[actionIndex]) {
                    auto action = game.getPointFromIndex(actionIndex);
                    game.board[action.x][action.y] = game.currentPlayer;
                    auto result = dfsVCT(game.getOtherPlayer(), game.getOtherPlayer(), game,
                                         Point(), Point(), Point(),
                                         false, 0, 0, 99, level, timeout);
                    if (result.first) {
                        finished[actionIndex] = true;

                    }
                    game.board[action.x][action.y] = 0;
                }
            }
        }


    }
}