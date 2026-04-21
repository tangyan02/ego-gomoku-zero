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

using namespace std;

/**
 * 在 C++ 端生成平衡开局库。
 * 复用 Game::getState() 构造真实输入通道（含 VCF），用模型 value head 做平衡过滤。
 *
 * 配置项（application.conf）：
 *   genOpenings_trainCount   训练用开局数量，默认 300
 *   genOpenings_evalCount    评估用开局数量，默认 50
 *   genOpenings_minMoves     最少步数，默认 1
 *   genOpenings_maxMoves     最多步数，默认 8
 *   genOpenings_threshold    平衡阈值 |value|，默认 0.4
 *   genOpenings_maxAttempts  最大尝试次数，默认 15000
 *   genOpenings_nearCenter   落子距中心范围，默认 6
 */
void runGenerateOpenings() {
    int boardSize = stoi(ConfigReader::get("boardSize"));
    string modelPath = ConfigReader::get("modelPath");
    string coreType = ConfigReader::get("coreType");

    int numTrain = stoi(ConfigReader::getOrDefault("genOpenings_trainCount", "300"));
    int numEval = stoi(ConfigReader::getOrDefault("genOpenings_evalCount", "50"));
    int minMoves = stoi(ConfigReader::getOrDefault("genOpenings_minMoves", "1"));
    int maxMoves = stoi(ConfigReader::getOrDefault("genOpenings_maxMoves", "8"));
    float threshold = stof(ConfigReader::getOrDefault("genOpenings_threshold", "0.4"));
    int maxAttempts = stoi(ConfigReader::getOrDefault("genOpenings_maxAttempts", "15000"));
    int nearCenter = stoi(ConfigReader::getOrDefault("genOpenings_nearCenter", "6"));

    int numOpenings = numTrain + numEval;

    // 加载模型
    Model model;
    model.init(modelPath, coreType);
    cout << "[Openings] Model loaded: " << modelPath << endl;

    int center = boardSize / 2;
    mt19937 rng(random_device{}());
    uniform_int_distribution<int> moveDist(minMoves, maxMoves);

    // 收集平衡开局
    struct Opening {
        vector<Point> moves; // 相对中心坐标
    };
    vector<Opening> balanced;
    int attempts = 0;

    // 预分配 state buffer
    int planeSize = boardSize * boardSize;
    vector<float> stateBuf(INPUT_CHANNELS * planeSize, 0.0f);

    while ((int)balanced.size() < numOpenings && attempts < maxAttempts) {
        attempts++;
        int numMoves = moveDist(rng);

        // 在中心附近随机落子
        Game game(boardSize);
        vector<Point> absMoves; // 绝对坐标
        bool valid = true;

        for (int step = 0; step < numMoves; step++) {
            // 收集中心附近的空位
            vector<Point> candidates;
            for (int r = max(0, center - nearCenter); r < min(boardSize, center + nearCenter + 1); r++) {
                for (int c = max(0, center - nearCenter); c < min(boardSize, center + nearCenter + 1); c++) {
                    if (game.board[r][c] == 0) {
                        candidates.emplace_back(r, c);
                    }
                }
            }
            if (candidates.empty()) {
                valid = false;
                break;
            }
            uniform_int_distribution<int> pickDist(0, (int)candidates.size() - 1);
            Point p = candidates[pickDist(rng)];
            game.makeMove(p);
            absMoves.push_back(p);
        }

        if (!valid || (int)absMoves.size() < minMoves) continue;

        // 双视角评估：用 Game::getState() 构造真实输入（含 VCF 通道）
        // 视角 1：当前行棋方（game 已经是落子后的状态，currentPlayer 已翻转）
        // 需要构造两个视角的 Game，分别调用 getState
        int currentPlayer = game.currentPlayer; // 落子后轮到的一方
        int otherPlayer = game.getOtherPlayer();

        // 视角1：从 currentPlayer 看局面
        // getState() 内部用 currentPlayer 填 ch0/ch2，otherPlayer 填 ch1/ch3
        // 所以直接用当前 game 即可
        game.getState(stateBuf.data(), INPUT_CHANNELS);
        auto [v1, _] = model.evaluate_state(stateBuf.data(), INPUT_CHANNELS, boardSize, boardSize);

        // 视角2：从 otherPlayer 看局面（交换 currentPlayer）
        // 需要临时翻转 currentPlayer 来调用 getState
        game.currentPlayer = otherPlayer;
        // 清 VCF 缓存，因为 currentPlayer 变了
        game.myVcfDone = false;
        game.oppVcfDone = false;
        game.myVcfMoves.clear();
        game.oppVcfMoves.clear();
        game.myAllAttackMoves.clear();
        game.oppVcfAttackMoves.clear();
        game.oppVcfDefenceMoves.clear();
        game.getState(stateBuf.data(), INPUT_CHANNELS);
        auto [v2, __] = model.evaluate_state(stateBuf.data(), INPUT_CHANNELS, boardSize, boardSize);

        // 恢复（虽然 game 是局部变量马上就销毁，但保持一致）
        game.currentPlayer = currentPlayer;

        // 平衡判定：双视角绝对值之和 < 阈值*2
        float balanceScore = fabs(v1) + fabs(v2);
        if (balanceScore < threshold * 2) {
            Opening op;
            for (auto& p : absMoves) {
                op.moves.emplace_back(p.x - center, p.y - center);
            }
            balanced.push_back(op);
        }
    }

    if (balanced.empty()) {
        cout << "[Openings] 生成失败：没有找到平衡开局" << endl;
        return;
    }

    // 打乱后分割训练集/评估集
    shuffle(balanced.begin(), balanced.end(), rng);
    int trainCount = min(numTrain, (int)balanced.size());
    int evalCount = min(numEval, (int)balanced.size() - trainCount);

    auto writeOpenings = [](const string& path, const vector<Opening>& openings) {
        // 确保 openings 目录存在
        string dir = path.substr(0, path.find_last_of('/'));
        // 简单创建目录（如果不存在）
        #ifdef _WIN32
            system(("if not exist \"" + dir + "\" mkdir \"" + dir + "\"").c_str());
        #else
            system(("mkdir -p \"" + dir + "\"").c_str());
        #endif

        ofstream file(path);
        if (!file.is_open()) {
            cerr << "[Openings] 无法写入: " << path << endl;
            return;
        }
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

    float passRate = (float)balanced.size() / attempts * 100;
    cout << "[Openings] 生成 " << trainCount << " 训练 + " << evalCount << " 评估开局"
         << "（尝试 " << attempts << " 次，通过率 " << passRate << "%），"
         << "阈值 |value|<" << threshold << endl;
}
