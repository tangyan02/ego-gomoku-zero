
#include "Game.h"
#include "Analyzer.h"

using namespace std;

bool testGetWinnerMove() {
    cout << "testWinnerMove" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 3));
    auto moves = game.getEmptyPoints();
    auto result = getWinningMoves(game.currentPlayer, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 2) {
        return true;
    } else {
        return false;
    }
}


bool testGetActiveFourMoves() {
    cout << "testGetActiveFourMoves" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));
    auto moves = game.getEmptyPoints();
    auto result = getActiveFourMoves(game.currentPlayer, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 1) {
        return true;
    } else {
        return false;
    }
}

bool testGetActiveFourMoves2() {
    cout << "testGetActiveFourMoves2" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(0, 4));
    auto moves = game.getEmptyPoints();
    auto result = getActiveFourMoves(2, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 2) {
        return true;
    } else {
        return false;
    }
}

bool testGetActiveFourMoves3() {
    cout << "testGetActiveFourMoves3" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(0, 0));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(1, 3));
    game.makeMove(Point(0, 8));
    game.makeMove(Point(1, 4));

    auto moves = game.getEmptyPoints();
    auto result = getActiveFourMoves(1, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 2) {
        return true;
    } else {
        return false;
    }
}

bool testGetSleepyFourMoves() {
    cout << "testGetSleepyFourMoves" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    auto result = getSleepyFourMoves(1, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 2) {
        return true;
    } else {
        return false;
    }
}

bool testGetSleepyFourMoves2() {
    cout << "testGetSleepyFourMoves2" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    auto result = getSleepyFourMoves(1, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 2) {
        return true;
    } else {
        return false;
    }
}


bool testGetThreeDefenceMoves() {
    cout << "testGetThreeDefenceMoves" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    auto result = getThreeDefenceMoves(2, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 3) {
        return true;
    } else {
        return false;
    }
}

bool testGetThreeDefenceMoves2() {
    cout << "testGetThreeDefenceMoves2" << endl;
    Game game;
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    auto result = getThreeDefenceMoves(2, game, moves);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 2) {
        return true;
    } else {
        return false;
    }
}