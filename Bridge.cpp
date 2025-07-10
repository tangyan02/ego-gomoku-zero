
#include "Bridge.h"


// 解析指令参数
Point Bridge::parse_coordinates(const string &args) {
    size_t comma = args.find(',');
    if (comma == string::npos) throw invalid_argument("Invalid format");
    return {stoi(args.substr(0, comma)), stoi(args.substr(comma + 1))};
}

Bridge::Bridge() {

    // Read configuration

    // Parse parameters with defaults if not found
    int boardSize = stoi(ConfigReader::get("boardSize"));
    float explorationFactor = stof(ConfigReader::get("explorationFactor"));
    string modelPath = ConfigReader::get("modelPath");
    string coreType = ConfigReader::get("coreType");

    model = new Model();
    model->init(modelPath, coreType);

    game = new Game(boardSize);
    mcts = new MonteCarloTree(model, explorationFactor);

    node = new Node();

    addGameToHistroy(*game);
}

void Bridge::startGame() {

    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            // 分割指令和参数
            istringstream iss(line);
            string command;
            iss >> command;

            string args;
            getline(iss, args);
            args.erase(0, args.find_first_not_of(' ')); // 去除前导空格

            // 处理不同指令
            if (command == "MOVE") {
                move(args);
            } else if (command == "PREDICT") {
                predict(args);
            } else if (command == "END_CHECK") {
                end_check(args);
            } else if (command == "WINNER_CHECK") {
                winner_check(args);
            } else if (command == "CURRENT_PLAYER") {
                current_player(args);
            } else if (command == "BOARD") {
                board(args);
            } else if (command == "GET_MOVES") {
                get_moves(args);
            } else if (command == "ROLLBACK") {
                rollback(args);
            } else {
                throw invalid_argument("Unknown command");
            }
        } catch (...) {
            std::cerr << "发生异常" << std::endl;
        }
    }

}

void  Bridge::addGameToHistroy(Game game) {
    history.emplace_back(game);
}

void Bridge::move(string &args) {
    auto move = parse_coordinates(args);
    game->makeMove(Point(move.x, move.y));
    addGameToHistroy(*game);
    //更新node
    for (const auto &item: node->children) {
        if (item.first != move) {
            item.second->release();
        }
    }
    for (const auto item: node->children) {
        if (item.first == move) {
            node = item.second;
        }
    }
    cout << "MOVE SUCCESS" << endl;
}


void Bridge::predict(string &args) {
    int simiNum = stoi(args);
    mcts->search(*game, node, simiNum);

    std::vector<Point> moves;
    std::vector<float> moves_probs;
    std::tie(moves, moves_probs) = mcts->get_action_probabilities();

    // 构造矩阵
    vector<tuple<int, int, float>> probs;

    for (int k = 0; k < moves.size(); k++) {
        probs.emplace_back(moves[k].x, moves[k].y, moves_probs[k]);
    }

    for (const auto &[x, y, prob]: probs) {
        cout << x << "," << y << "," << prob << " ";
    }
    cout << endl;
}

void Bridge::end_check(string &args) {
    if (game->isGameOver()) {
        cout << "true" << endl;
    } else {
        cout << "false" << endl;
    }
}

void Bridge::winner_check(string &args) {
    bool win = game->checkWin(game->lastAction.x, game->lastAction.y, game->getOtherPlayer());
    if (win) {
        cout << game->getOtherPlayer() << endl;
    } else {
        cout << game->currentPlayer << endl;
    }
}

void Bridge::current_player(string &args) {
    cout << game->currentPlayer << endl;
}

void Bridge::board(string &args) {
    for (int i = 0; i < game->boardSize; i++) {
        for (int j = 0; j < game->boardSize; j++) {
            cout << game->board[i][j] << " ";
        }
    }
    cout << endl;
}

void Bridge::get_moves(string &args) {
    auto [win, points, info] = selectActions(*game);
    for (const auto &point: points) {
        cout << point.x << "," << point.y << " ";
    }
    cout << endl;
}

void Bridge::rollback(string& args)
{
    if (history.size()>1)
    {
        history.pop_back();
        game = &(this->history[history.size()-2]);
        delete node;
        node = new Node();
    }
    cout<<"rollback finish"<<endl;
}
