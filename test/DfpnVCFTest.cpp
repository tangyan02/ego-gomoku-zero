#include "../Game.h"
#include "../Analyzer.h"
#include "../DfpnVCF.h"
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace std;

// ======== VCF 基准测试用例 ========

struct VCFBenchCase {
    string name;
    int boardSize;
    vector<pair<int,int>> moves;
    int attackPlayer;  // 要检查 VCF 的一方（1=黑, 2=白）
    bool expectedVCF;
};

// pointStr 已在 DfpnVCTTest.cpp 中定义

static vector<VCFBenchCase> getVCFBenchCases() {
    vector<VCFBenchCase> cases;
    
    // Case 1: testDfsVCF - 经典 VCF（P1黑方有VCF, 13手后当前方=P2）
    cases.push_back({"vcf_classic_1", 20, {
        {1,4}, {2,0}, {1,5}, {2,1}, {1,6}, {2,2},
        {2,6}, {1,3}, {3,5}, {0,8}, {6,6}, {7,8}, {7,7},
    }, 1, true});
    
    // Case 2: testDfsVCF2 - dfsVCF判断无VCF，但DFPN发现了真实VCF路径
    // (1,7)冲四→堵(1,8)→(4,4)冲四→堵(5,3)→(5,5)活四
    // dfsVCF 因搜索范围限制漏掉了这条路径
    cases.push_back({"vcf_dfs_miss", 20, {
        {1,4}, {7,3}, {1,5}, {8,3}, {1,6}, {9,3},
        {2,6}, {1,3}, {3,5}, {0,8}, {6,6}, {7,8}, {7,7},
    }, 1, true});
    
    // Case 3: testDfsVCF3 - P1 有 VCF（20x20 棋盘）
    cases.push_back({"vcf_classic_2", 20, {
        {1,4}, {5,5}, {1,5}, {5,6}, {1,6}, {5,7},
        {2,6}, {1,3}, {3,5}, {0,8}, {6,4}, {7,8}, {7,4},
    }, 1, true});
    
    // Case 4: testDfsVCF4 - P1 有边角 VCF
    cases.push_back({"vcf_edge", 20, {
        {0,1}, {1,0}, {0,3}, {1,1}, {0,4}, {1,2}, {0,5}, {1,3},
    }, 1, true});
    
    // Case 5: 少子 - 无 VCF
    cases.push_back({"vcf_few_pieces", 20, {
        {9,9}, {10,10},
    }, 1, false});
    
    // Case 6: 中盘复杂局面 - 当前方(P1)检查
    cases.push_back({"vcf_complex_1", 20, {
        {9,13}, {9,15},
        {13,15}, {14,14},
        {16,11}, {11,14},
        {11,15}, {10,14},
        {12,14}, {11,13},
        {12,12}, {12,15},
        {13,13}, {14,12},
        {14,11}, {10,13},
        {9,12}, {10,16},
        {10,12}, {13,12},
    }, 1, false});  // 有 VCT 但可能没有 VCF
    
    // Case 7: P1 有多步冲四
    cases.push_back({"vcf_long_sequence", 20, {
        {7,7}, {0,0},
        {7,8}, {0,1},
        {7,9}, {0,2},
        {8,6}, {1,0},
        {9,5}, {1,1},
        {6,8}, {2,0},
        {5,9}, {2,1},
        {6,6}, {3,0},
        {8,8}, {3,1},
    }, 1, true});
    
    // Case 8: 双冲四局面（P1 当前方）
    cases.push_back({"vcf_double_four", 20, {
        {7,7}, {0,0},
        {7,8}, {0,1},
        {7,9}, {0,2},
        {7,10}, {0,3},
        {6,7}, {1,0},
        {6,8}, {1,1},
        {6,9}, {1,2},
        {6,10}, {1,3},
    }, 1, true});
    
    return cases;
}

