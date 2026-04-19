
#ifndef EGO_GOMOKU_ZERO_IMODEL_H
#define EGO_GOMOKU_ZERO_IMODEL_H

#include <vector>
#include <string>
#include <future>
#include <memory>
#include <utility>

// ========== 推理后端接口 ==========
class IModel {
public:
    virtual ~IModel() = default;

    virtual void init(const std::string& modelPath, const std::string& coreType) = 0;

    // 单条推理（嵌套 vector 版，兼容旧接口）
    virtual std::pair<float, std::vector<float>>
        evaluate_state(std::vector<std::vector<std::vector<float>>>& data) = 0;

    // 单条推理（连续内存版，高性能）
    virtual std::pair<float, std::vector<float>>
        evaluate_state(const float* data, int channels, int height, int width) = 0;

    // 批量推理
    virtual std::vector<std::pair<float, std::vector<float>>>
        evaluate_state_batch(const std::vector<std::vector<std::vector<std::vector<float>>>>& batchData) = 0;

    // 异步入队（用于批推理攒批）
    virtual std::future<std::pair<float, std::vector<float>>>
        enqueueData(std::vector<std::vector<std::vector<float>>> data) = 0;
};

// ========== 工厂函数声明 ==========
std::unique_ptr<IModel> createModel();

#endif //EGO_GOMOKU_ZERO_IMODEL_H
