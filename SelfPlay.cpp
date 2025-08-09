#include "SelfPlay.h"

#include "ConfigReader.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <chrono>
#include <future>
#include "Analyzer.h"

using namespace std;

// 创建一个随机数生成器
std::random_device rd;
std::mt19937 gen(rd());

void printGame(Game &game, Point action, float rate, vector<float> probs,
               float temperature, const std::string &prefix, const string selectInfo, Model *model) {
    game.printBoard(prefix);
    std::string line;
    for (int i = 0; i < game.boardSize * game.boardSize; i++) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(3) << probs[i];
        line += ss.str() + " ";
        if ((i + 1) % game.boardSize == 0) {
            cout << line << endl;
            line = "";
        }
    }

    float value = 1;
    if (model != nullptr) {
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

Game randomGame(Game &game, const string &prefix) {
    std::uniform_real_distribution<double> dis(0.0, 1.0); // 生成 0 到 1 之间的均匀分布的随机数
    double randomNum = dis(gen); // 生成随机数
    // cout << randomNum << endl;

    float randomRate = stof(ConfigReader::get("randomRate"));
    if (randomNum > randomRate) {
        std::ifstream file("openings/openings.txt"); // 打开文件
        std::vector<std::string> lines; // 存储文件中的每一行

        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    // 检查行是否为空
                    lines.push_back(line); // 将非空行添加到 lines 向量中
                }
            }
            file.close(); // 关闭文件
        } else {
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

        while (std::getline(ss, token, ',')) {
            Point point;
            point.x = std::stoi(token);

            std::getline(ss, token, ',');
            point.y = std::stoi(token);

            points.push_back(point);
        }

        for (const auto &item: points) {
            int x = item.x + game.boardSize / 2;
            int y = item.y + game.boardSize / 2;
            cout << prefix << "make move " << x << "," << y << endl;
            game.makeMove(Point(x, y));
        }

        return game;
    } else {
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

tuple<float, Point, float> getNextMove(int step, float temperatureDefault,vector<float>& move_probs,
                                       vector<Point>& moves, MonteCarloTree& mcts)
{
    //按温度决策
    float temperature;
    Point move;
    float rate;

    //大于n步，温度为0
    if (step >= stoi(ConfigReader::get("temperatureDownBeginStep")))
    {
        //温度为0
        temperature = 0;
        move = mcts.get_max_visit_move();
        rate = 1;
        return tuple(temperature, move, rate);
    }

    //前n步，温度为1
    temperature = temperatureDefault;
    std::discrete_distribution<int> distribution(move_probs.begin(), move_probs.end());
    int index = distribution(gen);
    move = moves[index];
    rate = move_probs[index];
    return tuple(temperature, move, rate);
}


void tryVctCut(int numSimulations, MonteCarloTree& mcts, string& prefix, Game& game, vector<Point>& winMoves,
               Node& winRootNode)
{
    auto [win,moves,info] = selectActions(game);
    if (win)
    {
        return;
    }
    for (auto winMove : winMoves)
    {
        if (mcts.root->children.find(winMove) == mcts.root->children.end())
        {
            cout << " win moves not in children " << endl;
            cout << " win moves ";
            for (auto move : winMoves)
            {
                cout << move.x << "," << move.y << " ";
            }
            cout << endl;


            cout << " children moves ";
            for (auto move : mcts.root->children)
            {
                cout << move.first.x << "," << move.first.y << " ";
            }
            cout << endl;
            exit(0);
        }
    }
    //如果有胜利点，则剪枝
    if (!winMoves.empty())
    {
        mcts.search(game, &winRootNode, 1);

        vector<Point> excludePoints;
        for (const auto& item : winRootNode.children)
        {
            bool exclude = true;
            for (auto winMove : winMoves)
            {
                if (winMove == item.first)
                {
                    exclude = false;
                }
            }
            if (exclude)
            {
                excludePoints.emplace_back(item.first);
            }
        }

        for (auto excludePoint : excludePoints)
        {
            winRootNode.children[excludePoint]->release();
            winRootNode.children.erase(excludePoint);
        }

        //多个胜利点，则再搜一次
        mcts.search(game, &winRootNode, 1);
        if (winMoves.size() > 1)
        {
            long long startTime = getSystemTime();
            mcts.search(game, &winRootNode, numSimulations - 1);
            cout << prefix << " finish second search." << " cost " << getSystemTime() - startTime << " ms, simi num " <<
                numSimulations <<
                ", " << "per simi " << (getSystemTime() - startTime) / numSimulations << " ms" << endl;
        }
    }
}

std::vector<std::tuple<vector<vector<vector<float> > >, std::vector<float>, std::vector<float> > > selfPlay(
    int boardSize,
    Context* context,
    int numSimulations,
    float temperatureDefault,
    float explorationFactor,
    int shard,
    Model &model
) {
    MonteCarloTree mcts = MonteCarloTree(&model, explorationFactor, true);
    std::vector<std::tuple<vector<vector<vector<float> > >, std::vector<float>, std::vector<float> > > training_data;

    while (true){
        int gameNum = context->counter.fetch_add(1);
        if (gameNum >= context->max) {
            break;
        }
        string prefix = "[" + to_string(shard) + "-" + std::to_string(gameNum) + "]";

        cout << "============= " << prefix << "============" << endl;

        Game game(boardSize);
        std::vector<std::tuple<vector<vector<vector<float> > >, int, std::vector<float> > > game_data;

        game = randomGame(game, prefix);

        int step = 0;
        while (!game.isGameOver()) {
            Node node;
            //开始mcts预测
            long long startTime = getSystemTime();

            //并行模拟和VCT计算
            std::atomic running(true);
            // 用 std::packaged_task 包装带返回值的函数
            std::packaged_task task(dfsVCTIter);
            auto result = task.get_future();

            // 启动子线程
            thread t(std::move(task),game.currentPlayer, &game, std::ref(running));

            int realNumSimulations = 1;
            mcts.search(game, &node, 1);
            //如果子节点选择大于1，才继续模拟
            if (mcts.root->children.size() > 1)
            {
                mcts.search(game, &node, numSimulations - 1);
                realNumSimulations = numSimulations;
            } else
            {
                mcts.search(game, &node, 1);
                realNumSimulations = 2;
            }

            cout << prefix << " search cost " << getSystemTime() - startTime << " ms, simi num " << realNumSimulations <<
                    ", "
                    << "per simi " << (getSystemTime() - startTime) / numSimulations << " ms" << endl;

            running.store(false);
            t.join();

            auto [level, winMoves] = result.get();
            cout << prefix << " vct level " << level << ", win move size: " << winMoves.size() << endl;


            vector<Point> moves;
            vector<float> move_probs;

            //有vct的时候剪枝
            Node winRootNode;
            tryVctCut(numSimulations, mcts, prefix, game, winMoves, winRootNode);

            tie(moves, move_probs)= mcts.get_action_probabilities();

            //决策下一步
            auto [temperature, move, rate] = getNextMove(step, temperatureDefault, move_probs, moves, mcts);

            // 构造矩阵
            vector<float> probs_matrix(game.boardSize * game.boardSize, 0);
            if (!moves[0].isNull()) {
                for (int k = 0; k < moves.size(); k++) {
                    auto p = moves[k];
                    probs_matrix[game.getActionIndex(p)] = move_probs[k];
                }
            }

            //记录局面
            auto state = game.getState();
            std::tuple record(state, game.currentPlayer, probs_matrix);
            game.makeMove(move);
            game_data.push_back(record);

            //打印局面
            printGame(game, move, rate, probs_matrix, temperature, prefix, node.selectInfo, &model);
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

        cout << prefix << "winner is " << winner << endl;
    }
    return training_data;
}

void recordSelfPlay(
    int boardSize,
    Context* context,
    int numSimulations,
    float temperatureDefault,
    float explorationFactor,
    int shard) {
    string modelPath = ConfigReader::get("modelPath");
    string coreType = ConfigReader::get("coreType");
    Model *model = new Model();
    model->init(modelPath, coreType);

    // 创建文件流对象
    std::ofstream file("record/data_" + to_string(shard) + ".txt");

    if (file.is_open()) {
        auto data = selfPlay(boardSize, context, numSimulations, temperatureDefault,
                             explorationFactor, shard, *model);
        file << data.size() << endl;
        std::cout << "data count " << data.size() << endl;
        for (auto &item: data) {
            auto state = get<0>(item);

            // 获取张量的维度
            int64_t dim0 = state.size();
            int64_t dim1 = state[0].size();
            int64_t dim2 = state[0][0].size();

            file << dim0 << " " << dim1 << " " << dim2 << endl;
            // 遍历张量并打印数值
            for (int64_t i = 0; i < dim0; ++i) {
                for (int64_t j = 0; j < dim1; ++j) {
                    for (int64_t k = 0; k < dim2; ++k) {
                        file << state[i][j][k] << " ";
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
