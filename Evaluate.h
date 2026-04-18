#ifndef EGO_GOMOKU_ZERO_EVALUATE_H
#define EGO_GOMOKU_ZERO_EVALUATE_H

#include <string>

struct EvalResult {
    int wins1;       // 模型1胜场
    int wins2;       // 模型2胜场
    int draws;       // 平局
    int totalGames;
    double winRate1;  // 模型1胜率
    double eloDiff;   // 模型1相对模型2的Elo差
};

/**
 * 让两个模型对弈，评估相对棋力
 * @param modelPath1 挑战者模型路径
 * @param modelPath2 基准模型路径
 * @param boardSize 棋盘大小
 * @param numGames 总对弈局数（自动分为各执黑白各一半）；-1 表示使用全部开局（每个开局先后手各一局）
 * @param numSimulation 每步MCTS模拟次数
 * @param explorationFactor PUCT探索系数
 * @param coreType 推理后端
 * @return 评估结果
 */
EvalResult evaluateModels(
    const std::string& modelPath1,
    const std::string& modelPath2,
    int boardSize,
    int numGames,
    int numSimulation,
    float explorationFactor,
    const std::string& coreType
);

void runEvaluate();

#endif //EGO_GOMOKU_ZERO_EVALUATE_H
