


#ifdef _WIN32
#define NOMINMAX
#include "pisqpipe.h"
#include <windows.h>
#include "Game.h"
#include "MCTS.h"
#include "math.h"
#include "SelfPlay.h"
#include <direct.h>
#include <iostream>
#include "Utils.h"
#include "Shape.h"
#include "DfpnVCT.h"

const char* infotext = "name=\"Ego-Zero\", author=\"TangYan\", version=\"2.0\", country=\"China\", email=\"tangyan1412@foxmail.com\"";

static unsigned seed;
static int boardSize;
static bool piskvorkMessageEnable;
static Game* game;

static int firstCost = -1;
static double exp_factor = 5.0;
static int searchThreadCount = 1;

static Node* node;
static Model* model;

using namespace std;

string getPrefix() {
    char path[MAX_PATH];

    if (GetModuleFileName(NULL, path, MAX_PATH) != 0) {
        int lastSlash = -1;

        for (int i = strlen(path); i >= 0; --i) {
            if (path[i] == '\\') {
                lastSlash = i;
                break;
            }
        }

        if (lastSlash > 0 && lastSlash + 1 <= strlen(path)) {
            auto result = string(&path[0], &path[lastSlash]);
            return result;
        }
    }
    return "";
}

void brain_init()
{
    pipeOut("MESSAGE : STARTING...");
    const int MAXPATH = 250;
    auto prefix = getPrefix();
    auto subfix = string("/model/model_best.onnx");

    auto fullPath = prefix + subfix;

    setbuf(stdout, NULL);
    if (width != 20 || height != 20) {
        pipeOut("ERROR size of the board");
        return;
    }
    boardSize = width;
    model = new Model();
    model->init(fullPath, "cpu");

    pipeOut("MESSAGE : LOADED");

    game = new Game(boardSize);
    node = new Node();
    initShape();
    pipeOut("OK");
}

void brain_restart()
{
    delete game;
    game = new Game(boardSize);
    MonteCarloTree mcts = MonteCarloTree(model, exp_factor);
    node->release();
    node = new Node();
    pipeOut("MESSAGE : RESTARTED");
    pipeOut("OK");
}

int isFree(int x, int y)
{
    return x >= 0 && y >= 0 && x < width && y < height && game->board[x][y] == 0;
}

void tree_down(int x, int y) {

    Node* select = nullptr;
    for (auto item : node->children) {
        int visit = item.second->visits;
        if (Point(x, y) == item.first) {
            //pipeOut("MESSAGE selected�� ");
            select = item.second;
        }
    }

    MonteCarloTree mcts = MonteCarloTree(model, exp_factor);
    for (auto item : node->children) {
        if (item.second != select) {
            item.second->release();
        }
    }

    if (select != nullptr) {
        node = select;
    }

    if (select == nullptr && !game->historyMoves.empty()) {
        pipeOut("MESSAGE =====renew node====== ");
        node = new Node();
    }

}

void brain_my(int x, int y)
{
    if (isFree(x, y)) {
        game->currentPlayer = 1;
        //pipeOut("MESSAGE my move %d %d", x, y);
        game->makeMove(Point(x, y));

        tree_down(x, y);
    }
    else {
        pipeOut("ERROR my move [%d,%d]", x, y);
    }
}

void brain_opponents(int x, int y)
{
    if (isFree(x, y)) {
        game->currentPlayer = 2;
        //pipeOut("MESSAGE opp move %d %d", x, y);
        game->makeMove(Point(x, y));

        tree_down(x, y);
    }
    else {
        pipeOut("ERROR opponents's move [%d,%d]", x, y);
    }
}

void brain_block(int x, int y)
{
    if (isFree(x, y)) {
        game->board[x][y] = 3;
    }
    else {
        pipeOut("ERROR winning move [%d,%d]", x, y);
    }
}

int brain_takeback(int x, int y)
{
    if (x >= 0 && y >= 0 && x < width && y < height && game->board[x][y] != 0) {
        game->board[x][y] = 0;
        return 0;
    }
    return 2;
}

unsigned rnd(unsigned n)
{
    seed = seed * 367413989 + 174680251;
    return (unsigned)(UInt32x32To64(n, seed) >> 32);
}

