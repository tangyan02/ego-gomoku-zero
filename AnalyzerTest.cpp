
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
    if (result.size() == 14) {
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

bool testDfsVCT() {
    cout << "testDfsVCT" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(10, 4));
    game.makeMove(Point(2, 0));
    game.makeMove(Point(1, 5));
    game.makeMove(Point(2, 1));
    game.makeMove(Point(1, 6));
    game.makeMove(Point(2, 2));
    game.makeMove(Point(2, 6));
    game.makeMove(Point(10, 3));
    game.makeMove(Point(3, 5));
    game.makeMove(Point(0, 8));
    game.makeMove(Point(6, 6));
    game.makeMove(Point(7, 8));
    game.makeMove(Point(7, 7));

    auto startTime = getSystemTime();
    int nodeRecord = 0;
    auto result = dfsVCT(1, 1, game, Point(), Point(), Point(), false, nodeRecord);
    auto costTime = getSystemTime() - startTime;
    cout << "cost time " << costTime << endl;
    cout << "node record " << nodeRecord << endl;
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


bool testDfsVCT2() {
    cout << "testDfsVCT2" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(16, 14));
    game.makeMove(Point(15, 15));
    game.makeMove(Point(15, 17));
    game.makeMove(Point(16, 5));
    game.makeMove(Point(16, 16));
    game.makeMove(Point(16, 17));

    int nodeRecord = 0;
    auto result = dfsVCT(1, 1, game, Point(), Point(), Point(), false, nodeRecord);
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


bool testDfsVCT3() {
    cout << "testDfsVCT3" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(3, 17));
    game.makeMove(Point(2, 18));
    game.makeMove(Point(3, 15));
    game.makeMove(Point(3, 16));
    game.makeMove(Point(2, 16));
    game.makeMove(Point(1, 17));
    game.makeMove(Point(1, 18));
    game.makeMove(Point(2, 3));
    game.makeMove(Point(6, 15));
    game.makeMove(Point(4, 15));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(5, 14));
    game.makeMove(Point(6, 13));
    game.makeMove(Point(6, 16));
    game.makeMove(Point(5, 16));
    game.makeMove(Point(4, 17));
    game.makeMove(Point(7, 14));
    game.makeMove(Point(8, 13));
    game.makeMove(Point(8, 15));
    game.makeMove(Point(5, 12));
    game.makeMove(Point(7, 15));
    game.makeMove(Point(9, 15));
    game.makeMove(Point(7, 16));
    game.makeMove(Point(7, 17));
    game.makeMove(Point(5, 13));
    game.makeMove(Point(3, 13));
    game.makeMove(Point(5, 15));
    game.makeMove(Point(6, 14));
    game.makeMove(Point(9, 16));
    game.makeMove(Point(10, 17));
    game.makeMove(Point(5, 18));
    game.makeMove(Point(6, 17));
    game.makeMove(Point(5, 17));
    game.makeMove(Point(5, 19));
    game.makeMove(Point(6, 18));
    game.makeMove(Point(4, 16));
    game.makeMove(Point(4, 18));
    game.makeMove(Point(7, 18));
    game.makeMove(Point(8, 17));
    game.makeMove(Point(9, 18));
    game.makeMove(Point(8, 16));
    game.makeMove(Point(4, 14));
    game.makeMove(Point(4, 13));
    game.makeMove(Point(8, 14));
    game.makeMove(Point(1, 15));
    game.makeMove(Point(0, 14));
    game.makeMove(Point(2, 15));
    game.makeMove(Point(11, 16));
    game.makeMove(Point(8, 19));
    game.makeMove(Point(8, 18));
    game.makeMove(Point(7, 13));
    game.makeMove(Point(7, 12));
    game.makeMove(Point(10, 15));
    game.makeMove(Point(11, 18));
    game.makeMove(Point(10, 18));
    game.makeMove(Point(11, 17));
    game.makeMove(Point(11, 14));
    game.makeMove(Point(12, 13));
    game.makeMove(Point(10, 16));
    game.makeMove(Point(11, 19));
    game.makeMove(Point(11, 15));
    game.makeMove(Point(13, 17));
    game.makeMove(Point(12, 17));
    game.makeMove(Point(8, 12));
    game.makeMove(Point(6, 12));
    game.makeMove(Point(8, 11));
    game.makeMove(Point(8, 10));

    int nodeRecord = 0;
    auto result = dfsVCT(2, 2, game, Point(), Point(), Point(), false, nodeRecord);
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


