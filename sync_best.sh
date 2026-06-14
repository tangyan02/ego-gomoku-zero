#!/bin/bash
# 从 Linux 服务器下载 best 模型覆盖本地
REMOTE="kaeltang@9.134.253.12"
PORT=36000
REMOTE_DIR="~/ego-gomoku-zero/train/model"
LOCAL_DIR="$(cd "$(dirname "$0")" && pwd)/train/model"

echo "下载 model_best.onnx ..."
scp -P $PORT $REMOTE:$REMOTE_DIR/model_best.onnx $LOCAL_DIR/model_best.onnx && echo "✅ model_best.onnx 已更新" || echo "❌ 下载失败"
