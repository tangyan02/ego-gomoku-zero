#ifndef EGO_GOMOKU_ZERO_GAME_H
#define EGO_GOMOKU_ZERO_GAME_H

#include <iostream>
#include <vector>
#include <unordered_set>
#include <cstdint>

using namespace std;

class Point {
public:
    int x;
    int y;

    Point();

    Point(int x, int y);

    bool isNull();

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Point & other) const {
        return x != other.x || y != other.y;
    }
};

struct PointHash {
    size_t operator()(const Point &p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

struct PointEqual {
    bool operator()(const Point &lhs, const Point &rhs) const {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
};

const int MAX_BOARD_SIZE = 20;
const int CONNECT = 5;
const int INPUT_CHANNELS = 4;  // 通道: 己方棋子, 对方棋子, 最近一步, 最近两步

#define NONE_P 0
#define BLACK 1
#define WHITE 2
#define FLAG1 3
#define FLAG2 4

// Zobrist hash 表（全局初始化一次）
struct ZobristTable {
    uint64_t pieces[MAX_BOARD_SIZE][MAX_BOARD_SIZE][3];  // [row][col][player: 0=empty, 1=black, 2=white]
    uint64_t currentPlayerHash;  // 用于区分当前玩家
    bool initialized = false;
    void init();
};
extern ZobristTable zobristTable;

class Game {
public:
    int board[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    vector<Point> historyMoves;
    Point lastAction;
    Point lastLastAction;
    int boardSize;
    int currentPlayer;
    int emptyCount = 0;  // 空位计数，用于 O(1) 的 isGameOver 判断
    int vctTimeOut = 0;
    uint64_t zobristHash = 0;  // 局面的 Zobrist hash，makeMove 时增量更新

    bool myVcfDone = false;
    bool oppVcfDone = false;
    vector<Point> myVcfMoves;
    vector<Point> myAllAttackMoves;
    vector<Point> oppVcfMoves;
    vector<Point> oppVcfAttackMoves;
    vector<Point> oppVcfDefenceMoves;


    Game(int boardSize);

    int getOtherPlayer();

    int getActionIndex(Point &p);

    Point getPointFromIndex(int actionIndex);

    vector<Point> getEmptyPoints();

    vector<Point> getAllEmptyPoints();

    vector<Point> getNearEmptyPoints(int range = 2);

    vector<vector<vector<float>>> getState();

    // 高性能版本：直接写入预分配的连续内存，避免动态分配
    // buffer 大小须 >= channels * boardSize * boardSize
    void getState(float* buffer, int channels) const;

    bool isGameOver();

    void printBoard(const string &part = "");

    bool makeMove(Point p);

    bool checkWin(int row, int col, int player);

    vector<Point> getMyVCFMoves();

    vector<Point> getOppVCFMoves();

};

std::vector<Point> removeDuplicates(const std::vector<Point> &points);

#endif //EGO_GOMOKU_ZERO_GAME_H