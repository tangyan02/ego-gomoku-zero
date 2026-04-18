#include "Evaluate.h"
#include "Game.h"
#include "MCTS.h"
#include "Model.h"
#include "Analyzer.h"
#include "ConfigReader.h"
#include "Utils.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

static std::mt19937 evalRng(std::random_device{}());

/**
 * 加载 openings 文件（只读一次）
 * 评估时合并生成开局 + 手动开局，覆盖更广
 */
static std::vector<std::string>& getOpenings() {
    static std::vector<std::string> lines;
    static bool loaded = false;
    if (!loaded) {
        // 生成开局的文件路径候选（评估只用生成开局）
        vector<pair<string, vector<string>>> sources = {
            {"generated", {"openings/openings.txt", "../train/openings/openings.txt", "../openings/openings.txt"}},
        };
        for (auto& [label, paths] : sources) {
            for (auto& path : paths) {
                std::ifstream file(path);
                if (file.is_open()) {
                    int count = 0;
                    std::string line;
                    while (std::getline(file, line)) {
                        if (!line.empty()) {
                            lines.push_back(line);
                            count++;
                        }
                    }
                    file.close();
                    cout << "[Evaluate] Loaded " << count << " " << label << " openings from " << path << endl;
                    break;
                }
            }
        }
        if (lines.empty()) {
            cout << "[Evaluate] WARNING: No openings file found, using empty board" << endl;
        } else {
            cout << "[Evaluate] Total openings for evaluation: " << lines.size() << endl;
        }
        loaded = true;
    }
    return lines;
}

/**
 * 对 game 应用指定序号的开局
 */
static void applyOpening(Game& game, int index) {
    auto& openings = getOpenings();
    if (index < 0 || index >= (int)openings.size()) return;

    const std::string& line = openings[index];
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        int x = std::stoi(token);
        std::getline(ss, token, ',');
        int y = std::stoi(token);
        game.makeMove(Point(x + game.boardSize / 2, y + game.boardSize / 2));
    }
}

/**
 * 对 game 应用一个随机开局
 */
static void applyRandomOpening(Game& game) {
    auto& openings = getOpenings();
    if (openings.empty()) return;

    std::uniform_int_distribution<int> dist(0, openings.size() - 1);
    int idx = dist(evalRng);
    applyOpening(game, idx);
}

/**
 * 单局对弈：模型A执黑，模型B执白
 * @return 1=A胜, 2=B胜, 0=平局
 */
static int playOneGame(
    Model* modelBlack,
    Model* modelWhite,
    int boardSize,
    int numSimulation,
    float explorationFactor
) {
    Game game(boardSize);
    applyRandomOpening(game);  // 使用随机开局
    MonteCarloTree mctsBlack(modelBlack, explorationFactor);
    MonteCarloTree mctsWhite(modelWhite, explorationFactor);

    while (!game.isGameOver()) {
        Node node;
        MonteCarloTree& mcts = (game.currentPlayer == BLACK) ? mctsBlack : mctsWhite;

        mcts.search(game, &node, numSimulation);
        Point move = mcts.get_max_visit_move();
        game.makeMove(move);
        node.release();
    }

    if (game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
        return game.getOtherPlayer(); // 最后落子方获胜
    }
    return 0; // 平局
}

/**
 * 从胜率计算 Elo 差值
 * winRate 是模型1的胜率 (0~1)
 * Elo差 = 400 * log10(winRate / (1 - winRate))
 */
static double winRateToElo(double winRate) {
    if (winRate <= 0.0) return -999.0;
    if (winRate >= 1.0) return 999.0;
    return 400.0 * log10(winRate / (1.0 - winRate));
}

/**
 * 单局对弈：使用指定开局序号
 */
static int playOneGameWithOpening(
    Model* modelBlack,
    Model* modelWhite,
    int boardSize,
    int numSimulation,
    float explorationFactor,
    int openingIndex
) {
    Game game(boardSize);
    applyOpening(game, openingIndex);
    MonteCarloTree mctsBlack(modelBlack, explorationFactor);
    MonteCarloTree mctsWhite(modelWhite, explorationFactor);

    while (!game.isGameOver()) {
        Node node;
        MonteCarloTree& mcts = (game.currentPlayer == BLACK) ? mctsBlack : mctsWhite;

        mcts.search(game, &node, numSimulation);
        Point move = mcts.get_max_visit_move();
        game.makeMove(move);
        node.release();
    }

    if (game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
        return game.getOtherPlayer();
    }
    return 0;
}

