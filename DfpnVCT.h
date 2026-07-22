#ifndef EGO_GOMOKU_ZERO_DFPN_VCT_H
#define EGO_GOMOKU_ZERO_DFPN_VCT_H

#include "Game.h"
#include <atomic>
#include <climits>
#include <unordered_map>
#include <vector>
#include <cstdint>

// DF-PN (Depth-First Proof Number) based VCT solver
// Reference: Nagai 2002, "Df-pn Algorithm for Searching AND/OR Trees"
//
// 核心思想：
//   - 进攻方（OR 节点）：任一子节点证明即证明 → pn = min(子pn), dn = sum(子dn)
//   - 防守方（AND 节点）：所有子节点证明才证明 → pn = sum(子pn), dn = min(子dn)
//   - 用 (pn_threshold, dn_threshold) 控制搜索深度，避免无谓探索

struct DfpnEntry {
    uint32_t pn = 1;   // proof number
    uint32_t dn = 1;   // disproof number
    std::vector<Point> moves;  // 缓存的走法（避免重复生成）
    bool movesGenerated = false;
};

static const uint32_t DFPN_INF = 100000000;

// VCT 搜索结果
struct VCTResult {
    bool found;          // 是否找到 VCT
    bool exhaustive;     // 搜索是否完整（true=结论可靠，false=资源不足提前退出）
    std::vector<Point> moves;  // VCT 首步
};

// df-pn VCT 搜索
VCTResult dfpnVCT(int attackPlayer, Game& game, std::atomic<bool>& running, int maxNodes = 2000000, int maxDepth = 40, int timeLimitMs = INT_MAX);

// df-pn VCT 迭代加深版（按活三数逐层放宽 + 时间控制）
// threeLimits: 1→4→7→10→13→16（从1开始每次+3），逐步允许更多活三
// timeLimitMs: 最大搜索时间（毫秒），超时即返回当前结果
VCTResult dfpnVCTIterDeepen(int attackPlayer, Game& game, std::atomic<bool>& running, int maxNodes = 200000, int timeLimitMs = 2000);

// 设置/恢复 VCT 搜索的活三次数上限（用于 bench 测试单独的 threeLimit）
void dfpnVCTSetThreeLimit(int limit);
void dfpnVCTResetThreeLimit();

// 从转置表中提取完整 PV 线（搜索后调用）
// 返回进攻方和防守方交替的走法序列
std::vector<Point> dfpnExtractPV(int attackPlayer, Game& game, int maxDepth = 100);

// 获取上次搜索使用的节点数
int dfpnVCTGetNodeCount();

#endif // EGO_GOMOKU_ZERO_DFPN_VCT_H
