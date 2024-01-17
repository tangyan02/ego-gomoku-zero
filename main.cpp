#include "SelfPlay.h"
#include "Train.h"
#include "ReplayBuffer.h"

using namespace std;

int main() {
    auto data = selfPlay(getDevice(), 10, 800, 1, 3);
    cout << "training_data size:" << data.size() << endl;

    auto replayBuffer = ReplayBuffer(10000);
    replayBuffer.add_samples(data);
    auto network = getNetwork();
    train(replayBuffer, network, getDevice(), 0.001,5,128);
    return 0;
}