// 用 dfsVCF 跑一个局面
static pair<bool, double> runDfsVCF(Game& game, int attackPlayer) {
    auto start = chrono::high_resolution_clock::now();
    auto result = dfsVCF(attackPlayer, attackPlayer, game, Point(), Point());
    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();
    return {result.first, ms};
}

// 用 dfpnVCF 跑一个局面
static pair<bool, double> runDfpnVCF(Game& game, int attackPlayer) {
    auto start = chrono::high_resolution_clock::now();
    auto result = dfpnVCF(attackPlayer, game, 2000000);
    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();
    return {result.first, ms};
}

// ======== 性能对比测试 ========

TEST_CASE("bench_dfpn_vs_dfs_vcf") {
    auto cases = getVCFBenchCases();
    
    cout << "\n============================================" << endl;
    cout << " DF-PN vs DFS VCF Performance Benchmark" << endl;
    cout << "============================================\n" << endl;
    cout << left << setw(22) << "Case"
         << right << setw(12) << "DFS(ms)"
         << setw(12) << "DFPN(ms)"
         << setw(10) << "Speedup"
         << setw(8) << "DFS"
         << setw(8) << "DFPN"
         << setw(8) << "Match"
         << endl;
    cout << string(80, '-') << endl;
    
    int correctCount = 0;
    double totalDfs = 0, totalDfpn = 0;
    
    for (auto& tc : cases) {
        Game game(tc.boardSize);
        game.currentPlayer = 1;
        for (auto& [r, c] : tc.moves) {
            game.makeMove(Point(r, c));
        }
        
        // 多轮取平均
        const int ROUNDS = 10;
        double dfsTimeTotal = 0, dfpnTimeTotal = 0;
        bool dfsResult = false, dfpnResult = false;
        
        for (int round = 0; round < ROUNDS; round++) {
            Game g1 = game;
            auto [dr, dt] = runDfsVCF(g1, tc.attackPlayer);
            dfsResult = dr;
            dfsTimeTotal += dt;
            
            Game g2 = game;
            auto [pr, pt] = runDfpnVCF(g2, tc.attackPlayer);
            dfpnResult = pr;
            dfpnTimeTotal += pt;
        }
        
        double dfsTime = dfsTimeTotal / ROUNDS;
        double dfpnTime = dfpnTimeTotal / ROUNDS;
        
        // DFPN 应该与预期一致（它可能比 DFS 找到更多 VCF）
        bool dfpnCorrect = (dfpnResult == tc.expectedVCF);
        correctCount += dfpnCorrect ? 1 : 0;
        totalDfs += dfsTime;
        totalDfpn += dfpnTime;
        
        double speedup = (dfpnTime > 0.0001) ? dfsTime / dfpnTime : 0;
        
        string note = dfpnCorrect ? "OK" : "FAIL";
        if (dfsResult != dfpnResult) note += "*";  // * = DFS disagrees
        
        cout << left << setw(22) << tc.name
             << right << fixed << setprecision(3)
             << setw(12) << dfsTime
             << setw(12) << dfpnTime
             << setw(9) << setprecision(2) << speedup << "x"
             << setw(8) << (dfsResult ? "VCF" : "---")
             << setw(8) << (dfpnResult ? "VCF" : "---")
             << setw(8) << note
             << endl;
    }
    
    cout << string(80, '-') << endl;
    cout << left << setw(22) << "TOTAL"
         << right << fixed << setprecision(3)
         << setw(12) << totalDfs
         << setw(12) << totalDfpn
         << setw(9) << setprecision(2) << (totalDfpn > 0.0001 ? totalDfs / totalDfpn : 0) << "x"
         << endl;
    cout << "\n" << correctCount << "/" << cases.size() << " DFPN results correct" << endl;
    cout << "(* = DFPN found VCF that DFS missed due to range limits)" << endl;
    
    // DFPN 结果必须与预期一致
    CHECK(correctCount == (int)cases.size());
}

// ======== 正确性测试 ========

