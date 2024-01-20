#include "Utils.h"

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

bool directoryExists(const char *dirPath) {
#ifdef _WIN32
    struct _stat info;
    if (_stat(dirPath, &info) != 0) {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else
    struct stat info;
    if (stat(dirPath, &info) != 0) {
        return false;
    }
    return S_ISDIR(info.st_mode);
#endif
}


void createDirectory(const char *dirPath) {

    if (!directoryExists(dirPath)) {
#ifdef _WIN32
        int result = _mkdir(dirPath);  // Windows
#else
        int result = mkdir(dirPath, 0777);  // Linux/Unix
#endif

        if (result == 0) {
            std::cout << "Created '" << dirPath << "' directory." << std::endl;
        } else {
            std::cout << "Failed to create '" << dirPath << "' directory." << std::endl;
        }
    } else {
        std::cout << "'" << dirPath << "' directory already exists." << std::endl;
    }
}