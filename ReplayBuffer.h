
#ifndef EGO_GOMOKU_ZERO_REPLAYBUFFER_H
#define EGO_GOMOKU_ZERO_REPLAYBUFFER_H

#include <vector>
#include <random>
#include <torch/torch.h>

using Sample = std::tuple<torch::Tensor, std::vector<float>, std::vector<float>>;

class ReplayBuffer : public torch::data::datasets::Dataset<ReplayBuffer, Sample> {
public:
    ReplayBuffer(size_t max_size = 0);

    void add_samples(const std::vector<Sample>& new_samples);
    torch::optional<size_t> size() const;
    Sample get(size_t index);

private:
    std::vector<Sample> training_data;
    size_t max_size;
};


#endif //EGO_GOMOKU_ZERO_REPLAYBUFFER_H
