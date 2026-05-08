#include "VCTLabeler.h"

#include "ConfigReader.h"
#include "Game.h"
#include "Analyzer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

// ========== 进度日志 ==========
// 格式: [VCTLabel] file=data_X.txt progress=done/total labeled=N elapsed=Xs
// 前端通过解析这些行在"训练进度"面板展示 VCT 标注进度

static mutex g_logMutex;

static void logProgress(const string& msg) {
    lock_guard<mutex> lk(g_logMutex);
    cout << msg << endl;
    cout.flush();
}

// ========== 单个样本的数据结构 ==========
struct Sample {
    int dim0, dim1, dim2;                   // state 维度（通常 4, 20, 20）
    vector<vector<vector<float>>> state;    // 输入张量
    int probsSize;                          // policy 长度
    vector<float> probs;                    // MCTS 访问分布
    int valueSize;                          // value 长度（通常是 1）
    vector<float> value;                    // value 目标（record 里 TD 混合后的值）

    // 标注后标记
    bool labeledWin = false;
};

// ========== Record 文件格式 ==========
// 首行：样本数 N
// 每个样本：
//   dim0 dim1 dim2
//   <state 数据：dim0 行 × (dim1 * dim2) 个 float>
//   probsSize
//   <probs 数据：一行 probsSize 个 float>
//   valueSize
//   <value 数据：一行 valueSize 个 float>

static bool loadRecord(const string& filepath, vector<Sample>& samples) {
    ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    int sampleCount;
    if (!(file >> sampleCount)) {
        return false;
    }
    samples.clear();
    samples.reserve(sampleCount);

    for (int s = 0; s < sampleCount; s++) {
        Sample sample;
        if (!(file >> sample.dim0 >> sample.dim1 >> sample.dim2)) {
            cerr << "[VCTLabel] ERROR: failed to read dims at sample " << s << " in " << filepath << endl;
            return false;
        }

        sample.state.assign(sample.dim0,
            vector<vector<float>>(sample.dim1, vector<float>(sample.dim2, 0.0f)));

        for (int i = 0; i < sample.dim0; i++) {
            for (int j = 0; j < sample.dim1; j++) {
                for (int k = 0; k < sample.dim2; k++) {
                    if (!(file >> sample.state[i][j][k])) {
                        cerr << "[VCTLabel] ERROR: failed to read state[" << i << "][" << j
                             << "][" << k << "] at sample " << s << endl;
                        return false;
                    }
                }
            }
        }

        if (!(file >> sample.probsSize)) return false;
        sample.probs.resize(sample.probsSize);
        for (int i = 0; i < sample.probsSize; i++) {
            if (!(file >> sample.probs[i])) return false;
        }

        if (!(file >> sample.valueSize)) return false;
        sample.value.resize(sample.valueSize);
        for (int i = 0; i < sample.valueSize; i++) {
            if (!(file >> sample.value[i])) return false;
        }

        samples.push_back(std::move(sample));
    }
    return true;
}

static bool saveRecord(const string& filepath, const vector<Sample>& samples) {
    ofstream file(filepath);
    if (!file.is_open()) return false;

    file << samples.size() << endl;
    for (const auto& sample : samples) {
        file << sample.dim0 << " " << sample.dim1 << " " << sample.dim2 << endl;
        for (int i = 0; i < sample.dim0; i++) {
            for (int j = 0; j < sample.dim1; j++) {
                for (int k = 0; k < sample.dim2; k++) {
                    file << sample.state[i][j][k] << " ";
                }
                file << endl;
            }
        }
        file << sample.probsSize << endl;
        for (auto f : sample.probs) file << f << " ";
        file << endl;
        file << sample.valueSize << endl;
        for (auto f : sample.value) file << f << " ";
        file << endl;
    }
    return true;
}