TEST_CASE("dfpn_vcf_correctness_classic1") {
    // testDfsVCF — P1 有 VCF
    Game game(20);
    game.currentPlayer = 1;
    vector<pair<int,int>> moves = {
        {1,4}, {2,0}, {1,5}, {2,1}, {1,6}, {2,2},
        {2,6}, {1,3}, {3,5}, {0,8}, {6,6}, {7,8}, {7,7},
    };
    for (auto& [r,c] : moves) game.makeMove(Point(r, c));
    
    int attacker = 1;  // P1 黑方
    auto result = dfpnVCF(attacker, game);
    CHECK(result.first);
    
    if (result.first) {
        auto pv = dfpnVCFExtractPV(attacker, game);
        cout << "\n[vcf_classic_1] PV (" << pv.size() << " moves): ";
        for (auto& p : pv) cout << pointStr(p) << " ";
        cout << endl;
    }
}

TEST_CASE("dfpn_vcf_correctness_dfs_miss") {
    // 这个局面 dfsVCF 因搜索范围限制返回无 VCF，但 DFPN 找到了真实的 VCF：
    // (1,7)冲四 → P2堵(1,8) → (4,4)冲四 → P2堵(5,3) → (5,5)活四胜
    Game game(20);
    game.currentPlayer = 1;
    vector<pair<int,int>> moves = {
        {1,4}, {7,3}, {1,5}, {8,3}, {1,6}, {9,3},
        {2,6}, {1,3}, {3,5}, {0,8}, {6,6}, {7,8}, {7,7},
    };
    for (auto& [r,c] : moves) game.makeMove(Point(r, c));
    
    int attacker = 1;
    
    // dfsVCF 漏掉了
    auto dfsResult = dfsVCF(attacker, attacker, game, Point(), Point());
    CHECK(!dfsResult.first);  // dfsVCF 确实返回 false
    
    // DFPN 找到了
    auto dfpnResult = dfpnVCF(attacker, game);
    CHECK(dfpnResult.first);  // DFPN 正确找到 VCF
}

TEST_CASE("dfpn_vcf_correctness_classic2") {
    // testDfsVCF3 — P1 有 VCF
    Game game(20);
    game.currentPlayer = 1;
    vector<pair<int,int>> moves = {
        {1,4}, {5,5}, {1,5}, {5,6}, {1,6}, {5,7},
        {2,6}, {1,3}, {3,5}, {0,8}, {6,4}, {7,8}, {7,4},
    };
    for (auto& [r,c] : moves) game.makeMove(Point(r, c));
    
    int attacker = 1;
    auto result = dfpnVCF(attacker, game);
    CHECK(result.first);
    
    if (result.first) {
        auto pv = dfpnVCFExtractPV(attacker, game);
        cout << "\n[vcf_classic_2] PV (" << pv.size() << " moves): ";
        for (auto& p : pv) cout << pointStr(p) << " ";
        cout << endl;
    }
}

TEST_CASE("dfpn_vcf_correctness_edge") {
    // testDfsVCF4 — P1 有 VCF
    Game game(20);
    game.currentPlayer = 1;
    vector<pair<int,int>> moves = {
        {0,1}, {1,0}, {0,3}, {1,1}, {0,4}, {1,2}, {0,5}, {1,3},
    };
    for (auto& [r,c] : moves) game.makeMove(Point(r, c));
    
    int attacker = 1;
    auto result = dfpnVCF(attacker, game);
    CHECK(result.first);
    
    if (result.first) {
        auto pv = dfpnVCFExtractPV(attacker, game);
        cout << "\n[vcf_edge] PV (" << pv.size() << " moves): ";
        for (auto& p : pv) cout << pointStr(p) << " ";
        cout << endl;
    }
}

// ======== PV 验证测试 ========