int min(int a, int b) {
    if (a < b)
        return a;
    return b;
}

int max(int a, int b) {
    if (a > b)
        return a;
    return b;
}

bool checkNeedBreak(long long passTime, long long thisTimeOut, int simiNum, int searchThreadCount) {
    int total = node->visits;
	if (passTime / (float)thisTimeOut > 0.25) {
        //安全比例，减少误差
        double beta = 2;
        Point maxP;

        //最大值
        int max = -1;
        for (auto item : node->children) {
            int visit = item.second->visits;
            if (visit > max) {
                max = visit;
                maxP = item.first;
            }
        }

        //第二大值
        int secondMax = -1;
        Point secondP;
        for (auto item : node->children) {
            int visit = item.second->visits;
            if (visit > secondMax && visit != max) {
                secondMax = visit;
                secondP = item.first;
            }
        }

        //估计值
        int estimateVisit = total - simiNum + ((int)((double)simiNum * searchThreadCount / (double)passTime * (double)thisTimeOut));

        if ((secondMax + (estimateVisit - total)) * beta < max && max > 10) {
            pipeOut("MESSAGE prebreak at max %d (%d,%d), secondMax %d (%d,%d), total %d, simiNum %d, estimateVisit %d", max, maxP.x, maxP.y, secondMax, secondP.x, secondP.y, total, simiNum, estimateVisit);
            return true;
        }
        if (simiNum>1 && (((double)passTime / (double)simiNum) + passTime > (double)thisTimeOut)) {
            pipeOut("MESSAGE stop for estimate may timeout");
            return true;
        }
    }
    return false;
}

// ============ brain_turn 辅助函数 ============

// 搜索己方 VCT，找到则直接走并返回 true
bool searchMyVCT(MonteCarloTree& mcts, int myVctBudget, int& thisTimeOut) {
    Game gameCopy = *game;
    std::atomic<bool> vctRunning(true);
    int vctMaxNodes = 200000;
    auto vctStart = getSystemTime();
    auto myVctResult = dfpnVCTIterDeepen(gameCopy.currentPlayer, gameCopy, vctRunning, vctMaxNodes, myVctBudget);
    auto vctCost = getSystemTime() - vctStart;
    if (myVctResult.found && !myVctResult.moves.empty()) {
        auto p = myVctResult.moves[0];
        std::string pvStr;
        for (size_t i = 0; i < myVctResult.moves.size() && i < 20; i++) {
            if (i > 0) pvStr += " ";
            pvStr += std::to_string(myVctResult.moves[i].x) + "," + std::to_string(myVctResult.moves[i].y);
        }
        pipeOut("MESSAGE VCT found! cost %dms, action %d,%d, pv: %s", (int)vctCost, p.x, p.y, pvStr.c_str());
        do_mymove(p.x, p.y);
        mcts.search(*game, node, 1);
        return true;
    }
    pipeOut("MESSAGE no my VCT, cost %dms/%dms budget", (int)vctCost, myVctBudget);
    thisTimeOut -= (int)vctCost;
    return false;
}

