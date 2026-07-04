#include "BenchSelectActions.h"

#include "ConfigReader.h"
#include "Game.h"
#include "Analyzer.h"
#include "DfpnVCT.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <random>

using namespace std;

// 读取开局文件 → 构造一组 Game 局面
static vector<Game> loadOpenings(const string& path, int boardSize, int maxLoad) {
    vector<Game> games;
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "[Bench] 无法打开开局文件: " << path << endl;
        return games;
    }

    string line;
    int center = boardSize / 2;
    while (getline(file, line) && (int)games.size() < maxLoad) {
        if (line.empty()) continue;

        Game game(boardSize);
        stringstream ss(line);
        string token;
        bool ok = true;
        while (getline(ss, token, ',')) {
            int x, y;
            try {
                x = stoi(token);
                if (!getline(ss, token, ',')) { ok = false; break; }
                y = stoi(token);
            } catch (...) { ok = false; break; }
            int absX = x + center;
            int absY = y + center;
            if (absX < 0 || absX >= boardSize || absY < 0 || absY >= boardSize) {
                ok = false;
                break;
            }
            game.makeMove(Point(absX, absY));
        }
        if (ok) games.push_back(game);
    }
    return games;
}

// 在 Game 上再往下走几步（随机扩展），得到更丰富的测试局面
// 不走棋盘中心对称的点，避免构造重复局面
static vector<Game> expandGames(const vector<Game>& base, int extraMoves, int samplesPerBase) {
    vector<Game> result;
    mt19937 rng(20260505);

    for (auto& baseGame : base) {
        for (int s = 0; s < samplesPerBase; s++) {
            Game g = baseGame;
            for (int m = 0; m < extraMoves; m++) {
                if (g.isGameOver()) break;
                auto empties = g.getNearEmptyPoints(2);
                if (empties.empty()) break;
                uniform_int_distribution<int> pick(0, (int)empties.size() - 1);
                g.makeMove(empties[pick(rng)]);
            }
            if (!g.isGameOver()) {
                result.push_back(g);
            }
        }
    }
    return result;
}

// 针对一组局面：跑某个"策略"(提供 checkWin 的 lambda)，收集耗时 + 必胜发现数
struct BenchResult {
    string name;
    double totalMs = 0.0;
    double medianMs = 0.0;
    double p75Ms = 0.0;
    double p95Ms = 0.0;
    double p99Ms = 0.0;
    double maxMs = 0.0;
    int winsFound = 0;
    int total = 0;
};

template <typename Fn>
static BenchResult benchOne(const string& name, const vector<Game>& games, Fn check) {
    BenchResult r;
    r.name = name;
    r.total = (int)games.size();

    vector<double> latencies;
    latencies.reserve(games.size());

    for (auto game : games) {
        auto t0 = chrono::steady_clock::now();
        bool isWin = check(game);
        auto t1 = chrono::steady_clock::now();
        double ms = chrono::duration<double, milli>(t1 - t0).count();
        latencies.push_back(ms);
        r.totalMs += ms;
        r.maxMs = std::max(r.maxMs, ms);
        if (isWin) r.winsFound++;
    }

    sort(latencies.begin(), latencies.end());
    if (!latencies.empty()) {
        auto pct = [&](double p) {
            int idx = std::min((int)latencies.size() - 1, (int)(latencies.size() * p));
            return latencies[idx];
        };
        r.medianMs = pct(0.50);
        r.p75Ms = pct(0.75);
        r.p95Ms = pct(0.95);
        r.p99Ms = pct(0.99);
    }
    return r;
}



