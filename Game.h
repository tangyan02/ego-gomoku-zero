#ifndef EGO_GOMOKU_ZERO_GAME_H
#define EGO_GOMOKU_ZERO_GAME_H

#include <iostream>
#include <vector>
#include <torch/torch.h>
#include <unordered_set>

class Point {
public:
    int x;
    int y;

    Point();

    Point(int x, int y);
};


const int MAX_BOARD_SIZE = 15;
const int CONNECT = 5;

#define NONE_P 0
#define BLACK 1
#define WHITE 2

class Game {
public:
    int board[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    std::vector<Point> historyMoves;
    Point lastAction;
    Point lastLastAction;
    int boardSize;
    int currentPlayer;

    Game(int boardSize);

    int getOtherPlayer();

    int getActionIndex(Point &p);

    Point getPointFromIndex(int actionIndex);

    std::vector<Point> getEmptyPoints();

    torch::Tensor getState();

    bool isGameOver();

    void printBoard();

    bool makeMove(Point p);

    bool checkWin(int row, int col, int player);

};


#endif //EGO_GOMOKU_ZERO_GAME_H