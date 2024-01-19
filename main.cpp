#include "SelfPlay.h"
#include "Train.h"
#include "ReplayBuffer.h"
#include "utils.h"

using namespace std;

// 获取当前时间的字符串表示
std::string getTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "[%Y-%m-%d %H:%M:%S] ");
    return ss.str();
}


int main() {

    createModelDirectory();

    int episode = 100000;
    int numGames = 10;
    int concurrent = 10;
    int sumSimulations = 800;
    int temperatureDefault = 1;
    int explorationFactor = 3;
    int buffer_size = 10000;
    float lr = 0.001;
    int num_epochs = 5;
    int batch_size = 128;

    auto replayBuffer = ReplayBuffer(buffer_size);

    auto network = getNetwork();
    for (int i = 1; i <= episode; i++) {
        auto start_time = std::chrono::steady_clock::now();
        auto data = concurrentSelfPlay(getDevice(), numGames ,
                                       sumSimulations, temperatureDefault, explorationFactor, concurrent);
        cout << "training_data size:" << data.size() << endl;

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << getTimeStr() << "训练完毕，用时" << duration.count() / 1000.0 << "秒" << std::endl;
        replayBuffer.add_samples(data);
        train(replayBuffer, network, getDevice(), lr, num_epochs, batch_size);

        if (i % 100 == 0) {
            saveNetwork(network, "model/net_" + to_string(i) + ".mdl");
        }
        saveNetwork(network);
        cout << "模型保存完毕 episode " << i << endl;
    }

    return 0;
}