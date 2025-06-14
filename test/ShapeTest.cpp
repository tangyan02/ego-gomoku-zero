
TEST_CASE("testGetKeysInGame")
{
    // cout << "testGetKeysInGame" << endl;
    Game game(15);

    game.makeMove(Point(0, 1));
    game.makeMove(Point(1, 4));
    game.makeMove(Point(0, 2));
    game.makeMove(Point(1, 1));
    game.makeMove(Point(0, 3));
    game.makeMove(Point(0, 6));
    game.makeMove(Point(0, 4));


    auto action = Point(0, 5);
    game.board[action.x][action.y] = 3;
    // game.printBoard();
    game.board[action.x][action.y] = 0;

    //    for (int k = 0; k < 4; k++) {
    //        auto result = getKeysInGame(game, game.currentPlayer, action, k);
    //        printKeys(result);
    //
    //    }
}