// 搜索对手 VCT，逐层迭代 ban 掉必死点。找到唯一活路则直接走并返回 true
bool searchOpponentVCT(MonteCarloTree& mcts, int oppVctBudget, int& thisTimeOut, vector<Point>& vctBannedMoves) {
    int opponent = 3 - game->currentPlayer;
    auto oppVctStart = getSystemTime();

    auto [win, candidateMoves, info] = selectActions(*game);

    if (!win && !candidateMoves.empty()) {
        // 快速预判对手是否有 VCT
        Game gameCopy = *game;
        std::atomic<bool> vctRunning(true);
        int quickNodes = 50000;
        dfpnVCTSetThreeLimit(1);
        auto quickResult = dfpnVCT(opponent, gameCopy, vctRunning, quickNodes);
        dfpnVCTResetThreeLimit();

        if (quickResult.found || !quickResult.exhaustive) {
            pipeOut("MESSAGE opponent may have VCT (quick=%s), scanning candidates layer by layer...",
                    quickResult.found ? "proven" : "inconclusive");

            vector<int> moveStatus(candidateMoves.size(), 0);
            int vctMaxNodes = 200000;
            std::vector<int> threeLimits = {1, 4, 7, 10, 13, 16};

            for (int threeLimit : threeLimits) {
                if ((int)(getSystemTime() - oppVctStart) >= oppVctBudget) break;

                dfpnVCTSetThreeLimit(threeLimit);
                int layerBanned = 0;

                for (int i = 0; i < (int)candidateMoves.size(); i++) {
                    if (moveStatus[i] != 0) continue;
                    if ((int)(getSystemTime() - oppVctStart) >= oppVctBudget) break;

                    Game testGame = *game;
                    testGame.makeMove(candidateMoves[i]);
                    std::atomic<bool> testRunning(true);
                    auto testResult = dfpnVCT(opponent, testGame, testRunning, vctMaxNodes);

                    if (testResult.found) {
                        moveStatus[i] = 1;
                        vctBannedMoves.push_back(candidateMoves[i]);
                        layerBanned++;
                    } else if (testResult.exhaustive) {
                        moveStatus[i] = 2;
                    }
                }

                int undecided = 0, safe = 0;
                for (int s : moveStatus) {
                    if (s == 0) undecided++;
                    else if (s == 2) safe++;
                }

                pipeOut("MESSAGE layer threeLimit=%d: +%d banned, %d safe, %d undecided",
                        threeLimit, layerBanned, safe, undecided);

                // 只剩 1 个非 banned 且全部已决 → 直接走
                int remaining = (int)candidateMoves.size() - (int)vctBannedMoves.size();
                if (remaining == 1 && undecided == 0) {
                    for (int i = 0; i < (int)candidateMoves.size(); i++) {
                        if (moveStatus[i] != 1) {
                            auto totalCost = getSystemTime() - oppVctStart;
                            thisTimeOut -= (int)totalCost;
                            dfpnVCTResetThreeLimit();
                            pipeOut("MESSAGE unique saving move %d,%d (%d banned), cost %dms",
                                    candidateMoves[i].x, candidateMoves[i].y,
                                    (int)vctBannedMoves.size(), (int)totalCost);
                            do_mymove(candidateMoves[i].x, candidateMoves[i].y);
                            mcts.search(*game, node, 1);
                            return true;
                        }
                    }
                }

                if (undecided == 0) break;
            }

            dfpnVCTResetThreeLimit();
        } else {
            pipeOut("MESSAGE no opponent VCT (quick proven), cost %dms",
                    (int)(getSystemTime() - oppVctStart));
        }
    }

    auto totalOppCost = getSystemTime() - oppVctStart;
    thisTimeOut -= (int)totalOppCost;

    // 后处理
    if (!vctBannedMoves.empty()) {
        int remaining = (int)candidateMoves.size() - (int)vctBannedMoves.size();
        if (remaining == 0) {
            pipeOut("MESSAGE all %d moves banned! opponent VCT unavoidable, cost %dms",
                    (int)candidateMoves.size(), (int)totalOppCost);
            vctBannedMoves.clear();
        } else {
            pipeOut("MESSAGE %d banned moves, %d remaining, cost %dms",
                    (int)vctBannedMoves.size(), remaining, (int)totalOppCost);
        }
    }
    return false;
}

// 从 MCTS 树中剪除 banned 分支
void applyBannedMoves(MonteCarloTree& mcts, const vector<Point>& vctBannedMoves) {
    mcts.search(*game, node, 1);  // 先 expand root
    for (auto it = node->children.begin(); it != node->children.end(); ) {
        bool isBanned = false;
        for (auto& bm : vctBannedMoves) {
            if (it->first.x == bm.x && it->first.y == bm.y) {
                isBanned = true;
                break;
            }
        }
        if (isBanned) {
            it->second->release();
            it = node->children.erase(it);
        } else {
            ++it;
        }
    }
    pipeOut("MESSAGE MCTS: %d branches remaining after banning", (int)node->children.size());
}

