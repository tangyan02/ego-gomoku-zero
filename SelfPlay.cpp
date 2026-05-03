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
struct OpeningCache {
    std::vector<std::string> generated;  // 生成开局
    std::vector<std::string> manual;     // 手工设计开局
};

static OpeningCache& getCachedOpenings() {
    static OpeningCache cache;
    static bool loaded = false;
    if (!loaded) {
        // 加载生成开局
        std::ifstream genFile("openings/openings_train.txt");
        if (genFile.is_open()) {
            std::string line;
            while (std::getline(genFile, line)) {
                if (!line.empty()) {
                    cache.generated.push_back(line);
                }
            }
            genFile.close();
        }
        // 加载手工设计开局
        std::ifstream manFile("openings/openings_manual.txt");
        if (manFile.is_open()) {
            std::string line;
            while (std::getline(manFile, line)) {
                if (!line.empty()) {
                    cache.manual.push_back(line);
                }
            }
            manFile.close();
        }
        cout << "[SelfPlay] Loaded " << cache.generated.size() << " generated openings, "
             << cache.manual.size() << " manual openings" << endl;
        loaded = true;
    }
    return cache;
}

Game randomGame(Game &game, const string &prefix) {
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    double randomNum = dis(gen);

    // 开局策略：
    // 5% 概率：空棋盘直接开始（训练第一手选点能力）
    // 95% 概率：使用开局库
    if (randomNum < 0.05) {
        // 空棋盘直接开始
        cout << prefix << "empty board start" << endl;
        return game;
    }

    // 使用开局库：50% 生成开局 vs 50% 手工开局
    auto& cache = getCachedOpenings();
    std::vector<std::string>* pool = nullptr;
    string poolName;

    if (cache.generated.empty() && cache.manual.empty()) {
        std::cout << prefix << "No openings loaded, empty board start" << std::endl;
        return game;
    }

    if (cache.generated.empty()) {
        pool = &cache.manual;
        poolName = "manual";
    } else if (cache.manual.empty()) {
        pool = &cache.generated;
        poolName = "generated";
    } else {
        // 50% 概率选择生成开局或手工开局
        if (dis(gen) < 0.5) {
            pool = &cache.generated;
            poolName = "generated";
        } else {
            pool = &cache.manual;
            poolName = "manual";
        }
    }

    std::uniform_int_distribution<int> disInt(0, pool->size() - 1);
    int randomIndex = disInt(gen);

    std::cout << prefix << "Opening pool=" << poolName << " index=" << randomIndex << std::endl;
    std::string randomLine = (*pool)[randomIndex];
    std::cout << prefix << "Opening coordinates: " << randomLine << std::endl;

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

    // 随机对称变换（8种：4旋转 × 2翻转），在绝对坐标上操作避免越界
    std::uniform_int_distribution<int> transDist(0, 7);
    int transform = transDist(gen);
    int center = game.boardSize / 2;
    int maxIdx = game.boardSize - 1;

    for (auto &p : points) {
        // 先转为绝对坐标
        int r = p.x + center, c = p.y + center;
        // 旋转 (0/90/180/270): (r,c) → (c, max-r) 每次
        int rot = transform % 4;
        for (int i = 0; i < rot; i++) {
            int tmp = r;
            r = c;
            c = maxIdx - tmp;
        }
        // 翻转: (r,c) → (r, max-c)
        if (transform >= 4) {
            c = maxIdx - c;
        }
        p.x = r - center;
        p.y = c - center;
    }

    for (const auto &item: points) {
        int x = item.x + center;
        int y = item.y + center;
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


bool tryVctCut(int numSimulations, MonteCarloTree& mcts, string& prefix, Game& game, vector<Point>& winMoves,
               Node& winRootNode)
{
    auto [win,moves,info] = selectActions(game);
    if (win)
    {
        return true;
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
    return false;
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
        std::vector<std::tuple<vector<vector<vector<float> > >, int, std::vector<float>, float>> game_data;

        game = randomGame(game, prefix);

        int step = 0;

        // 每局清空 Transposition Table
        mcts.clearTranspositionTable();

        // Tree Reuse：整局共用一棵树
        Node* rootNode = new Node();
        int earlyWinner = 0;

        while (!game.isGameOver()) {
            long long startTime = getSystemTime();

            // 补齐模拟次数
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

            vector<Point> moves;
            vector<float> move_probs;

            // 必胜检测
            auto [currentWin, _, __] = selectActions(game);

            tie(moves, move_probs)= mcts.get_action_probabilities();

            auto [temperature, move, rate] = getNextMove(step, temperatureDefault, tempDownStep, move_probs, moves, mcts);

            vector<float> probs_matrix(game.boardSize * game.boardSize, 0);
            if (!moves[0].isNull()) {
                for (int k = 0; k < moves.size(); k++) {
                    auto p = moves[k];
                    probs_matrix[game.getActionIndex(p)] = move_probs[k];
                }
            }

            auto state = game.getState();
            float mcts_q = (rootNode->visits > 0) ? (float)(rootNode->value_sum / rootNode->visits) : 0.0f;
            std::tuple record(state, game.currentPlayer, probs_matrix, mcts_q);
            game.makeMove(move);
            game_data.push_back(record);

            printGame(game, move, rate, probs_matrix, temperature, prefix, rootNode->selectInfo, &model);

            // Early stop：必胜检测
            if (currentWin) {
                cout << prefix << " early stop: forced win detected" << endl;
                earlyWinner = game.getOtherPlayer();
                rootNode->release();
                delete rootNode;
                rootNode = new Node();
                break;
            }

            // Tree Reuse
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

                // Dirichlet noise
                if (!rootNode->children.empty()) {
                    int numChildren = rootNode->children.size();
                    std::vector<float> priors(numChildren);
                    std::vector<Node*> childNodes(numChildren);
                    int idx = 0;
                    for (auto& [pt, child] : rootNode->children) {
                        priors[idx] = (float)child->prior_prob;
                        childNodes[idx] = child;
                        idx++;
                    }
                    MonteCarloTree::add_dirichlet_noise(priors, 0.25, 0.03, gen);
                    for (int i = 0; i < numChildren; i++) {
                        childNodes[i]->prior_prob = priors[i];
                    }
                }
            } else {
                delete rootNode;
                rootNode = new Node();
            }

            step++;
        }

        rootNode->release();
        delete rootNode;

        int winner = earlyWinner;
        if (winner == 0 && game.lastAction.x >= 0) {
            bool win = game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer());
            if (win) {
                winner = game.getOtherPlayer();
            }
        }

        // n-step TD bootstrapping
        static int td_n = stoi(ConfigReader::getOrDefault("tdN", "5"));
        static float td_gamma = stof(ConfigReader::getOrDefault("tdGamma", "0.7"));
        float gamma_n = pow(td_gamma, td_n);

        int game_len = game_data.size();
        for (int t = 0; t < game_len; t++) {
            const auto &[state, player, mcts_probs, mcts_q] = game_data[t];
            float final_value = (winner == player) ? 1.0f : ((winner == (3 - player)) ? -1.0f : 0.0f);

            float value;
            if (t + td_n < game_len) {
                float mcts_q_tn = get<3>(game_data[t + td_n]);
                value = gamma_n * mcts_q_tn + (1 - gamma_n) * final_value;
            } else {
                value = final_value;
            }

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
    int shard,
    Model* sharedModel) {
    // 使用共享模型（主线程已初始化，避免多线程并发 MPS 初始化崩溃）
    Model* model = sharedModel;

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
