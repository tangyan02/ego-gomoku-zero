#include "ReplayBuffer.h"

ReplayBuffer::ReplayBuffer(size_t max_size) : max_size(max_size) {}

void ReplayBuffer::add_samples(const std::vector<Sample>& new_samples) {
    training_data.insert(training_data.end(), new_samples.begin(), new_samples.end());
    if (max_size != 0 && training_data.size() > max_size) {
        training_data.erase(training_data.begin(), training_data.begin() + (training_data.size() - max_size));
    }
}

torch::optional<size_t> ReplayBuffer::size() const {
    return training_data.size();
}

Sample ReplayBuffer::get(size_t index) {
    return training_data[index];
}