#include "DfpnVCF.h"
#include "Analyzer.h"
#include <algorithm>
#include <unordered_map>

static bool vcfExistPoints(const std::vector<Point>& moves, const Point& target) {
    for (const auto& item : moves) {
        if (item.x == target.x && item.y == target.y) return true;
    }
    return false;
}

// DF-PN VCF 搜索实现
//
// VCF 规则：
//   进攻方：只能下冲四（sleepy four），活四直接判定胜利
//   防守方：只能堵五连点
// 
// 与 dfsVCF 的走法生成逻辑完全对齐。
// DFPN 框架参照 DfpnVCT.cpp，但简化了很多（无 threeCount、无 fourMode）。

// 线程局部存储：每个线程独立的搜索状态
static thread_local int vcfNodeCount = 0;
static thread_local int vcfMaxNodesLimit = 2000000;

// -------- 转置表（线程局部）--------
static thread_local std::unordered_map<uint64_t, DfpnVCFEntry> vcfTable;

// -------- 走法生成 --------

// 返回: true = 即时证明/证伪（entry 的 pn/dn 已更新），false = 正常走法已生成
static constexpr int debugVCF = 0;  // 0=off, 1=on (set to 1 for debugging)

static bool generateVCFMoves(int attacker, int currentPlayer, Game& game,
                             DfpnVCFEntry& entry, Point lastMove, Point lastLastMove) {
    if (entry.movesGenerated) return false;
    entry.movesGenerated = true;
    
    bool isAttacker = (currentPlayer == attacker);
    int defender = 3 - attacker;
    
    // 搜索范围：与 dfsVCF 对齐
    std::vector<Point> nearMoves;
    if (lastLastMove.isNull()) {
        nearMoves = game.getEmptyPoints();
    } else {
        if (isAttacker) {
            nearMoves = getNearByEmptyPoints(lastLastMove, game);  // 默认 range=5
        } else {
            nearMoves = getNearByEmptyPoints(lastMove, game);
        }
    }
    
    if (isAttacker) {
        // ---- 进攻方 ----
        // 对方附近点（用于检测对方五连）
        std::vector<Point> oppNearMoves = getNearByEmptyPoints(lastMove, game);
        if (oppNearMoves.empty()) {
            oppNearMoves = game.getEmptyPoints();
        }
        
        // 我方五连 → 直接胜利
        auto winMoves = getWinningMoves(attacker, game, nearMoves);
        if (!winMoves.empty()) {
            if (debugVCF) std::cout << "  [genVCF] ATK: my WIN! pn=0" << std::endl;
            entry.pn = 0; entry.dn = DFPN_VCF_INF;
            return true;
        }
        
        // 对方有 2+ 五连点 → 失败
        auto oppWinMoves = getWinningMoves(defender, game, oppNearMoves);
        if (oppWinMoves.size() > 1) {
            if (debugVCF) std::cout << "  [genVCF] ATK: opp 2+ wins, pn=INF" << std::endl;
            entry.pn = DFPN_VCF_INF; entry.dn = 0;
            return true;
        }
        
        // 我方活四 → 直接胜利
        auto activeMoves = getActiveFourMoves(attacker, game, nearMoves);
        if (!activeMoves.empty()) {
            if (debugVCF) std::cout << "  [genVCF] ATK: my ACTIVE_FOUR! pn=0" << std::endl;
            entry.pn = 0; entry.dn = DFPN_VCF_INF;
            return true;
        }
        
        // 我方冲四
        auto sleepMoves = getSleepyFourMoves(attacker, game, nearMoves);
        
        if (debugVCF) {
            std::cout << "  [genVCF] ATK: sleepFours=" << sleepMoves.size()
                      << " oppWins=" << oppWinMoves.size()
                      << " lastMove=(" << lastMove.x << "," << lastMove.y << ")"
                      << " lastLast=(" << lastLastMove.x << "," << lastLastMove.y << ")" << std::endl;
        }
        
        std::vector<Point> moves;
        
        if (oppWinMoves.size() == 1) {
            // 对方有一个五连点，必须下这里（且必须同时是我方冲四才有意义）
            auto oppWinMove = oppWinMoves[0];
            if (vcfExistPoints(sleepMoves, oppWinMove)) {
                moves.push_back(oppWinMove);
            }
            // 如果不是冲四点 → 被迫堵但不是进攻 → VCF 失败
        } else {
            // 正常进攻：只走冲四
            moves = std::move(sleepMoves);
        }
        
        if (moves.empty()) {
            if (debugVCF) std::cout << "  [genVCF] ATK: no moves, pn=INF" << std::endl;
            entry.pn = DFPN_VCF_INF; entry.dn = 0;
            return true;
        }
        
        entry.moves = removeDuplicates(moves);
        
    } else {
        // ---- 防守方 ----
        // 检查进攻方有几个五连点
        auto oppWinMoves = getWinningMoves(attacker, game, nearMoves);
        
        if (debugVCF) {
            std::cout << "  [genVCF] DEF: oppWins=" << oppWinMoves.size()
                      << " lastMove=(" << lastMove.x << "," << lastMove.y << ")"
                      << " nearMoves=" << nearMoves.size() << std::endl;
        }
        
        if (oppWinMoves.empty()) {
            // 进攻方没有五连威胁 → 防守成功
            if (debugVCF) std::cout << "  [genVCF] DEF: no threat, pn=INF (defense ok)" << std::endl;
            entry.pn = DFPN_VCF_INF; entry.dn = 0;
            return true;
        }
        
        if (oppWinMoves.size() == 1) {
            // 一个五连点，必须堵
            if (debugVCF) std::cout << "  [genVCF] DEF: block (" << oppWinMoves[0].x << "," << oppWinMoves[0].y << ")" << std::endl;
            entry.moves.push_back(oppWinMoves[0]);
        } else {
            // 2+ 五连点 → 堵不住，进攻方胜利
            if (debugVCF) std::cout << "  [genVCF] DEF: 2+ wins, pn=0 (attacker wins)" << std::endl;
            entry.pn = 0; entry.dn = DFPN_VCF_INF;
            return true;
        }
    }
    
    // 初始化 pn/dn
    entry.pn = 1;
    entry.dn = 1;
    return false;
}