EvalResult evaluateModels(
    const string& modelPath1,
    const string& modelPath2,
    int boardSize,
    int numGames,
    int numSimulation,
    float explorationFactor,
    const string& coreType
) {
    // 加载两个模型
    auto model1 = make_unique<Model>();
    model1->init(modelPath1, coreType);

    auto model2 = make_unique<Model>();
    model2->init(modelPath2, coreType);

    int wins1 = 0, wins2 = 0, draws = 0;

    auto& openings = getOpenings();
    bool useAllOpenings = (numGames == -1);

    if (useAllOpenings && !openings.empty()) {
        // 全开局模式：每个开局先后手各一局，2 线程并行
        int totalOpenings = openings.size();
        int totalGamesExpected = totalOpenings * 2;
        cout << "[Evaluate] " << modelPath1 << " vs " << modelPath2 << endl;
        cout << "[Evaluate] All openings mode: " << totalOpenings << " openings x 2 sides = "
             << totalGamesExpected << " games, " << numSimulation << " simulations/move (parallel)" << endl;

        std::atomic<int> atomicWins1(0), atomicWins2(0), atomicDraws(0), atomicGameCount(0);
        std::mutex coutMutex;

        // 任务列表：(openingIndex, m1IsBlack)
        struct EvalTask { int openingIdx; bool m1IsBlack; };
        std::vector<EvalTask> tasks;
        tasks.reserve(totalGamesExpected);
        for (int i = 0; i < totalOpenings; i++) {
            tasks.push_back({i, true});   // M1 执黑
            tasks.push_back({i, false});  // M1 执白
        }

        std::atomic<int> taskIdx(0);

        auto worker = [&]() {
            while (true) {
                int idx = taskIdx.fetch_add(1);
                if (idx >= (int)tasks.size()) break;

                auto& t = tasks[idx];
                long long startTime = getSystemTime();
                int result;
                if (t.m1IsBlack) {
                    result = playOneGameWithOpening(model1.get(), model2.get(), boardSize, numSimulation, explorationFactor, t.openingIdx);
                } else {
                    result = playOneGameWithOpening(model2.get(), model1.get(), boardSize, numSimulation, explorationFactor, t.openingIdx);
                }
                long long cost = getSystemTime() - startTime;

                // 统计结果
                if (t.m1IsBlack) {
                    if (result == BLACK) atomicWins1++;
                    else if (result == WHITE) atomicWins2++;
                    else atomicDraws++;
                } else {
                    if (result == WHITE) atomicWins1++;
                    else if (result == BLACK) atomicWins2++;
                    else atomicDraws++;
                }

                int count = atomicGameCount.fetch_add(1) + 1;
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    cout << "[Evaluate] Game " << count << "/" << totalGamesExpected
                         << " Opening " << t.openingIdx
                         << (t.m1IsBlack ? " (M1=Black): " : " (M1=White): ")
                         << (t.m1IsBlack
                             ? (result == BLACK ? "M1 WIN" : (result == WHITE ? "M2 WIN" : "DRAW"))
                             : (result == WHITE ? "M1 WIN" : (result == BLACK ? "M2 WIN" : "DRAW")))
                         << " (" << cost << "ms)" << endl;
                }
            }
        };

        // 启动 2 个线程
        std::thread t1(worker);
        std::thread t2(worker);
        t1.join();
        t2.join();

        wins1 = atomicWins1.load();
        wins2 = atomicWins2.load();
        draws = atomicDraws.load();
    } else {
        // 原有随机开局模式
        int halfGames = numGames / 2;
        cout << "[Evaluate] " << modelPath1 << " vs " << modelPath2 << endl;
        cout << "[Evaluate] " << numGames << " games (" << halfGames << " each side), "
             << numSimulation << " simulations/move" << endl;

        // 前半：模型1执黑，模型2执白
        for (int i = 0; i < halfGames; i++) {
            long long startTime = getSystemTime();
            int result = playOneGame(model1.get(), model2.get(), boardSize, numSimulation, explorationFactor);
            long long cost = getSystemTime() - startTime;

            if (result == BLACK) wins1++;
            else if (result == WHITE) wins2++;
            else draws++;

            cout << "[Evaluate] Game " << (i + 1) << "/" << numGames
                 << " (M1=Black): " << (result == BLACK ? "M1 WIN" : (result == WHITE ? "M2 WIN" : "DRAW"))
                 << " (" << cost << "ms)" << endl;
        }

        // 后半：模型2执黑，模型1执白
        for (int i = 0; i < halfGames; i++) {
            long long startTime = getSystemTime();
            int result = playOneGame(model2.get(), model1.get(), boardSize, numSimulation, explorationFactor);
            long long cost = getSystemTime() - startTime;

            if (result == WHITE) wins1++;
            else if (result == BLACK) wins2++;
            else draws++;

            cout << "[Evaluate] Game " << (halfGames + i + 1) << "/" << numGames
                 << " (M1=White): " << (result == WHITE ? "M1 WIN" : (result == BLACK ? "M2 WIN" : "DRAW"))
                 << " (" << cost << "ms)" << endl;
        }
    }

    int totalGames = wins1 + wins2 + draws;
    double score1 = wins1 + draws * 0.5;
    double winRate1 = score1 / totalGames;
    double eloDiff = winRateToElo(winRate1);

    cout << endl;
    cout << "[Evaluate] ========== Results ==========" << endl;
    cout << "[Evaluate] Model1 (challenger): " << modelPath1 << endl;
    cout << "[Evaluate] Model2 (baseline):   " << modelPath2 << endl;
    cout << "[Evaluate] Wins: " << wins1 << " | Losses: " << wins2 << " | Draws: " << draws << endl;
    cout << "[Evaluate] Win rate: " << (winRate1 * 100) << "%" << endl;
    cout << "[Evaluate] Elo diff: " << (eloDiff >= 0 ? "+" : "") << round(eloDiff) << endl;
    cout << "[Evaluate] ============================" << endl;

    return EvalResult{wins1, wins2, draws, totalGames, winRate1, eloDiff};
}

void runEvaluate() {
    int boardSize = stoi(ConfigReader::get("boardSize"));
    int numSimulation = stoi(ConfigReader::get("evalSimulation"));
    float explorationFactor = stof(ConfigReader::get("explorationFactor"));
    string coreType = ConfigReader::get("coreType");
    string modelPath1 = ConfigReader::get("evalModelPath1");
    string modelPath2 = ConfigReader::get("evalModelPath2");
    int numGames = stoi(ConfigReader::get("evalGames"));

    evaluateModels(modelPath1, modelPath2, boardSize, numGames, numSimulation, explorationFactor, coreType);
}
