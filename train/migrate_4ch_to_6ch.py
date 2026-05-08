"""
权重迁移脚本：4 通道模型 → 6 通道模型

策略：
- 第一层 conv1: 4ch → 6ch
  - 旧权重 [128, 4, 3, 3] 复制到新权重 [128, 6, 3, 3] 的 [:, :4, :, :]
  - 新通道 ch4/ch5 的权重置 0（让网络从头学习）
- 其他层：完全保留

用法：
  python migrate_4ch_to_6ch.py model/backup_4ch_g129000/checkpoint.pth model/checkpoint.pth
"""
import sys
import os
import torch
import shutil


def migrate(src_pth, dst_pth):
    print(f"[Migrate] 加载源权重: {src_pth}")
    checkpoint = torch.load(src_pth, map_location='cpu')

    src_state = checkpoint['model_state_dict']

    # 找第一层 conv 权重
    conv1_key = 'conv1.weight'
    if conv1_key not in src_state:
        print(f"[Migrate] ERROR: 未找到 {conv1_key}")
        return False

    src_w = src_state[conv1_key]
    print(f"[Migrate] 源 conv1.weight shape: {tuple(src_w.shape)}")

    if src_w.shape[1] == 6:
        print("[Migrate] 源已经是 6 通道，无需迁移")
        return True

    if src_w.shape[1] != 4:
        print(f"[Migrate] ERROR: 源不是 4 通道，无法迁移（实际 {src_w.shape[1]}）")
        return False

    out_ch = src_w.shape[0]
    new_w = torch.zeros(out_ch, 6, src_w.shape[2], src_w.shape[3], dtype=src_w.dtype)
    new_w[:, :4, :, :] = src_w
    # ch4/ch5 权重保持 0：网络从 0 学习新通道的影响
    print(f"[Migrate] 新 conv1.weight shape: {tuple(new_w.shape)} (ch4/ch5 初始化为 0)")

    # 替换权重
    src_state[conv1_key] = new_w

    # 注意：optimizer state 中也可能保存了 conv1 相关的 momentum/variance，但形状不匹配
    # 直接丢弃 optimizer 状态，让新优化器从头开始（简单稳妥）
    if 'optimizer_state_dict' in checkpoint:
        print("[Migrate] 丢弃 optimizer_state_dict（避免形状不匹配）")
        del checkpoint['optimizer_state_dict']

    # 备份目标（如果存在）
    if os.path.exists(dst_pth):
        backup = dst_pth + '.before_migrate'
        shutil.copy2(dst_pth, backup)
        print(f"[Migrate] 备份原目标到: {backup}")

    print(f"[Migrate] 保存到: {dst_pth}")
    torch.save(checkpoint, dst_pth)
    print("[Migrate] ✅ 迁移完成")
    return True


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("用法: python migrate_4ch_to_6ch.py <src.pth> <dst.pth>")
        sys.exit(1)
    src, dst = sys.argv[1], sys.argv[2]
    if not os.path.exists(src):
        print(f"源文件不存在: {src}")
        sys.exit(1)
    success = migrate(src, dst)
    sys.exit(0 if success else 1)
