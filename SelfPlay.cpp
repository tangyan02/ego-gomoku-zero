#include "SelfPlay.h"

#include "ConfigReader.h"

using namespace std;

// 创建一个随机数生成器
std::random_device rd;
std::mt19937 gen(rd());

void printGame(Game& game, Point action, float rate, vector<float> probs,
               float temperature, const std::string& prefix, const string selectInfo, Model* model)
{
    game.printBoard(prefix);
    std::string line;
    for (int i = 0; i < game.boardSize * game.boardSize; i++)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << probs[i];
        line += ss.str() + " ";
        if ((i + 1) % game.boardSize == 0)
        {
            cout << line << endl;
            line = "";
        }
    }

    float value = 1;
    if (model != nullptr)
    {
        auto state = game.getState();
        //        auto eval = model->evaluate_state(state);
        //        value = -eval.first;
    }

    std::string pic = (game.getOtherPlayer() == 1) ? "x" : "o";
    cout << prefix << " " << pic << " action is " << action.x << ","
        << action.y
        << " on rate " << round(rate * 1000) / 1000
        << " temperature " << round(temperature * 100) / 100
        //         << " value " << value
        << selectInfo << endl;
}

void addAction(Game& game,
               Point action,
               std::vector<std::tuple<vector<vector<vector<float>>>, int, std::vector<float>>>& game_data,
               std::vector<float>& action_probs
)
{
    auto state = game.getState();
    std::tuple record(state, game.currentPlayer, action_probs);
    game.makeMove(action);
    game_data.push_back(record);
}

Game randomGame(Game& game, const string& prefix)
{
    std::uniform_real_distribution<double> dis(0.0, 1.0); // 生成 0 到 1 之间的均匀分布的随机数
    double randomNum = dis(gen); // 生成随机数
    // cout << randomNum << endl;
    //    if (randomNum < 0.5) {
    if (randomNum < 0)
    {
        std::ifstream file("openings/openings.txt"); // 打开文件
        std::vector<std::string> lines; // 存储文件中的每一行

        if (file.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                if (!line.empty())
                {
                    // 检查行是否为空
                    lines.push_back(line); // 将非空行添加到 lines 向量中
                }
            }
            file.close(); // 关闭文件
        }
        else
        {
            std::cout << "Failed to open the file." << std::endl;
            return 1;
        }

        std::uniform_int_distribution<int> disInt(0, lines.size() - 1);
        int randomIndex = disInt(gen); // 生成随机数

        std::cout << prefix << "Randomly selected index: " << randomIndex << std::endl;
        std::string randomLine = lines[randomIndex]; // 获取随机选择的行
        std::cout << prefix << "Randomly selected coordinates: " << randomLine << std::endl;

        std::vector<Point> points; // 存储 Point 对象的数组
        // 将字符串分割为坐标点，并将它们转换为 Point 对象
        std::stringstream ss(randomLine);
        std::string token;

        while (std::getline(ss, token, ','))
        {
            Point point;
            point.x = std::stoi(token);

            std::getline(ss, token, ',');
            point.y = std::stoi(token);

            points.push_back(point);
        }

        for (const auto& item : points)
        {
            int x = item.x + game.boardSize / 2;
            int y = item.y + game.boardSize / 2;
            cout << prefix << "make move " << x << "," << y << endl;
            game.makeMove(Point(x, y));
        }

        return game;
    }
    else
    {
        //开局随机去下完后，价值接近0的点
        auto moves = game.getEmptyPoints();

        std::uniform_int_distribution<> dis(0, moves.size() - 1);
        // 生成一个随机索引
        int random_index = dis(gen);

        // 使用随机索引从数组中获取一个元素
        auto random_element = moves[random_index];
        game.makeMove(random_element);

        cout << prefix << "random action is " << random_element.x << "," << random_element.y << " on game" << endl;
    }

    return game;
}

