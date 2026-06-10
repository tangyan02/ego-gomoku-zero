#!/usr/bin/env python3
"""补跑评估：用已有 checkpoint 按顺序对基准模型评估，输出到 elo.log"""
import os
import sys
import json
import shutil
import glob

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from MainCpp import run_evaluate
from ConfigReader import ConfigReader
import Logger

ConfigReader.init()

eval_simulation = int(ConfigReader.get('evalSimulation') if 'evalSimulation' in ConfigReader.config else 100)
cppPathEval = ConfigReader.get("cppPathEval") if 'cppPathEval' in ConfigReader.config else ConfigReader.get("cppPath")
eval_interval = int(ConfigReader.get('evalInterval') if 'evalInterval' in ConfigReader.config else 1000)

# 找到所有 checkpoint（按对局数排序）
checkpoints = []
for f in glob.glob("model/checkpoint_g*.onnx"):
    try:
        g = int(f.replace("model/checkpoint_g", "").replace(".onnx", ""))
        checkpoints.append(g)
    except ValueError:
        pass
checkpoints.sort()

print(f"找到 {len(checkpoints)} 个 checkpoint: g{checkpoints[0]} ~ g{checkpoints[-1]}")
print(f"评估间隔: {eval_interval}, 模拟次数: {eval_simulation}")

# 只评估整千的 checkpoint
eval_points = [g for g in checkpoints if g % eval_interval == 0 and g > 0]
print(f"待评估节点: {len(eval_points)} 个")

# 清空 elo.log
open("log/elo.log", "w").close()

# 初始基准模型
baseline_games = 0
baseline_onnx = "model/checkpoint_g0.onnx"
best_path = "model/model_best.onnx"

results = []
for i, g in enumerate(eval_points):
    current_path = f"model/checkpoint_g{g}.onnx"
    if not os.path.exists(current_path):
        continue

    print(f"\n[{i+1}/{len(eval_points)}] g{g} vs baseline g{baseline_games} ...")
    eval_result = run_evaluate(cppPathEval, current_path, baseline_onnx, -1, eval_simulation)

    if not eval_result.get("valid", True):
        Logger.infoD(f"⚠️ g{g} 评估无效，跳过", "elo.log")
        continue

    baseline_upgraded = eval_result["win_rate"] >= 0.80
    entry = {
        "total_games": g,
        "vs_baseline": baseline_games,
        "elo_diff": eval_result["elo_diff"],
        "win_rate": eval_result["win_rate"],
        "wins": eval_result["wins"],
        "losses": eval_result["losses"],
        "draws": eval_result["draws"],
        "baseline_upgraded": baseline_upgraded
    }
    results.append(entry)

    Logger.infoD(
        f"Elo 评估完成: g{g} vs baseline g{baseline_games} → "
        f"胜率 {eval_result['win_rate'] * 100:.1f}%, Elo {eval_result['elo_diff']:+.0f}"
        f"{' ★ 基准升格' if baseline_upgraded else ''}",
        "elo.log"
    )
    Logger.infoD(json.dumps(entry), "elo.log")

    # 胜率 >= 50%：更新 best
    if eval_result["win_rate"] >= 0.50:
        shutil.copy2(current_path, best_path)
        current_pt = current_path.replace('.onnx', '.pt')
        if os.path.exists(current_pt):
            shutil.copy2(current_pt, best_path.replace('.onnx', '.pt'))

    # 胜率 >= 75%：升格基准
    if baseline_upgraded:
        baseline_onnx = current_path
        baseline_games = g
        Logger.infoD(f"✅ 基准模型升格为 g{g}", "elo.log")
        print(f"  ★ 基准升格到 g{g}")

# 持久化最终基准
with open("model/baseline.txt", "w") as f:
    f.write(str(baseline_games))

print(f"\n补评完成! 共 {len(results)} 条记录，最终基准: g{baseline_games}")
