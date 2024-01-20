#include "SelfPlay.h"
#include "ReplayBuffer.h"
#include "Utils.h"

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

    createDirectory("model");
    createDirectory("record");

    int numGames = 1;
    int concurrent = 1;
    int sumSimulations = 800;
    float temperatureDefault = 1;
    float explorationFactor = 3;

    auto network = getNetwork();

    recordConcurrentSelfPlay(numGames ,sumSimulations, temperatureDefault, explorationFactor, concurrent);

    return 0;
}