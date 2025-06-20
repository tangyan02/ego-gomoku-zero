#include "../Game.h"
#include "../Analyzer.h"

using namespace std;

TEST_CASE("testGetWinnerMove")
{
    // cout << "testWinnerMove" << endl;
    Game game(boardSize);
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
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 2);
}

TEST_CASE("testGetActiveFourMoves")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));
    auto moves = game.getEmptyPoints();
    auto result = getActiveFourMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 1);
}


TEST_CASE("testGetActiveFourMoves2")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(0, 4));
    auto moves = game.getEmptyPoints();
    auto result = getActiveFourMoves(2, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 2);
}


TEST_CASE("testGetActiveFourMoves3")
{
    Game game(boardSize);
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
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;

    CHECK(result.size() == 2);
}


TEST_CASE("testGetActiveFourMoves4")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 3));
    auto moves = game.getEmptyPoints();
    auto result = getActiveFourMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;

    CHECK(result.size() == 1);
}


TEST_CASE("testGetActiveFourMoves5")
{
    Game game(20);
    game.currentPlayer = 1;

    game.makeMove(Point(19, 2));
    game.makeMove(Point(17, 3));
    game.makeMove(Point(15, 5));
    game.makeMove(Point(13, 5));
    game.makeMove(Point(14, 6));
    game.makeMove(Point(13, 7));
    game.makeMove(Point(14, 4));
    game.makeMove(Point(16, 6));
    game.makeMove(Point(14, 3));
    game.makeMove(Point(14, 5));
    game.makeMove(Point(15, 4));
    game.makeMove(Point(15, 6));
    game.makeMove(Point(13, 4));
    game.makeMove(Point(16, 4));
    game.makeMove(Point(12, 4));
    game.makeMove(Point(11, 4));
    game.makeMove(Point(13, 2));
    game.makeMove(Point(0, 0));

    auto moves = game.getEmptyPoints();
    auto result = getActiveFourMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 2);
}


TEST_CASE("testGetSleepyFourMoves")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    auto result = getSleepyFourMoves(1, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }

    CHECK(result.size() == 2);
}


TEST_CASE("testGetSleepyFourMoves2")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    auto result = getSleepyFourMoves(1, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 2);
}


TEST_CASE("testGetActiveThreeMoves")
{
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(4, 9));
    game.makeMove(Point(2, 16));
    game.makeMove(Point(4, 10));
    game.makeMove(Point(6, 10));
    game.makeMove(Point(3, 10));
    game.makeMove(Point(6, 11));

    auto allMoves = game.getEmptyPoints();
    auto result = getActiveThreeMoves(game.currentPlayer, game, allMoves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 10);
}

TEST_CASE("testGetActiveThreeMoves2")
{
    Game game(20);
    game.currentPlayer = 1;

    game.makeMove(Point(19, 2));
    game.makeMove(Point(17, 3));
    game.makeMove(Point(15, 5));
    game.makeMove(Point(13, 5));
    game.makeMove(Point(14, 6));
    game.makeMove(Point(13, 7));
    game.makeMove(Point(14, 4));
    game.makeMove(Point(16, 6));
    game.makeMove(Point(14, 3));
    game.makeMove(Point(14, 5));
    game.makeMove(Point(15, 4));
    game.makeMove(Point(15, 6));
    game.makeMove(Point(13, 4));
    game.makeMove(Point(16, 4));
    game.makeMove(Point(12, 4));
    game.makeMove(Point(11, 4));

    auto allMoves = game.getEmptyPoints();
    auto result = getActiveThreeMoves(game.currentPlayer, game, allMoves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 8);
}


TEST_CASE("testGetActiveThreeMoves3")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 4));
    game.makeMove(Point(0, 0));
    auto moves = game.getEmptyPoints();
    auto result = getActiveThreeMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 2);
}


TEST_CASE("testGetActiveThreeMoves4")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 2));
    auto moves = game.getEmptyPoints();
    auto result = getActiveThreeMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    //     game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 2);
}


TEST_CASE("testGetSleepyThreeMoves")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    //    vector<Point> moves = {Point(0,10)};
    auto result = getSleepyThreeMoves(1, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    CHECK(result.size() == 2);
}

TEST_CASE("testGetSleepyThreeMoves2")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));

    auto moves = game.getEmptyPoints();
    auto result = getSleepyThreeMoves(1, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 3);
}


