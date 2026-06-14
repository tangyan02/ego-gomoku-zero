#!/usr/bin/env python3
"""整理 elo.log：去重、排序、删除指定条目"""
import json

remove_games = set()

entries = []
with open("log/elo.log") as f:
    for line in f:
        idx = line.find("{")
        if idx >= 0 and "elo_diff" in line:
            try:
                e = json.loads(line[idx:])
                entries.append(e)
            except json.JSONDecodeError:
                pass

before = len(entries)
# 去重 + 排序
seen = set()
unique = []
for e in sorted(entries, key=lambda x: x["total_games"]):
    g = e["total_games"]
    if g not in seen and g not in remove_games:
        seen.add(g)
        unique.append(e)

# 重写
with open("log/elo.log", "w") as f:
    for e in unique:
        g = e["total_games"]
        vs = e.get("vs_baseline", "?")
        wr = e["win_rate"] * 100
        elo = e["elo_diff"]
        tag = " ★ 基准升格" if e.get("baseline_upgraded") else ""
        f.write("Elo: g%d vs g%s → %.1f%% %+.0f%s\n" % (g, vs, wr, elo, tag))
        f.write(json.dumps(e) + "\n")
        if e.get("baseline_upgraded"):
            f.write("✅ 基准模型升格为 g%d\n" % g)

first_g = unique[0]["total_games"] if unique else 0
last_g = unique[-1]["total_games"] if unique else 0
print("整理完成: %d 条（原 %d 条，删除 g%s）" % (len(unique), before, ",".join(str(g) for g in remove_games)))
print("范围: g%d ~ g%d" % (first_g, last_g))

# 检查缺失
missing = []
for i in range(1, len(unique)):
    gap = unique[i]["total_games"] - unique[i-1]["total_games"]
    if gap > 1000:
        for mg in range(unique[i-1]["total_games"] + 1000, unique[i]["total_games"], 1000):
            missing.append(mg)
if missing:
    print("缺失: " + ",".join("g%d" % g for g in missing))
else:
    print("无缺失 ✅")
