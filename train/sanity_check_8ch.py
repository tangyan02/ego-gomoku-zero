"""8ch 模型 sanity check：实例化网络 + 前向一次 + 导出 onnx/pt"""
import os
import sys
import torch

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from Network import PolicyValueNetwork, get_model, save_model
from Utils import getDevice, dirPreBuild


def main():
    dirPreBuild()
    device = getDevice()
    print(f"[SanityCheck] device = {device}")

    network, optimizer = get_model(device, lr=5e-5, wd=1e-4)
    network.eval()

    # 确认第一层输入通道 = 8
    first_conv = network.conv1.weight
    print(f"[SanityCheck] conv1.weight shape = {tuple(first_conv.shape)} (expect [128, 8, 3, 3])")
    assert first_conv.shape[1] == 8, "conv1 input channel != 8"

    # dummy 前向
    dummy = torch.zeros(1, 8, 20, 20, device=device)
    with torch.no_grad():
        v, logits = network(dummy)
    print(f"[SanityCheck] forward ok: value.shape={tuple(v.shape)} logits.shape={tuple(logits.shape)}")

    # 保存 onnx + pt
    save_model(network, optimizer)
    print(f"[SanityCheck] saved model/checkpoint.pth / model_latest.onnx / model_latest.pt")

    # 读 onnx 确认输入形状
    import onnx
    onnx_path = "model/model_latest.onnx"
    m = onnx.load(onnx_path)
    in_shape = [d.dim_value for d in m.graph.input[0].type.tensor_type.shape.dim]
    print(f"[SanityCheck] onnx input shape = {in_shape}  (expect dims at ch=8)")

    print("[SanityCheck] ✅ all checks passed")


if __name__ == "__main__":
    main()