TEST_CASE("testGetActiveTwoMoves")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(2, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(2, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(8, 8));

    auto moves = game.getEmptyPoints();
    //    vector<Point> moves;
    //    moves.emplace_back(1,8);
    auto result = getActiveTwoMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    CHECK(result.size() == 2);
}


TEST_CASE("testGetSleepyTwoMoves")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(2, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(2, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(8, 8));

    auto moves = game.getEmptyPoints();
    //    vector <Point> moves;
    //    moves.emplace_back(0, 1);
    auto result = getSleepyTwoMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    CHECK(result.size() == 13);
}


TEST_CASE("testDfsVCF")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(1, 4));
    game.makeMove(Point(2, 0));
    game.makeMove(Point(1, 5));
    game.makeMove(Point(2, 1));
    game.makeMove(Point(1, 6));
    game.makeMove(Point(2, 2));
    game.makeMove(Point(2, 6));
    game.makeMove(Point(1, 3));
    game.makeMove(Point(3, 5));
    game.makeMove(Point(0, 8));
    game.makeMove(Point(6, 6));
    game.makeMove(Point(7, 8));
    game.makeMove(Point(7, 7));

    auto result = dfsVCF(1, 1, game, Point(), Point());
    for (Point& move : result.second)
    {
        game.board[move.x][move.y] = 3;
    }
    CHECK(result.first);
}

TEST_CASE("testDfsVCF2")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(1, 4));
    game.makeMove(Point(7, 3));
    game.makeMove(Point(1, 5));
    game.makeMove(Point(8, 3));
    game.makeMove(Point(1, 6));
    game.makeMove(Point(9, 3));
    game.makeMove(Point(2, 6));
    game.makeMove(Point(1, 3));
    game.makeMove(Point(3, 5));
    game.makeMove(Point(0, 8));
    game.makeMove(Point(6, 6));
    game.makeMove(Point(7, 8));
    game.makeMove(Point(7, 7));

    auto result = dfsVCF(1, 1, game, Point(), Point());
    // cout << "first " << result.first << endl;
    for (Point& move : result.second)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();

    // cout << result.second.size() << endl;
    CHECK(!result.first);
}


TEST_CASE("testDfsVCF3")
{
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(1, 4));
    game.makeMove(Point(5, 5));
    game.makeMove(Point(1, 5));
    game.makeMove(Point(5, 6));
    game.makeMove(Point(1, 6));
    game.makeMove(Point(5, 7));
    game.makeMove(Point(2, 6));
    game.makeMove(Point(1, 3));
    game.makeMove(Point(3, 5));
    game.makeMove(Point(0, 8));
    game.makeMove(Point(6, 4));
    game.makeMove(Point(7, 8));
    game.makeMove(Point(7, 4));

    auto result = dfsVCF(1, 1, game, Point(), Point());
    for (Point& move : result.second)
    {
        game.board[move.x][move.y] = 3;
    }
    CHECK(result.first);
}


TEST_CASE("testDfsVCF4")
{
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 3));
    auto result = dfsVCF(1, 1, game, Point(), Point());
    for (Point& move : result.second)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();

    // cout << result.second.size() << endl;
    CHECK(result.first);
}


TEST_CASE("testGetNearByEmptyPoints")
{
    // cout << "testGetNearByEmptyPoints" << endl;
    Game game(boardSize);
    game.makeMove(Point(0, 4));
    auto result = getNearByEmptyPoints(Point(2, 6), game);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 30);
}


TEST_CASE("testGetNearByEmptyPoints")
{
    // cout << "testGetNearByEmptyPoints" << endl;
    Game game(boardSize);
    game.makeMove(Point(0, 4));
    for (int i = 0; i < 4; i++)
    {
        auto result = getLineEmptyPoints(Point(2, 6), game, i);
        for (Point& move : result)
        {
            game.board[move.x][move.y] = 3;
        }
        // game.printBoard();
        // cout << result.size() << endl;
        for (Point& move : result)
        {
            game.board[move.x][move.y] = 0;
        }
    }
}


TEST_CASE("testGetThreeDefenceMoves")
{
    // cout << "testGetThreeDefenceMoves" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(2, 0));

    auto moves = game.getEmptyPoints();
    auto result = getThreeDefenceMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size() == 5);
}


TEST_CASE("testGetThreeDefenceMoves2")
{
    // cout << "testGetThreeDefenceMoves2" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(2, 0));

    auto moves = game.getEmptyPoints();
    auto result = getThreeDefenceMoves(game.currentPlayer, game, moves);
    for (Point& move : result)
    {
        game.board[move.x][move.y] = 3;
    }
    // game.printBoard();
    // cout << result.size() << endl;
    CHECK(result.size()== 4);
}


