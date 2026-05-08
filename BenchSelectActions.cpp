#include "BenchSelectActions.h"

#include "ConfigReader.h"
#include "Game.h"
#include "Analyzer.h"

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

// 运行 dfsVCT（迭代加深，从 L2 开始到 maxLevel）
// 返回：是否存在 VCT 必胜
static bool checkVCT(Game& game, int maxLevel) {
    atomic<bool> running(true);
    // 迭代加深：L2 → L4 → ... → maxLevel（复刻 dfsVCTIter 行为，但允许自定义 maxLevel）
    for (int level = 2; level <= maxLevel; level += 2) {
        auto result = dfsVCT(game.currentPlayer, game.currentPlayer, game, running,
                             game.lastAction, game.lastLastAction, Point(),
                             /*fourMode=*/false, 0, 0, /*maxThreeCount=*/99,
                             level);
        if (result.first) return true;
        if (!running.load()) break;
    }
    return false;
}

void runBenchSelectActions() {
    int boardSize = stoi(ConfigReader::getOrDefault("boardSize", "20"));
    string openingsPath = ConfigReader::getOrDefault("benchOpeningsPath", "openings/openings_train.txt");
    int maxLoad = stoi(ConfigReader::getOrDefault("benchMaxLoad", "200"));
    int extraMoves = stoi(ConfigReader::getOrDefault("benchExtraMoves", "8"));
    int samplesPerBase = stoi(ConfigReader::getOrDefault("benchSamplesPerBase", "3"));

    cout << "[Bench] 加载开局: " << openingsPath << endl;
    auto base = loadOpenings(openingsPath, boardSize, maxLoad);
    cout << "[Bench] 读取 " << base.size() << " 条开局" << endl;

    cout << "[Bench] 扩展为中盘局面：每个开局继续随机走 " << extraMoves
         << " 步，采样 " << samplesPerBase << " 次" << endl;
    auto games = expandGames(base, extraMoves, samplesPerBase);
    cout << "[Bench] 生成 " << games.size() << " 个测试局面" << endl;

    if (games.empty()) {
        cout << "[Bench] ERROR: 没有可测试的局面" << endl;
        return;
    }

    cout << "\n========== 开始基准测试 ==========\n" << endl;

    // 基线：selectActions 本身
    auto r0 = benchOne("selectActions (baseline)", games, [](Game& g) {
        auto [win, _, __] = selectActions(g);
        return win;
    });

    // 基线 + 5 层 VCT
    auto r5 = benchOne("selectActions + VCT L5", games, [](Game& g) {
        auto [win, _, __] = selectActions(g);
        if (win) return true;
        return checkVCT(g, 5);
    });

    // 基线 + 7 层 VCT
    auto r7 = benchOne("selectActions + VCT L7", games, [](Game& g) {
        auto [win, _, __] = selectActions(g);
        if (win) return true;
        return checkVCT(g, 7);
    });

    // 基线 + 9 层 VCT
    auto r9 = benchOne("selectActions + VCT L9", games, [](Game& g) {
        auto [win, _, __] = selectActions(g);
        if (win) return true;
        return checkVCT(g, 9);
    });

    // 输出
    auto printRow = [](const BenchResult& r) {
        double avgMs = r.total > 0 ? r.totalMs / r.total : 0.0;
        cout << "  " << r.name << endl
             << "    avg=" << avgMs << "ms"
             << "  P50=" << r.medianMs << "ms"
             << "  P75=" << r.p75Ms << "ms"
             << "  P95=" << r.p95Ms << "ms"
             << "  P99=" << r.p99Ms << "ms"
             << "  Max=" << r.maxMs << "ms" << endl
             << "    WinsFound=" << r.winsFound << "/" << r.total
             << "  (必胜发现率=" << (100.0 * r.winsFound / std::max(1, r.total)) << "%)"
             << endl;
    };

    cout.precision(3);
    cout << fixed;
    printRow(r0);
    printRow(r5);
    printRow(r7);
    printRow(r9);

    cout << "\n========== 增量对比（相对 baseline）==========\n" << endl;
    auto compare = [&](const BenchResult& r, int level) {
        double avg0 = r0.total > 0 ? r0.totalMs / r0.total : 0.0;
        double avg = r.total > 0 ? r.totalMs / r.total : 0.0;
        double delta = avg - avg0;
        int extraWins = r.winsFound - r0.winsFound;
        cout << "  VCT L" << level << ": 每次调用额外耗时 +" << delta << "ms"
             << "   额外发现必胜 +" << extraWins
             << " (" << (100.0 * extraWins / std::max(1, r.total)) << "%)" << endl;
    };
    compare(r5, 5);
    compare(r7, 7);
    compare(r9, 9);

    // MCTS 每步 200 次模拟，估算实际拖慢幅度
    cout << "\n========== MCTS 影响估算（每步 200 次模拟）==========\n" << endl;
    double baseOverhead = r0.totalMs / r0.total * 200.0;  // 基线 200 次 selectActions
    cout << "  基线：每步 " << baseOverhead << "ms" << endl;
    for (auto& [r, lv] : vector<pair<BenchResult*, int>>{{&r5, 5}, {&r7, 7}, {&r9, 9}}) {
        double overhead = r->totalMs / r->total * 200.0;
        cout << "  VCT L" << lv << "：每步 " << overhead << "ms"
             << "  (增加 +" << (overhead - baseOverhead) << "ms)" << endl;
    }

    // ========== getState 性能（4ch vs 6ch）==========
    cout << "\n========== getState 性能（INPUT_CHANNELS=" << INPUT_CHANNELS << "）==========\n" << endl;
    {
        const int planeSize = boardSize * boardSize;
        vector<float> buffer(INPUT_CHANNELS * planeSize, 0.0f);

        auto rGet = benchOne("getState (flat)", games, [&](Game& g) {
            g.getState(buffer.data(), INPUT_CHANNELS);
            return false;
        });
        auto print = [](const string& name, const BenchResult& r) {
            double avg = r.total > 0 ? r.totalMs / r.total : 0.0;
            cout << "  " << name << endl
                 << "    avg=" << avg << "ms"
                 << "  P50=" << r.medianMs << "ms"
                 << "  P95=" << r.p95Ms << "ms"
                 << "  P99=" << r.p99Ms << "ms"
                 << "  Max=" << r.maxMs << "ms" << endl;
        };
        print("getState (flat)", rGet);

        double overheadPerStep = rGet.totalMs / rGet.total * 200.0;
        cout << "  → MCTS 每步 200 次 getState 总耗时 ≈ " << overheadPerStep << "ms" << endl;
    }
}
