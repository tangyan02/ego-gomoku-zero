#!/usr/bin/env python3
"""补评指定范围的 checkpoint"""
import sys, json, shutil, os, subprocess, platform

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ConfigReader import ConfigReader
import Logger

ConfigReader.init()
cppPathEval = ConfigReader.get("cppPathEval")
eval_simulation = int(ConfigReader.get("evalSimulation"))
cpp_dir = os.path.dirname(os.path.abspath(cppPathEval))

baseline_games = int(open("model/baseline.txt").read().strip())
baseline_onnx = os.path.abspath(f"model/checkpoint_g{baseline_games}.onnx")

start_g = int(sys.argv[1]) if len(sys.argv) > 1 else 154000
end_g = int(sys.argv[2]) if len(sys.argv) > 2 else 163000
step = int(sys.argv[3]) if len(sys.argv) > 3 else 1000

print(f"补评 g{start_g}~g{end_g} vs baseline g{baseline_games}")

for g in range(start_g, end_g, step):
    current_path = os.path.abspath(f"model/checkpoint_g{g}.onnx")
    if not os.path.exists(current_path):
        print(f"跳过 g{g}（文件不存在）")
        continue

    # 写临时配置
    conf_path = os.path.join(cpp_dir, "application.conf")
    backup_conf = conf_path + ".bak"
    if os.path.exists(conf_path):
        shutil.copy2(conf_path, backup_conf)

    with open(conf_path, 'w') as f:
        f.write(f"mode=evaluate\n")
        f.write(f"coreType={ConfigReader.get('coreType')}\n")
        f.write(f"boardSize={ConfigReader.get('boardSize')}\n")
        f.write(f"explorationFactor={ConfigReader.get('explorationFactor')}\n")
        f.write(f"evalModelPath1={current_path}\n")
        f.write(f"evalModelPath2={baseline_onnx}\n")
        f.write(f"evalGames=-1\n")
        f.write(f"evalSimulation={eval_simulation}\n")

    env = os.environ.copy()
    lib_dir = os.path.join(cpp_dir, '..', 'onnxruntime', 'lib')
    lib_key = 'DYLD_LIBRARY_PATH' if platform.system() == 'Darwin' else 'LD_LIBRARY_PATH'
    env[lib_key] = lib_dir + (':/usr/local/cuda/lib64' if platform.system() == 'Linux' else '') + ':' + env.get(lib_key, '')

    print(f"评估 g{g} vs g{baseline_games} ...", flush=True)
    process = subprocess.Popen([os.path.abspath(cppPathEval)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=cpp_dir, env=env)
    output_lines = []
    for line in process.stdout:
        decoded = line.decode()
        output_lines.append(decoded)
    process.wait()

    # 恢复配置
    if os.path.exists(backup_conf):
        shutil.copy2(backup_conf, conf_path)
        os.remove(backup_conf)

    # 解析结果
    result = {"wins": 0, "losses": 0, "draws": 0, "win_rate": 0.0, "elo_diff": 0.0}
    for line in output_lines:
        if "Win rate:" in line:
            result["win_rate"] = float(line.split("Win rate:")[1].strip().replace("%", "")) / 100
        elif "Elo diff:" in line:
            result["elo_diff"] = float(line.split("Elo diff:")[1].strip())
        elif "Wins:" in line and "Losses:" in line:
            parts = line.split("|")
            for part in parts:
                part = part.strip()
                if "Wins:" in part:
                    result["wins"] = int(part.split("Wins:")[1].strip())
                elif "Losses:" in part:
                    result["losses"] = int(part.split("Losses:")[1].strip())
                elif "Draws:" in part:
                    result["draws"] = int(part.split("Draws:")[1].strip())

    total = result["wins"] + result["losses"] + result["draws"]
    if total == 0:
        print(f"  ❌ 无效（0局）")
        continue

    upgraded = result["win_rate"] >= 0.80
    entry = {
        "total_games": g, "vs_baseline": baseline_games,
        "elo_diff": result["elo_diff"], "win_rate": result["win_rate"],
        "wins": result["wins"], "losses": result["losses"], "draws": result["draws"],
        "baseline_upgraded": upgraded
    }
    wr = result["win_rate"] * 100
    elo = result["elo_diff"]
    tag = " ★ 基准升格" if upgraded else ""
    Logger.infoD(f"Elo 评估完成: g{g} vs baseline g{baseline_games} → 胜率 {wr:.1f}%, Elo {elo:+.0f}{tag}", "elo.log")
    Logger.infoD(json.dumps(entry), "elo.log")

    if result["win_rate"] >= 0.50:
        shutil.copy2(current_path, "model/model_best.onnx")
        pt = current_path.replace(".onnx", ".pt")
        if os.path.exists(pt):
            shutil.copy2(pt, "model/model_best.pt")

    if upgraded:
        baseline_onnx = current_path
        baseline_games = g
        open("model/baseline.txt", "w").write(str(g))
        Logger.infoD(f"✅ 基准模型升格为 g{g}", "elo.log")
        print(f"  ★ 升格到 g{g}")

    print(f"  胜率 {wr:.0f}% Elo {elo:+.0f}")

print("补评完成")