// MCTS 搜索循环 + 选最优走法并落子
void mctsSearchAndPlay(MonteCarloTree& mcts, int thisTimeOut) {
    auto startTime = getSystemTime();
    int simiNum = 0;
    while (true) {
        mcts.search(*game, node, 1);
        auto passTime = getSystemTime() - startTime;
        simiNum += 1;
        if (passTime > thisTimeOut) break;
        if (node->children.size() <= 1) break;

        auto [win, moves, selectInfo] = selectActions(*game);
        if (win) break;
        if (checkNeedBreak(passTime, thisTimeOut, simiNum, searchThreadCount)) break;
    }

    pipeOut("MESSAGE children size %d", node->children.size());
    int max = -1;
    Point action;

    if (node->children.size() == 0) {
        auto selectAction = selectActions(*game);
        if (get<0>(selectAction)) {
            action = get<1>(selectAction)[0];
            max = 1;
        }
    }

    string info = node->selectInfo;
    int total = node->visits;
    float score = 0;
    for (auto item : node->children) {
        int visit = item.second->visits;
        if (visit > max) {
            action = item.first;
            max = visit;
            score = item.second->value_sum / visit;
        }
    }

    if (max == 1) {
        double probMax = 0;
        for (auto item : node->children) {
            double prob = item.second->prior_prob;
            if (prob > probMax) {
                action = item.first;
                probMax = prob;
                score = item.second->value_sum / item.second->visits;
            }
        }
    }

    auto p = action;
    pipeOut("MESSAGE : action %d,%d, max %d, total %d rate %.2f score %.2f last simi %d info %s",
            p.x, p.y, max, total, (float)max / total, score, total - simiNum, info.c_str());
    do_mymove(p.x, p.y);
    mcts.search(*game, node, 1);
}

// ============ brain_turn 主函数 ============

void brain_turn()
{
    MonteCarloTree mcts = MonteCarloTree(model, exp_factor);
    piskvorkMessageEnable = true;

    // 动态时间分配
    int movesPlayed = boardSize * boardSize - game->emptyCount;
    int myMoves = (movesPlayed + 1) / 2;
    int estimatedRemaining = max(10, 40 - myMoves);
    float bonus = (myMoves < 5) ? 1.5f : 1.0f;
    int thisTimeOut = (int)(info_time_left / estimatedRemaining * bonus);
    thisTimeOut = min(info_timeout_turn, thisTimeOut);

    if (firstCost == -1) {
        firstCost = info_timeout_match - info_time_left;
        thisTimeOut -= firstCost;
        firstCost = 0;
    }

    int vctTimeOut = thisTimeOut / 8;

    pipeOut("MESSAGE time limit %d", thisTimeOut);
    pipeOut("MESSAGE current player %d", game->currentPlayer);
    pipeOut("MESSAGE time limit updated %d", thisTimeOut);

    // VCF 快速判断
    auto vcfMoves = game->getMyVCFMoves();
    if (!vcfMoves.empty()) {
        auto p = vcfMoves[0];
        pipeOut("MESSAGE : action %d,%d vcf!", p.x, p.y);
        do_mymove(p.x, p.y);
        return;
    }

    // VCT 搜索
    int myVctBudget = vctTimeOut / 2;
    int oppVctBudget = vctTimeOut - myVctBudget;
    vector<Point> vctBannedMoves;

    if (searchMyVCT(mcts, myVctBudget, thisTimeOut)) return;
    if (searchOpponentVCT(mcts, oppVctBudget, thisTimeOut, vctBannedMoves)) return;

    // 剪除 banned 分支
    if (!vctBannedMoves.empty()) {
        applyBannedMoves(mcts, vctBannedMoves);
    }

    // MCTS 搜索 + 落子
    mctsSearchAndPlay(mcts, thisTimeOut);
}

void brain_end()
{
}

#ifdef DEBUG_EVAL
#include <windows.h>

void brain_eval(int x, int y)
{
    HDC dc;
    HWND wnd;
    RECT rc;
    char c;
    wnd = GetForegroundWindow();
    dc = GetDC(wnd);
    GetClientRect(wnd, &rc);
    c = (char)(board[x][y] + '0');
    TextOut(dc, rc.right - 15, 3, &c, 1);
    ReleaseDC(wnd, dc);
}

#endif


#endif //_WIN32