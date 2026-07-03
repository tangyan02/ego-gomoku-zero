#ifndef EGO_GOMOKU_ZERO_DFPN_VCF_H
#define EGO_GOMOKU_ZERO_DFPN_VCF_H

#include "Game.h"
#include <vector>
#include <cstdint>

// DF-PN (Depth-First Proof Number) based VCF solver
//
// VCF = Victory by Continuous Fours
// 进攻方只允许下冲四（sleepy four），防守方只堵五连点。
// 比 VCT 分支因子小得多，DFS 本身已高效，
// DFPN 化的优势：转置表去重 + proof number 自动剪枝。
//
// AND/OR 树：
//   - 进攻方（OR 节点）：任一子节点证明即证明 → pn = min(子pn), dn = sum(子dn)
//   - 防守方（AND 节点）：所有子节点证明才证明 → pn = sum(子pn), dn = min(子dn)

struct DfpnVCFEntry {
    uint32_t pn = 1;   // proof number
    uint32_t dn = 1;   // disproof number
    std::vector<Point> moves;  // 缓存的走法
    bool movesGenerated = false;
};

static const uint32_t DFPN_VCF_INF = 100000000;

// df-pn VCF 搜索
// 返回: (是否找到 VCF, 获胜走法序列)
// attackPoints: 如果非空，记录进攻方所有 VCF 走法（与 dfsVCF 的 allAttackPoints 参数对齐）
std::pair<bool, std::vector<Point>>
dfpnVCF(int attackPlayer, Game& game, int maxNodes = 2000000,
        std::vector<Point>* attackPoints = nullptr);

// 从转置表中提取完整 PV 线（搜索后调用）
std::vector<Point> dfpnVCFExtractPV(int attackPlayer, Game& game, int maxDepth = 100);

#endif // EGO_GOMOKU_ZERO_DFPN_VCF_H
