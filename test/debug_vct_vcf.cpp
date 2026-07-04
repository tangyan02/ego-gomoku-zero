#include <iostream>
#include <vector>
#include <atomic>
#include "../Game.h"
#include "../Analyzer.h"
#include "../DfpnVCT.h"
#include "../DfpnVCF.h"

using namespace std;

int main() {
    initShape();  // 必须初始化 shape lookup table!
    Game game(20);
    
    // Piskvork 中 brain_my 设 currentPlayer=1 再 makeMove
    // brain_opponents 设 currentPlayer=2 再 makeMove
    // 前5步通过 BOARD 命令设入（也用类似方式设 currentPlayer 再 makeMove）
    
    // 从 log 看: ego=P1 在奇数步号(5,7,9,...)出招, Wine=P2 在偶数步号(6,8,10,...)出招
    // 前5步(0-4): 标准 Gomocup 5步开局, 交替 P1,P2,P1,P2,P1
    // (黑先 → 步0=P1, 步1=P2, ..., 步4=P1)
    // 步4后轮到P2，但步5是ego(P1)走——说明前5步不是标准交替
    // 
    // 更合理: 在 Gomocup 中, 5步开局后第一个走的是白方(P2)还是黑方取决于规则
    // 但从log看ego在步5走且标记P1，最简单的做法：直接按步号奇偶分配
    // 步0=P2(白), 步1=P1(黑), 步2=P2, 步3=P1, 步4=P2
    // 这样步4后轮到P1，步5由P1(ego)走 ✓
    
    struct Move { int x, y, player; };
    vector<Move> allMoves = {
        {1,17, 2},  // 0) P2 (黑=对方)
        {3,18, 1},  // 1) P1 (白=ego)
        {2,16, 2},  // 2) P2
        {4,14, 1},  // 3) P1 (白=ego)
        {2,15, 2},  // 4) P2 (黑=对方)
        {2,14, 1},  // 5) ego P1
        {4,16, 2},  // 6) Wine P2
        {5,15, 1},  // 7) ego P1
        {3,16, 2},  // 8) Wine P2
        {1,16, 1},  // 9) ego P1
        {6,16, 2},  // 10) Wine P2
        {5,16, 1},  // 11) ego P1
        {5,14, 2},  // 12) Wine P2
        {6,13, 1},  // 13) ego P1
        {6,15, 2},  // 14) Wine P2
        {6,17, 1},  // 15) ego P1
        {5,17, 2},  // 16) Wine P2
        {7,15, 1},  // 17) ego P1
        {4,17, 2},  // 18) Wine P2
        {5,18, 1},  // 19) ego P1
        {2,17, 2},  // 20) Wine P2
        {3,17, 1},  // 21) ego P1
        {4,15, 2},  // 22) Wine P2
        {1,18, 1},  // 23) ego P1
        {7,16, 2},  // 24) Wine P2
        {4,13, 1},  // 25) ego P1
        {4,18, 2},  // 26) Wine P2
        {4,19, 1},  // 27) ego P1
        {4,12, 2},  // 28) Wine P2
        {7,11, 1},  // 29) ego P1
        {5,13, 2},  // 30) Wine P2
        {6,11, 1},  // 31) ego P1
        {7,10, 2},  // 32) Wine P2
        {6,10, 1},  // 33) ego P1
        {6,12, 2},  // 34) Wine P2
        {8,12, 1},  // 35) ego P1
        {9,13, 2},  // 36) Wine P2
        {8,11, 1},  // 37) ego P1
        {9,11, 2},  // 38) Wine P2
        {8,10, 1},  // 39) ego P1
        {8,13, 2},  // 40) Wine P2
        {5,9, 1},   // 41) ego P1
        {4,8, 2},   // 42) Wine P2
        {8,9, 1},   // 43) ego P1
        {8,8, 2},   // 44) Wine P2
        {6,9, 1},   // 45) ego P1
        {2,19, 2},  // 46) Wine P2
        {2,18, 1},  // 47) ego P1
        {9,9, 2},   // 48) Wine P2
        {6,7, 1},   // 49) ego P1
        {6,8, 2},   // 50) Wine P2
        {4,9, 1},   // 51) ego P1
        {7,9, 2},   // 52) Wine P2
        {2,9, 1},   // 53) ego P1
        {3,9, 2},   // 54) Wine P2
        {4,11, 1},  // 55) ego P1
        {5,11, 2},  // 56) Wine P2
    };
    
    cout << "Setting up " << allMoves.size() << " moves using makeMove..." << endl;
    
    for (auto& m : allMoves) {
        game.currentPlayer = m.player;
        bool ok = game.makeMove(Point(m.x, m.y));
        if (!ok) {
            cout << "FAILED at (" << m.x << "," << m.y << ") player=" << m.player << endl;
            return 1;
        }
    }
    
    // 第57步由 P1(ego) 走
    game.currentPlayer = 1;
    
    cout << "\n=== Position before move 57 ===" << endl;
    cout << "Current player: P" << game.currentPlayer << " (ego)" << endl;
    
    // 打印棋盘: board[row][col], row=第一维, col=第二维
    // Piskvork [x,y] → Point(x,y), makeMove: row=p.x, col=p.y
    // 所以 board[x][y] → x是Piskvork的x, y是Piskvork的y
    cout << "\nBoard (X=P1/ego, O=P2/Wine):" << endl;
    cout << "  y:";
    for (int y = 0; y < 20; y++) cout << (y%10);
    cout << endl;
    for (int x = 0; x < 20; x++) {
        printf("x%2d ", x);
        for (int y = 0; y < 20; y++) {
            if (game.board[x][y] == 0) cout << ".";
            else if (game.board[x][y] == 1) cout << "X";
            else cout << "O";
        }
        cout << endl;
    }
    
    // 先检查 (3,10) 是否在 near2 范围中
    auto emptyPts = game.getEmptyPoints();
    bool found310 = false;
    for (auto& p : emptyPts) {
        if (p.x == 3 && p.y == 10) { found310 = true; break; }
    }
    cout << "Point(3,10) in getEmptyPoints: " << (found310 ? "YES" : "NO") << endl;
    
    // 检查第57步前 ego 的活三点
    auto allPts = game.getAllEmptyPoints();
    auto preActiveThree = getActiveThreeMoves(1, game, allPts);
    cout << "ego activeThree points (before move 57): " << preActiveThree.size() << endl;
    for (auto& p : preActiveThree) {
        cout << "  (" << p.x << "," << p.y << ")";
    }
    if (!preActiveThree.empty()) cout << endl;
    
    // 模拟 DFPN 根节点选 [3,10] 后防守方是否有 VCF
    cout << "\n--- Simulate: ego plays [3,10], check Wine VCF ---" << endl;
    game.board[3][10] = 1;  // ego 落子
    {
        auto wineVCF = dfsVCF(2, 2, game, Point(), Point());
        cout << "After ego [3,10]: Wine VCF = " << (wineVCF.first ? "YES" : "NO");
        if (wineVCF.first && !wineVCF.second.empty()) cout << " first=[" << wineVCF.second[0].x << "," << wineVCF.second[0].y << "]";
        cout << endl;
    }
    game.board[3][10] = 0;  // 撤回
    
    // 模拟 PV: ego [3,10] → Wine [5,12] → 进攻方看到什么?
    cout << "\n--- Simulate PV: ego[3,10] Wine[5,12] ---" << endl;
    game.board[3][10] = 1;
    game.board[5][12] = 2;
    {
        auto allPts = game.getEmptyPoints();
        auto egoWin = getWinningMoves(1, game, allPts);
        auto egoAF = getActiveFourMoves(1, game, allPts);
        auto wineWin = getWinningMoves(2, game, allPts);
        cout << "ego winMoves=" << egoWin.size() << " activeFour=" << egoAF.size() << endl;
        cout << "Wine winMoves=" << wineWin.size() << endl;
        if (!egoWin.empty()) cout << "  ego WIN at [" << egoWin[0].x << "," << egoWin[0].y << "]" << endl;
        if (!egoAF.empty()) cout << "  ego AF at [" << egoAF[0].x << "," << egoAF[0].y << "]" << endl;
        if (!wineWin.empty()) {
            cout << "  Wine WIN at:";
            for (auto& p : wineWin) cout << " [" << p.x << "," << p.y << "]";
            cout << endl;
        }
    }
    game.board[3][10] = 0;
    game.board[5][12] = 0;
    
    // 检查 VCT
    cout << "\n--- Checking ego's VCT at move 57 position ---" << endl;
    atomic<bool> running(true);
    auto vctResult = dfpnVCT(game.currentPlayer, game, running, 2000000);
    cout << "dfpnVCT result: " << (vctResult.found ? "HAS VCT" : "NO VCT") 
         << " (exhaustive=" << vctResult.exhaustive << ")" << endl;
    if (vctResult.found && !vctResult.moves.empty()) {
        cout << "VCT first move: [" << vctResult.moves[0].x << "," << vctResult.moves[0].y << "]" << endl;
        // 提取完整 PV
        auto pv = dfpnExtractPV(game.currentPlayer, game);
        cout << "VCT PV (" << pv.size() << " moves): ";
        for (int i = 0; i < (int)pv.size(); i++) {
            cout << "[" << pv[i].x << "," << pv[i].y << "]";
            if (i % 2 == 0) cout << "(atk) "; else cout << "(def) ";
        }
        cout << endl;
    }
    
    auto vctIter = dfpnVCTIterDeepen(game.currentPlayer, game, running, 2000000, 5000);
    cout << "dfpnVCTIterDeepen result: " << (vctIter.found ? "HAS VCT" : "NO VCT")
         << " (exhaustive=" << vctIter.exhaustive << ")" << endl;
    if (vctIter.found && !vctIter.moves.empty()) {
        cout << "VCT first move: [" << vctIter.moves[0].x << "," << vctIter.moves[0].y << "]" << endl;
    }
    
    // VCF
    auto egoVCF = dfsVCF(1, 1, game, Point(), Point());
    cout << "ego VCF: " << (egoVCF.first ? "YES" : "NO") << endl;
    
    // 模拟第57步: ego 走 [3,10]
    cout << "\n--- Move 57: ego plays [3,10] ---" << endl;
    game.currentPlayer = 1;
    game.makeMove(Point(3, 10));
    
    cout << "Current player after move 57: P" << game.currentPlayer << " (Wine)" << endl;
    
    // 检查 Wine VCF
    auto wineVCF = dfsVCF(game.currentPlayer, game.currentPlayer, game, Point(), Point());
    cout << "Wine VCF (after move 57): " << (wineVCF.first ? "YES" : "NO") << endl;
    if (wineVCF.first && !wineVCF.second.empty()) {
        cout << "Wine VCF first: [" << wineVCF.second[0].x << "," << wineVCF.second[0].y << "]" << endl;
    }
    
    // 直接检查 Point(3,10) 的 shape
    {
        Point p310(3, 10);
        cout << "--- Shape check for (3,10) direction 0 (diagonal \\) ---" << endl;
        auto keys = getKeysInGame(game, 1, p310, 0);
        cout << "Keys: ";
        for (int k : keys) cout << k;
        cout << " (0=空 1=己 2=对 3=wall)" << endl;
        
        // 手动调用 activeThree 函数验证
        vector<int> keyVec(keys.begin(), keys.end());
        // 先看 activeFour
        vector<int> kCopy = keyVec;
        cout << "Manual activeFour check:" << endl;
        int afCount = 0;
        for (int i = 0; i < 9; i++) {
            if (kCopy[i] == 0) {
                kCopy[i] = 1;
                // check win
                int wc = 0; bool isWin = false;
                for (int v : kCopy) { if (v==1) { wc++; if (wc>=5) isWin=true; } else wc=0; }
                if (isWin) { cout << "  pos " << i << " → win! "; afCount++; }
                kCopy[i] = 0;
            }
        }
        cout << "afCount=" << afCount << (afCount>=2?" → ACTIVE_FOUR":"") << endl;
        
        // 然后看 activeThree
        cout << "Manual activeThree check:" << endl;
        int atCount = 0;
        for (int i = 0; i < 9; i++) {
            if (keyVec[i] == 0) {
                keyVec[i] = 1;
                // check if this is activeFour
                int af2 = 0;
                for (int j = 0; j < 9; j++) {
                    if (keyVec[j] == 0) {
                        keyVec[j] = 1;
                        int wc2 = 0; bool w2 = false;
                        for (int v : keyVec) { if (v==1) { wc2++; if (wc2>=5) w2=true; } else wc2=0; }
                        if (w2) af2++;
                        keyVec[j] = 0;
                    }
                }
                if (af2 >= 2) { cout << "  pos " << i << " → makes activeFour! "; atCount++; }
                keyVec[i] = 0;
            }
        }
        cout << "atCount=" << atCount << (atCount>=1?" → ACTIVE_THREE":"") << endl;
        
        bool isAT = checkPointDirectShape(game, 1, p310, 0, ACTIVE_THREE);
        bool isSF = checkPointDirectShape(game, 1, p310, 0, SLEEPY_FOUR);
        bool isAF = checkPointDirectShape(game, 1, p310, 0, ACTIVE_FOUR);
        cout << "shapeMapping result: activeThree=" << isAT << " sleepyFour=" << isSF << " activeFour=" << isAF << endl;
        
        // 检查所有4方向
        for (int d = 0; d < 4; d++) {
            auto k = getKeysInGame(game, 1, p310, d);
            bool at = checkPointDirectShape(game, 1, p310, d, ACTIVE_THREE);
            bool sf = checkPointDirectShape(game, 1, p310, d, SLEEPY_FOUR);
            cout << "dir=" << d << " keys=";
            for (int x : k) cout << x;
            cout << " AT=" << at << " SF=" << sf << endl;
        }
    }
    
    // 检查 ego threats
    auto allEmpty = game.getEmptyPoints();
    auto allFull = game.getAllEmptyPoints();
    cout << "getEmptyPoints: " << allEmpty.size() << ", getAllEmptyPoints: " << allFull.size() << endl;
    
    auto atkSleepFour = getSleepyFourMoves(1, game, allFull);
    auto atkActiveThree = getActiveThreeMoves(1, game, allFull);
    auto atkActiveFour = getActiveFourMoves(1, game, allFull);
    
    // 检查 (3,10) 附近的连子情况（4个方向）
    cout << "--- Checking diagonal around (3,10) ---" << endl;
    // 方向1: 主对角线 (x+1,y+1)
    cout << "Diagonal (\\): ";
    for (int d = -4; d <= 4; d++) {
        int cx = 3+d, cy = 10+d;
        if (cx >= 0 && cx < 20 && cy >= 0 && cy < 20) {
            int v = game.board[cx][cy];
            cout << "(" << cx << "," << cy << ")=" << (v==0?"." : v==1?"X":"O") << " ";
        }
    }
    cout << endl;
    // 方向2: 副对角线 (x+1,y-1)
    cout << "Diagonal (/): ";
    for (int d = -4; d <= 4; d++) {
        int cx = 3+d, cy = 10-d;
        if (cx >= 0 && cx < 20 && cy >= 0 && cy < 20) {
            int v = game.board[cx][cy];
            cout << "(" << cx << "," << cy << ")=" << (v==0?"." : v==1?"X":"O") << " ";
        }
    }
    cout << endl;
    // 方向3: 水平 (x+1,y)
    cout << "Horizontal: ";
    for (int d = -4; d <= 4; d++) {
        int cx = 3+d, cy = 10;
        if (cx >= 0 && cx < 20 && cy >= 0 && cy < 20) {
            int v = game.board[cx][cy];
            cout << "(" << cx << "," << cy << ")=" << (v==0?"." : v==1?"X":"O") << " ";
        }
    }
    cout << endl;
    // 方向4: 垂直 (x,y+1)
    cout << "Vertical: ";
    for (int d = -4; d <= 4; d++) {
        int cx = 3, cy = 10+d;
        if (cx >= 0 && cy >= 0 && cx < 20 && cy < 20) {
            int v = game.board[cx][cy];
            cout << "(" << cx << "," << cy << ")=" << (v==0?"." : v==1?"X":"O") << " ";
        }
    }
    cout << endl;
    
    cout << "Attacker(ego) sleepFour=" << atkSleepFour.size() 
         << " activeThree=" << atkActiveThree.size()
         << " activeFour=" << atkActiveFour.size() << endl;
    for (auto& p : atkActiveThree) cout << "  activeThree: [" << p.x << "," << p.y << "]" << endl;
    
    // 第58步: Wine 走 [5,12]
    cout << "\n--- Move 58: Wine plays [5,12] ---" << endl;
    game.currentPlayer = 2;
    game.makeMove(Point(5, 12));
    
    auto wineVCF2 = dfsVCF(2, 2, game, Point(), Point());
    cout << "Wine VCF (after move 58): " << (wineVCF2.first ? "YES" : "NO") << endl;
    if (wineVCF2.first && !wineVCF2.second.empty()) {
        cout << "Wine VCF first: [" << wineVCF2.second[0].x << "," << wineVCF2.second[0].y << "]" << endl;
    }
    
    return 0;
}
