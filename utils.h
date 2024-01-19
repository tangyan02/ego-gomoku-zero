//
// Created by 唐雁 on 2024/1/17.
//

#ifndef EGO_GOMOKU_ZERO_UTILS_H
#define EGO_GOMOKU_ZERO_UTILS_H

#include <torch/torch.h>
#include <torch/nn.h>
#include <torch/optim.h>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <direct.h>  // Windows
#else
#include <sys/stat.h>  // Linux/Unix
#endif

torch::Device getDevice();

bool fileExists(const std::string &filePath);

void createModelDirectory();

#endif //EGO_GOMOKU_ZERO_UTILS_H
