
#include "Game.h"

using namespace std;
const int boardSize = 15;

bool testGetState() {

    cout << "testWinnerMove" << endl;
    Game game(boardSize);

    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(0, 4));

    game.printBoard();
    auto tensor = game.getState();
    for (int i = 0; i < tensor.size(0); i++) {
        cout << "tensor " << i << endl;
        cout << tensor[i] << endl;
    }
    game.printBoard();

    return true;
}

bool testGetState2() {

    cout << "testWinnerMove" << endl;
    Game game(boardSize);

    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));

    game.printBoard();
    auto tensor = game.getState();
    for (int i = 0; i < tensor.size(0); i++) {
        cout << "tensor " << i << endl;
        cout << tensor[i] << endl;
    }
    game.printBoard();

    return true;
}


bool testGetState3() {

    cout << "testWinnerMove" << endl;
    Game game(boardSize);

    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));

    game.printBoard();
    auto tensor = game.getState();
    for (int i = 0; i < tensor.size(0); i++) {
        cout << "tensor " << i << endl;
        cout << tensor[i] << endl;
    }
    game.printBoard();

    return true;
}