// -------- df-pn MID 递归 --------

static void vcfMid(int attacker, int currentPlayer, Game& game,
                   uint32_t pnThreshold, uint32_t dnThreshold,
                   Point lastMove, Point lastLastMove, int depth) {
    if (vcfNodeCount >= vcfMaxNodesLimit) return;
    vcfNodeCount++;
    
    uint64_t hash = game.zobristHash;
    auto& entry = vcfTable[hash];
    
    // 阈值检查
    if (entry.pn >= pnThreshold || entry.dn >= dnThreshold) return;
    
    // 深度限制（VCF 理论上不需要太深，但留个上限防止极端情况）
    if (depth > 200) {
        entry.pn = DFPN_VCF_INF;
        entry.dn = 0;
        return;
    }
    
    // 生成走法（缓存）
    if (generateVCFMoves(attacker, currentPlayer, game, entry, lastMove, lastLastMove)) {
        return;  // 即时证明/证伪
    }
    
    // 阈值再检查
    if (entry.pn >= pnThreshold || entry.dn >= dnThreshold) return;
    
    bool isAttacker = (currentPlayer == attacker);
    const auto& moves = entry.moves;
    
    // ---- df-pn 主循环 ----
    while (true) {
        if (vcfNodeCount >= vcfMaxNodesLimit) return;
        
        // 收集子节点 pn/dn
        struct ChildInfo { int idx; uint32_t pn; uint32_t dn; };
        std::vector<ChildInfo> children;
        children.reserve(moves.size());
        
        for (int i = 0; i < (int)moves.size(); i++) {
            auto& m = moves[i];
            uint64_t childHash = hash
                ^ zobristTable.pieces[m.x][m.y][0]
                ^ zobristTable.pieces[m.x][m.y][currentPlayer]
                ^ zobristTable.currentPlayerHash;
            
            auto cit = vcfTable.find(childHash);
            uint32_t cpn = 1, cdn = 1;
            if (cit != vcfTable.end()) {
                cpn = cit->second.pn;
                cdn = cit->second.dn;
            }
            children.push_back({i, cpn, cdn});
        }
        
        // 计算当前节点 pn/dn
        uint32_t sumPn = 0, sumDn = 0;
        uint32_t minPn = DFPN_VCF_INF, minDn = DFPN_VCF_INF;
        
        for (auto& c : children) {
            minPn = std::min(minPn, c.pn);
            minDn = std::min(minDn, c.dn);
            sumPn = (sumPn > DFPN_VCF_INF - c.pn) ? DFPN_VCF_INF : sumPn + c.pn;
            sumDn = (sumDn > DFPN_VCF_INF - c.dn) ? DFPN_VCF_INF : sumDn + c.dn;
        }
        
        if (isAttacker) {
            // OR 节点
            entry.pn = minPn;
            entry.dn = sumDn;
        } else {
            // AND 节点
            entry.pn = sumPn;
            entry.dn = minDn;
        }
        
        // 阈值检查
        if (entry.pn >= pnThreshold || entry.dn >= dnThreshold) return;
        if (entry.pn == 0 || entry.dn == 0) return;
        
        // 选择 MPN（Most Proving Node）
        int bestIdx = -1;
        if (isAttacker) {
            uint32_t bestPn = DFPN_VCF_INF;
            for (auto& c : children) {
                if (c.pn < bestPn) { bestPn = c.pn; bestIdx = c.idx; }
            }
        } else {
            uint32_t bestDn = DFPN_VCF_INF;
            for (auto& c : children) {
                if (c.dn < bestDn) { bestDn = c.dn; bestIdx = c.idx; }
            }
        }
        
        if (bestIdx < 0) return;
        
        // 找到 bestChild
        ChildInfo bestChild = {0, 1, 1};
        for (auto& c : children) {
            if (c.idx == bestIdx) { bestChild = c; break; }
        }
        
        // 计算子节点阈值
        uint32_t childPnTh, childDnTh;
        if (isAttacker) {
            uint32_t secondMinPn = DFPN_VCF_INF;
            for (auto& c : children) {
                if (c.idx != bestIdx && c.pn < secondMinPn) secondMinPn = c.pn;
            }
            childPnTh = std::min(pnThreshold, (secondMinPn < DFPN_VCF_INF - 1) ? secondMinPn + 1 : DFPN_VCF_INF);
            childDnTh = (dnThreshold >= DFPN_VCF_INF || sumDn >= DFPN_VCF_INF) ? DFPN_VCF_INF :
                         dnThreshold - sumDn + bestChild.dn;
        } else {
            uint32_t secondMinDn = DFPN_VCF_INF;
            for (auto& c : children) {
                if (c.idx != bestIdx && c.dn < secondMinDn) secondMinDn = c.dn;
            }
            childPnTh = (pnThreshold >= DFPN_VCF_INF || sumPn >= DFPN_VCF_INF) ? DFPN_VCF_INF :
                          pnThreshold - sumPn + bestChild.pn;
            childDnTh = std::min(dnThreshold, (secondMinDn < DFPN_VCF_INF - 1) ? secondMinDn + 1 : DFPN_VCF_INF);
        }
        
        // 递归
        Point bestMove = moves[bestIdx];
        game.board[bestMove.x][bestMove.y] = currentPlayer;
        game.zobristHash ^= zobristTable.pieces[bestMove.x][bestMove.y][0]
                         ^  zobristTable.pieces[bestMove.x][bestMove.y][currentPlayer]
                         ^  zobristTable.currentPlayerHash;
        
        vcfMid(attacker, 3 - currentPlayer, game,
               childPnTh, childDnTh,
               bestMove, lastMove, depth + 1);
        
        game.zobristHash ^= zobristTable.pieces[bestMove.x][bestMove.y][0]
                         ^  zobristTable.pieces[bestMove.x][bestMove.y][currentPlayer]
                         ^  zobristTable.currentPlayerHash;
        game.board[bestMove.x][bestMove.y] = 0;
    }
}

