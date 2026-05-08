"""Sanity check: 验证 6ch 网络可以正确加载迁移后的权重，能前向计算并保存为 onnx/pt"""
import sys
import os
import torch
import numpy as np

# 添加 train 目录到 path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from Network import PolicyValueNetwork, get_model, save_model


def check():
    # 设备：MPS / CUDA / CPU
    if torch.backends.mps.is_available():
        device = torch.device("mps")
    elif torch.cuda.is_available():
        device = torch.device("cuda")
    else:
        device = torch.device("cpu")
    print(f"[SanityCheck] 使用设备: {device}")

    # 加载（走 get_model 的标准路径，会自动加载 checkpoint.pth）
    print("[SanityCheck] 加载迁移后的权重...")
    network, optimizer = get_model(device, lr=5e-5, wd=1e-4)
    network.eval()

    # 检查第一层 conv 的 shape
    conv1_w = network.conv1.weight.data
    print(f"[SanityCheck] conv1.weight shape: {tuple(conv1_w.shape)}")
    assert conv1_w.shape[1] == 6, f"期望 6 通道，实际 {conv1_w.shape[1]}"

    # 检查 ch4/ch5 是否为 0
    ch4_norm = conv1_w[:, 4, :, :].abs().sum().item()
    ch5_norm = conv1_w[:, 5, :, :].abs().sum().item()
    print(f"[SanityCheck] ch4 权重 abs sum: {ch4_norm:.6f} (期望 0)")
    print(f"[SanityCheck] ch5 权重 abs sum: {ch5_norm:.6f} (期望 0)")
    assert ch4_norm < 1e-6 and ch5_norm < 1e-6, "ch4/ch5 应为 0"

    # 检查 ch0-ch3 权重保留（与备份对比）
    backup = torch.load("model/backup_4ch_g129000/checkpoint.pth", map_location='cpu')
    backup_w = backup['model_state_dict']['conv1.weight']  # [128, 4, 3, 3]
    diff = (conv1_w[:, :4, :, :].cpu() - backup_w).abs().max().item()
    print(f"[SanityCheck] ch0-3 权重与原版差异: {diff:.6f} (期望 0)")
    assert diff < 1e-6, "ch0-3 权重应该保留"

    # 前向测试
    print("[SanityCheck] 前向测试...")
    dummy = torch.randn(2, 6, 20, 20, device=device)
    with torch.no_grad():
        value, policy = network(dummy)
    print(f"[SanityCheck] value shape: {value.shape}, range: [{value.min():.3f}, {value.max():.3f}]")
    print(f"[SanityCheck] policy shape: {policy.shape} (log_softmax)")

    # 验证 ch4/ch5 全 0 时输出与 4ch 网络在 ch0-3 相同输入下输出一致
    # （因为新通道权重为 0，对结果无影响）
    dummy_ch_zero = dummy.clone()
    dummy_ch_zero[:, 4:, :, :] = 0
    dummy_ch_random = dummy.clone()
    dummy_ch_random[:, 4:, :, :] = torch.randn_like(dummy_ch_zero[:, 4:, :, :])
    with torch.no_grad():
        v0, p0 = network(dummy_ch_zero)
        v1, p1 = network(dummy_ch_random)
    v_diff = (v0 - v1).abs().max().item()
    p_diff = (p0 - p1).abs().max().item()
    print(f"[SanityCheck] ch4/5 不同输入下，value 差异: {v_diff:.6f}, policy 差异: {p_diff:.6f}")
    print(f"[SanityCheck] 期望：差异为 0（因为 ch4/5 权重为 0）")
    assert v_diff < 1e-5, "新通道应不影响 value 输出"
    assert p_diff < 1e-5, "新通道应不影响 policy 输出"

    # 保存（生成新 onnx 和 pt）
    print("[SanityCheck] 保存 model/model_latest.onnx 和 .pt ...")
    save_model(network, optimizer)

    print("[SanityCheck] ✅ 所有检查通过")


if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    check()
