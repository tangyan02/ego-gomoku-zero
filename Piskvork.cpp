


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


const char* infotext = "name=\"Ego-Zero\", author=\"TangYan\", version=\"1.0\", country=\"China\", email=\"tangyan1412@foxmail.com\"";

static unsigned seed;
static int boardSize;
static bool piskvorkMessageEnable;
static Game* game;

static int firstCost = -1;

static Node *node;
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
    const int MAXPATH = 250;
    auto prefix = getPrefix();
    auto subfix = string("/model/model_latest.onnx");
    auto fullPath = prefix + subfix;

    setbuf(stdout, NULL);
    if (width != 20 || height != 20) {
        pipeOut("ERROR size of the board");
        return;
    }
    boardSize = width;
    model = new Model();
    model->init(fullPath);

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
    MonteCarloTree mcts = MonteCarloTree(model, 1);
    mcts.release(node);
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
        if (game->getActionIndex(Point(x, y)) == item.first) {
            //pipeOut("MESSAGE selected�� ");
            select = item.second;
        }
    }

    MonteCarloTree mcts = MonteCarloTree(model, 1);
    for (auto item : node->children) {
        if (item.second != select) {
            mcts.release(item.second);
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

        tree_down(x,y);
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

        tree_down(x,y);
    } else {
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

int min(int a,int b) {
    if (a < b)
        return a;
    return b;
}

bool checkNeedBreak(long long passTime, long long thisTimeOut, int simiNum) {
    int total = node->visits;
	if (simiNum > 10 && total > 30) {
        //安全比例，减少误差
        double beta = 1.05;

        //最大值
        int max = -1;
        for (auto item : node->children) {
            int visit = item.second->visits;
            if (visit > max) {
                max = visit;
            }
        }

        //第二大值
        int secondMax = -1;
        for (auto item : node->children) {
            int visit = item.second->visits;
            if (visit > secondMax && visit != max) {
                secondMax = visit;
            }
        }

        //估计值
        int estimateVisit = total - simiNum + ((int)((double)simiNum / (double)passTime * (double)thisTimeOut));

        if ((secondMax + (estimateVisit - total)) * beta < max) {
            pipeOut("MESSAGE prebreak at max %d, secondMax %d, total %d, simiNum %d, estimateVisit %d", max, secondMax, total, simiNum, estimateVisit);
            return true;
        }
    }
    return false;
}

void brain_turn()
{
    MonteCarloTree mcts = MonteCarloTree(model, 1);

    piskvorkMessageEnable = true;

    int thisTimeOut = info_time_left / 20;
    thisTimeOut = min(info_timeout_turn, thisTimeOut);

    if (firstCost == -1) {
        firstCost = info_timeout_match - info_time_left;
        thisTimeOut -= firstCost;
        firstCost = 0;
    }

    pipeOut("MESSAGE time limit %d", thisTimeOut);
    pipeOut("MESSAGE current player %d", game->currentPlayer);

    auto startTime = getSystemTime();
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

        if (checkNeedBreak(passTime, thisTimeOut, simiNum)) {
            break;
        }
    }

    pipeOut("MESSAGE children size %d", node->children.size());
    int max = -1;
    int action = -1;


    if (node->children.size() == 0) {
        auto selectAction = selectActions(*game);
        if (get<0>(selectAction)) {
            action = game->getActionIndex(get<1>(selectAction)[0]);
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

    auto p = game->getPointFromIndex(action);

    pipeOut("MESSAGE : action %d,%d, max %d, total %d rate %.2f score %.2f last simi %d info %s", p.x, p.y, max, total, (float)max / total,score, total-simiNum, info.c_str());
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