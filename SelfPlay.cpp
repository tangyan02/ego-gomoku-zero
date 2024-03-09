#include "SelfPlay.h"

using namespace std;

// 创建一个随机数生成器
std::random_device rd;
std::mt19937 gen(rd());

void printGame(Game &game, int action, std::vector<float> &action_probs,
               float temperature, const std::string &part, const string selectInfo, MonteCarloTree *mcts) {
    game.printBoard(part);
    std::string line;
    for (int i = 0; i < game.boardSize * game.boardSize; i++) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << action_probs[i];
        line += ss.str() + " ";
        if ((i + 1) % game.boardSize == 0) {
            cout << line << endl;
            line = "";
        }
    }

    float value = 1;
    if (mcts != nullptr) {
        auto state = game.getState();
        auto eval = mcts->evaluate_state(state);
        value = -eval.first;
    }

    std::string pic = (game.getOtherPlayer() == 1) ? "x" : "o";
    cout << part << " " << pic << " action is " << game.getPointFromIndex(action).x << ","
         << game.getPointFromIndex(action).y
         << " on rate " << round(action_probs[action] * 1000) / 1000
         << " temperature " << round(temperature * 100) / 100
         << " value " << value
         << selectInfo << endl;
}

torch::jit::Module getNetwork(torch::Device device, std::string path = "model/model_latest.pt") {
    auto model = torch::jit::load(path, device);
    model.eval();
    torch::NoGradGuard no_grad;
    return model;
}

torch::Device getDevice() {
    if (torch::cuda::is_available()) {
        return torch::kCUDA;
    } else {
        return torch::kCPU;
    }
//    return torch::kCPU;
}

void addAction(Game &game,
               int action,
               std::vector<std::tuple<torch::Tensor, int, std::vector<float>>> &game_data,
               std::vector<float> &action_probs
) {
    auto state = game.getState();
    std::tuple<torch::Tensor, int, std::vector<float>> record(state, game.currentPlayer, action_probs);
    game.makeMove(game.getPointFromIndex(action));
    game_data.push_back(record);
}

Game randomGame(Game &game, MonteCarloTree &mcts) {
    //开局随机去下完后，价值接近0的点
    auto moves = game.getEmptyPoints();
//    vector<pair<float, Point>> moveValues;
//    for (const auto &item: moves) {
//        Game gameTemp = game;
//        gameTemp.makeMove(item);
//        auto state = gameTemp.getState();
//        auto eval = mcts.evaluate_state(state);
//        moveValues.emplace_back(eval.first, item);
//    }
//
//    // 定义一个比较函数，用于按照 pair 的第一个元素的绝对值从小到大排序
//    auto compare = [](const pair<float, Point> &a, const pair<float, Point> &b) {
//        return abs(a.first) < abs(b.first);
//    };
//
//    sort(moveValues.begin(), moveValues.end(), compare);
//    // 计算前 10% 的元素个数
//    int numElements = moveValues.size() * 0.1;
//
//    std::vector<Point> result;
//    for (int i = 0; i < numElements; ++i) {
//        result.push_back(moveValues[i].second);
//    }
//
//    for (const auto &item: result){
//        game.board[item.x][item.y] = 3;
//    }
//    game.printBoard();
//    for (const auto &item: result){
//        game.board[item.x][item.y] = 0;
//    }

    std::uniform_int_distribution<> dis(0, moves.size() - 1);
    // 生成一个随机索引
    int random_index = dis(gen);

    // 使用随机索引从数组中获取一个元素
    auto random_element = moves[random_index];
    game.makeMove(random_element);

    return game;
}

