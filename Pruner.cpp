
#include "Pruner.h"

void pruning(Node *node, Game &game) {

    if (!node->children.empty()) {
        long long timeout = game.vctTimeOut + getSystemTime();
        int maxLevel = 20;
        array<bool, 400> loss;
        int lossCount = 0;
        int winMoveIndex = -1;

        //搜索VCT点
        for (int level = 4; level <= maxLevel; level += 4) {
            cout << "level=" << level << endl;
            auto result = dfsVCT(game.currentPlayer, game.currentPlayer, game,
                                 Point(), Point(), Point(),
                                 false, 0, 0, 99, level, timeout);
            if (result.first) {
                winMoveIndex = game.getActionIndex(result.second[0]);
                cout << "发现胜利点" << endl;
                break;
            }

            for (const auto &item: node->children) {
                int actionIndex = item.first;
                if (!loss[actionIndex]) {
                    auto action = game.getPointFromIndex(actionIndex);
                    game.board[action.x][action.y] = game.currentPlayer;
                    auto result = dfsVCT(game.getOtherPlayer(), game.getOtherPlayer(), game,
                                         Point(), Point(), Point(),
                                         false, 0, 0, 99, level, timeout);
                    if (result.first) {
                        loss[actionIndex] = true;
                        lossCount++;
                        auto pLose = game.getPointFromIndex(actionIndex);
                        cout << "发现必输点" << pLose.x << "," << pLose.y << endl;
                    }
                    game.board[action.x][action.y] = 0;
                }
            }
        }

        //剪枝
        if (winMoveIndex >= 0) {
            vector<int> excludeIndex;
            for (const auto &item: node->children) {
                if (item.first != winMoveIndex) {
                    excludeIndex.emplace_back(item.first);
                }
            }
            for (const auto &item: excludeIndex) {
                node->visits -= node->children[item]->visits;
                node->value_sum -= node->children[item]->value_sum;
                node->children[item]->release();
                node->children.erase(item);
            }
        } else {
            if (lossCount == node->children.size()) {
                cout << "全是必输点" << endl;
            }
            if (lossCount < node->children.size()) {
                vector<int> excludeIndex;
                for (const auto &item: node->children) {
                    if (loss[item.first]) {
                        excludeIndex.emplace_back(item.first);
                    }
                }
                for (const auto &item: excludeIndex) {
                    node->visits -= node->children[item]->visits;
                    node->value_sum -= node->children[item]->value_sum;
                    node->children[item]->release();
                    node->children.erase(item);
                }
            }
        }
    }
}