#include "utils.h"

torch::Device getDevice() {
    if (torch::cuda::is_available()) {
        return torch::kCUDA;
    } else {
        return torch::kCPU;
    }
}

bool fileExists(const std::string &filePath) {
    std::ifstream file(filePath);
    return file.good();
}