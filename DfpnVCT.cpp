#include "DfpnVCT.h"
#include "Analyzer.h"
#include "Shape.h"
#include <algorithm>
#include <cstring>

// DF-PN VCT 搜索实现
//
// 走法生成与 dfsVCT 保持一致，但搜索框架用 df-pn 替代迭代加深 DFS。
// 关键优化：走法生成结果缓存到转置表，避免 df-pn 循环中重复计算。

// 线程局部存储：每个线程独立的搜索状态，避免多线程竞争
static thread_local int nodeCount = 0;
static thread_local int maxNodesLimit = 2000000;

// -------- 转置表（线程局部）--------
static thread_local std::unordered_map<uint64_t, DfpnEntry> ttable;

// -------- 走法生成（与 dfsVCT 一致）--------

// 生成当前局面的走法并缓存
// 返回: true = 即时证明/证伪（entry 的 pn/dn 已更新），false = 正常走法已生成
static bool generateMoves(int attacker, int currentPlayer, Game& game,
                          DfpnEntry& entry, Point lastMove, Point lastLastMove,
                          int threeCount) {
    if (entry.movesGenerated) return false;
    entry.movesGenerated = true;
    
    bool isAttacker = (currentPlayer == attacker);
    int defender = 3 - attacker;
    int checkPlayer = attacker;  // VCT 检查方始终是进攻方
    bool fourMode = (threeCount >= 9);
    
    // 搜索范围：与 dfsVCT 一致
    // dfsVCT 用 lastLastMove（上上步）和 attackPoint 来确定搜索范围
    // 根节点和第一步时 lastLastMove 为空，用全部空点
    std::vector<Point> nearMoves4;
    std::vector<Point> nearMoves3;
    
    if (lastLastMove.isNull()) {
        nearMoves4 = game.getEmptyPoints();
        nearMoves3 = game.getEmptyPoints();
    } else {
        nearMoves4 = getNearByEmptyPoints(lastLastMove, game, 4);
        nearMoves3 = getNearByEmptyPoints(lastLastMove, game, 3);
        // 补充 lastMove 附近的点
        if (!lastMove.isNull()) {
            auto extra4 = getNearByEmptyPoints(lastMove, game, 4);
            auto extra3 = getNearByEmptyPoints(lastMove, game, 3);
            nearMoves4.insert(nearMoves4.end(), extra4.begin(), extra4.end());
            nearMoves3.insert(nearMoves3.end(), extra3.begin(), extra3.end());
        }
        nearMoves4 = removeDuplicates(nearMoves4);
        nearMoves3 = removeDuplicates(nearMoves3);
    }
    
    if (isAttacker) {
        // ---- 进攻方 ----
        // 我方五连
        auto winMoves = getWinningMoves(attacker, game, nearMoves4);
        if (!winMoves.empty()) {
            entry.pn = 0; entry.dn = DFPN_INF;
            return true;
        }
        
        // 对方五连检查
        auto oppWinMoves = getWinningMoves(defender, game, nearMoves4);
        if (oppWinMoves.size() > 1) {
            entry.pn = DFPN_INF; entry.dn = 0;
            return true;
        }
        
        // 我方活四
        auto activeFourMoves = getActiveFourMoves(attacker, game, nearMoves4);
        if (!activeFourMoves.empty()) {
            entry.pn = 0; entry.dn = DFPN_INF;
            return true;
        }
        
        std::vector<Point> moves;
        
        if (oppWinMoves.size() == 1) {
            // 对方有一个五连点，必须下这里
            moves.push_back(oppWinMoves[0]);
        } else {
            // 正常进攻：冲四 + 活三
            if (!fourMode) {
                auto activeThree = getActiveThreeMoves(attacker, game, nearMoves3);
                moves.insert(moves.end(), activeThree.begin(), activeThree.end());
            }
            auto sleepFour = getSleepyFourMoves(attacker, game, nearMoves4);
            moves.insert(moves.end(), sleepFour.begin(), sleepFour.end());
        }
        
        moves = removeDuplicates(moves);
        
        if (moves.empty()) {
            entry.pn = DFPN_INF; entry.dn = 0;
            return true;
        }
        
        entry.moves = std::move(moves);
        
    } else {
        // ---- 防守方 ----
        auto oppWinMoves = getWinningMoves(checkPlayer, game, nearMoves4);
        
        // 进攻方有 2+ 个五连点 → 无法防守
        if (oppWinMoves.size() > 1) {
            entry.pn = 0; entry.dn = DFPN_INF;
            return true;
        }
        
        std::vector<Point> moves;
        
        if (oppWinMoves.size() == 1) {
            // 进攻方有一个五连点，必须堵
            moves.push_back(oppWinMoves[0]);
        } else {
            // 进攻方没有五连点
            auto allMoves = game.getEmptyPoints();
            
            // 防守方有活四 → 防守成功
            auto myActiveFour = getActiveFourMoves(currentPlayer, game, allMoves);
            if (!myActiveFour.empty()) {
                entry.pn = DFPN_INF; entry.dn = 0;
                return true;
            }
            
            // 防守方有 VCF → 防守成功
            auto myVCF = dfsVCF(currentPlayer, currentPlayer, game, Point(), Point());
            if (myVCF.first) {
                entry.pn = DFPN_INF; entry.dn = 0;
                return true;
            }
            
            // 防活三
            if (!fourMode) {
                auto threeDefence = getThreeDefenceMoves(currentPlayer, game, nearMoves4);
                moves.insert(moves.end(), threeDefence.begin(), threeDefence.end());
            }
        }
        
        moves = removeDuplicates(moves);
        
        if (moves.empty()) {
            // 防守方无走法（进攻方没有需要防的威胁或防不住）
            // 需要判断：进攻方是否真的有威胁？
            // 如果走到这里说明 oppWinMoves 为空且 fourMode=true（没有活三要防）
            // 在 fourMode 下，进攻方只用冲四，防守方堵五连即可
            // oppWinMoves 为空说明进攻方冲四没形成五连 → 可能是进攻方这步无威胁
            entry.pn = DFPN_INF; entry.dn = 0;
            return true;
        }
        
        entry.moves = std::move(moves);
    }
    
    // 初始化 pn/dn
    entry.pn = 1;
    entry.dn = 1;
    return false;
}