bool testDfsVCT4() {
    cout << "testDfsVCT4" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(3, 16));
    game.makeMove(Point(4, 15));
    game.makeMove(Point(4, 17));
    game.makeMove(Point(2, 15));
    game.makeMove(Point(3, 15));
    game.makeMove(Point(3, 18));
    game.makeMove(Point(2, 16));
    game.makeMove(Point(3, 14));
    game.makeMove(Point(1, 17));
    game.makeMove(Point(4, 14));
    game.makeMove(Point(5, 16));
    game.makeMove(Point(1, 16));
    game.makeMove(Point(4, 16));
    game.makeMove(Point(6, 16));
    game.makeMove(Point(0, 17));
    game.makeMove(Point(4, 13));
    game.makeMove(Point(5, 12));
    game.makeMove(Point(12, 19));
    game.makeMove(Point(3, 17));
    game.makeMove(Point(2, 17));
    game.makeMove(Point(5, 15));
    game.makeMove(Point(6, 14));

    int nodeRecord = 0;
    auto result = dfsVCT(1, 1, game, Point(), Point(), Point(), false, nodeRecord);
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


bool testDfsVCT5() {
    cout << "testDfsVCT5" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(2, 3));
    game.makeMove(Point(3, 2));
    game.makeMove(Point(4, 3));
    game.makeMove(Point(3, 3));
    game.makeMove(Point(3, 4));
    game.makeMove(Point(16, 16));
    game.makeMove(Point(1, 2));
    game.makeMove(Point(4, 5));
    game.makeMove(Point(2, 5));
    game.makeMove(Point(5, 2));
    game.makeMove(Point(2, 4));
    game.makeMove(Point(2, 6));
    game.makeMove(Point(1, 6));
    game.makeMove(Point(0, 7));
    game.makeMove(Point(1, 4));
    game.makeMove(Point(4, 4));
    game.makeMove(Point(2, 2));
    game.makeMove(Point(2, 1));
    game.makeMove(Point(1, 3));
    game.makeMove(Point(1, 5));
    game.makeMove(Point(4, 8));
    game.makeMove(Point(3, 6));
    game.makeMove(Point(5, 4));
    game.makeMove(Point(4, 6));
    game.makeMove(Point(5, 5));
    game.makeMove(Point(5, 6));
    game.makeMove(Point(6, 6));
    game.makeMove(Point(15, 16));
    game.makeMove(Point(17, 16));
    game.makeMove(Point(16, 17));
    game.makeMove(Point(16, 15));
    game.makeMove(Point(14, 15));
    game.makeMove(Point(17, 18));
    game.makeMove(Point(15, 14));
    game.makeMove(Point(15, 13));
    int nodeRecord = 0;
    auto result = dfsVCT(2, 2, game, Point(), Point(), Point(), false, nodeRecord);
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


bool testDfsVCT6() {
    cout << "testDfsVCT6" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(3, 3));
    game.makeMove(Point(4, 2));
    game.makeMove(Point(2, 3));
    game.makeMove(Point(4, 3));
    game.makeMove(Point(5, 2));
    game.makeMove(Point(4, 1));
    game.makeMove(Point(4, 4));
    game.makeMove(Point(5, 5));
    game.makeMove(Point(2, 4));
    game.makeMove(Point(3, 4));
    game.makeMove(Point(2, 5));
    game.makeMove(Point(2, 2));
    game.makeMove(Point(2, 6));
    game.makeMove(Point(2, 7));
    game.makeMove(Point(3, 5));
    game.makeMove(Point(5, 3));
    game.makeMove(Point(4, 6));
    game.makeMove(Point(5, 7));
    game.makeMove(Point(7, 5));
    game.makeMove(Point(1, 3));
    game.makeMove(Point(3, 6));
    game.makeMove(Point(5, 6));
    game.makeMove(Point(5, 4));
    game.makeMove(Point(1, 6));
    game.makeMove(Point(4, 7));
    game.makeMove(Point(5, 8));
    game.makeMove(Point(5, 9));
    game.makeMove(Point(4, 8));
    game.makeMove(Point(3, 8));
    game.makeMove(Point(1, 7));
    game.makeMove(Point(1, 5));
    game.makeMove(Point(0, 5));
    game.makeMove(Point(4, 5));
    game.makeMove(Point(3, 1));
    game.makeMove(Point(6, 4));
    game.makeMove(Point(7, 4));
    game.makeMove(Point(0, 4));
    game.makeMove(Point(3, 9));
    game.makeMove(Point(6, 6));
    game.makeMove(Point(6, 3));
    game.makeMove(Point(1, 4));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(2, 1));
    game.makeMove(Point(2, 8));
    game.makeMove(Point(4, 10));
    game.makeMove(Point(2, 10));
    game.makeMove(Point(1, 11));
    game.makeMove(Point(2, 9));
    game.makeMove(Point(2, 11));
    game.makeMove(Point(3, 11));
    game.makeMove(Point(1, 9));
    game.makeMove(Point(8, 3));
    game.makeMove(Point(7, 3));
    game.makeMove(Point(8, 4));
    game.makeMove(Point(8, 2));
    game.makeMove(Point(9, 4));
    game.makeMove(Point(10, 4));
    game.makeMove(Point(9, 2));
    game.makeMove(Point(6, 5));
    game.makeMove(Point(10, 1));
    game.makeMove(Point(11, 0));

    int nodeRecord = 0;
    auto result = dfsVCT(game.currentPlayer, game.currentPlayer, game, Point(), Point(), Point(), false, nodeRecord);
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