// ========== State → Game 局面重建 ==========
//
// state 约定：
//   ch0 = 己方棋子位置（即 currentPlayer 的子）
//   ch1 = 对方棋子位置
//   ch2/ch3 = VCF 通道（不需要从 state 还原，重建后自动计算）
//
// 返回的 Game：
//   - board 已填好
//   - currentPlayer 根据棋子数量推断（黑先手，差值为 0 则黑先，差值为 1 则白先）
//     * 由于 ch0 = 己方，ch1 = 对方，我们默认 currentPlayer=1（黑），然后通过棋子数修正
//
// 注意：不还原 historyMoves / lastAction（VCT 标注不需要）
static Game reconstructGame(const Sample& sample) {
    int boardSize = sample.dim1;
    Game game(boardSize);

    // 统计棋子数，确定 currentPlayer
    int myCount = 0, oppCount = 0;
    for (int r = 0; r < boardSize; r++) {
        for (int c = 0; c < boardSize; c++) {
            bool my = sample.state[0][r][c] > 0.5f;
            bool opp = sample.state[1][r][c] > 0.5f;
            if (my) myCount++;
            if (opp) oppCount++;
        }
    }

    // 黑先手。若黑白子数相同，轮到黑走；若黑比白多一子，轮到白走
    // ch0=己方，ch1=对方
    //   - 己方=黑 且 对方=白：myCount == oppCount（黑要走）或 oppCount+1（不对，黑一定先）
    //     实际上：黑先走，myCount 先 +1，所以 currentPlayer=黑 时 myCount >= oppCount
    //   - 己方=白 且 对方=黑：myCount < oppCount（黑比白多一子，轮到白）
    //
    // 结论：
    //   - myCount >= oppCount → currentPlayer = 1 (黑)
    //   - myCount < oppCount → currentPlayer = 2 (白)
    int currentPlayer = (myCount >= oppCount) ? 1 : 2;
    int otherPlayer = 3 - currentPlayer;

    for (int r = 0; r < boardSize; r++) {
        for (int c = 0; c < boardSize; c++) {
            bool my = sample.state[0][r][c] > 0.5f;
            bool opp = sample.state[1][r][c] > 0.5f;
            if (my) {
                game.board[r][c] = currentPlayer;
                game.emptyCount--;
            } else if (opp) {
                game.board[r][c] = otherPlayer;
                game.emptyCount--;
            }
        }
    }

    game.currentPlayer = currentPlayer;
    // lastAction/lastLastAction 保持默认 Point()（VCT 搜索会重新评估所有方向）

    return game;
}

// ========== 单样本标注 ==========
//
// 返回值：
//   0 = 未必胜，保留原值
//   1 = 必胜，且原值已经是 +1（无变化，无增益）
//   2 = 必胜，且原值不是 +1（被覆盖，真正的标注增益）
static int tryLabelSample(Sample& sample, int maxTimeMs) {
    Game game = reconstructGame(sample);

    bool isWin = false;

    // Step 1: 快速路径 selectActions
    auto [win, _, __] = selectActions(game);
    if (win) {
        isWin = true;
    } else {
        // Step 2: 深度 VCT 搜索
        atomic<bool> running(true);
        thread timer([&]() {
            this_thread::sleep_for(chrono::milliseconds(maxTimeMs));
            running.store(false);
        });
        timer.detach();

        extern std::pair<int, std::vector<Point>>
            dfsVCTIter(int currentPlayer, Game game, atomic<bool>& running);

        auto [level, winMoves] = dfsVCTIter(game.currentPlayer, game, running);
        if (!winMoves.empty()) {
            isWin = true;
        }
    }

    if (!isWin) return 0;

    // 必胜：判断原 value 是否已经接近 +1（avoid float 比较精确性问题，用 0.99 阈值）
    bool alreadyOne = !sample.value.empty() && sample.value[0] >= 0.99f;

    sample.value.assign(sample.value.size(), 1.0f);
    sample.labeledWin = true;

    return alreadyOne ? 1 : 2;
}

// ========== 单个 record 文件的处理 ==========
struct FileStat {
    string filename;
    int totalSamples = 0;
    int alreadyOneCount = 0;     // 必胜，但原值已是 +1（无增益）
    int changedCount = 0;        // 必胜，且原值不是 +1（真正被覆盖）
    double elapsedSec = 0.0;

    int labeledTotal() const { return alreadyOneCount + changedCount; }
};

static FileStat processFile(const string& filepath, int maxTimeMs) {
    FileStat stat;
    stat.filename = fs::path(filepath).filename().string();

    auto t0 = chrono::steady_clock::now();

    vector<Sample> samples;
    if (!loadRecord(filepath, samples)) {
        logProgress("[VCTLabel] ERROR: failed to load " + filepath);
        return stat;
    }
    stat.totalSamples = (int)samples.size();

    // 立刻打一条 progress=0 让前端进度条第一时间显示出来（0% + 文件名）
    {
        ostringstream oss;
        oss << "[VCTLabel] file=" << stat.filename
            << " progress=0/" << stat.totalSamples
            << " labeled=0 changed=0 elapsed=0s";
        logProgress(oss.str());
    }

    // 进度日志触发：每 3 秒 或 每 5% 样本（取较先到达者），让前端进度更新更及时
    int logInterval = std::max(1, stat.totalSamples / 20);  // 5% 兜底
    auto lastLogTime = t0;

    for (int i = 0; i < (int)samples.size(); i++) {
        int r = tryLabelSample(samples[i], maxTimeMs);
        if (r == 1) stat.alreadyOneCount++;
        else if (r == 2) stat.changedCount++;

        auto tNow = chrono::steady_clock::now();
        bool sampleTrigger = (i + 1) % logInterval == 0;
        bool timeTrigger = chrono::duration<double>(tNow - lastLogTime).count() >= 3.0;
        bool finalTrigger = (i + 1 == stat.totalSamples);

        if (sampleTrigger || timeTrigger || finalTrigger) {
            ostringstream oss;
            oss << "[VCTLabel] file=" << stat.filename
                << " progress=" << (i + 1) << "/" << stat.totalSamples
                << " labeled=" << stat.labeledTotal()
                << " changed=" << stat.changedCount
                << " elapsed=" << fixed << chrono::duration_cast<chrono::seconds>(tNow - t0).count() << "s";
            logProgress(oss.str());
            lastLogTime = tNow;
        }
    }

    // 原子写回：先写 tmp，再 rename
    string tmpPath = filepath + ".tmp";
    if (!saveRecord(tmpPath, samples)) {
        logProgress("[VCTLabel] ERROR: failed to save " + tmpPath);
        return stat;
    }
    if (rename(tmpPath.c_str(), filepath.c_str()) != 0) {
        logProgress("[VCTLabel] ERROR: failed to rename " + tmpPath + " -> " + filepath);
        return stat;
    }

    auto t1 = chrono::steady_clock::now();
    stat.elapsedSec = chrono::duration<double>(t1 - t0).count();

    ostringstream oss;
    oss << "[VCTLabel] DONE file=" << stat.filename
        << " total=" << stat.totalSamples
        << " labeled=" << stat.labeledTotal()
        << " changed=" << stat.changedCount
        << " alreadyOne=" << stat.alreadyOneCount
        << " (" << fixed;
    oss.precision(1);
    double labelRatio = stat.totalSamples > 0 ? 100.0 * stat.labeledTotal() / stat.totalSamples : 0.0;
    double changeRatio = stat.totalSamples > 0 ? 100.0 * stat.changedCount / stat.totalSamples : 0.0;
    oss << "label=" << labelRatio << "% change=" << changeRatio
        << "%) elapsed=" << stat.elapsedSec << "s";
    logProgress(oss.str());

    return stat;
}

