#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#include "Console.h"
#include <iostream>
#include <vector>
#include <termios.h>
#include <cstdlib>
#include "SelfPlay.h"


static const int BOARD_SIZE = 20;
static std::vector <std::vector<char>> board(BOARD_SIZE, std::vector<char>(BOARD_SIZE, '.'));
static int cursorX = BOARD_SIZE / 2, cursorY = BOARD_SIZE / 2;

static Game game(20);
static auto network = getNetwork(torch::kCPU, "model/model_latest.pt");

// Function to set terminal to raw mode
void setRawMode() {
    termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Clear ICANON and ECHO.
    tcsetattr(0, TCSANOW, &term);
}

void printBoard() {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (i == cursorX && j == cursorY) {
                std::cout << "□" << " ";
            } else {
                std::cout << board[i][j] << " ";
            }
        }
        std::cout << '\n';
    }
}

Point aiMove() {

    MonteCarloTree mcts = MonteCarloTree(&network, torch::kCPU, 1);
    Node node;
    mcts.search(game, &node, 200);

    int max = 0;
    int action = -1;
    for (auto item: node.children) {
        int visit = item.second->visits;
        if (visit > max) {
            action = item.first;
            max = visit;
        }
    }

    return game.getPointFromIndex(action);
}

void aiAction() {
    Point p = aiMove();
    board[p.x][p.y] = 'O';
    system("clear");
    game.makeMove(p);
    printBoard();
    cout << "ai move " << p.x << "," << p.y << "" << endl;
}

int startConsole() {

    aiAction();

    setRawMode();
    char input;
    printBoard(); // Print the board initially
    while ((input = getchar()) != 'q') {
        switch (input) {
            case 'w':
                if (cursorX > 0) --cursorX;
                system("clear");
                printBoard();
                break;
            case 'a':
                if (cursorY > 0) --cursorY;
                system("clear");
                printBoard();
                break;
            case 's':
                if (cursorX < BOARD_SIZE - 1) ++cursorX;
                system("clear");
                printBoard();
                break;
            case 'd':
                if (cursorY < BOARD_SIZE - 1) ++cursorY;
                system("clear");
                printBoard();
                break;
            case 'e': {
                board[cursorX][cursorY] = 'X';
                system("clear");
                game.makeMove(Point(cursorX, cursorY));
                printBoard();

                aiAction();
                break;
            }
            default:
                std::cout << "Invalid input!\n";
        }
    }
    return 0;
}

#endif // __unix__
