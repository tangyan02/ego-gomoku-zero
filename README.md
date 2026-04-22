# EGO-Gomoku-Zero

AlphaZero 风格的五子棋（20×20）自我对弈训练系统。

## 架构

- **C++ 端**：MCTS 搜索 + 自对弈数据生成 + 棋型分析（VCF/VCT）
- **Python 端**：神经网络训练（PyTorch）+ 训练流程管理
- **双推理后端**：
  - ONNX Runtime（默认，跨平台，用于评估/对弈）
  - libtorch + MPS（macOS 专用，用于自对弈加速）

## 网络结构

```
输入: 4ch × 20 × 20 (己方棋子, 对方棋子, 我方VCF点, 对方VCF点)
  → Conv3×3 + BN → 128ch
  → 10× ResBlock (后2块带 SE 注意力)
  → Policy Head: Conv1×1→8ch, FC→256→400, log_softmax
  → Value Head: Conv1×1→32ch, GAP→32→1, tanh
```

参数量约 3M，FP32 ONNX ~12MB。

## 依赖

### ONNX Runtime (必须)

下载 [onnxruntime](https://github.com/microsoft/onnxruntime/releases)，解压后设置环境变量：

```bash
# macOS arm64
wget https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-osx-arm64-1.22.0.tgz
tar xzf onnxruntime-osx-arm64-1.22.0.tgz
export ONNXRUNTIME_ROOTDIR=$(pwd)/onnxruntime-osx-arm64-1.22.0
```

### libtorch (可选，macOS MPS 加速)

仅 macOS Apple Silicon 自对弈需要，不影响评估和对弈。

```bash
# libtorch 2.6.0 macOS arm64
wget https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.6.0.zip
unzip libtorch-macos-arm64-2.6.0.zip
export LIBTORCH_ROOTDIR=$(pwd)/libtorch
```

### Python 依赖

```bash
pip install torch numpy
```

## 构建

### ONNX 后端（默认，用于评估/对弈）

```bash
mkdir cmake-build-debug && cd cmake-build-debug
cmake ..
cmake --build . -j
```

### libtorch + MPS 后端（macOS 自对弈加速）

```bash
mkdir cmake-build-torch && cd cmake-build-torch
LIBTORCH_ROOTDIR=/path/to/libtorch cmake .. -DUSE_LIBTORCH=ON
cmake --build . -j
```

运行需要设置动态库路径：

```bash
# 创建 run.sh
echo '#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
DYLD_LIBRARY_PATH="$DIR/../libtorch/lib" "$DIR/ego-gomoku-zero" "$@"' > run.sh
chmod +x run.sh
```

## 训练

```bash
cd train
python3 MainCpp.py
```

训练配置在 `train/application.conf`，主要参数：

| 参数 | 默认值 | 说明 |
|---|---|---|
| `boardSize` | 20 | 棋盘大小 |
| `numSimulation` | 200 | MCTS 模拟次数 |
| `numGames` | 100 | 每 episode 自对弈局数 |
| `lr` | 5e-5 | 学习率 |
| `batchSize` | 256 | 训练批大小 |
| `trainEpochs` | 2 | 每 episode 训练轮数 |
| `replayBufferSize` | 300000 | 经验回放池容量 |
| `tdN` / `tdGamma` | 5 / 0.7 | N-step TD bootstrapping |
| `evalInterval` | 1000 | Elo 评估间隔（局数） |

### 训练流程

```
自对弈(MPS) → 数据增强(8倍) → 经验回放采样(偏向新数据70%) → 训练 → 循环
                                                                  ↓
                                                    每1000局: Elo评估(ONNX)
                                                    每2000局: 刷新平衡开局库
```

### 模型文件

| 文件 | 说明 |
|---|---|
| `model_latest.onnx/pt` | 最新训练模型，自对弈使用 |
| `model_best.onnx/pt` | 最后一次 Elo 上升的模型 |
| `model_backup.onnx/pt` | latest 的隐式备份 |
| `checkpoint_gXXXX.*` | 历史快照（每 1000 局） |

## 运行模式

| 模式 | 说明 |
|---|---|
| `train` | 自对弈生成训练数据 |
| `evaluate` | 两个模型对弈评估 |
| `generate_openings` | 生成平衡开局库 |
| `predict` | 通过 Bridge 启动对局 |
| Windows | 自动进入 Piskvork 协议 |

## 开局库

系统维护两类开局：

- **生成开局**（`openings_train.txt` / `openings_eval.txt`）：C++ 端随机生成，用模型双视角评估筛选平衡局面，渐进 threshold (0.3→0.4→0.5→0.6)
- **手工开局**（`openings_manual.txt`）：比赛数据

自对弈选取策略：10% 空棋盘 + 45% 生成开局 + 45% 手工开局。

## 可视化

```bash
cd train
python3 openings_viewer.py
# 浏览器打开 http://127.0.0.1:8765 查看开局库
```
