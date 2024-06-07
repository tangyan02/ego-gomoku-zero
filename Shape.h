
#ifndef EGO_GOMOKU_ZERO_SHAPE_H
#define EGO_GOMOKU_ZERO_SHAPE_H

#define LONG_FIVE 0
#define ACTIVE_FOUR 1
#define SLEEPY_FOUR 2
#define ACTIVE_THREE 3
#define SLEEPY_THREE 4
#define ACTIVE_TWO 5
#define SLEEPY_TWO 6
#define ACTIVE_ONE 7
#define SLEEPY_ONE 8

#define SLEEPY_FOUR_MORE 9

using Shape = int;

#include <vector>
#include <iostream>
#include <array>
#include "Game.h"

using namespace std;

std::array<int, 9> getKeysInGame(Game &game, int player, Point &action, int direct);

bool checkPointDirectShape(Game &game, int player, Point &action, int direct, Shape shape);

int countPointShape(Game &game, int player, Point &action, Shape shape);

void printKeys(const vector<int> &keys);

void printKeys(const std::array<int, 9> &keys);

void printShape();

void initShape();

#endif //EGO_GOMOKU_ZERO_SHAPE_H