// 验证 DFPN VCF 的 PV 序列每一步是否合法
TEST_CASE("dfpn_vcf_pv_verify") {
    struct PVTestCase {
        string name;
        int boardSize;
        vector<pair<int,int>> moves;
        int attackPlayer;
    };
    
    vector<PVTestCase> pvCases = {
        {"vcf_classic_1", 20, {
            {1,4}, {2,0}, {1,5}, {2,1}, {1,6}, {2,2},
            {2,6}, {1,3}, {3,5}, {0,8}, {6,6}, {7,8}, {7,7},
        }, 1},
        {"vcf_classic_2", 20, {
            {1,4}, {5,5}, {1,5}, {5,6}, {1,6}, {5,7},
            {2,6}, {1,3}, {3,5}, {0,8}, {6,4}, {7,8}, {7,4},
        }, 1},
        {"vcf_edge", 20, {
            {0,1}, {1,0}, {0,3}, {1,1}, {0,4}, {1,2}, {0,5}, {1,3},
        }, 1},
    };
    
    for (auto& tc : pvCases) {
        cout << "\n=== PV Verify: " << tc.name << " ===" << endl;
        
        Game game(tc.boardSize);
        game.currentPlayer = 1;
        for (auto& [r, c] : tc.moves) game.makeMove(Point(r, c));
        
        int attacker = tc.attackPlayer;
        int defender = 3 - attacker;
        
        auto result = dfpnVCF(attacker, game);
        if (!result.first) {
            cout << "  No VCF found, skipping PV verify" << endl;
            continue;
        }
        
        auto pv = dfpnVCFExtractPV(attacker, game);
        cout << "  PV length: " << pv.size() << endl;
        
        bool valid = true;
        bool terminated = false;
        int currentPlayer = attacker;
        
        for (int i = 0; i < (int)pv.size(); i++) {
            Point move = pv[i];
            bool isAttack = (currentPlayer == attacker);
            
            if (isAttack) {
                // 进攻方：必须是冲四/活四/五连
                auto allMoves = game.getEmptyPoints();
                auto sf = getSleepyFourMoves(attacker, game, allMoves);
                auto af = getActiveFourMoves(attacker, game, allMoves);
                auto wm = getWinningMoves(attacker, game, allMoves);
                
                bool isSF = false, isAF = false, isWin = false;
                for (auto& p : sf) if (p == move) isSF = true;
                for (auto& p : af) if (p == move) isAF = true;
                for (auto& p : wm) if (p == move) isWin = true;
                
                string type = isWin ? "WIN" : (isAF ? "ACTIVE_FOUR" : (isSF ? "SLEEPY_FOUR" : "???"));
                cout << "  Step " << setw(2) << (i+1) << ": ATK " << pointStr(move) << " -> " << type;
                
                if (!isSF && !isAF && !isWin) {
                    // 可能是被迫堵对方五连的同时冲四
                    auto oppWin = getWinningMoves(defender, game, allMoves);
                    bool forced = false;
                    for (auto& p : oppWin) if (p == move) { forced = true; break; }
                    if (forced) {
                        cout << " (FORCED_BLOCK+FOUR)";
                    } else {
                        cout << " *** INVALID ***";
                        valid = false;
                    }
                }
            } else {
                // 防守方：必须是堵五连点
                auto allMoves = game.getEmptyPoints();
                auto oppWin = getWinningMoves(attacker, game, allMoves);
                bool isBlock = false;
                for (auto& p : oppWin) if (p == move) { isBlock = true; break; }
                
                cout << "  Step " << setw(2) << (i+1) << ": DEF " << pointStr(move) << " -> " << (isBlock ? "BLOCK_WIN" : "???");
                if (!isBlock) {
                    cout << " *** INVALID ***";
                    valid = false;
                }
            }
            
            // 下子
            game.board[move.x][move.y] = currentPlayer;
            
            if (isAttack && game.checkWin(move.x, move.y, attacker)) {
                cout << " *** FIVE IN A ROW ***";
                terminated = true;
                game.board[move.x][move.y] = 0;
                cout << endl;
                break;
            }
            
            // 检查下子后是否有活四（不可阻挡）
            if (isAttack) {
                auto nearM = game.getEmptyPoints();
                auto af = getActiveFourMoves(attacker, game, nearM);
                if (!af.empty()) {
                    cout << " -> LEADS_TO_ACTIVE_FOUR";
                    terminated = true;
                    game.board[move.x][move.y] = 0;
                    cout << endl;
                    break;
                }
                
                // 双冲四
                auto oppNear = game.getEmptyPoints();
                auto oppWinNext = getWinningMoves(attacker, game, oppNear);
                if (oppWinNext.size() >= 2) {
                    cout << " -> DOUBLE_FOUR(unblockable)";
                    terminated = true;
                    game.board[move.x][move.y] = 0;
                    cout << endl;
                    break;
                }
            }
            
            cout << endl;
            currentPlayer = 3 - currentPlayer;
        }
        
        // 恢复棋盘
        for (int i = (int)pv.size() - 1; i >= 0; i--) {
            game.board[pv[i].x][pv[i].y] = 0;
        }
        
        if (terminated) {
            cout << "  RESULT: VCF VERIFIED" << endl;
        } else if (!valid) {
            cout << "  RESULT: INVALID PV" << endl;
        } else {
            cout << "  RESULT: PV incomplete" << endl;
        }
        
        CHECK(valid);
    }
}

