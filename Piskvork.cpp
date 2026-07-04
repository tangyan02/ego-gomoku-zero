


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

void brain_turn()
{
    MonteCarloTree mcts = MonteCarloTree(model, exp_factor);

    piskvorkMessageEnable = true;

    // 动态时间分配：基于预估剩余步数
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
    //pipeOut("MESSAGE vctTimeOut limit %d", vctTimeOut);
    pipeOut("MESSAGE current player %d", game->currentPlayer);

    auto startTime = getSystemTime();
    //mcts.search(*game, node, 1);
    //game->vctTimeOut = vctTimeOut;
    //pruning(node, *game, "MESSAGE ");
    //auto vctCost = getSystemTime() - startTime;
    //thisTimeOut -= vctCost;
    pipeOut("MESSAGE time limit updated %d", thisTimeOut);
    
    auto vcfMoves = game->getMyVCFMoves();
    if (!vcfMoves.empty()) {
        auto p = vcfMoves[0];
        pipeOut("MESSAGE : action %d,%d vcf!", p.x, p.y);
        do_mymove(p.x, p.y);
        return;
    }

    // VCT 搜索：1/16 总预算算自己，1/16 算对手 + 找救命解
    int myVctBudget = vctTimeOut / 2;
    int oppVctBudget = vctTimeOut - myVctBudget;
    vector<Point> vctBannedMoves;  // 确认必死的落子（落子后对手仍有 VCT）

    // (1) 自己的 VCT
    {
        Game gameCopy = *game;
        std::atomic<bool> vctRunning(true);
        int vctMaxNodes = 200000;
        auto vctStart = getSystemTime();
        auto myVctResult = dfpnVCTIterDeepen(gameCopy.currentPlayer, gameCopy, vctRunning, vctMaxNodes, myVctBudget);
        auto vctCost = getSystemTime() - vctStart;
        if (myVctResult.found && !myVctResult.moves.empty()) {
            auto p = myVctResult.moves[0];
            pipeOut("MESSAGE VCT found! cost %dms, action %d,%d", (int)vctCost, p.x, p.y);
            do_mymove(p.x, p.y);
            mcts.search(*game, node, 1);
            return;
        }
        pipeOut("MESSAGE no my VCT, cost %dms/%dms budget", (int)vctCost, myVctBudget);
        thisTimeOut -= (int)vctCost;
    }

    // (2) 对手的 VCT + 寻找救命解（迭代加深：逐步放宽 threeLimit）
    {
        int opponent = 3 - game->currentPlayer;
        auto oppVctStart = getSystemTime();

        // 先获取候选落子，用于分配时间预算
        auto [win, candidateMoves, info] = selectActions(*game);
        int numCandidates = max(1, (int)candidateMoves.size());

        // 首次 VCT 检测预算 = 总预算 / (候选数+1)，保守分配留更多给 banned 检测
        int firstCheckBudget = max(1, oppVctBudget / (numCandidates + 1));

        // 检测对手是否有 VCT
        // dfpnVCTIterDeepen 内部自动处理 attackPlayer != currentPlayer 时的 hash 修正
        Game gameCopy = *game;
        std::atomic<bool> vctRunning(true);
        int vctMaxNodes = 200000;
        auto oppResult = dfpnVCTIterDeepen(opponent, gameCopy, vctRunning, vctMaxNodes, firstCheckBudget);
        auto oppCheckCost = getSystemTime() - oppVctStart;

        if (oppResult.found) {
            pipeOut("MESSAGE opponent has VCT! check cost %dms/%dms budget, finding banned moves...",
                    (int)oppCheckCost, firstCheckBudget);
            int savingBudget = oppVctBudget - (int)oppCheckCost;

            int testedCount = 0;
            if (!win && !candidateMoves.empty()) {
                // 对每个候选落子，检查落子后对手是否还有 VCT
                // found=true → 这个点必死，加入 banned（确定性结论，增量安全）
                int perMoveBudget = max(1, savingBudget / (int)candidateMoves.size());
                for (auto& move : candidateMoves) {
                    if ((int)(getSystemTime() - oppVctStart) >= oppVctBudget) break;

                    Game testGame = *game;
                    testGame.makeMove(move);
                    std::atomic<bool> testRunning(true);
                    auto testResult = dfpnVCTIterDeepen(opponent, testGame, testRunning, vctMaxNodes, perMoveBudget);
                    testedCount++;
                    // found=true 是确定性结论（证明有 VCT），这个点必死
                    if (testResult.found) {
                        vctBannedMoves.push_back(move);
                    }
                }
            }
            bool allTested = (testedCount == (int)candidateMoves.size());

            auto totalOppCost = getSystemTime() - oppVctStart;
            thisTimeOut -= (int)totalOppCost;

            int remaining = (int)candidateMoves.size() - (int)vctBannedMoves.size();
            if (allTested && remaining == 1) {
                // 全部测试完，只剩唯一非 banned 点 → 直接走
                for (auto& move : candidateMoves) {
                    bool banned = false;
                    for (auto& bm : vctBannedMoves) {
                        if (move.x == bm.x && move.y == bm.y) { banned = true; break; }
                    }
                    if (!banned) {
                        pipeOut("MESSAGE unique saving move %d,%d (%d banned), cost %dms",
                                move.x, move.y, (int)vctBannedMoves.size(), (int)totalOppCost);
                        do_mymove(move.x, move.y);
                        mcts.search(*game, node, 1);
                        return;
                    }
                }
            } else if (allTested && remaining == 0) {
                pipeOut("MESSAGE all %d moves banned! opponent VCT unavoidable, cost %dms",
                        (int)candidateMoves.size(), (int)totalOppCost);
                vctBannedMoves.clear();  // 全部必死，不限制 MCTS（让它选最不差的）
            } else if (vctBannedMoves.empty()) {
                pipeOut("MESSAGE tested %d/%d, no banned moves found, cost %dms",
                        testedCount, (int)candidateMoves.size(), (int)totalOppCost);
            } else {
                pipeOut("MESSAGE %d banned moves (tested %d/%d), will exclude from MCTS, cost %dms",
                        (int)vctBannedMoves.size(), testedCount, (int)candidateMoves.size(), (int)totalOppCost);
            }
        } else {
            pipeOut("MESSAGE no opponent VCT, cost %dms/%dms budget", (int)oppCheckCost, oppVctBudget);
            thisTimeOut -= (int)oppCheckCost;
        }
    }

    // 如果有 banned moves，先 expand root 然后从 MCTS 中排除 banned 分支
    if (!vctBannedMoves.empty()) {
        mcts.search(*game, node, 1);  // 先 expand root
        // 删除 banned 对应的 children
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

    startTime = getSystemTime();
    int simiNum = 0;
    while (true) {
		mcts.search(*game, node, 1);
        auto passTime = getSystemTime() - startTime;
        simiNum += 1;
        if (passTime > thisTimeOut) {
            break;
        }

        if (node->children.size() <= 1) {
            break;
        }

        auto [win, moves, selectInfo] = selectActions(*game);
        if (win) {
            break;
        }

        if (checkNeedBreak(passTime, thisTimeOut, simiNum, searchThreadCount)) {
            break;
        }
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

    pipeOut("MESSAGE : action %d,%d, max %d, total %d rate %.2f score %.2f last simi %d info %s", p.x, p.y, max, total, (float)max / total, score, total - simiNum, info.c_str());
    do_mymove(p.x, p.y);

    mcts.search(*game, node, 1);
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