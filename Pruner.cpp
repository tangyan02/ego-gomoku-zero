
#include "Pruner.h"

void pruning(Node *node, Game &game, const string &logPrefix) {

    if (node->children.size() > 1) {
        int childCountBefore = node->children.size();

        long long timeout = game.vctTimeOut + getSystemTime();
        int maxLevel = 32;
        array<array<bool, 400>, 20> lose = {{{false}}};
        array<int, 20> loseCount = {false};

        int winMoveIndex = -1;
        int iterLevel = 0;

        //搜索VCT点
        int interval = 2;
        for (int level = 4; level <= maxLevel; level += interval) {
            //更新前一层的必败点
            iterLevel++;
            for (const auto &item: node->children) {
                lose[iterLevel][item.first] = lose[iterLevel - 1][item.first];
            }
            loseCount[iterLevel] = loseCount[iterLevel - 1];

            auto result = dfsVCT(game.currentPlayer, game.currentPlayer, game,
                                 Point(), Point(), Point(),
                                 false, 0, 0, 99, level, timeout);
            if (result.first) {
                winMoveIndex = game.getActionIndex(result.second[0]);
                break;
            }

            auto oppResult = dfsVCT(game.getOtherPlayer(), game.getOtherPlayer(), game,
                                    Point(), Point(), Point(),
                                    false, 0, 0, 99, level, timeout);
            if (oppResult.first) {
                for (const auto &item: node->children) {
                    int actionIndex = item.first;
                    if (!lose[iterLevel][actionIndex]) {
                        auto action = game.getPointFromIndex(actionIndex);
                        game.board[action.x][action.y] = game.currentPlayer;
                        auto result = dfsVCT(game.getOtherPlayer(), game.getOtherPlayer(), game,
                                             Point(), Point(), Point(),
                                             false, 0, 0, 99, level, timeout);
                        if (result.first) {
                            lose[iterLevel][actionIndex] = true;
                            loseCount[iterLevel]++;
                        }
                        game.board[action.x][action.y] = 0;
                    }
                }
            }

            if (getSystemTime() > timeout) {
                break;
            }
//            cout << "level=" << level << ", lose count=" << loseCount[iterLevel] << endl;
        }

        if (winMoveIndex != -1) {
            auto p = game.getPointFromIndex(winMoveIndex);
            cout << logPrefix << "found vct (" << p.x << "," << p.y << ") on " << iterLevel * interval << endl;
        }

        if (loseCount[iterLevel] == node->children.size()) {
            cout << logPrefix << "level " << iterLevel * interval << ", all lose and bake up" << endl;
        }
        while (loseCount[iterLevel] == node->children.size()) {
            iterLevel--;
        }

        if (loseCount[iterLevel] > 0) {
            cout << logPrefix << "handle lose points ";
            for (const auto &item: node->children) {
                int actionIndex = item.first;
                if (lose[iterLevel][actionIndex]) {
                    auto pLose = game.getPointFromIndex(actionIndex);
//                    cout << "(" << pLose.x << "," << pLose.y << "),";
                }
            }
            cout << endl;
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
            vector<int> excludeIndex;
            for (const auto &item: node->children) {
                if (lose[iterLevel][item.first]) {
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

        int childCountNow = node->children.size();
        if (childCountBefore != childCountNow) {
            cout << logPrefix << "level=" << iterLevel * interval
                 << " cut " << childCountBefore << "->" << childCountNow
                 << endl;
        } else {
            cout << logPrefix << "vct search at level " << iterLevel * interval << endl;
        }
    }
}