// -------- df-pn MID 递归 --------

static void mid(int attacker, int currentPlayer, Game& game, std::atomic<bool>& running,
                uint32_t pnThreshold, uint32_t dnThreshold,
                Point lastMove, Point lastLastMove,
                int threeCount, int depth, int maxDepth) {
    if (!running.load()) return;
    if (nodeCount >= maxNodesLimit) { running.store(false); return; }
    nodeCount++;
    
    uint64_t hash = game.zobristHash;
    
    // 查询 / 初始化转置表
    auto& entry = ttable[hash];
    
    // 阈值检查
    if (entry.pn >= pnThreshold || entry.dn >= dnThreshold) return;
    
    // 深度限制
    if (depth > maxDepth) {
        entry.pn = DFPN_INF;
        entry.dn = 0;
        return;
    }
    
    // 生成走法（缓存）
    if (generateMoves(attacker, currentPlayer, game, entry, lastMove, lastLastMove, threeCount)) {
        return;  // 即时证明/证伪
    }
    
    // 阈值再检查（generateMoves 可能更新了 pn/dn）
    if (entry.pn >= pnThreshold || entry.dn >= dnThreshold) return;
    
    bool isAttacker = (currentPlayer == attacker);
    const auto& moves = entry.moves;
    
    // 即时胜利检查已移除。
    // 原设计：进攻方试着下一步后检查活四/双冲四，但这与 df-pn 递归搜索重复且逻辑错误
    // （VCT 中活三→活四需要经过防守方回应，不能跳过防守方直接判定）。
    // 五连和活四判定在 generateMoves 中已处理（检查己方五连、己方活四）。
    // df-pn 递归本身就能正确处理所有多步 VCT 序列。
    
    // ---- df-pn 主循环 ----
    while (running.load()) {
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
            
            auto cit = ttable.find(childHash);
            uint32_t cpn = 1, cdn = 1;
            if (cit != ttable.end()) {
                cpn = cit->second.pn;
                cdn = cit->second.dn;
            }
            children.push_back({i, cpn, cdn});
        }
        
        // 计算当前节点 pn/dn
        uint32_t sumPn = 0, sumDn = 0;
        uint32_t minPn = DFPN_INF, minDn = DFPN_INF;
        
        for (auto& c : children) {
            minPn = std::min(minPn, c.pn);
            minDn = std::min(minDn, c.dn);
            sumPn = (sumPn > DFPN_INF - c.pn) ? DFPN_INF : sumPn + c.pn;
            sumDn = (sumDn > DFPN_INF - c.dn) ? DFPN_INF : sumDn + c.dn;
        }
        
        if (isAttacker) {
            entry.pn = minPn;
            entry.dn = sumDn;
        } else {
            entry.pn = sumPn;
            entry.dn = minDn;
        }
        
        // 阈值检查
        if (entry.pn >= pnThreshold || entry.dn >= dnThreshold) return;
        if (entry.pn == 0 || entry.dn == 0) return;
        
        // 选择 MPN（Most Proving Node）
        int bestIdx = -1;
        if (isAttacker) {
            uint32_t bestPn = DFPN_INF;
            for (auto& c : children) {
                if (c.pn < bestPn) { bestPn = c.pn; bestIdx = c.idx; }
            }
        } else {
            uint32_t bestDn = DFPN_INF;
            for (auto& c : children) {
                if (c.dn < bestDn) { bestDn = c.dn; bestIdx = c.idx; }
            }
        }
        
        if (bestIdx < 0) return;
        
        auto& bestChild = children[isAttacker ? 0 : 0];  // 临时
        for (auto& c : children) {
            if (c.idx == bestIdx) { bestChild = c; break; }
        }
        
        // 计算子节点阈值
        uint32_t childPnTh, childDnTh;
        if (isAttacker) {
            uint32_t secondMinPn = DFPN_INF;
            for (auto& c : children) {
                if (c.idx != bestIdx && c.pn < secondMinPn) secondMinPn = c.pn;
            }
            childPnTh = std::min(pnThreshold, (secondMinPn < DFPN_INF - 1) ? secondMinPn + 1 : DFPN_INF);
            childDnTh = (dnThreshold >= DFPN_INF || sumDn >= DFPN_INF) ? DFPN_INF : 
                         dnThreshold - sumDn + bestChild.dn;
        } else {
            uint32_t secondMinDn = DFPN_INF;
            for (auto& c : children) {
                if (c.idx != bestIdx && c.dn < secondMinDn) secondMinDn = c.dn;
            }
            childPnTh = (pnThreshold >= DFPN_INF || sumPn >= DFPN_INF) ? DFPN_INF :
                          pnThreshold - sumPn + bestChild.pn;
            childDnTh = std::min(dnThreshold, (secondMinDn < DFPN_INF - 1) ? secondMinDn + 1 : DFPN_INF);
        }
        
        // threeCount 更新
        Point bestMove = moves[bestIdx];
        int newThreeCount = threeCount;
        if (isAttacker) {
            for (int dir = 0; dir < 4; dir++) {
                if (checkPointDirectShape(game, attacker, bestMove, dir, ACTIVE_THREE)) {
                    newThreeCount++;
                    break;
                }
            }
        }
        
        // 递归
        Point& m = bestMove;
        game.board[m.x][m.y] = currentPlayer;
        game.zobristHash ^= zobristTable.pieces[m.x][m.y][0]
                         ^  zobristTable.pieces[m.x][m.y][currentPlayer]
                         ^  zobristTable.currentPlayerHash;
        mid(attacker, 3 - currentPlayer, game, running,
            childPnTh, childDnTh,
            m, lastMove,
            newThreeCount, depth + 1, maxDepth);
        game.zobristHash ^= zobristTable.pieces[m.x][m.y][0]
                         ^  zobristTable.pieces[m.x][m.y][currentPlayer]
                         ^  zobristTable.currentPlayerHash;
        game.board[m.x][m.y] = 0;
    }
}