// -------- 公开接口 --------

std::pair<bool, std::vector<Point>>
dfpnVCF(int attackPlayer, Game& game, int maxNodes,
        std::vector<Point>* attackPoints) {
    vcfTable.clear();
    vcfTable.reserve(std::min(maxNodes, 1000000));
    vcfNodeCount = 0;
    vcfMaxNodesLimit = maxNodes;
    
    // 少子局面快速返回
    int pieceCount = 0;
    for (int r = 0; r < game.boardSize; r++)
        for (int c = 0; c < game.boardSize; c++)
            if (game.board[r][c] != 0) pieceCount++;
    if (pieceCount < 6) {
        return {false, {}};
    }
    
    // Hash 校正：VCF 搜索中 currentPlayer 的含义是"搜索中谁在走"（始终从 attackPlayer 开始），
    // 但 game.zobristHash 编码的是 game.currentPlayer。当两者不同时需要翻转。
    bool hashFlipped = (attackPlayer != game.currentPlayer);
    if (hashFlipped) {
        game.zobristHash ^= zobristTable.currentPlayerHash;
    }
    
    // MID 搜索
    vcfMid(attackPlayer, attackPlayer, game,
           DFPN_VCF_INF, DFPN_VCF_INF,
           Point(), Point(), 0);
    
    uint64_t hash = game.zobristHash;
    auto it = vcfTable.find(hash);
    if (it != vcfTable.end() && it->second.pn == 0) {
        // 找到 VCF，提取首步和 PV
        std::vector<Point> firstMoves;
        
        // 从 PV 中提取进攻方走法
        auto pv = dfpnVCFExtractPV(attackPlayer, game);
        
        if (!pv.empty()) {
            firstMoves.push_back(pv[0]);
            
            // 记录所有进攻方走法到 attackPoints
            if (attackPoints != nullptr) {
                for (int i = 0; i < (int)pv.size(); i += 2) {
                    attackPoints->push_back(pv[i]);
                }
                *attackPoints = removeDuplicates(*attackPoints);
            }
        }
        
        // 恢复 hash
        if (hashFlipped) game.zobristHash ^= zobristTable.currentPlayerHash;
        return {true, firstMoves};
    }
    
    // 恢复 hash
    if (hashFlipped) game.zobristHash ^= zobristTable.currentPlayerHash;
    return {false, {}};
}

