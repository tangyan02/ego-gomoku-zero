
#include "AnalyzerTest.h"

using namespace std;

static const int boardSize = 15;

bool testGetWinnerMove() {
    cout << "testWinnerMove" << endl;
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

bool testGetActiveFourMoves4() {
    cout << "testGetActiveFourMoves4" << endl;
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

bool testGetActiveThreeMoves() {
    cout << "testGetActiveThreeMoves" << endl;
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
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 10) {
        return true;
    } else {
        return false;
    }
}


bool testGetActiveThreeMoves2() {
    cout << "testGetActiveThreeMoves2" << endl;
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
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 8) {
        return true;
    } else {
        return false;
    }
}


bool testGetThreeDefenceMoves() {
    cout << "testGetThreeDefenceMoves" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));

    auto result = getThreeDefenceMoves(2, game);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 5) {
        return true;
    } else {
        return false;
    }
}

bool testGetThreeDefenceMoves2() {
    cout << "testGetThreeDefenceMoves2" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(1, 2));

    auto result = getThreeDefenceMoves(2, game);
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


bool testGetThreeDefenceMoves3() {
    cout << "testGetThreeDefenceMoves3" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(13,9));
    game.makeMove(Point(15,12));
    game.makeMove(Point(14,12));
    game.makeMove(Point(14,13));
    game.makeMove(Point(14,10));
    game.makeMove(Point(13,14));
    game.makeMove(Point(16,11));
    game.makeMove(Point(12,15));
    game.makeMove(Point(11,16));
    game.makeMove(Point(16,14));
    game.makeMove(Point(12,8));

    auto result = getThreeDefenceMoves(game.currentPlayer, game);
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

bool testGetActiveThreeMoves3() {
    cout << "testGetActiveThreeMoves3" << endl;
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

bool testGetSleepyFourMoves() {
    cout << "testGetSleepyFourMoves" << endl;
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

bool testDfsVCF() {
    cout << "testDfsVCF" << endl;
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
    for (Point &move: result.second) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();

    cout << result.second.size() << endl;
    if (result.first) {
        return true;
    } else {
        return false;
    }

}


bool testDfsVCF2() {
    cout << "testDfsVCF2" << endl;
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
    for (Point &move: result.second) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();

    cout << result.second.size() << endl;
    if (!result.first) {
        return true;
    } else {
        return false;
    }

}


bool testDfsVCF3() {
    cout << "testDfsVCF3" << endl;
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
    for (Point &move: result.second) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();

    cout << result.second.size() << endl;
    if (result.first) {
        return true;
    } else {
        return false;
    }

}

bool testGetVCFDefenceMoves() {
    cout << "testGetVCFDefenceMoves" << endl;
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

    auto result = getVCFDefenceMoves(2, game);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 15) {
        return true;
    } else {
        return false;
    }
}


bool testGetVCFDefenceMoves2() {
    cout << "testGetVCFDefenceMoves2" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(1, 2));


    auto result = getVCFDefenceMoves(2, game);
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


bool testGetVCFDefenceMoves3() {
    cout << "testGetVCFDefenceMoves3" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(0, 4));
    game.makeMove(Point(1, 0));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 2));


    auto result = getVCFDefenceMoves(2, game);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 5) {
        return true;
    } else {
        return false;
    }
}


bool testGetVCFDefenceMoves4() {
    cout << "testGetVCFDefenceMoves4" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(4, 4));
    game.makeMove(Point(3, 5));
    game.makeMove(Point(4, 5));
    game.makeMove(Point(5, 5));
    game.makeMove(Point(4, 7));
    game.makeMove(Point(1, 10));
    game.makeMove(Point(3, 4));


    auto result = getVCFDefenceMoves(game.currentPlayer, game);
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


bool testGetVCFDefenceMoves5() {
    cout << "testGetVCFDefenceMoves5" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(7, 12));
    game.makeMove(Point(6, 13));
    game.makeMove(Point(6, 11));
    game.makeMove(Point(4, 18));
    game.makeMove(Point(1, 18));
    game.makeMove(Point(1, 15));
    game.makeMove(Point(8, 2));
    game.makeMove(Point(5, 10));
    game.makeMove(Point(2, 14));
    game.makeMove(Point(3, 9));
    game.makeMove(Point(5, 12));
    game.makeMove(Point(9, 3));
    game.makeMove(Point(7, 10));
    game.makeMove(Point(4, 13));
    game.makeMove(Point(2, 8));
    game.makeMove(Point(5, 9));
    game.makeMove(Point(7, 9));
    game.makeMove(Point(7, 13));
    game.makeMove(Point(5, 13));
    game.makeMove(Point(3, 10));
    game.makeMove(Point(9, 12));
    game.makeMove(Point(6, 12));
    game.makeMove(Point(5, 11));

    auto result = getVCFDefenceMoves(game.currentPlayer, game);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 17) {
        return true;
    } else {
        return false;
    }
}

bool testGetNearByEmptyPoints() {
    cout << "testGetNearByEmptyPoints" << endl;
    Game game(boardSize);
    game.makeMove(Point(0, 4));
    auto result = getNearByEmptyPoints(Point(2, 6), game);
    for (Point &move: result) {
        game.board[move.x][move.y] = 3;
        cout << move.x << "," << move.y << endl;
    }
    game.printBoard();
    cout << result.size() << endl;
    if (result.size() == 30) {
        return true;
    } else {
        return false;
    }
}