// -------- 公开接口 --------

std::pair<bool, std::vector<Point>>
dfpnVCT(int attackPlayer, Game& game, std::atomic<bool>& running, int maxNodes, int maxDepth) {
    ttable.clear();
    ttable.reserve(std::min(maxNodes, 1000000));
    nodeCount = 0;
    maxNodesLimit = maxNodes;
    
    // 少子局面快速返回
    int pieceCount = 0;
    for (int r = 0; r < game.boardSize; r++)
        for (int c = 0; c < game.boardSize; c++)
            if (game.board[r][c] != 0) pieceCount++;
    if (pieceCount < 6) {
        return {false, {}};
    }
    
    // MID 搜索（根节点 lastMove/lastLastMove 都为空 → 用全部空点）
    mid(attackPlayer, attackPlayer, game, running,
        DFPN_INF, DFPN_INF,
        Point(), Point(),
        0, 0, maxDepth);
    
    uint64_t hash = game.zobristHash;
    auto it = ttable.find(hash);
    if (it != ttable.end() && it->second.pn == 0) {
        // 找到 VCT
        if (!it->second.moves.empty()) {
            // 从 moves 中找 pn=0 的第一步
            for (auto& m : it->second.moves) {
                uint64_t childHash = hash
                    ^ zobristTable.pieces[m.x][m.y][0]
                    ^ zobristTable.pieces[m.x][m.y][attackPlayer]
                    ^ zobristTable.currentPlayerHash;
                auto cit = ttable.find(childHash);
                if (cit != ttable.end() && cit->second.pn == 0) {
                    return {true, {m}};
                }
            }
        }
        return {true, {}};
    }
    
    return {false, {}};
}

