
#ifndef EGO_GOMOKU_ZERO_MODEL_H
#define EGO_GOMOKU_ZERO_MODEL_H

#include "IModel.h"

// ========== Model 类（兼容别名，保持现有代码无需改动） ==========
// 编译期选择后端：USE_LIBTORCH 时用 TorchBackend，否则用 ONNXBackend
#ifdef USE_LIBTORCH
#include "TorchBackend.h"
using Model = TorchBackend;
#else
#include "ONNXBackend.h"
using Model = ONNXBackend;
#endif

// ========== using namespace 兼容（原有代码 using namespace std） ==========
using namespace std;

#endif //EGO_GOMOKU_ZERO_MODEL_H
