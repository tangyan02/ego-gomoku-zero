


#ifdef _WIN32
#define NOMINMAX
#include "pisqpipe.h"
#include <windows.h>
#include "Game.h"
#include "MCTS.h"
#include "math.h"
#include <sys/timeb.h>
#include "SelfPlay.h"
#include <direct.h>  

const char* infotext = "name=\"Ego\", author=\"TangYan\", version=\"7.0\", country=\"China\", email=\"tangyan1412@foxmail.com\"";

static unsigned seed;
static int boardSize;
static bool piskvorkMessageEnable;
static Game* game;
static torch::jit::Module network;

static auto device = torch::kCPU;

long long getSystemTime() {
	struct timeb t;
	ftime(&t);
	return 1000 * t.time + t.millitm;
}

void brain_init()
{
	const int MAXPATH = 250;
	char buffer[MAXPATH];
	getcwd(buffer, MAXPATH);
	pipeOut("MESSAGE The current directory is: %s", buffer);

	pipeOut("MESSAGE : START TO INIT");
	pipeOut("MESSAGE : START TO INIT2");
	setbuf(stdout, NULL);
	if (width != 15 || height != 15) {
		pipeOut("ERROR size of the board");
		return;
	}
	boardSize = width;
	network = getNetwork(device);
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
	MonteCarloTree mcts = MonteCarloTree(&network, device, 1);

	piskvorkMessageEnable = true;

	int thisTimeOut = info_time_left / 10;
	thisTimeOut = min(info_timeout_turn, thisTimeOut);

	pipeOut("MESSAGE time limit %d", thisTimeOut);
	pipeOut("MESSAGE current player %d", game->currentPlayer);
	int timeOut = thisTimeOut / 4 * 3;
	int comboTimeOut = thisTimeOut - timeOut;
	

	Node node;
	auto startTime = getSystemTime();
	int simiNum = 0;
	while (true) {
		auto currentTime = getSystemTime();

		mcts.search(*game, &node, 1);
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

	pipeOut("MESSAGE : action %d,%d, max %d, total %d rate %f simi num %d", p.x, p.y, max, node.visits, (float)max / node.visits, simiNum);
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