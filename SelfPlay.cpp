#include "SelfPlay.h"

#include "ConfigReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <chrono>
#include <future>
#include <mutex>
#include <iomanip>
#include "Analyzer.h"

using namespace std;

// 线程局部随机数生成器，避免多线程竞态
static thread_local std::mt19937 gen(std::random_device{}());

// 200 vs 400 sims 一致性统计
static std::mutex simsLogMutex;
static int simsTotal = 0;
static int simsAgree = 0;

// 调用时必须已持有 simsLogMutex
static void logSimsConsistencyLocked(const std::string &logPath) {
    if (simsTotal > 0 && simsTotal % 100 == 0) {
        std::ofstream ofs(logPath, std::ios::app);
        if (ofs.is_open()) {
            double rate = 100.0 * simsAgree / simsTotal;
            ofs << "[SimsCompare] total=" << simsTotal
                << " agree=" << simsAgree
                << " rate=" << std::fixed << std::setprecision(1) << rate << "%"
                << std::endl;
            ofs.close();
        }
    }
}

void printGame(Game &game, Point action, float rate, float temperature,
               const std::string &prefix, const string selectInfo) {
    std::string pic = (game.getOtherPlayer() == 1) ? "x" : "o";
    cout << prefix << " " << pic << " " << action.x << ","
            << action.y
            << " rate=" << round(rate * 1000) / 1000
            << " T=" << temperature
            << selectInfo
            << endl;
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
    //   5%  空棋盘（训练第一手选点能力）
    //  95%  生成开局库 (openings_train.txt)
    //   手工开局已废弃（评估/训练统一用生成开局做公平对比）
    if (randomNum < 0.05) {
        cout << prefix << "empty board start" << endl;
        return game;
    }

    auto& cache = getCachedOpenings();
    std::vector<std::string>* pool = nullptr;
    string poolName;

    if (cache.generated.empty() && cache.manual.empty()) {
        std::cout << prefix << "No openings loaded, empty board start" << std::endl;
        return game;
    }

    // 优先用生成开局；生成池为空时退化为手工（兜底）
    if (!cache.generated.empty()) {
        pool = &cache.generated;
        poolName = "generated";
    } else {
        pool = &cache.manual;
        poolName = "manual";
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

tuple<float, Point, float, bool> getNextMove(MonteCarloTree& mcts, int step)
{
    // 配置项 temperatureSteps：前 N 步用 T=1.0 按 visit 分布采样（增加开局多样性，缓解平局率上升）
    // N 步后用 T=0 贪心（保证对局质量）
    // 注意：selectActions 已强制必胜手作为唯一 child，T=1 在这种情况退化为唯一选择，不会破坏战术
    static int temperature_steps = stoi(ConfigReader::getOrDefault("temperatureSteps", "10"));
    if (step < temperature_steps) {
        float temperature = 1.0f;
        auto [moves, probs] = mcts.get_action_probabilities(temperature);
        if (moves.empty()) {
            return tuple(temperature, Point(), 1.0f, false);
        }
        // 按 probs 采样
        static thread_local std::mt19937 rng(std::random_device{}());
        std::discrete_distribution<int> dist(probs.begin(), probs.end());
        int idx = dist(rng);
        Point move = moves[idx];
        return tuple(temperature, move, probs[idx], false);
    }

    // 后续：贪心
    float temperature = 0;
    Point move = mcts.get_max_visit_move();
    float rate = 1;
    return tuple(temperature, move, rate, false);
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
        std::vector<std::tuple<vector<vector<vector<float> > >, int, std::vector<float>, float>> game_data;

        game = randomGame(game, prefix);

        // 每局清空 Transposition Table
        mcts.clearTranspositionTable();

        // Tree Reuse：整局共用一棵树
        Node* rootNode = new Node();

        // 提前终止标记（root selectActions win=true 时设置）
        int winnerOverride = 0;

        // 网络决策步数（开局后从 0 开始计数；前 6 步用 T=1 采样增加多样性）
        int decisionStep = 0;

        while (!game.isGameOver()) {
            // 在 MCTS 之前先做一次 selectActions（我方视角，game 还未 makeMove）
            // win=true 表示 root 局面我方有快速路径必胜手（长5/活四/VCF）
            auto [rootWin, rootMoves, rootLabel] = selectActions(game);

            // 补齐模拟次数
            int existingVisits = rootNode->visits;
            int targetSimulations = max(numSimulations - existingVisits, 1);

            // 中间检查点：200 sims 时记录贪心选择
            int halfSims = numSimulations / 2;  // 400 → 200
            int halfTarget = max(halfSims - existingVisits, 0);
            Point halfMove;
            bool hasHalfCheck = (numSimulations > halfSims && halfTarget > 0);

            mcts.search(game, rootNode, 1);
            if (mcts.root->children.size() > 1)
            {
                if (hasHalfCheck) {
                    // 先搜到 halfSims
                    mcts.searchBatched(game, rootNode, halfTarget - 1, 16);
                    halfMove = mcts.get_max_visit_move();
                    // 继续搜剩余部分
                    int remaining = targetSimulations - halfTarget;
                    if (remaining > 0) {
                        mcts.searchBatched(game, rootNode, remaining, 16);
                    }
                } else {
                    mcts.searchBatched(game, rootNode, targetSimulations - 1, 16);
                }
            } else
            {
                mcts.search(game, rootNode, 1);
            }

            // 比对 200 vs 400 sims 的贪心选择
            if (hasHalfCheck && mcts.root->children.size() > 1) {
                Point fullMove = mcts.get_max_visit_move();
                std::lock_guard<std::mutex> lock(simsLogMutex);
                simsTotal++;
                if (halfMove.x == fullMove.x && halfMove.y == fullMove.y) {
                    simsAgree++;
                }
                logSimsConsistencyLocked("log/sims_consistency.log");
            }

            vector<Point> moves;
            vector<float> move_probs;

            tie(moves, move_probs)= mcts.get_action_probabilities();

            auto [temperature, move, rate, forced] = getNextMove(mcts, decisionStep);
            decisionStep++;

            vector<float> probs_matrix(game.boardSize * game.boardSize, 0);
            if (!moves[0].isNull()) {
                for (int k = 0; k < moves.size(); k++) {
                    auto p = moves[k];
                    probs_matrix[game.getActionIndex(p)] = move_probs[k];
                }
            }

            auto state = game.getState();
            float mcts_q = (rootNode->visits > 0) ? (float)(rootNode->value_sum / rootNode->visits) : 0.0f;
            int currentPlayerBeforeMove = game.currentPlayer;
            std::tuple record(state, currentPlayerBeforeMove, probs_matrix, mcts_q);
            game.makeMove(move);
            game_data.push_back(record);

            printGame(game, move, rate, temperature, prefix, rootNode->selectInfo);

            // 1) 直接 5 连：严格必胜
            bool currentWin = game.checkWin(game.lastAction.x, game.lastAction.y, currentPlayerBeforeMove);
            if (currentWin) {
                winnerOverride = currentPlayerBeforeMove;
                break;
            }

            // 2) root selectActions win=true：MCTS 之前已证明我方有必胜手（长5/活四/VCF）
            //    move 来自 max visit，且 MCTS 内部 children 仅限 rootMoves，所以走的就是必胜手
            //    剩余 VCF 链交给后续 step 的 checkWin 自动收尾过于慢，这里直接终止
            if (rootWin) {
                winnerOverride = currentPlayerBeforeMove;
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
                    MonteCarloTree::add_dirichlet_noise(priors, 0.25, 0.2, gen);
                    for (int i = 0; i < numChildren; i++) {
                        childNodes[i]->prior_prob = priors[i];
                    }
                }
            } else {
                delete rootNode;
                rootNode = new Node();
            }

        }

        rootNode->release();
        delete rootNode;

        // winnerOverride 在每步 makeMove 后由 checkWin 即时设置；
        // 走到这里时若仍为 0，说明循环是因棋盘下满或其它非胜负原因退出，视为平局。
        int winner = winnerOverride;

        // n-step TD bootstrapping
        static int td_n = stoi(ConfigReader::getOrDefault("tdN", "5"));
        static float td_gamma = stof(ConfigReader::getOrDefault("tdGamma", "0.7"));
        float gamma_n = pow(td_gamma, td_n);
        // 视角推导：
        //   root.value_sum 是 backpropagate(node, -value) 起始 + 沿途翻转累积出来的，
        //   推导可得：root.value_sum/root.visits 视角 = root 玩家的对手视角（即 root 父视角）。
        //   所以 mcts_q[t+td_n] 视角 = player[t+td_n] 的对手视角。
        //   要 align 到 final_value 的视角（player[t] 视角）：
        //     - player[t+td_n] == player[t]（td_n 偶）→ 对手视角 vs player[t] 视角 → 乘 -1
        //     - player[t+td_n] != player[t]（td_n 奇）→ 对手视角 vs player[t] 视角恰好对齐 → 乘 +1
        float perspective_sign = (td_n % 2 == 0) ? -1.0f : 1.0f;

        cout << prefix << "winner is " << winner << endl;
        MonteCarloTree::printPerfStats();

        // 丢弃平局对局：value target 全是 0，对 value head 无监督信号；
        // policy 也是双方互防到棋盘满的低质量数据。
        // 配置项 skipDrawGames=true 时跳过；默认 true。
        static bool skip_draw = ConfigReader::getOrDefault("skipDrawGames", "true") == "true";
        if (skip_draw && winner == 0) {
            cout << prefix << "draw game discarded" << endl;
            continue;
        }

        int game_len = game_data.size();
        for (int t = 0; t < game_len; t++) {
            const auto &[state, player, mcts_probs, mcts_q] = game_data[t];
            float final_value = (winner == player) ? 1.0f : ((winner == (3 - player)) ? -1.0f : 0.0f);

            float value;
            if (t + td_n < game_len) {
                float mcts_q_tn = perspective_sign * get<3>(game_data[t + td_n]);
                value = gamma_n * mcts_q_tn + (1 - gamma_n) * final_value;
            } else {
                value = final_value;
            }

            training_data.emplace_back(state, mcts_probs, std::vector<float>{value});
        }
    }
    return training_data;
}

void recordSelfPlay(
    int boardSize,
    Context* context,
    int numSimulations,
    float explorationFactor,
    int shard,
    Model* sharedModel) {
    // 使用共享模型（主线程已初始化，避免多线程并发 MPS 初始化崩溃）
    Model* model = sharedModel;

    // 创建文件流对象
    std::ofstream file("record/data_" + to_string(shard) + ".txt");

    if (file.is_open()) {
        auto data = selfPlay(boardSize, context, numSimulations,
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