// -------- PV 提取（验证用）--------

std::vector<Point> dfpnExtractPV(int attackPlayer, Game& game, int maxDepth) {
    std::vector<Point> pv;
    int currentPlayer = attackPlayer;
    
    for (int depth = 0; depth < maxDepth; depth++) {
        uint64_t hash = game.zobristHash;
        auto it = ttable.find(hash);
        if (it == ttable.end()) break;
        
        auto& entry = it->second;
        
        // 已证明 (pn=0) 或已证伪 (dn=0)，若无走法则终止
        if (entry.moves.empty()) break;
        
        bool isAttacker = (currentPlayer == attackPlayer);
        
        // 进攻方: 找 pn=0 的子节点（任一即可）
        // 防守方: 找 pn=0 的子节点（所有都要，选第一个继续）
        // 但这里只走一条线，所以统一找第一个 pn=0 的子节点
        Point bestMove;
        bool found = false;
        
        if (isAttacker) {
            // OR 节点：找 pn=0 的孩子
            for (auto& m : entry.moves) {
                uint64_t childHash = hash
                    ^ zobristTable.pieces[m.x][m.y][0]
                    ^ zobristTable.pieces[m.x][m.y][currentPlayer]
                    ^ zobristTable.currentPlayerHash;
                auto cit = ttable.find(childHash);
                if (cit != ttable.end() && cit->second.pn == 0) {
                    bestMove = m;
                    found = true;
                    break;
                }
            }
            // 也可能是即时胜利（pn=0 但子节点没有 pn=0，因为是 generateMoves 直接判定的）
            if (!found && entry.pn == 0) {
                // 尝试找任意一个走法（即时胜利的走法）
                if (!entry.moves.empty()) {
                    bestMove = entry.moves[0];
                    found = true;
                }
            }
        } else {
            // AND 节点：所有孩子都要 pn=0，选第一个继续走
            for (auto& m : entry.moves) {
                uint64_t childHash = hash
                    ^ zobristTable.pieces[m.x][m.y][0]
                    ^ zobristTable.pieces[m.x][m.y][currentPlayer]
                    ^ zobristTable.currentPlayerHash;
                auto cit = ttable.find(childHash);
                if (cit != ttable.end() && cit->second.pn == 0) {
                    bestMove = m;
                    found = true;
                    break;
                }
            }
            if (!found && entry.pn == 0 && !entry.moves.empty()) {
                bestMove = entry.moves[0];
                found = true;
            }
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
        // 逆序恢复: 最后下的棋子对应的 player
        int piecePlayer = ((i % 2 == 0) ? attackPlayer : 3 - attackPlayer);
        game.zobristHash ^= zobristTable.pieces[pv[i].x][pv[i].y][0]
                         ^  zobristTable.pieces[pv[i].x][pv[i].y][piecePlayer]
                         ^  zobristTable.currentPlayerHash;
        game.board[pv[i].x][pv[i].y] = 0;
    }
    
    return pv;
}
