
#include "IModel.h"

// ========== 工厂函数：编译期选择后端 ==========

#ifdef USE_LIBTORCH
#include "TorchBackend.h"
#else
#include "ONNXBackend.h"
#endif

std::unique_ptr<IModel> createModel() {
#ifdef USE_LIBTORCH
    return std::make_unique<TorchBackend>();
#else
    return std::make_unique<ONNXBackend>();
#endif
}