bool testDfsVCTIter() {
    cout << "testDfsVCTIter" << endl;
    Game game(boardSize);
    game.currentPlayer = 1;
    game.makeMove(Point(10, 4));
    game.makeMove(Point(2, 0));
    game.makeMove(Point(1, 5));
    game.makeMove(Point(2, 1));
    game.makeMove(Point(1, 6));
    game.makeMove(Point(2, 2));
    game.makeMove(Point(2, 6));
    game.makeMove(Point(10, 3));
    game.makeMove(Point(3, 5));
    game.makeMove(Point(0, 8));
    game.makeMove(Point(6, 6));
    game.makeMove(Point(7, 8));
    game.makeMove(Point(7, 7));

    auto result = dfsVCTIter(1, game, 1000);
    for (Point &move: get<1>(result)) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << "break level " << get<2>(result) << endl;
    cout << "size " << get<1>(result).size() << endl;
    cout << get<0>(result) << endl;
    if (get<0>(result)) {
        return true;
    } else {
        return false;
    }
}


bool testDfsVCTDefenceIter() {
    cout << "testDfsVCTDefenceIter" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(9, 17));
    game.makeMove(Point(5, 9));
    game.makeMove(Point(2, 12));
    game.makeMove(Point(5, 14));
    game.makeMove(Point(6, 15));
    game.makeMove(Point(6, 16));
    game.makeMove(Point(7, 16));
    game.makeMove(Point(6, 17));
    game.makeMove(Point(7, 18));
    game.makeMove(Point(7, 14));
    game.makeMove(Point(8, 14));
    game.makeMove(Point(7, 15));
    game.makeMove(Point(8, 15));
    game.makeMove(Point(8, 16));
    game.makeMove(Point(9, 14));
    int limit = 3000;
    auto result = dfsVCTDefenceIter(game.currentPlayer, game, limit);
    for (Point &move: get<1>(result)) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << get<1>(result).size() << endl;
    if (get<1>(result).size() == 2) {
        return true;
    } else {
        return false;
    }
}


bool testDfsVCTDefenceIter2() {
    cout << "testDfsVCTDefenceIter2" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(15, 9));
    game.makeMove(Point(12, 10));
    game.makeMove(Point(14, 8));
    game.makeMove(Point(13, 14));
    game.makeMove(Point(12, 9));
    game.makeMove(Point(14, 9));
    game.makeMove(Point(12, 6));
    int limit = 3000;
    auto result = dfsVCTDefenceIter(game.currentPlayer, game, limit);
    for (Point &move: get<1>(result)) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << get<1>(result).size() << endl;
    if (get<1>(result).size() == 3) {
        return true;
    } else {
        return false;
    }
}


bool testDfsVCTDefenceIter3() {
    cout << "testDfsVCTDefenceIter3" << endl;
    Game game(20);
    game.currentPlayer = 1;
    game.makeMove(Point(4, 8));
    game.makeMove(Point(2, 16));
    game.makeMove(Point(4, 10));
    game.makeMove(Point(6, 10));
    game.makeMove(Point(3, 10));

    int timeLimit = 30000;
    auto result = dfsVCTDefenceIter(game.currentPlayer, game, timeLimit);
    for (Point &move: get<1>(result)) {
        game.board[move.x][move.y] = 3;
    }
    game.printBoard();
    cout << get<1>(result).size() << endl;
    if (get<1>(result).size() == 2) {
        return true;
    } else {
        return false;
    }
}
