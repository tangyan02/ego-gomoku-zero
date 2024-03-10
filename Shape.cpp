
#include "Shape.h"

using namespace std;

static int ddx[4] = {1, 1, 1, 0};
static int ddy[4] = {1, 0, -1, 1};

static bool shapeMapping[9][300000] = {false};

bool win(const vector<int> &keys) {
    int count = 0;
    for (int i: keys) {
        if (i == 1) {
            count++;
            if (count == 5) {
                return true;
            }
        } else {
            count = 0;
        }
    }
    return false;
}

bool activeFour(vector<int> &keys) {
    if (win(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (win(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count >= 2) {
        return true;
    }
    return false;
}


bool sleepyFour(vector<int> &keys) {
    if (win(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (win(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count == 1) {
        return true;
    }
    return false;
}

bool activeThree(vector<int> &keys) {
    if (activeFour(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (activeFour(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count >= 1) {
        return true;
    }
    return false;
}


bool sleepyThree(vector<int> &keys) {
    if (sleepyFour(keys) || activeThree(keys) || activeFour(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (sleepyFour(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count >= 1) {
        return true;
    }
    return false;
}


bool activeTwo(vector<int> &keys) {
    if (activeThree(keys) || activeFour(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (activeThree(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count >= 1) {
        return true;
    }
    return false;
}


bool sleepyTwo(vector<int> &keys) {
    if (sleepyFour(keys) || sleepyThree(keys) || activeThree(keys) || activeFour(keys) || activeTwo(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (sleepyThree(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count >= 1) {
        return true;
    }
    return false;
}


bool activeOne(vector<int> &keys) {
    if (activeThree(keys) || activeFour(keys) || activeTwo(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (activeTwo(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count >= 1) {
        return true;
    }
    return false;
}


bool sleepyOne(vector<int> &keys) {
    if (sleepyFour(keys) || sleepyThree(keys) || sleepyTwo(keys) ||
        activeThree(keys) || activeFour(keys) || activeTwo(keys) || activeOne(keys)) {
        return false;
    }
    int count = 0;
    for (int i = 0; i < keys.size(); i++) {
        if (keys[i] == 0) {
            keys[i] = 1;
            if (sleepyTwo(keys)) {
                count++;
            }
            keys[i] = 0;
        }
    }
    if (count >= 1) {
        return true;
    }
    return false;
}

void generateCombinations(std::vector<std::vector<int>> &combinations, std::vector<int> &combination, int index) {
    if (index == combination.size()) {
        combinations.push_back(combination);
        return;
    }

    for (int i = 0; i <= 3; i++) {
        if (index == 4 && i != 1) {
            continue;
        }
//        if (index >= 1 && i == 3) {
//            if (combination[index - 1] != 3) {
//                continue;
//            }
//        }

        combination[index] = i;
        generateCombinations(combinations, combination, index + 1);
    }
}

std::vector<std::vector<int>> generateAllCombinations() {
    std::vector<std::vector<int>> combinations;
    std::vector<int> combination(9, 0); // 初始化为全0的数组

    generateCombinations(combinations, combination, 0);

    return combinations;
}


void printKeys(const vector<int> &keys) {
    vector<char> values = {'.', 'x', 'o', '#'};
    for (const auto &item: keys) {
        cout << values[item];
    }
    cout << endl;
}

vector<int> getKeysInGame(Game &game, int player, Point &action, int direct) {
    vector<int> result;
    int x = action.x;
    int y = action.y;
    for (int k = -4; k <= 4; k++) {
        if (k == 0) {
            result.emplace_back(1);
            continue;
        }
        int tx = x + k * ddx[direct];
        int ty = y + k * ddy[direct];
        if (!(tx >= 0 && tx < game.boardSize && ty >= 0 && ty < game.boardSize)) {
            result.emplace_back(3);
            continue;
        }
        if (game.board[tx][ty] == 0) {
            result.emplace_back(0);
        }
        if (game.board[tx][ty] == player) {
            result.emplace_back(1);
        }
        if (game.board[tx][ty] == 3 - player) {
            result.emplace_back(2);
        }
    }
    return result;
}

int hashKeys(const std::vector<int> &vec) {
    int hashValue = 0;
    int multiplier = 1;

    for (int i = vec.size() - 1; i >= 0; i--) {
        hashValue += vec[i] * multiplier;
        multiplier *= 4;
    }

    return hashValue;
}

bool checkPointDirectShape(Game &game, int player, Point &action, int direct, Shape shape) {
//    game.board[action.x][action.y] = 3;
//    game.printBoard();
//    game.board[action.x][action.y] = 0;

    auto keys = getKeysInGame(game, player, action, direct);
//    printKeys(keys);
    auto hash = hashKeys(keys);
//    cout << "direct " << direct << " result is " << shapeMapping[shape][hash] << endl;
    return shapeMapping[shape][hash];
}

void initShape() {
    std::vector<std::vector<int>> combinations = generateAllCombinations();
    for (const auto &item: combinations) {
        vector<int> keyList = item;
        int key = hashKeys(item);
        if (win(keyList)) {
            shapeMapping[LONG_FIVE][key] = true;
        }
        if (activeFour(keyList)) {
            shapeMapping[ACTIVE_FOUR][key] = true;
        }
        if (sleepyFour(keyList)) {
            shapeMapping[SLEEPY_FOUR][key] = true;
        }
        if (activeThree(keyList)) {
            shapeMapping[ACTIVE_THREE][key] = true;
        }
        if (sleepyThree(keyList)) {
            shapeMapping[SLEEPY_THREE][key] = true;
        }
        if (activeTwo(keyList)) {
            shapeMapping[ACTIVE_TWO][key] = true;
        }
        if (sleepyTwo(keyList)) {
            shapeMapping[SLEEPY_TWO][key] = true;
        }
        if (activeOne(keyList)) {
            shapeMapping[ACTIVE_ONE][key] = true;
        }
        if (sleepyOne(keyList)) {
            shapeMapping[SLEEPY_ONE][key] = true;
        }
    }
}

void printShape() {
    cout << "长5" << endl;
    std::vector<std::vector<int>> combinations = generateAllCombinations();
    for (const auto &item: combinations) {
        if (win(item)) {
            printKeys(item);
        }
    }
    cout << "活4" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (activeFour(keys)) {
            printKeys(item);
        }
    }
    cout << "眠4" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (sleepyFour(keys)) {
            printKeys(item);
        }
    }

    cout << "活3" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (activeThree(keys)) {
            printKeys(item);
        }
    }

    cout << "眠3" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (sleepyThree(keys)) {
            printKeys(item);
        }
    }

    cout << "活2" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (activeTwo(keys)) {
            printKeys(item);
        }
    }

    cout << "眠2" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (sleepyTwo(keys)) {
            printKeys(item);
        }
    }
    cout << "活1" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (activeOne(keys)) {
            printKeys(item);
        }
    }

    cout << "眠1" << endl;
    for (const auto &item: combinations) {
        vector<int> keys = item;
        if (sleepyOne(keys)) {
            printKeys(item);
        }
    }
}