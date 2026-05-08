"""检查 conv1 第一层各输入通道的权重激活情况。
ch0-3 是迁移自 4ch 的旧权重，ch4-5 是新增的 VCT 种子通道（迁移时置 0）。
对比当前权重 vs 起点权重，看 ch4-5 是否真的被训练激活了。
"""
import torch
import os

CUR = "model/checkpoint.pth"
INIT = "model/backup_4ch_g129000/checkpoint.pth"

cur = torch.load(CUR, map_location="cpu", weights_only=False)
init = torch.load(INIT, map_location="cpu", weights_only=False)

cur_state = cur["model_state_dict"] if "model_state_dict" in cur else cur
init_state = init["model_state_dict"] if "model_state_dict" in init else init

# 找第一层 conv 的 weight key
key = None
for k in cur_state:
    if "conv1.weight" in k or k.endswith("conv.weight") or k == "conv1.0.weight":
        key = k
        break
if key is None:
    # 取第一个 4d 权重
    for k, v in cur_state.items():
        if v.ndim == 4 and v.shape[1] in (4, 6):
            key = k
            break

print(f"Conv1 weight key: {key}")
W_cur = cur_state[key]   # [out, in, k, k]
print(f"Current shape: {tuple(W_cur.shape)}")
print()

print("=== 当前权重各输入通道统计 (g142000, 6ch) ===")
print(f"{'ch':<4}{'mean':>12}{'std':>12}{'abs_mean':>12}{'max':>12}")
for ch in range(W_cur.shape[1]):
    w = W_cur[:, ch]
    print(f"{ch:<4}{w.mean().item():>12.6f}{w.std().item():>12.6f}{w.abs().mean().item():>12.6f}{w.abs().max().item():>12.6f}")

# 起点对比（4ch checkpoint）
W_init = None
if key in init_state:
    W_init = init_state[key]
else:
    # 起点是 4ch，key 可能不同；找第一个 [out, 4, k, k]
    for k, v in init_state.items():
        if v.ndim == 4 and v.shape[1] == 4:
            W_init = v
            print(f"(init key fallback: {k})")
            break

if W_init is not None:
    print()
    print("=== 起点权重 (g129000, 4ch) ===")
    print(f"{'ch':<4}{'mean':>12}{'std':>12}{'abs_mean':>12}{'max':>12}")
    for ch in range(W_init.shape[1]):
        w = W_init[:, ch]
        print(f"{ch:<4}{w.mean().item():>12.6f}{w.std().item():>12.6f}{w.abs().mean().item():>12.6f}{w.abs().max().item():>12.6f}")

    print()
    print("=== 变化量 (current - init, 仅对比 ch0-3) ===")
    print(f"{'ch':<4}{'delta_abs_mean':>16}{'delta_max':>14}")
    for ch in range(min(W_init.shape[1], W_cur.shape[1])):
        d = (W_cur[:, ch] - W_init[:, ch]).abs()
        print(f"{ch:<4}{d.mean().item():>16.6f}{d.max().item():>14.6f}")

# 单独突出 ch4/ch5
print()
print("=== ch4 / ch5 激活判定 ===")
if W_cur.shape[1] >= 6:
    for ch in [4, 5]:
        w = W_cur[:, ch]
        nonzero = (w.abs() > 1e-6).float().mean().item()
        print(f"ch{ch}: abs_mean={w.abs().mean().item():.6f}, max={w.abs().max().item():.6f}, "
              f"非零比例={nonzero*100:.1f}%, "
              f"{'✅ 已激活' if w.abs().mean().item() > 1e-4 else '⚠️ 仍接近 0'}")