TEST_CASE("testSelectActions")
{
    // cout << "testSelectActions" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(9, 13));
    game.makeMove(Point(9, 15));
    game.makeMove(Point(13, 15));
    game.makeMove(Point(14, 14));
    game.makeMove(Point(16, 11));
    game.makeMove(Point(11, 14));
    game.makeMove(Point(11, 15));
    game.makeMove(Point(10, 14));
    game.makeMove(Point(12, 14));
    game.makeMove(Point(11, 13));
    game.makeMove(Point(12, 12));
    game.makeMove(Point(12, 15));
    game.makeMove(Point(13, 13));
    game.makeMove(Point(14, 12));
    game.makeMove(Point(14, 11));
    game.makeMove(Point(10, 13));
    game.makeMove(Point(9, 12));
    game.makeMove(Point(10, 16));
    game.makeMove(Point(10, 12));
    game.makeMove(Point(13, 12));
    //    game.makeMove(Point(12, 10));


    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);
    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;

    bool asset = false;
    for (Point& move : get<1>(result))
    {
        if (move.x == 12 && move.y == 10)
        {
            asset = true;
        }
    }
    CHECK(asset);
}

TEST_CASE("testSelectActions2")
{
    // cout << "testSelectActions2" << endl;
    Game game(20);
    game.makeMove(Point(16, 16));
    game.makeMove(Point(14, 16));
    game.makeMove(Point(16, 14));
    game.makeMove(Point(14, 14));
    game.makeMove(Point(16, 12));
    game.makeMove(Point(14, 12));
    game.makeMove(Point(14, 15));
    game.makeMove(Point(16, 15));
    game.makeMove(Point(15, 14));
    game.makeMove(Point(16, 13));
    game.makeMove(Point(15, 11));
    game.makeMove(Point(17, 13));
    game.makeMove(Point(15, 13));
    game.makeMove(Point(15, 12));
    game.makeMove(Point(17, 14));
    game.makeMove(Point(13, 10));
    game.makeMove(Point(14, 11));
    game.makeMove(Point(16, 11));
    game.makeMove(Point(15, 17));
    game.makeMove(Point(18, 14));
    game.makeMove(Point(12, 17));
    game.makeMove(Point(13, 16));
    game.makeMove(Point(12, 15));
    game.makeMove(Point(11, 15));
    game.makeMove(Point(11, 16));
    game.makeMove(Point(13, 14));
    game.makeMove(Point(10, 15));
    game.makeMove(Point(13, 18));

    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);
    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;

    bool asset = false;
    for (Point& move : get<1>(result))
    {
        if (move.x == 10 && move.y == 14)
        {
            asset = true;
        }
    }
    CHECK(asset);
}


TEST_CASE("testSelectActions3")
{
    // cout << "testSelectActions3" << endl;
    Game game(20);
    game.makeMove(Point(2, 17));
    game.makeMove(Point(4, 17));
    game.makeMove(Point(5, 15));
    game.makeMove(Point(3, 13));
    game.makeMove(Point(3, 12));
    game.makeMove(Point(5, 16));
    game.makeMove(Point(6, 15));
    game.makeMove(Point(4, 15));
    game.makeMove(Point(4, 13));
    game.makeMove(Point(5, 14));
    game.makeMove(Point(3, 14));
    game.makeMove(Point(5, 12));
    game.makeMove(Point(4, 11));
    game.makeMove(Point(5, 10));
    game.makeMove(Point(5, 13));
    game.makeMove(Point(4, 10));
    game.makeMove(Point(6, 10));
    game.makeMove(Point(7, 12));
    game.makeMove(Point(6, 13));
    game.makeMove(Point(4, 16));
    game.makeMove(Point(4, 18));
    game.makeMove(Point(6, 16));
    game.makeMove(Point(7, 16));
    game.makeMove(Point(6, 11));
    game.makeMove(Point(8, 13));
    game.makeMove(Point(7, 13));
    game.makeMove(Point(5, 11));
    game.makeMove(Point(8, 12));
    game.makeMove(Point(9, 12));
    game.makeMove(Point(10, 11));
    game.makeMove(Point(9, 11));
    game.makeMove(Point(9, 10));
    game.makeMove(Point(8, 9));
    game.makeMove(Point(8, 14));
    game.makeMove(Point(7, 11));
    game.makeMove(Point(10, 9));
    game.makeMove(Point(8, 11));
    game.makeMove(Point(10, 10));
    game.makeMove(Point(10, 12));
    game.makeMove(Point(9, 15));
    game.makeMove(Point(10, 16));
    game.makeMove(Point(7, 14));
    game.makeMove(Point(6, 14));
    game.makeMove(Point(4, 12));
    game.makeMove(Point(6, 12));
    game.makeMove(Point(8, 10));
    game.makeMove(Point(11, 10));
    game.makeMove(Point(3, 15));
    game.makeMove(Point(11, 14));
    game.makeMove(Point(10, 7));
    game.makeMove(Point(10, 8));
    game.makeMove(Point(10, 13));
    game.makeMove(Point(11, 13));
    game.makeMove(Point(2, 16));
    game.makeMove(Point(3, 16));
    game.makeMove(Point(11, 15));
    game.makeMove(Point(12, 14));
    game.makeMove(Point(13, 15));
    game.makeMove(Point(10, 15));

    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);

    game.board[12][13] = 4;
    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;

    bool asset = false;
    for (Point& move : get<1>(result))
    {
        if (move.x == 12 && move.y == 13)
        {
            asset = true;
        }
    }
    CHECK(asset);
}