std::vector<std::tuple<vector<vector<vector<float>>>, std::vector<float>, std::vector<float>>> selfPlay(
    int boardSize,
    int numGames,
    int numSimulations,
    float temperatureDefault,
    float explorationFactor,
    int shard,
    Model& model
)
{
    MonteCarloTree mcts = MonteCarloTree(&model, explorationFactor, true);
    std::vector<std::tuple<vector<vector<vector<float>>>, std::vector<float>, std::vector<float>>> training_data;

    for (int i = 0; i < numGames; i++)
    {
        string prefix = "[" + to_string(shard) + "-" + std::to_string(i) + "]";

        cout << "============= " << prefix << "============" << endl;

        Game game(boardSize);
        std::vector<std::tuple<vector<vector<vector<float>>>, int, std::vector<float>>> game_data;

        game = randomGame(game, prefix);

        int step = 0;
        while (!game.isGameOver())
        {
            Node node;
            //开始mcts预测
            long startTime = getSystemTime();
            mcts.search(game, &node, numSimulations);

            cout << prefix << "search cost " << getSystemTime() - startTime << " ms, simi num " << numSimulations <<
                ", "
                << "per simi " << (getSystemTime() - startTime) / numSimulations << " ms" << endl;

            //计算温度
            float temperature = temperatureDefault;

            auto [actions, action_probs] = mcts.get_action_probabilities(temperature);


            // 随机选择
            std::discrete_distribution<int> distribution(action_probs.begin(),
                                                         action_probs.end());

            auto index = distribution(gen);
            Point action = actions[index];
            auto rate = action_probs[index];

            // 构造矩阵
            vector<float> probs_matrix(game.boardSize * game.boardSize, 0);

            if (!actions[0].isNull()) {
                for (int k = 0; k < actions.size(); k++) {
                    auto p = actions[k];
                    probs_matrix[game.getActionIndex(p)] = action_probs[k];
                }
            }
            addAction(game, action, game_data, probs_matrix);
            printGame(game, action, rate, probs_matrix, temperature, prefix, node.selectInfo, &model);
            step++;
        }

        bool win = game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer());
        int winner = 0;
        if (win)
        {
            winner = game.getOtherPlayer();
        }
        for (const auto& [state, player, mcts_probs] : game_data)
        {
            float value = (winner == player) ? 1.0f : ((winner == (3 - player)) ? -1.0f : 0.0f);
            training_data.emplace_back(state, mcts_probs, std::vector<float>{value});
        }

        cout << prefix << "winner is " << winner << endl;
    }
    return training_data;
}

void recordSelfPlay(
    int boardSize,
    int numGames,
    int numSimulations,
    float temperatureDefault,
    float explorationFactor,
    int shard)
{
    string modelPath = ConfigReader::get("modelPath");
    string coreType = ConfigReader::get("coreType");
    Model* model = new Model();
    model->init(modelPath, coreType);

    // 创建文件流对象
    std::ofstream file("record/data_" + to_string(shard) + ".txt");

    if (file.is_open())
    {
        auto data = selfPlay(boardSize, numGames, numSimulations, temperatureDefault,
                             explorationFactor, shard, *model);
        file << data.size() << endl;
        std::cout << "data count " << data.size() << endl;
        for (auto& item : data)
        {
            auto state = get<0>(item);

            // 获取张量的维度
            int64_t dim0 = state.size();
            int64_t dim1 = state[0].size();
            int64_t dim2 = state[0][0].size();

            file << dim0 << " " << dim1 << " " << dim2 << endl;
            // 遍历张量并打印数值
            for (int64_t i = 0; i < dim0; ++i)
            {
                for (int64_t j = 0; j < dim1; ++j)
                {
                    for (int64_t k = 0; k < dim2; ++k)
                    {
                        file << state[i][j][k] << " ";
                    }
                    file << endl;
                }
            }

            vector<float> mctsProbList = get<1>(item);
            file << mctsProbList.size() << endl;
            for (auto f : mctsProbList)
            {
                file << f << " ";
            }
            file << endl;

            vector<float> valueList = get<2>(item);
            file << valueList.size() << endl;
            for (auto f : valueList)
            {
                file << f << " ";
            }
            file << endl;
        }

        // 关闭文件
        file.close();
        std::cout << "Data has been written to file." << std::endl;
    }
    else
    {
        std::cerr << "Failed to open file." << std::endl;
    }
}
