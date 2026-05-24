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

    // 步数均匀抽样：minMoves..maxMoves 等概率
    int numBuckets = maxMoves - minMoves + 1;
    vector<double> moveWeights(numBuckets, 1.0);
    discrete_distribution<int> moveDist(moveWeights.begin(), moveWeights.end());

    struct Opening {
        vector<Point> moves;
        float balanceScore;
        int numMoves;
    };
    vector<Opening> candidates;
    int attempts = 0;

    // 渐进阈值（单视角 |value|）：全部要求 < 0.1，前 3 档用于 log 细分
    float thresholds[] = {threshold * 0.05f, threshold * 0.1f, threshold * 0.15f, threshold * 0.2f};
    int numThresholds = 4;
    float maxThreshold = thresholds[numThresholds - 1];

    // 预分配 state buffer
    int planeSize = boardSize * boardSize;
    vector<float> stateBuf(INPUT_CHANNELS * planeSize, 0.0f);

    // 步数桶配额：每个步数最多收集 bucketCap 个候选（最宽阈值下）
    int bucketCap = (numOpenings + numBuckets - 1) / numBuckets;  // 向上取整
    vector<int> bucketCount(numBuckets, 0);
    vector<int> bucketAttempts(numBuckets, 0);  // 每个桶被尝试的次数（含已满后跳过的）

    while (attempts < maxAttempts) {
        attempts++;
        int numMoves = moveDist(rng) + minMoves;
        int bucketIdx = numMoves - minMoves;
        bucketAttempts[bucketIdx]++;

        // 桶配额：该步数已达上限则跳过本次尝试（不浪费推理）
        if (bucketCount[bucketIdx] >= bucketCap) continue;

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
            op.numMoves = numMoves;
            candidates.push_back(op);
            bucketCount[bucketIdx]++;

            // 提前退出：所有桶都达到 bucketCap 即结束
            bool allBucketsFull = true;
            for (int b = 0; b < numBuckets; b++) {
                if (bucketCount[b] < bucketCap) { allBucketsFull = false; break; }
            }
            if (allBucketsFull) break;
        }
    }

    // 按桶分组，桶内按 balanceScore 排序
    vector<vector<Opening>> bucketCands(numBuckets);
    for (auto& op : candidates) {
        int idx = op.numMoves - minMoves;
        if (idx >= 0 && idx < numBuckets) bucketCands[idx].push_back(op);
    }
    for (auto& bc : bucketCands) {
        sort(bc.begin(), bc.end(),
             [](const Opening& a, const Opening& b) { return a.balanceScore < b.balanceScore; });
    }

    // 每桶按 bucketCap 选取（已经按 score 排序，直接拿前 bucketCap 个）
    int perBucketTarget = bucketCap;
    vector<Opening> balanced;
    for (int b = 0; b < numBuckets; b++) {
        int take = min((int)bucketCands[b].size(), perBucketTarget);
        for (int i = 0; i < take; i++) balanced.push_back(bucketCands[b][i]);
    }

    // 不足补齐：若某桶凑不够，按总目标用其他桶的次优样本补
    if ((int)balanced.size() < numOpenings) {
        // 候选池里剩余的（每桶 perBucketTarget 之后的）
        vector<Opening> spare;
        for (int b = 0; b < numBuckets; b++) {
            for (int i = perBucketTarget; i < (int)bucketCands[b].size(); i++) {
                spare.push_back(bucketCands[b][i]);
            }
        }
        sort(spare.begin(), spare.end(),
             [](const Opening& a, const Opening& b) { return a.balanceScore < b.balanceScore; });
        int need = numOpenings - (int)balanced.size();
        for (int i = 0; i < min(need, (int)spare.size()); i++) {
            balanced.push_back(spare[i]);
        }
    }

    if (balanced.empty()) {
        cout << "[Openings] 生成失败：没有找到平衡开局" << endl;
        return;
    }

    // train/eval 切分：每个桶内按比例切，保证两集合都步数均匀
    vector<vector<Opening>> finalBuckets(numBuckets);
    for (auto& op : balanced) {
        int idx = op.numMoves - minMoves;
        if (idx >= 0 && idx < numBuckets) finalBuckets[idx].push_back(op);
    }
    // 每桶按 numTrain : numEval 比例切
    vector<Opening> trainOpenings, evalOpenings;
    float totalTarget = (float)(numTrain + numEval);
    for (int b = 0; b < numBuckets; b++) {
        auto& bc = finalBuckets[b];
        // 桶内打散（balanceScore 顺序无所谓了，结果两集合都要打散）
        shuffle(bc.begin(), bc.end(), rng);
        int evalCnt = (int)((float)bc.size() * numEval / totalTarget + 0.5f);
        evalCnt = min(evalCnt, (int)bc.size());
        int trainCnt = (int)bc.size() - evalCnt;
        for (int i = 0; i < trainCnt; i++) trainOpenings.push_back(bc[i]);
        for (int i = trainCnt; i < (int)bc.size(); i++) evalOpenings.push_back(bc[i]);
    }
    // 全集再打散一次（让不同桶混合）
    shuffle(trainOpenings.begin(), trainOpenings.end(), rng);
    shuffle(evalOpenings.begin(), evalOpenings.end(), rng);
    int trainCount = (int)trainOpenings.size();
    int evalCount = (int)evalOpenings.size();

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

    // 步数分布
    string stepInfo;
    for (int b = 0; b < numBuckets; b++) {
        int trainCnt = 0, evalCnt = 0;
        for (auto& op : trainOpenings) if (op.numMoves == minMoves + b) trainCnt++;
        for (auto& op : evalOpenings) if (op.numMoves == minMoves + b) evalCnt++;
        stepInfo += (b > 0 ? " " : "") + to_string(minMoves + b) + "步:" +
                    to_string(trainCnt) + "/" + to_string(evalCnt);
    }
    cout << "[Openings] 步数分布（train/eval）：" << stepInfo << endl;

    // 按步数桶细分：尝试次数（含跳过的）+ 通过候选数 + 各 value 档分布
    cout << "[Openings] 按步数桶分档（尝试 / 候选 / 通过率 / <t1<t2<t3<t4）：" << endl;
    for (int b = 0; b < numBuckets; b++) {
        int totalInBucket = 0;
        vector<int> tierCounts(numThresholds, 0);
        for (auto& op : candidates) {
            if (op.numMoves != minMoves + b) continue;
            totalInBucket++;
            for (int t = 0; t < numThresholds; t++) {
                if (op.balanceScore < thresholds[t]) tierCounts[t]++;
            }
        }
        float bucketPassRate = bucketAttempts[b] > 0
                               ? (float)totalInBucket / bucketAttempts[b] * 100
                               : 0.0f;
        cout << "[Openings]   " << (minMoves + b) << "步：尝试=" << bucketAttempts[b]
             << " 候选=" << totalInBucket
             << " 通过率=" << bucketPassRate << "%";
        for (int t = 0; t < numThresholds; t++) {
            cout << (t == 0 ? " 各档=" : "/") << tierCounts[t];
        }
        cout << endl;
    }
}
