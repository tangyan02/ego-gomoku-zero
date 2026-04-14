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

using namespace std;

static std::mt19937 evalRng(std::random_device{}());

/**
 * 加载 openings 文件（只读一次）
 */
static std::vector<std::string>& getOpenings() {
    static std::vector<std::string> lines;
    static bool loaded = false;
    if (!loaded) {
        // 尝试多个可能的路径
        vector<string> paths = {"openings/openings.txt", "../train/openings/openings.txt", "../openings/openings.txt"};
        for (auto& path : paths) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    if (!line.empty()) {
                        lines.push_back(line);
                    }
                }
                file.close();
                cout << "[Evaluate] Loaded " << lines.size() << " openings from " << path << endl;
                break;
            }
        }
        if (lines.empty()) {
            cout << "[Evaluate] WARNING: No openings file found, using empty board" << endl;
        }
        loaded = true;
    }
    return lines;
}

/**
 * 对 game 应用一个随机开局
 */
static void applyRandomOpening(Game& game) {
    auto& openings = getOpenings();
    if (openings.empty()) return;

    std::uniform_int_distribution<int> dist(0, openings.size() - 1);
    int idx = dist(evalRng);
    const std::string& line = openings[idx];

    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        int x = std::stoi(token);
        std::getline(ss, token, ',');
        int y = std::stoi(token);
        // openings 使用相对中心的坐标
        game.makeMove(Point(x + game.boardSize / 2, y + game.boardSize / 2));
    }
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

        if (result == WHITE) wins1++;  // 模型1执白获胜
        else if (result == BLACK) wins2++;  // 模型2执黑获胜
        else draws++;

        cout << "[Evaluate] Game " << (halfGames + i + 1) << "/" << numGames
             << " (M1=White): " << (result == WHITE ? "M1 WIN" : (result == BLACK ? "M2 WIN" : "DRAW"))
             << " (" << cost << "ms)" << endl;
    }

    int totalGames = wins1 + wins2 + draws;
    // 胜率计算：胜=1分，平=0.5分
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
