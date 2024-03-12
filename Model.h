

#ifndef EGO_GOMOKU_ZERO_MODEL_H
#define EGO_GOMOKU_ZERO_MODEL_H

#include <vector>
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <numeric>

using namespace std;

class Model {
public:

    Model();

    void init(string model_path);

    std::pair<float, std::vector<float>> evaluate_state(vector<vector<vector<float>>> &state);

private:
    Ort::Env *env;
    Ort::SessionOptions *sessionOptions;
    Ort::Session *session;
    Ort::MemoryInfo memoryInfo;

};


#endif //EGO_GOMOKU_ZERO_MODEL_H
