
#include "Pruner.h"

void pruning(Node *node, Game &game, const string logPrefix) {

    if (!node->children.empty()) {
        int childCountBefore = node->children.size();

        long long timeout = game.vctTimeOut + getSystemTime();
        int maxLevel = 20;
        array<bool, 400> lose = {false};
        array<bool, 400> loseLast = {false};
        int loseCount = 0;
        int loseCountLast = 0;
        int winMoveIndex = -1;
        int currentLevel = 0;

        //搜索VCT点
        for (int level = 4; level <= maxLevel; level += 4) {
            //更新前一层的必败点
            if (loseCount != loseCountLast) {
                for (const auto &item: node->children) {
                    loseLast[item.first] = lose[item.first];
                    loseCountLast = loseCount;
                }
            }

            currentLevel = level;
            auto result = dfsVCT(game.currentPlayer, game.currentPlayer, game,
                                 Point(), Point(), Point(),
                                 false, 0, 0, 99, level, timeout);
            if (result.first) {
                winMoveIndex = game.getActionIndex(result.second[0]);
                break;
            }

            for (const auto &item: node->children) {
                int actionIndex = item.first;
                if (!lose[actionIndex]) {
                    auto action = game.getPointFromIndex(actionIndex);
                    game.board[action.x][action.y] = game.currentPlayer;
                    auto result = dfsVCT(game.getOtherPlayer(), game.getOtherPlayer(), game,
                                         Point(), Point(), Point(),
                                         false, 0, 0, 99, level, timeout);
                    if (result.first) {
                        lose[actionIndex] = true;
                        loseCount++;
                    }
                    game.board[action.x][action.y] = 0;
                }
            }

            cout << "level=" << level << ", lose count=" << loseCount << " last lose count=" << loseCountLast << endl;
        }

        if (winMoveIndex != -1) {
            auto p = game.getPointFromIndex(winMoveIndex);
            cout << logPrefix << "发现胜利点(" << p.x << "," << p.y << ")" << endl;
        }

        if (loseCount == node->children.size()) {
            cout << logPrefix << "全是必输点，还原上层必败点集合" << endl;
            loseCount = 0;
            for (const auto &item: node->children) {
                lose[item.first] = loseLast[item.first];
                if (loseLast[item.first]) {
                    loseCount++;
                }
            }
        }

        if (loseCount > 0) {
            cout << logPrefix << "存在必输点";
            for (const auto &item: node->children) {
                int actionIndex = item.first;
                if (lose[actionIndex]) {
                    auto pLose = game.getPointFromIndex(actionIndex);
                    cout << "(" << pLose.x << "," << pLose.y << "),";
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
                if (lose[item.first]) {
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
            cout << logPrefix << "level=" << currentLevel
                 << " 剪枝 " << childCountBefore << "->" << childCountNow
                 << endl;
        }
    }
}