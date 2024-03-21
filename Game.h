#ifndef EGO_GOMOKU_ZERO_GAME_H
#define EGO_GOMOKU_ZERO_GAME_H

#include <iostream>
#include <vector>
#include <unordered_set>

using namespace std;

class Point {
public:
    int x;
    int y;

    Point();

    Point(int x, int y);

    bool isNull();
};

struct PointHash {
    size_t operator()(const Point &p) const {
        return hash<int>()(p.x) ^ hash<int>()(p.y);
    }
};

struct PointEqual {
    bool operator()(const Point &lhs, const Point &rhs) const {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
};

const int MAX_BOARD_SIZE = 20;
const int CONNECT = 5;

#define NONE_P 0
#define BLACK 1
#define WHITE 2

class Game {
public:
    int board[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    vector<Point> historyMoves;
    Point lastAction;
    Point lastLastAction;
    int boardSize;
    int currentPlayer;

    bool myVcfDone = false;
    bool oppVcfDone = false;
    vector<Point> myVcfMoves;
    vector<Point> oppVcfMoves;


    Game(int boardSize);

    int getOtherPlayer();

    int getActionIndex(Point &p);

    Point getPointFromIndex(int actionIndex);

    vector<Point> getEmptyPoints();

    vector<Point> getNearEmptyPoints(int range = 2);

    vector<vector<vector<float>>> getState();

    bool isGameOver();

    void printBoard(const string &part = "");

    bool makeMove(Point p);

    bool checkWin(int row, int col, int player);

    vector<Point> getMyVCFMoves();

    vector<Point> getOppVCFMoves();
};


#endif //EGO_GOMOKU_ZERO_GAME_H