TEST_CASE("testSelectActions4")
{
    // cout << "testSelectActions4" << endl;
    Game game(20);
    game.makeMove(Point(6, 11));
    game.makeMove(Point(6, 12));
    game.makeMove(Point(6, 9));
    game.makeMove(Point(6, 10));
    game.makeMove(Point(8, 10));
    game.makeMove(Point(7, 10));
    game.makeMove(Point(10, 10));
    game.makeMove(Point(9, 10));
    game.makeMove(Point(10, 12));
    game.makeMove(Point(10, 11));
    game.makeMove(Point(9, 8));
    game.makeMove(Point(10, 9));
    game.makeMove(Point(8, 7));
    game.makeMove(Point(7, 8));
    game.makeMove(Point(13, 8));
    game.makeMove(Point(13, 7));
    game.makeMove(Point(13, 10));
    game.makeMove(Point(13, 9));
    game.makeMove(Point(13, 11));
    game.makeMove(Point(13, 12));
    game.makeMove(Point(8, 9));
    game.makeMove(Point(8, 11));
    game.makeMove(Point(7, 12));

    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);

    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;
}


TEST_CASE("testSelectActions5")
{
    // cout << "testSelectActions5" << endl;
    Game game(20);
    game.makeMove(Point(9, 13));
    game.makeMove(Point(9, 15));
    game.makeMove(Point(13, 15));
    game.makeMove(Point(14, 14));
    game.makeMove(Point(16, 11));
    game.makeMove(Point(14, 13));
    game.makeMove(Point(12, 13));
    game.makeMove(Point(15, 15));
    game.makeMove(Point(13, 13));
    game.makeMove(Point(14, 15));
    game.makeMove(Point(14, 16));
    game.makeMove(Point(12, 14));
    game.makeMove(Point(11, 14));
    //    game.makeMove(Point(13,12));

    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);

    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;

    bool asset = false;
    for (Point& move : get<1>(result))
    {
        if (move.x == 13 && move.y == 12)
        {
            asset = true;
        }
    }
    CHECK(asset);
}


