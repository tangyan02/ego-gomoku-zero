
#include "Game.h"

using namespace std;
static const int boardSize = 15;

bool testGetState() {

    cout << "testGetState" << endl;
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

    cout << "testGetState2" << endl;
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

    cout << "testGetState3" << endl;
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


bool testGetNearEmptyPoints() {
    cout << "testGetNearEmptyPoints" << endl;
    Game game(boardSize);

    game.makeMove(Point(8, 8));
    game.makeMove(Point(8, 9));
    game.makeMove(Point(0, 2));

    auto points = game.getEmptyPoints();
    cout << points.size() << endl;
    for (const auto &item: points) {
        game.board[item.x][item.y] = 3;
    }
    game.printBoard();
    if (points.size() == 77) {
        return true;
    } else {
        return false;
    }
}