std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> selfPlay(int boardSize,
                                                                                        int numGames,
                                                                                        int numSimulations,
                                                                                        float temperatureDefault,
                                                                                        float explorationFactor,
                                                                                        const std::string &part
) {
    torch::Device device = getDevice();
    auto network = getNetwork(device, "model/agent_model.pt");
    MonteCarloTree mcts = MonteCarloTree(&network, device, explorationFactor);
    std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> training_data;

    for (int i = 0; i < numGames; i++) {
        Game game(boardSize);
        std::vector<std::tuple<torch::Tensor, int, std::vector<float>>> game_data;

        game = randomGame(game, mcts);

        int step = 0;
        while (!game.isGameOver()) {
            //如果只有唯一选择，则直接选择
            auto nextActions = selectActions(game);
            if (get<1>(nextActions).size() == 1) {
                int actionIndex = game.getActionIndex(get<1>(nextActions)[0]);
                vector<float> probs(game.boardSize * game.boardSize);
                probs[actionIndex] = 1;
                addAction(game, actionIndex, game_data, probs);
                printGame(game, actionIndex, probs, 0, part, get<2>(nextActions), nullptr);
                step++;
                continue;
            }

            //开始mcts预测
            Node node;

            long startTime = getSystemTime();
            mcts.search(game, &node, numSimulations);
            cout << part << "search cost " << getSystemTime() - startTime << endl;

            std::vector<int> actions;
            std::vector<float> action_probs;
            std::tie(actions, action_probs) = mcts.get_action_probabilities(game);
            mcts.release(&node);

            //计算温度
            float temperature =
                    temperatureDefault * (game.boardSize * game.boardSize - step * 8) /
                    (game.boardSize * game.boardSize);
            if (temperature < 0.2) {
                temperature = 0.2;
            }
            std::vector<float> action_probs_temperature = mcts.apply_temperature(action_probs, temperature);

            // 归一化概率分布
            std::vector<float> action_probs_normalized;
            float sum = std::accumulate(action_probs_temperature.begin(), action_probs_temperature.end(), 0.0f);
            for (const auto &prob: action_probs_temperature) {
                action_probs_normalized.push_back(prob / sum);
            }

            // 随机选择
            std::discrete_distribution<int> distribution(action_probs_normalized.begin(),
                                                         action_probs_normalized.end());
            int action = actions[distribution(gen)];

            addAction(game, action, game_data, action_probs);
            printGame(game, action, action_probs_normalized, temperature, part, node.selectInfo, &mcts);
            step++;
        }

        bool win = game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer());
        int winner = 0;
        if (win) {
            winner = game.getOtherPlayer();
        }
        for (const auto &[state, player, mcts_probs]: game_data) {
            float value = (winner == player) ? 1.0f : ((winner == (3 - player)) ? -1.0f : 0.0f);
            training_data.emplace_back(state, mcts_probs, std::vector<float>{value});
        }

        cout << part << "winner is " << winner << endl;
    }
    return training_data;
}

void recordSelfPlay(
        int boardSize,
        int numGames,
        int numSimulations,
        float temperatureDefault,
        float explorationFactor,
        const std::string &part) {
    // 创建文件流对象
    std::ofstream file("record/data" + part + ".txt");

    if (file.is_open()) {

        auto data = selfPlay(boardSize, numGames, numSimulations, temperatureDefault, explorationFactor,
                             "[" + part + "] ");
        file << data.size() << endl;
        std::cout << "data count " << data.size() << endl;
        for (auto &item: data) {
            auto state = get<0>(item);

            // 获取张量的维度
            int64_t dim0 = state.size(0);
            int64_t dim1 = state.size(1);
            int64_t dim2 = state.size(2);

            file << dim0 << " " << dim1 << " " << dim2 << endl;
            // 遍历张量并打印数值
            for (int64_t i = 0; i < dim0; ++i) {
                for (int64_t j = 0; j < dim1; ++j) {
                    for (int64_t k = 0; k < dim2; ++k) {
                        file << state[i][j][k].item<float>() << " ";
                    }
                    file << endl;
                }
            }

            vector<float> mctsProbList = get<1>(item);
            file << mctsProbList.size() << endl;
            for (auto f: mctsProbList) {
                file << f << " ";
            }
            file << endl;

            vector<float> valueList = get<2>(item);
            file << valueList.size() << endl;
            for (auto f: valueList) {
                file << f << " ";
            }
            file << endl;
        }

        // 关闭文件
        file.close();
        std::cout << "Data has been written to file." << std::endl;
    } else {
        std::cerr << "Failed to open file." << std::endl;
    }
}