// ======== 与 dfsVCF 的 attackPoints 对比 ========

TEST_CASE("dfpn_vcf_attack_points_match") {
    cout << "\n=== attackPoints comparison ===" << endl;
    
    struct TestCase {
        string name;
        vector<pair<int,int>> moves;
        int attackPlayer;
    };
    
    vector<TestCase> cases = {
        {"classic_1", {
            {1,4}, {2,0}, {1,5}, {2,1}, {1,6}, {2,2},
            {2,6}, {1,3}, {3,5}, {0,8}, {6,6}, {7,8}, {7,7},
        }, 1},
        {"classic_2", {
            {1,4}, {5,5}, {1,5}, {5,6}, {1,6}, {5,7},
            {2,6}, {1,3}, {3,5}, {0,8}, {6,4}, {7,8}, {7,4},
        }, 1},
        {"edge", {
            {0,1}, {1,0}, {0,3}, {1,1}, {0,4}, {1,2}, {0,5}, {1,3},
        }, 1},
    };
    
    for (auto& tc : cases) {
        Game game(20);
        game.currentPlayer = 1;
        for (auto& [r,c] : tc.moves) game.makeMove(Point(r, c));
        
        int atk = tc.attackPlayer;
        
        // dfsVCF with allAttackPoints
        vector<Point> dfsAttackPts;
        auto dfsResult = dfsVCF(atk, atk,
                                game, Point(), Point(), 0,
                                nullptr, nullptr, &dfsAttackPts);
        dfsAttackPts = removeDuplicates(dfsAttackPts);
        
        // dfpnVCF with attackPoints
        vector<Point> dfpnAttackPts;
        auto dfpnResult = dfpnVCF(atk, game, 2000000, &dfpnAttackPts);
        
        cout << tc.name << ": dfs=" << (dfsResult.first ? "VCF" : "---")
             << " dfpn=" << (dfpnResult.first ? "VCF" : "---")
             << " dfsAtk=" << dfsAttackPts.size()
             << " dfpnAtk=" << dfpnAttackPts.size() << endl;
        
        if (dfsResult.first && dfpnResult.first) {
            cout << "  DFS attack points: ";
            for (auto& p : dfsAttackPts) cout << pointStr(p) << " ";
            cout << endl;
            cout << "  DFPN attack points: ";
            for (auto& p : dfpnAttackPts) cout << pointStr(p) << " ";
            cout << endl;
        }
        
        CHECK(dfsResult.first == dfpnResult.first);
    }
}
