#include "GenerateOpenings.h"
#include "Game.h"
#include "Model.h"
#include "ConfigReader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>

using namespace std;

/**
 * 在 C++ 端生成平衡开局库。
 * 用 policy 采样生成候选开局，用双视角 value head 做平衡过滤。
 */
void runGenerateOpenings() {
    int boardSize = stoi(ConfigReader::get("boardSize"));
    string modelPath = ConfigReader::get("modelPath");
    string coreType = ConfigReader::get("coreType");

    int numTrain = stoi(ConfigReader::getOrDefault("genOpenings_trainCount", "300"));
    int numEval = stoi(ConfigReader::getOrDefault("genOpenings_evalCount", "50"));
    int minMoves = stoi(ConfigReader::getOrDefault("genOpenings_minMoves", "1"));
    int maxMoves = stoi(ConfigReader::getOrDefault("genOpenings_maxMoves", "4"));
    float threshold = stof(ConfigReader::getOrDefault("genOpenings_threshold", "0.5"));
    int maxAttempts = stoi(ConfigReader::getOrDefault("genOpenings_maxAttempts", "20000"));
    int nearCenter = stoi(ConfigReader::getOrDefault("genOpenings_nearCenter", "6"));

    int numOpenings = numTrain + numEval;

    // 加载模型
    Model model;
    model.init(modelPath, coreType);
    cout << "[Openings] Model loaded: " << modelPath << endl;

    auto startTime = chrono::steady_clock::now();

    int center = boardSize / 2;
    mt19937 rng(random_device{}());

    // 加权步数分布
    vector<double> moveWeights;
    for (int m = minMoves; m <= maxMoves; m++) {
        if (m == 1)      moveWeights.push_back(15);
        else if (m == 2) moveWeights.push_back(35);
        else if (m == 3) moveWeights.push_back(35);
        else if (m == 4) moveWeights.push_back(15);
        else             moveWeights.push_back(10);
    }
    discrete_distribution<int> moveDist(moveWeights.begin(), moveWeights.end());

    struct Opening {
        vector<Point> moves;
        float balanceScore;
    };
    vector<Opening> candidates;
    int attempts = 0;

    // 渐进阈值（单视角 |value|）
    float thresholds[] = {threshold * 0.2f, threshold * 0.3f, threshold * 0.4f, threshold * 0.6f};
    int numThresholds = 4;
    float maxThreshold = thresholds[numThresholds - 1];

    // 预分配 state buffer
    int planeSize = boardSize * boardSize;
    vector<float> stateBuf(INPUT_CHANNELS * planeSize, 0.0f);

    float strictThreshold = thresholds[0];
    int strictCount = 0;

    while (attempts < maxAttempts) {
        attempts++;
        int numMoves = moveDist(rng) + minMoves;

        // 用 policy 引导落子
        Game game(boardSize);
        vector<Point> absMoves;
        bool valid = true;

        for (int step = 0; step < numMoves; step++) {
            // 收集中心区域内的空位
            vector<Point> cands;
            for (int r = max(0, center - nearCenter); r < min(boardSize, center + nearCenter + 1); r++) {
                for (int c = max(0, center - nearCenter); c < min(boardSize, center + nearCenter + 1); c++) {
                    if (game.board[r][c] == 0) {
                        // 第 2 步起：必须在已有棋子 3 格范围内
                        if (step > 0) {
                            bool near = false;
                            for (auto& m : absMoves) {
                                if (abs(r - m.x) <= 3 && abs(c - m.y) <= 3) {
                                    near = true;
                                    break;
                                }
                            }
                            if (!near) continue;
                        }
                        cands.emplace_back(r, c);
                    }
                }
            }
            if (cands.empty()) { valid = false; break; }

            // 第一步随机
            if (step == 0) {
                uniform_int_distribution<int> pickDist(0, (int)cands.size() - 1);
                Point p = cands[pickDist(rng)];
                game.makeMove(p);
                absMoves.push_back(p);
                continue;
            }

            // 后续步：policy 采样
            game.getState(stateBuf.data(), INPUT_CHANNELS);
            auto [val, policy] = model.evaluate_state(stateBuf.data(), INPUT_CHANNELS, boardSize, boardSize);
            vector<float> probs;
            probs.reserve(cands.size());
            float temperature = 1.5f;
            float maxLogit = -1e9f;
            for (auto& p : cands) {
                float logit = policy[p.x * boardSize + p.y] / temperature;
                if (logit > maxLogit) maxLogit = logit;
                probs.push_back(logit);
            }
            float sumExp = 0;
            for (auto& p : probs) { p = exp(p - maxLogit); sumExp += p; }
            for (auto& p : probs) { p /= sumExp; }
            discrete_distribution<int> dist(probs.begin(), probs.end());
            Point chosen = cands[dist(rng)];
            game.makeMove(chosen);
            absMoves.push_back(chosen);
        }

        if (!valid || (int)absMoves.size() < minMoves) continue;

        // 当前行棋方视角评估（value 接近 0 = 均势）
        game.getState(stateBuf.data(), INPUT_CHANNELS);
        auto [v1, _] = model.evaluate_state(stateBuf.data(), INPUT_CHANNELS, boardSize, boardSize);

        float balanceScore = fabs(v1);
        if (balanceScore < maxThreshold) {
            Opening op;
            for (auto& p : absMoves) {
                op.moves.emplace_back(p.x - center, p.y - center);
            }
            op.balanceScore = balanceScore;
            candidates.push_back(op);
            if (balanceScore < strictThreshold) {
                strictCount++;
                if (strictCount >= numOpenings) break;  // 最严档已够，提前退出
            }
        }
    }

    // 按 balanceScore 排序，渐进选取
    sort(candidates.begin(), candidates.end(),
         [](const Opening& a, const Opening& b) { return a.balanceScore < b.balanceScore; });

    vector<Opening> balanced;
    for (int t = 0; t < numThresholds && (int)balanced.size() < numOpenings; t++) {
        float curThreshold = thresholds[t];
        for (auto& op : candidates) {
            if ((int)balanced.size() >= numOpenings) break;
            if (op.balanceScore < curThreshold) {
                bool dup = false;
                for (auto& sel : balanced) {
                    if (sel.moves == op.moves) { dup = true; break; }
                }
                if (!dup) balanced.push_back(op);
            }
        }
    }

    if (balanced.empty()) {
        cout << "[Openings] 生成失败：没有找到平衡开局" << endl;
        return;
    }

    shuffle(balanced.begin(), balanced.end(), rng);
    float totalTarget = (float)(numTrain + numEval);
    int evalCount = min(numEval, max(1, (int)(balanced.size() * numEval / totalTarget)));
    int trainCount = min(numTrain, (int)balanced.size() - evalCount);

    auto writeOpenings = [](const string& path, const vector<Opening>& openings) {
        string dir = path.substr(0, path.find_last_of('/'));
        #ifdef _WIN32
            system(("if not exist \"" + dir + "\" mkdir \"" + dir + "\"").c_str());
        #else
            system(("mkdir -p \"" + dir + "\"").c_str());
        #endif
        ofstream file(path);
        if (!file.is_open()) { cerr << "[Openings] 无法写入: " << path << endl; return; }
        for (const auto& op : openings) {
            for (int i = 0; i < (int)op.moves.size(); i++) {
                if (i > 0) file << ",";
                file << op.moves[i].x << "," << op.moves[i].y;
            }
            file << "\n";
        }
        file.close();
    };

    vector<Opening> trainOpenings(balanced.begin(), balanced.begin() + trainCount);
    vector<Opening> evalOpenings(balanced.begin() + trainCount, balanced.begin() + trainCount + evalCount);

    writeOpenings("openings/openings_train.txt", trainOpenings);
    writeOpenings("openings/openings_eval.txt", evalOpenings);

    auto endTime = chrono::steady_clock::now();
    int elapsedSec = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();

    float passRate = (float)candidates.size() / attempts * 100;
    string tierInfo;
    for (int t = 0; t < numThresholds; t++) {
        int count = 0;
        for (auto& op : candidates) {
            if (op.balanceScore < thresholds[t]) count++;
        }
        tierInfo += (t > 0 ? "/" : "") + to_string(count);
    }
    cout << "[Openings] 生成 " << trainCount << " 训练 + " << evalCount << " 评估开局"
         << "（尝试 " << attempts << " 次，候选 " << candidates.size() << " 个，通过率 " << passRate << "%），"
         << "阈值 " << thresholds[0] << "/" << thresholds[1] << "/" << thresholds[2] << "/" << thresholds[3]
         << "，各档 " << tierInfo
         << "，耗时 " << elapsedSec << "s" << endl;
}