// -------- PV 提取 --------

std::vector<Point> dfpnVCFExtractPV(int attackPlayer, Game& game, int maxDepth) {
    std::vector<Point> pv;
    int currentPlayer = attackPlayer;
    
    // Hash 校正：检查当前 hash 是否在表中，如果不在则翻转
    // （当从 dfpnVCF 内部调用时 hash 已翻转，无需再翻；从外部调用时需要翻转）
    bool hashFlipped = false;
    if (vcfTable.find(game.zobristHash) == vcfTable.end()) {
        // 当前 hash 不在表中，尝试翻转
        game.zobristHash ^= zobristTable.currentPlayerHash;
        if (vcfTable.find(game.zobristHash) != vcfTable.end()) {
            hashFlipped = true;
        } else {
            // 翻转后还是找不到，恢复并返回空
            game.zobristHash ^= zobristTable.currentPlayerHash;
            return pv;
        }
    }
    
    for (int depth = 0; depth < maxDepth; depth++) {
        uint64_t hash = game.zobristHash;
        auto it = vcfTable.find(hash);
        if (it == vcfTable.end()) {
            if (debugVCF) std::cout << "  [PV] depth=" << depth << " hash not found" << std::endl;
            break;
        }
        
        auto& entry = it->second;
        if (debugVCF) std::cout << "  [PV] depth=" << depth << " pn=" << entry.pn << " dn=" << entry.dn << " moves=" << entry.moves.size() << std::endl;
        
        if (entry.moves.empty()) break;
        
        bool isAttacker = (currentPlayer == attackPlayer);
        
        // 找 pn=0 的子节点
        Point bestMove;
        bool found = false;
        
        for (auto& m : entry.moves) {
            uint64_t childHash = hash
                ^ zobristTable.pieces[m.x][m.y][0]
                ^ zobristTable.pieces[m.x][m.y][currentPlayer]
                ^ zobristTable.currentPlayerHash;
            auto cit = vcfTable.find(childHash);
            if (debugVCF) {
                std::cout << "    [PV] child (" << m.x << "," << m.y << ") ";
                if (cit != vcfTable.end()) std::cout << "pn=" << cit->second.pn << " dn=" << cit->second.dn;
                else std::cout << "NOT_IN_TABLE";
                std::cout << std::endl;
            }
            if (cit != vcfTable.end() && cit->second.pn == 0) {
                bestMove = m;
                found = true;
                break;
            }
        }
        
        // 即时胜利（generateMoves 直接判定 pn=0，子节点没有 entry）
        if (!found && entry.pn == 0 && !entry.moves.empty()) {
            if (debugVCF) std::cout << "    [PV] using first move as immediate win" << std::endl;
            bestMove = entry.moves[0];
            found = true;
        }
        
        if (!found) break;
        
        pv.push_back(bestMove);
        game.zobristHash ^= zobristTable.pieces[bestMove.x][bestMove.y][0]
                         ^  zobristTable.pieces[bestMove.x][bestMove.y][currentPlayer]
                         ^  zobristTable.currentPlayerHash;
        game.board[bestMove.x][bestMove.y] = currentPlayer;
        currentPlayer = 3 - currentPlayer;
    }
    
    // 恢复棋盘和 hash
    for (int i = (int)pv.size() - 1; i >= 0; i--) {
        int piecePlayer = ((i % 2 == 0) ? attackPlayer : 3 - attackPlayer);
        game.zobristHash ^= zobristTable.pieces[pv[i].x][pv[i].y][0]
                         ^  zobristTable.pieces[pv[i].x][pv[i].y][piecePlayer]
                         ^  zobristTable.currentPlayerHash;
        game.board[pv[i].x][pv[i].y] = 0;
    }
    
    // 恢复 hash 翻转
    if (hashFlipped) {
        game.zobristHash ^= zobristTable.currentPlayerHash;
    }
    
    return pv;
}