void runBenchSelectActions() {
    int boardSize = stoi(ConfigReader::getOrDefault("boardSize", "20"));
    string openingsPath = ConfigReader::getOrDefault("benchOpeningsPath", "openings/openings_train.txt");
    int maxLoad = stoi(ConfigReader::getOrDefault("benchMaxLoad", "200"));
    int extraMoves = stoi(ConfigReader::getOrDefault("benchExtraMoves", "16"));
    int samplesPerBase = stoi(ConfigReader::getOrDefault("benchSamplesPerBase", "3"));

    cout << "[Bench] 加载开局: " << openingsPath << endl;
    auto base = loadOpenings(openingsPath, boardSize, maxLoad);
    // 也加载 manual 开局增加多样性
    string manualPath = ConfigReader::getOrDefault("benchManualPath", "openings/openings_manual.txt");
    auto baseManual = loadOpenings(manualPath, boardSize, maxLoad);
    base.insert(base.end(), baseManual.begin(), baseManual.end());
    cout << "[Bench] 读取 " << base.size() << " 条开局（train + manual）" << endl;

    cout << "[Bench] 扩展为中盘局面：每个开局继续随机走 " << extraMoves
         << " 步，采样 " << samplesPerBase << " 次" << endl;
    auto games = expandGames(base, extraMoves, samplesPerBase);
    cout << "[Bench] 生成 " << games.size() << " 个测试局面" << endl;

    // 统计平均棋子数
    double avgPieces = 0;
    for (auto& g : games) avgPieces += (g.boardSize * g.boardSize - g.emptyCount);
    avgPieces /= games.size();
    cout << "[Bench] 平均棋子数: " << avgPieces << endl;

    if (games.empty()) {
        cout << "[Bench] ERROR: 没有可测试的局面" << endl;
        return;
    }

    // 取前 200 局测试（中盘局面）
    int testCount = stoi(ConfigReader::getOrDefault("benchTestCount", "200"));
    auto testGames = vector<Game>(games.begin(), games.begin() + std::min((int)games.size(), testCount));
    cout << "[Bench] 实际测试局面数: " << testGames.size() << endl;

    // ===== 对比1: 固定 threeLimit 各自的发现率 (细粒度) =====
    cout << "\n===== 固定 threeLimit (maxNodes=200000, depth=30) =====\n";
    vector<int> threeLimits = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 15, 99};
    int fixedMaxNodes = 200000;

    for (int threeLimit : threeLimits) {
        cout << "\n--- threeLimit=" << threeLimit << " ---" << endl;
        dfpnVCTSetThreeLimit(threeLimit);
        double totalMs = 0;
        int vctCount = 0;
        double maxMs = 0;
        vector<double> latencies;

        for (int i = 0; i < (int)testGames.size(); i++) {
            Game gc = testGames[i];
            std::atomic<bool> running(true);
            auto t0 = chrono::steady_clock::now();
            auto vctResult = dfpnVCT(gc.currentPlayer, gc, running, fixedMaxNodes, 30);
            auto t1 = chrono::steady_clock::now();
            double ms = chrono::duration<double, milli>(t1 - t0).count();
            totalMs += ms;
            latencies.push_back(ms);
            if (ms > maxMs) maxMs = ms;
            if (vctResult.found) vctCount++;
        }
        sort(latencies.begin(), latencies.end());
        auto pct = [&](double p) {
            int idx = std::min((int)latencies.size() - 1, (int)(latencies.size() * p));
            return latencies[idx];
        };
        double avgMs = totalMs / testGames.size();
        cout << "  VCT=" << vctCount << "/" << testGames.size() << " (" << (100.0*vctCount/testGames.size()) << "%)"
             << "  avg=" << avgMs << "ms  P50=" << pct(0.50) << "ms  P95=" << pct(0.95) << "ms  Max=" << maxMs << "ms" << endl;
    }
    dfpnVCTResetThreeLimit();

    // ===== 对比2: 迭代加深版 (threeCount 策略，不同时间预算) =====
    cout << "\n\n===== 迭代加深 dfpnVCTIterDeepen [threeCount策略] (maxNodes=200000) =====\n";
    vector<int> timeBudgets = {50, 100, 200, 500, 1000, 2000};

    for (int budget : timeBudgets) {
        cout << "\n--- timeBudget=" << budget << "ms ---" << endl;
        double totalMs = 0;
        int vctCount = 0;
        double maxMs = 0;
        vector<double> latencies;

        for (int i = 0; i < (int)testGames.size(); i++) {
            Game gc = testGames[i];
            std::atomic<bool> running(true);
            auto t0 = chrono::steady_clock::now();
            auto vctResult = dfpnVCTIterDeepen(gc.currentPlayer, gc, running, 200000, budget);
            auto t1 = chrono::steady_clock::now();
            double ms = chrono::duration<double, milli>(t1 - t0).count();
            totalMs += ms;
            latencies.push_back(ms);
            if (ms > maxMs) maxMs = ms;
            if (vctResult.found) vctCount++;
        }
        sort(latencies.begin(), latencies.end());
        auto pct = [&](double p) {
            int idx = std::min((int)latencies.size() - 1, (int)(latencies.size() * p));
            return latencies[idx];
        };
        double avgMs = totalMs / testGames.size();
        cout << "  VCT=" << vctCount << "/" << testGames.size() << " (" << (100.0*vctCount/testGames.size()) << "%)"
             << "  avg=" << avgMs << "ms  P50=" << pct(0.50) << "ms  P95=" << pct(0.95) << "ms  Max=" << maxMs << "ms" << endl;
    }

}
