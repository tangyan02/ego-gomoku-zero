#define DOCTEST_CONFIG_IMPLEMENT

#include "ConfigReader.h"
#include "test/TestAssistant.cpp"
#include "SelfPlay.h"
#include "Pisqpipe.h"
#include "Shape.h"
#include "Model.h"
#include "Bridge.h"
#include "Evaluate.h"
#include "GenerateOpenings.h"
#include "GumbelMCTS.h"
#include "VCTLabeler.h"
#include "BenchSelectActions.h"
#include <memory>

using namespace std;

void selfPlay(int argc, char *argv[]) {
    int boardSize = stoi(ConfigReader::get("boardSize"));
    int numGames = stoi(ConfigReader::get("numGames"));
    int numSimulation = stoi(ConfigReader::get("numSimulation"));
    float explorationFactor = stof(ConfigReader::get("explorationFactor"));
    int numProcesses = stoi(ConfigReader::get("numProcesses"));

    // 主线程加载模型，避免多线程并发 MPS 初始化崩溃
    string modelPath = ConfigReader::get("modelPath");
    string coreType = ConfigReader::get("coreType");
    auto sharedModel = std::make_unique<Model>();
    sharedModel->init(modelPath, coreType);

    std::vector<std::thread> threads; // 存储线程的容器

    auto context = std::make_unique<Context>(numGames);
    // 创建n个线程并将函数作为入口点
    for (int i = 0; i < numProcesses; ++i) {
        threads.emplace_back(recordSelfPlay,
                             boardSize,
                             context.get(),
                             numSimulation,
                             explorationFactor,
                             i,
                             sharedModel.get());
    }

    // 等待所有线程执行完毕
    for (auto &thread: threads) {
        thread.join();
    };
}

int main(int argc, char *argv[]) {
    initShape();
    //    printShape();

    #ifdef _WIN32
    piskvork();
    return 0;

    #endif

    auto mode = ConfigReader::get("mode");
    if (mode == "train") {
        selfPlay(argc, argv);
        return 0;
    }

    if (mode == "predict") {
        Bridge bridge;
        bridge.startGame();
        return 0;
    }

    if (mode == "evaluate") {
        runEvaluate();
        return 0;
    }

    if (mode == "generate_openings") {
        runGenerateOpenings();
        return 0;
    }

    if (mode == "vct_label") {
        runVCTLabeling();
        return 0;
    }

    if (mode == "bench_select_actions") {
        runBenchSelectActions();
        return 0;
    }

    if (mode == "test_gumbel") {
        int boardSize = stoi(ConfigReader::get("boardSize"));
        string modelPath = ConfigReader::get("modelPath");
        string coreType = ConfigReader::get("coreType");
        int gumbelSims = stoi(ConfigReader::getOrDefault("gumbelSimulations", "32"));
        int gumbelTopK = stoi(ConfigReader::getOrDefault("gumbelTopK", "16"));
        int numGames = stoi(ConfigReader::getOrDefault("testGumbelGames", "5"));

        Model model;
        model.init(modelPath, coreType);
        GumbelMCTS gumbel(&model, boardSize, gumbelSims, gumbelTopK);

        cout << "[TestGumbel] sims=" << gumbelSims << " topK=" << gumbelTopK << " games=" << numGames << endl;

        for (int g = 0; g < numGames; g++) {
            Game game(boardSize);
            // 用开局库
            string prefix = "[Game " + to_string(g) + "] ";
            game = randomGame(game, prefix);
            gumbel.clearTranspositionTable();
            int step = 1;
            while (!game.isGameOver()) {
                auto result = gumbel.search(game);
                game.makeMove(result.action);
                string pic = (game.getOtherPlayer() == 1) ? "x" : "o";
                cout << "[Game " << g << "] step " << step << " " << pic << " "
                     << result.action.x << "," << result.action.y
                     << " Q=" << round(result.rootQ * 100) / 100 << endl;
                step++;
                if (step > 200) { cout << "[Game " << g << "] max steps reached" << endl; break; }
            }
            int winner = 0;
            if (game.lastAction.x >= 0 && game.checkWin(game.lastAction.x, game.lastAction.y, game.getOtherPlayer())) {
                winner = game.getOtherPlayer();
            }
            cout << "[Game " << g << "] finished in " << step << " steps, winner=" << winner << endl;
        }
        return 0;
    }

    return startTest(argc, argv);
}
