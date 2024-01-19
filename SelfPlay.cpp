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
         << " on rate " << round(action_probs[action] * 10) / 10
         << " temperature " << round(temperature * 100) / 100 << endl;
}

std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> extendData(
        int boardSize, std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> &play_data) {
    std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> extend_data;
    for (auto data: play_data) {
        auto &state = std::get<0>(data);
        auto &mcts_prob = std::get<1>(data);
        auto &value = std::get<2>(data);
        for (int i = 1; i <= 4; i++) {
            torch::Tensor rotated_state = torch::zeros_like(state);
            // 逆时针旋转 90 度
            for (int j = 0; j < state.size(0); ++j) {
                rotated_state[j] = state[j].rot90(i, {0, 1});
            }
            // 转换为 torch::Tensor， 逆时针旋转 90 度
            torch::Tensor rotated_mcts_prob_tensor = torch::from_blob(mcts_prob.data(), {boardSize, boardSize}).rot90(i,
                                                                                                                      {0,
                                                                                                                       1});

            // 转换回 std::vector<float>
            std::vector<float> rotated_mcts_prob(rotated_mcts_prob_tensor.data_ptr<float>(),
                                                 rotated_mcts_prob_tensor.data_ptr<float>() +
                                                 rotated_mcts_prob_tensor.numel());

            extend_data.emplace_back(rotated_state, rotated_mcts_prob, value);

            // 翻转
            torch::Tensor flip_state = torch::zeros_like(state);
            for (int j = 0; j < state.size(0); ++j) {
                flip_state[j] = rotated_state[j].flip({1});
            }
            torch::Tensor flip_mcts_prob_tensor = rotated_mcts_prob_tensor.flip({1});
            std::vector<float> flip_mcts_prob(flip_mcts_prob_tensor.data_ptr<float>(),
                                              flip_mcts_prob_tensor.data_ptr<float>() + flip_mcts_prob_tensor.numel());

            extend_data.emplace_back(flip_state.clone(), flip_mcts_prob, value);
        }
    }
    return extend_data;
}


void selfPlay(
        torch::Device device,
        int numGames,
        int numSimulations,
        float temperatureDefault,
        float explorationFactor,
        int threadNo,
        std::vector<std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>>> &result) {
    auto network = getNetwork();
    MonteCarloTree mcts = MonteCarloTree(network, device, explorationFactor);
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
    result[threadNo] = extendData(Game().boardSize, training_data);
}


std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> concurrentSelfPlay(
        torch::Device device,
        int numGames,
        int numSimulations,
        float temperatureDefault,
        float explorationFactor,
        int concurrent
) {
    std::vector<std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>>> result(
            numGames);  // 存储计算结果的向量
    std::vector<std::thread> threads;  // 存储线程的向量

    // 创建并启动线程
    for (int i = 0; i < numGames; ++i) {
        threads.emplace_back(selfPlay, device,
                             numGames / concurrent, numSimulations,
                             temperatureDefault, explorationFactor,
                             i, std::ref(result));
    }

    // 等待线程完成
    for (auto &thread: threads) {
        thread.join();
    }

    // 输出计算结果
    std::vector<std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>> totalResult;
    for (int i = 0; i < concurrent; ++i) {
        for (auto &j: result[i]) {
            totalResult.emplace_back(j);
        }
    }
    return totalResult;
}
