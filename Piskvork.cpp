


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


const char* infotext = "name=\"Ego-Zero\", author=\"TangYan\", version=\"1.0\", country=\"China\", email=\"tangyan1412@foxmail.com\"";

static unsigned seed;
static int boardSize;
static bool piskvorkMessageEnable;
static Game* game;
static torch::jit::Module network;

static auto device = torch::kCPU;

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
	auto subfix = string("/model/model_latest.pt");
	auto fullPath = prefix + subfix;

	setbuf(stdout, NULL);
	if (width != 20 || height != 20) {
		pipeOut("ERROR size of the board");
		return;
	}
	boardSize = width;
	network = getNetwork(device, fullPath);
	pipeOut("MESSAGE : LOADED");

	game = new Game(boardSize);
	pipeOut("OK");
}

void brain_restart()
{
	delete game;
	game = new Game(boardSize);
	pipeOut("MESSAGE : RESTARTED");
	pipeOut("OK");
}

int isFree(int x, int y)
{
	return x >= 0 && y >= 0 && x < width && y < height && game->board[x][y] == 0;
}

void brain_my(int x, int y)
{
	if (isFree(x, y)) {
		game->currentPlayer = 1;
		game->makeMove(Point(x, y));
	}
	else {
		pipeOut("ERROR my move [%d,%d]", x, y);
	}
}

void brain_opponents(int x, int y)
{
	if (isFree(x, y)) {
		game->currentPlayer = 2;
		game->makeMove(Point(x, y));
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

int min(int a,int b) {
	if (a < b)
		return a;
	return b;
}

void brain_turn()
{
    auto nextActions = selectActions(*game);
	if (get<1>(nextActions).size() == 1 || get<0>(nextActions)) {
        int actionIndex = game->getActionIndex(get<1>(nextActions)[0]);
        auto p = game->getPointFromIndex(actionIndex);
        pipeOut("MESSAGE : action %d,%d %s", p.x, p.y, get<2>(nextActions));
        do_mymove(p.x, p.y);
        return;
    }
	MonteCarloTree mcts = MonteCarloTree(&network, device, 1);

	piskvorkMessageEnable = true;

	int thisTimeOut = info_time_left / 10;
	thisTimeOut = min(info_timeout_turn, thisTimeOut);

	pipeOut("MESSAGE time limit %d", thisTimeOut);
	pipeOut("MESSAGE current player %d", game->currentPlayer);
	int comboTimeOut = thisTimeOut * 1 / 3;

	Node node;
	auto startTime = getSystemTime();
	int simiNum = 0;
	while (true) {
		auto currentTime = getSystemTime();

		mcts.search(*game, &node, 1, comboTimeOut);
		auto passTime = currentTime - startTime;
		simiNum += 1;
		if (passTime > thisTimeOut) {
			break;
		}
	}

	int max = 0;
	int action = -1;
	for (auto item : node.children) {
		int visit = item.second->visits;
		if (visit > max) {
			action = item.first;
			max = visit;
		}
	}

	auto p = game->getPointFromIndex(action);

	pipeOut("MESSAGE : action %d,%d, max %d, total %d rate %f simi num %d info %s", p.x, p.y, max, node.visits, (float)max / node.visits, simiNum, node.selectInfo.c_str());
	mcts.release(&node);
	do_mymove(p.x, p.y);
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