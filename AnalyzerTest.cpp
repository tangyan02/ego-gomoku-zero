
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
    game.printBoard();
    auto moves = game.getEmptyPoints();
    auto winnerMove = getWinningMoves(game.currentPlayer, game, moves);
    cout << winnerMove.size() << endl;
    if (winnerMove.size() == 2) {
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
    game.printBoard();
    auto moves = game.getEmptyPoints();
    auto winnerMove = getActiveFourMoves(game.currentPlayer, game, moves);
    cout << winnerMove.size() << endl;
    if (winnerMove.size() == 1) {
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
    game.printBoard();
    auto moves = game.getEmptyPoints();
    auto winnerMove = getActiveFourMoves(2, game, moves);
    cout << winnerMove.size() << endl;
    if (winnerMove.size() == 2) {
        return true;
    } else {
        return false;
    }
}