TEST_CASE("testSelectActions6")
{
    // cout << "testSelectActions6" << endl;
    Game game(20);
    game.makeMove(Point(9, 13));
    game.makeMove(Point(9, 15));
    game.makeMove(Point(13, 15));
    game.makeMove(Point(14, 14));
    game.makeMove(Point(16, 11));
    game.makeMove(Point(12, 14));
    game.makeMove(Point(11, 14));
    game.makeMove(Point(14, 15));
    game.makeMove(Point(12, 13));
    game.makeMove(Point(13, 13));
    game.makeMove(Point(14, 12));
    game.makeMove(Point(15, 15));
    game.makeMove(Point(16, 16));
    game.makeMove(Point(14, 16));
    game.makeMove(Point(14, 17));
    game.makeMove(Point(16, 14));
    game.makeMove(Point(13, 17));
    game.makeMove(Point(15, 14));
    game.makeMove(Point(13, 14));
    game.makeMove(Point(17, 14));
    game.makeMove(Point(18, 14));
    game.makeMove(Point(13, 16));
    game.makeMove(Point(16, 13));
    game.makeMove(Point(17, 13));
    game.makeMove(Point(18, 12));
    game.makeMove(Point(17, 15));
    game.makeMove(Point(17, 16));
    game.makeMove(Point(11, 16));
    game.makeMove(Point(12, 16));
    game.makeMove(Point(10, 14));
    game.makeMove(Point(11, 13));
    game.makeMove(Point(12, 17));
    game.makeMove(Point(11, 18));
    game.makeMove(Point(10, 15));
    game.makeMove(Point(9, 14));
    game.makeMove(Point(11, 15));
    game.makeMove(Point(10, 13));
    game.makeMove(Point(8, 13));
    game.makeMove(Point(8, 15));
    game.makeMove(Point(11, 12));
    game.makeMove(Point(18, 15));
    game.makeMove(Point(10, 16));
    game.makeMove(Point(9, 17));
    game.makeMove(Point(18, 16));
    game.makeMove(Point(15, 13));
    game.makeMove(Point(9, 12));
    game.makeMove(Point(12, 12));
    game.makeMove(Point(7, 14));
    game.makeMove(Point(6, 15));
    //    game.makeMove(Point(7,16));

    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);

    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;

    bool asset = false;
    for (Point& move : get<1>(result))
    {
        if (move.x == 7 && move.y == 16)
        {
            asset = true;
        }
    }
    CHECK(asset);
}


TEST_CASE("testSelectActions7")
{
    // cout << "testSelectActions7" << endl;
    Game game(20);
    game.makeMove(Point(5, 18));
    game.makeMove(Point(5, 16));
    game.makeMove(Point(4, 16));
    game.makeMove(Point(6, 15));
    game.makeMove(Point(3, 18));
    game.makeMove(Point(4, 15));
    game.makeMove(Point(3, 15));
    game.makeMove(Point(6, 17));
    game.makeMove(Point(3, 14));
    //    game.makeMove(Point(3, 16));

    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);

    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;

    bool asset = false;
    for (Point& move : get<1>(result))
    {
        if (move.x == 3 && move.y == 16)
        {
            asset = true;
        }
    }
    CHECK(asset);
}


TEST_CASE("testSelectActions8")
{
    // cout << "testSelectActions8" << endl;
    Game game(20);
    game.makeMove(Point(12, 19));
    game.makeMove(Point(8, 18));
    game.makeMove(Point(7, 15));
    game.makeMove(Point(7, 18));
    game.makeMove(Point(8, 14));
    game.makeMove(Point(6, 18));
    game.makeMove(Point(5, 18));
    game.makeMove(Point(6, 16));
    game.makeMove(Point(6, 14));
    game.makeMove(Point(5, 15));
    game.makeMove(Point(7, 17));
    game.makeMove(Point(5, 13));
    game.makeMove(Point(7, 14));
    game.makeMove(Point(5, 14));
    game.makeMove(Point(5, 16));
    game.makeMove(Point(6, 15));
    game.makeMove(Point(8, 15));
    game.makeMove(Point(7, 16));
    game.makeMove(Point(8, 17));
    game.makeMove(Point(6, 17));
    game.makeMove(Point(6, 19));
    game.makeMove(Point(8, 13));
    game.makeMove(Point(9, 15));
    game.makeMove(Point(5, 12));
    game.makeMove(Point(5, 11));
    game.makeMove(Point(4, 13));
    game.makeMove(Point(3, 12));
    game.makeMove(Point(7, 13));
    game.makeMove(Point(6, 13));
    game.makeMove(Point(10, 17));
    game.makeMove(Point(10, 14));
    game.makeMove(Point(9, 14));
    game.makeMove(Point(10, 15));
    game.makeMove(Point(11, 15));
    game.makeMove(Point(10, 12));
    game.makeMove(Point(10, 16));
    game.makeMove(Point(9, 17));
    //    game.makeMove(Point(9,18));

    long long startTime = getSystemTime();
    auto moves = game.getEmptyPoints();
    auto result = selectActions(game);

    for (Point& move : get<1>(result))
    {
        game.board[move.x][move.y] = 3;
    }
    // cout << "cost " << getSystemTime() - startTime << " ms" << endl;
    // game.printBoard();
    // cout << get<2>(result) << endl;
    // cout << get<1>(result).size() << endl;

    bool asset = false;
    for (Point& move : get<1>(result))
    {
        if (move.x == 9 && move.y == 18)
        {
            asset = true;
        }
    }
    CHECK(asset);
}
