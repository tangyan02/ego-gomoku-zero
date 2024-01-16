#include "SelfPlay.h"

using namespace std;

int main() {
    auto data = selfPlay(getDevice(), 1, 800, 1, 3);
    cout << "training_data size:" << data.size() << endl;
    return 0;
}