// ========== 主入口 ==========
void runVCTLabeling() {
    int maxTimeMs = stoi(ConfigReader::getOrDefault("vctLabelMaxTimeMs", "5000"));
    int numThreads = stoi(ConfigReader::getOrDefault("vctLabelProcesses", "8"));
    string recordDir = ConfigReader::getOrDefault("vctLabelRecordDir", "record");

    // 收集所有 record 文件
    vector<string> files;
    if (fs::exists(recordDir) && fs::is_directory(recordDir)) {
        for (const auto& entry : fs::directory_iterator(recordDir)) {
            string name = entry.path().filename().string();
            if (name.find("data_") == 0 && name.size() > 4 &&
                name.substr(name.size() - 4) == ".txt") {
                files.push_back(entry.path().string());
            }
        }
    }
    std::sort(files.begin(), files.end());

    if (files.empty()) {
        logProgress("[VCTLabel] WARN: no record files found in " + recordDir);
        return;
    }

    // 打印启动信息（一行紧凑信息，方便前端解析）
    {
        ostringstream oss;
        oss << "[VCTLabel] START files=" << files.size()
            << " threads=" << numThreads
            << " maxTimeMs=" << maxTimeMs;
        logProgress(oss.str());
    }

    auto t0 = chrono::steady_clock::now();

    // 多文件并行：每个线程取一个文件
    atomic<int> fileIdx(0);
    atomic<int> totalAlreadyOne(0);
    atomic<int> totalChanged(0);
    atomic<int> totalSamples(0);
    atomic<int> filesDone(0);

    vector<thread> workers;
    for (int t = 0; t < numThreads; t++) {
        workers.emplace_back([&]() {
            while (true) {
                int idx = fileIdx.fetch_add(1);
                if (idx >= (int)files.size()) break;
                FileStat stat = processFile(files[idx], maxTimeMs);
                totalAlreadyOne.fetch_add(stat.alreadyOneCount);
                totalChanged.fetch_add(stat.changedCount);
                totalSamples.fetch_add(stat.totalSamples);
                int done = filesDone.fetch_add(1) + 1;

                // 全局进度行（训练面板展示用）
                int aOne = totalAlreadyOne.load();
                int chg = totalChanged.load();
                int total = totalSamples.load();
                ostringstream oss;
                oss << "[VCTLabel] global progress=" << done << "/" << files.size()
                    << " labeled=" << (aOne + chg) << "/" << total
                    << " changed=" << chg
                    << " alreadyOne=" << aOne;
                logProgress(oss.str());
            }
        });
    }
    for (auto& w : workers) w.join();

    auto t1 = chrono::steady_clock::now();
    double totalElapsed = chrono::duration<double>(t1 - t0).count();

    int aOne = totalAlreadyOne.load();
    int chg = totalChanged.load();
    int total = totalSamples.load();
    int totalLabel = aOne + chg;

    ostringstream oss;
    oss << "[VCTLabel] FINISH totalFiles=" << files.size()
        << " totalSamples=" << total
        << " totalLabeled=" << totalLabel
        << " changed=" << chg
        << " alreadyOne=" << aOne
        << " (" << fixed;
    oss.precision(1);
    double labelRatio = total > 0 ? 100.0 * totalLabel / total : 0.0;
    double changeRatio = total > 0 ? 100.0 * chg / total : 0.0;
    oss << "label=" << labelRatio << "% change=" << changeRatio
        << "%) elapsed=" << totalElapsed << "s";
    logProgress(oss.str());
}
