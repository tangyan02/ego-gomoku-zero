
#ifndef EGO_ZERO_BRIDGE_H
#define EGO_ZERO_BRIDGE_H

#include "ConfigReader.h"
#include "Game.h"
#include "MCTS.h"
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <memory>


class Bridge {
public:
    void startGame();
    void addGameToHistroy(Game game);
    Bridge();
    ~Bridge();

private:
    Game *game;
    std::unique_ptr<Model> model;
    std::unique_ptr<MonteCarloTree> mcts;
    Node *node;

    vector<Game> history;

    Point parse_coordinates(const string &args);

    void move(string& args);

    void predict(string& args);

    void end_check(string& args);

    void winner_check(string& args);

    void current_player(string& args);

    void board(string& args);

    void get_moves(string& args);

    void rollback(string& args);
};


#endif //EGO_ZERO_BRIDGE_H
