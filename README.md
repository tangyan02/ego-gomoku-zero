# Ego-Gomoku-Zero

A Gomoku (Five in a Row) AI based on the **AlphaZero** algorithm, featuring a closed-loop pipeline of self-play, training, and inference on a **20×20** board.

- **C++ Engine**: Game logic, MCTS search, neural network inference via ONNX Runtime
- **Python Training**: Model definition, training, and ONNX export via PyTorch
- **Competition Ready**: Supports the [Piskvork](http://sourceforge.net/projects/piskvork) protocol for AI tournaments

## Architecture

```
┌──────────────┐   record/data_*.txt   ┌──────────────┐
│  C++ Engine   │ ────────────────────▶ │ Python Train  │
│  (Self-Play)  │                       │ (MainCpp.py)  │
│               │ ◀──────────────────── │               │
└──────────────┘   model_latest.onnx   └──────────────┘
```

### Training Loop

1. **Self-Play** (C++): Multiple threads play games using MCTS + neural network, generating training data
2. **Data Augmentation** (Python): 8× augmentation via rotation and flipping
3. **Training** (Python): Update the PolicyValueNetwork with MSE + cross-entropy loss
4. **Export** (Python): Save model as ONNX for C++ inference
5. Repeat

## Key Features

| Feature | Description |
|---------|-------------|
| **MCTS + PUCT** | AlphaZero-style Monte Carlo Tree Search with PUCT selection formula |
| **10-Block ResNet** | PolicyValueNetwork with 128 channels, dual-head (policy + value) |
| **Shape Lookup Table** | O(1) pattern recognition via precomputed hash table for all board shapes |
| **VCF Search** | Victory by Continuous Fours — deterministic winning move detection |
| **VCT Search** | Victory by Continuous Threats — iterative deepening threat search (parallel with MCTS) |
| **Opening Book** | 108 standard opening variations for diverse self-play |
| **Tree Reuse** | Preserves MCTS subtree across moves, pruning irrelevant branches |
| **Time Management** | Smart early termination in Piskvork mode based on visit count estimation |

## Project Structure

```
├── Main.cpp              # Entry point: train / predict / test modes
├── Game.h/cpp            # Board state, move logic, win detection
├── MCTS.h/cpp            # Monte Carlo Tree Search (PUCT + Dirichlet noise)
├── Model.h/cpp           # ONNX Runtime inference (CPU / CUDA / CoreML / TensorRT)
├── Analyzer.h/cpp        # Tactical engine: selectActions, VCF, VCT search
├── Shape.h/cpp           # Precomputed shape lookup table (O(1) pattern matching)
├── SelfPlay.h/cpp        # Self-play data generation with parallel VCT
├── Bridge.h/cpp          # stdin/stdout text protocol for Python integration
├── Piskvork.cpp          # Piskvork tournament protocol (Windows)
├── Pisqpipe.h/cpp        # Piskvork pipe communication layer
├── ConfigReader.h/cpp    # Configuration file parser (application.conf)
├── Utils.h/cpp           # Utility functions
├── test/                 # Unit tests (doctest framework, 36 test cases)
│   ├── AnalyzerTest.cpp
│   ├── GameTest.cpp
│   ├── MCTSTest.cpp
│   └── ShapeTest.cpp
└── train/                # Python training pipeline
    ├── MainCpp.py        # Main training loop: self-play → train → export
    ├── Network.py        # PolicyValueNetwork (10-block ResNet, 128ch)
    ├── Train.py          # Training logic (AdamW, MSE + CE loss)
    ├── Bridge.py         # Read self-play data, invoke C++ engine
    ├── Server.py         # Flask server for distributed self-play
    ├── WatchAgent.py     # Interactive game UI (Pygame) for debugging
    ├── GameUI.py         # Pygame board renderer with probability heatmap
    ├── ConfigReader.py   # Python config reader
    ├── SampleSet.py      # PyTorch Dataset wrapper
    ├── Logger.py         # Logging utility
    └── Utils.py          # Device detection, directory setup
```

## Dependencies

| Dependency          | Version       |
|---------------------|---------------|
| C++                 | 17            |
| Python              | 3.8.17        |
| PyTorch             | 1.13.1+cu117  |
| ONNX Runtime (C++)  | 1.17.1+       |
| CUDA (optional)     | 11.7          |
| CMake               | ≥ 3.18        |

## Build & Run

### Build

```bash
# Set ONNX Runtime path
export ONNXRUNTIME_ROOTDIR=/path/to/onnxruntime

# Build
./make.sh
```

### Configuration

Edit `train/application.conf`:

```ini
mode=train                    # train / predict
coreType=apple                # apple / cuda / cpu
boardSize=20
numGames=100                  # games per episode
numSimulation=200             # MCTS simulations per move
explorationFactor=5           # PUCT exploration constant
numProcesses=2                # parallel self-play threads
lr=3e-4
wd=1e-4
batchSize=256
episode=100000
temperatureDefault=1
temperatureDownBeginStep=4    # step to switch temperature to 0
randomRate=0.9                # probability of using opening book
modelPath=./model/model_latest.onnx
```

### Run Training

```bash
cd train
python MainCpp.py
```

### Run Tests

Set `mode` to anything other than `train` or `predict`, then run the executable. All 36 doctest test cases should pass.

### Piskvork (Windows)

Build on Windows and load the executable in [Piskvork](http://sourceforge.net/projects/piskvork) as a player.

## Neural Network

```
Input: [batch, 2, 20, 20]  (current player + opponent channels)
  │
  Conv2d(2→128, 3×3) + ReLU
  │
  10 × ResidualBlock(128)
  ┌──────┴──────┐
Policy Head    Value Head
Conv1×1(→4)    Conv1×1(→2)
BN + ReLU      BN + ReLU
FC → 400       FC → 64 → 1
LogSoftmax     Tanh
  │              │
[400]          [-1, 1]
```

## Author

**TangYan** — tangyan1412@foxmail.com
