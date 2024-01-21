#include "SelfPlay.h"

using namespace std;

void printGame(Game &game, int action, std::vector<float> &action_probs, float temperature) {
    game.printBoard();
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
    std::string pic = (game.getOtherPlayer() == 1) ? "x" : "o";
    cout << pic << " action is " << game.getPointFromIndex(action).x << "," << game.getPointFromIndex(action).y
         << " on rate " << round(action_probs[action] * 1000) / 1000
         << " temperature " << round(temperature * 100) / 100 << endl;
}

torch::jit::Module getNetwork(torch::Device device) {
    std::string path = "model/net_latest.mdl.pt";
    auto model = torch::jit::load(path);
    model.to(device);
    std::cout << "模型" << path << "已加载" << endl;
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

std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> selfPlay(int numGames,
                                                                                        int numSimulations,
                                                                                        float temperatureDefault,
                                                                                        float explorationFactor) {
    torch::Device device = getDevice();
    auto network = getNetwork(device);
    MonteCarloTree mcts = MonteCarloTree(&network, device, explorationFactor);
    std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> training_data;

    for (int i = 0; i < numGames; i++) {
        Game game;
        std::vector<std::tuple<torch::Tensor, int, std::vector<float>>> game_data;

        int step = 0;
        while (!game.isGameOver()) {
            Node node;
            mcts.search(game, &node, numSimulations);

            std::vector<int> actions;
            std::vector<float> action_probs;
            std::tie(actions, action_probs) = mcts.get_action_probabilities(game);
            mcts.release(&node);

            float temperature =
                    temperatureDefault * (game.boardSize * game.boardSize - step) / (game.boardSize * game.boardSize);
            std::vector<float> action_probs_temperature = mcts.apply_temperature(action_probs, temperature);

            // 归一化概率分布
            std::vector<float> action_probs_normalized;
            float sum = std::accumulate(action_probs_temperature.begin(), action_probs_temperature.end(), 0.0f);
            for (const auto &prob: action_probs_temperature) {
                action_probs_normalized.push_back(prob / sum);
            }

            // 随机选择
            std::random_device rd;
            std::mt19937 gen(rd());
            std::discrete_distribution<int> distribution(action_probs_normalized.begin(),
                                                         action_probs_normalized.end());
            int action = actions[distribution(gen)];

            auto state = game.getState();
            std::tuple<torch::Tensor, int, std::vector<float>> record(state, game.currentPlayer, action_probs);

            game.makeMove(game.getPointFromIndex(action));
            game_data.push_back(record);
            printGame(game, action, action_probs_normalized, temperature);
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

        cout << "winner is " << winner << endl;
    }
    return training_data;
}

void recordSelfPlay(
        int numGames,
        int numSimulations,
        float temperatureDefault,
        float explorationFactor,
        const std::string &shard) {
    // 创建文件流对象
    std::ofstream file("record/data" + shard + ".txt");

    if (file.is_open()) {

        auto data = selfPlay(numGames, numSimulations, temperatureDefault, explorationFactor);
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