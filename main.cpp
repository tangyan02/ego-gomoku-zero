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
// 创建目录
//void dirPreBuild() {
//    std::filesystem::path dirPath("model");
//
//    if (!std::filesystem::exists(dirPath)) {
//        std::filesystem::create_directory(dirPath);
//    }
//}

int main() {

//    dirPreBuild();
    int episode = 100000;
    auto replayBuffer = ReplayBuffer(10000);

    auto network = getNetwork();
    for (int i = 1; i <= episode; i++) {
        auto start_time = std::chrono::steady_clock::now();
        auto data = selfPlay(getDevice(), 10, 800, 1, 3);
        cout << "training_data size:" << data.size() << endl;

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << getTimeStr() << "训练完毕，用时" << duration.count() / 1000.0 << "秒" << std::endl;
        replayBuffer.add_samples(data);
        train(replayBuffer, network, getDevice(), 0.001, 5, 128);

        if (i % 100 == 0) {
            saveNetwork(network, "model/net_" + to_string(i) + ".mdl");
        }
        saveNetwork(network);
        cout << "模型保存完毕 episode" << episode << endl;
    }

    return 0;
}