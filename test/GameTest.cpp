
#include "../Game.h"

using namespace std;
static const int boardSize = 15;

TEST_CASE("testGetState") {

    Game game(boardSize);

    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(0, 4));

    //game.printBoard();

    CHECK(1);
}

TEST_CASE("testGetNearEmptyPoints") {
    Game game(boardSize);

    game.makeMove(Point(8, 8));
    game.makeMove(Point(8, 9));
    game.makeMove(Point(0, 2));

    auto points = game.getEmptyPoints();
    // cout << points.size() << endl;
    for (const auto &item: points) {
        game.board[item.x][item.y] = 3;
    }
    // game.printBoard();
    CHECK(points.size() == 42);
}