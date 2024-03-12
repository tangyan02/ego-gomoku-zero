

#ifndef EGO_GOMOKU_ZERO_MODEL_H
#define EGO_GOMOKU_ZERO_MODEL_H

#include <vector>
#include <torch/script.h>
#include <torch/torch.h>

using namespace std;

class Model {
public:

    Model();

    void init(string model_path);

    std::pair<float, std::vector<float>> evaluate_state(vector<vector<vector<float>>> &state);

private:

    torch::jit::Module network;
    torch::Device device;

};


#endif //EGO_GOMOKU_ZERO_MODEL_H
