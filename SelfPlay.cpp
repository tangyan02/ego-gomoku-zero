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

// 线程局部随机数生成器，避免多线程竞态
static thread_local std::mt19937 gen(std::random_device{}());

void printGame(Game &game, Point action, float rate, vector<float> probs,
               float temperature, const std::string &prefix, const string selectInfo, Model *model) {
    std::string pic = (game.getOtherPlayer() == 1) ? "x" : "o";
    cout << prefix << " " << pic << " " << action.x << ","
            << action.y
            << " rate=" << round(rate * 1000) / 1000
            << " T=" << round(temperature * 100) / 100
            << selectInfo << endl;
}

// 缓存开局库，只读一次文件
static std::vector<std::string>& getCachedOpenings() {
    static std::vector<std::string> lines;
    static bool loaded = false;
    if (!loaded) {
        std::ifstream file("openings/openings.txt");
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            file.close();
        }
        loaded = true;
    }
    return lines;
}

Game randomGame(Game &game, const string &prefix) {
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    double randomNum = dis(gen);

    // 开局策略：
    // 90% 概率：使用开局库
    // 10% 概率：空棋盘直接开始（训练第一手选点能力）
    if (randomNum < 0.1) {
        // 空棋盘直接开始
        cout << prefix << "empty board start" << endl;
        return game;
    }

    // 使用开局库
    auto& lines = getCachedOpenings();
    if (lines.empty()) {
        std::cout << prefix << "No openings loaded, empty board start" << std::endl;
        return game;
    }

    std::uniform_int_distribution<int> disInt(0, lines.size() - 1);
    int randomIndex = disInt(gen);

    std::cout << prefix << "Randomly selected index: " << randomIndex << std::endl;
    std::string randomLine = lines[randomIndex];
    std::cout << prefix << "Randomly selected coordinates: " << randomLine << std::endl;

    std::vector<Point> points;
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
}

tuple<float, Point, float> getNextMove(int step, float temperatureDefault, int tempDownStep,
                                       vector<float>& move_probs,
                                       vector<Point>& moves, MonteCarloTree& mcts)
{
    //按温度决策
    float temperature;
    Point move;
    float rate;

    // 三段式温度策略：
    // 步骤 0 ~ tempDownStep:        temperature = temperatureDefault (充分探索)
    // 步骤 tempDownStep+1 ~ +10:    temperature = 0.3 (有限探索)
    // 步骤 tempDownStep+11+:        temperature = 0 (贪心)
    int exploreEndStep = tempDownStep + 10;

    if (step >= exploreEndStep)
    {
        //温度为0
        temperature = 0;
        move = mcts.get_max_visit_move();
        rate = 1;
        return tuple(temperature, move, rate);
    }

    if (step >= tempDownStep)
    {
        //中间阶段，温度0.3
        temperature = 0.3f;
    } else {
        //前期，温度为默认值
        temperature = temperatureDefault;
    }

    // 使用温度采样
    auto [tempMoves, tempProbs] = mcts.get_action_probabilities(temperature);
    std::discrete_distribution<int> distribution(tempProbs.begin(), tempProbs.end());
    int index = distribution(gen);
    move = tempMoves[index];
    rate = tempProbs[index];
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
    int tempDownStep = stoi(ConfigReader::get("temperatureDownBeginStep"));

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

        // 开局落子顺序不反映真实战术意图，清掉最近落子记录
        // 避免通道2/3（最近一步/两步）在MCTS第一步收到误导信息
        game.lastAction = Point();
        game.lastLastAction = Point();

        int step = 0;
        // VCT 开关，读一次缓存
        static bool useVct = (ConfigReader::get("useVct") == "true");

        // 每局清空 Transposition Table（不同局不能共享缓存）
        mcts.clearTranspositionTable();

        // Tree Reuse：整局共用一棵树，每步保留选中子树
        Node* rootNode = new Node();

        while (!game.isGameOver()) {
            //开始mcts预测
            long long startTime = getSystemTime();

            // VCT 子线程：仅在 useVct=true 时启动
            std::atomic running(true);
            std::packaged_task<std::pair<int, std::vector<Point>>(int, Game*, std::atomic<bool>&)> task(dfsVCTIter);
            auto result = task.get_future();
            thread t;
            if (useVct) {
                t = thread(std::move(task), game.currentPlayer, &game, std::ref(running));
            }

            // 补齐模拟次数：子树已有 visits 算作已完成
            int existingVisits = rootNode->visits;
            int targetSimulations = max(numSimulations - existingVisits, 1);

            int realNumSimulations = 1;
            mcts.search(game, rootNode, 1);
            if (mcts.root->children.size() > 1)
            {
                mcts.searchBatched(game, rootNode, targetSimulations - 1, 16);
                realNumSimulations = targetSimulations;
            } else
            {
                mcts.search(game, rootNode, 1);
                realNumSimulations = 2;
            }

            vector<Point> winMoves;
            int level = 0;
            if (useVct) {
                running.store(false);
                t.join();
                tie(level, winMoves) = result.get();
                if(level > 0)
                {
                    cout << prefix << " vct level " << level << ", win move size: " << winMoves.size() << endl;
                }
            }

            vector<Point> moves;
            vector<float> move_probs;

            Node winRootNode;
            tryVctCut(numSimulations, mcts, prefix, game, winMoves, winRootNode);


            tie(moves, move_probs)= mcts.get_action_probabilities();

            //决策下一步
            auto [temperature, move, rate] = getNextMove(step, temperatureDefault, tempDownStep, move_probs, moves, mcts);

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
            printGame(game, move, rate, probs_matrix, temperature, prefix, rootNode->selectInfo, &model);

            // Tree Reuse：保留选中子树，释放其他分支
            Node* selectedChild = nullptr;
            for (auto& item : rootNode->children) {
                if (item.first == move) {
                    selectedChild = item.second;
                } else {
                    item.second->release();
                }
            }
            rootNode->children.clear();

            if (selectedChild != nullptr) {
                selectedChild->parent = nullptr;
                delete rootNode;
                rootNode = selectedChild;
            } else {
                delete rootNode;
                rootNode = new Node();
            }

            step++;
        }

        // 局结束，释放整棵树
        rootNode->release();
        delete rootNode;

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
    auto model = std::make_unique<Model>();
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
