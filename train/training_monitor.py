#!/usr/bin/env python3
"""训练监控面板 — 实时每步推送自对弈棋局 + 统计图表"""

import http.server
import json
import os
import re
import subprocess
import threading
import time
import webbrowser
import queue

PORT = 8766
TRAIN_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_DIR = os.path.join(TRAIN_DIR, "log")
LOG_PATH = os.path.join(LOG_DIR, "train_stdout.log")

# SSE 客户端队列
sse_clients = []
sse_lock = threading.Lock()

# 当前正在进行的对局状态
live_game = {"id": "", "moves": [], "winner": 0, "opening": "", "active": False}
live_lock = threading.Lock()

# 当前评估状态
eval_state = {"active": False, "m1_wins": 0, "m2_wins": 0, "draws": 0,
              "current": 0, "total": 0, "games": [], "info": ""}


def broadcast_sse(event, data):
    """向所有 SSE 客户端推送事件"""
    msg = f"event: {event}\ndata: {json.dumps(data)}\n\n"
    with sse_lock:
        dead = []
        for q in sse_clients:
            try:
                q.put_nowait(msg)
            except queue.Full:
                dead.append(q)
        for q in dead:
            sse_clients.remove(q)


def _backfill_eval_state():
    """启动时回扫日志尾部，恢复当前正在进行的评估状态"""
    if not os.path.exists(LOG_PATH):
        return
    
    # 读最后 2000 行（足够覆盖 200 局评估 + 一些自对弈日志）
    try:
        result = subprocess.run(
            ["tail", "-n", "2000", LOG_PATH],
            capture_output=True, text=True, timeout=5
        )
        lines = result.stdout.splitlines()
    except Exception:
        return
    
    # 从后往前找最近一次 "开始 Elo 评估" 或 elo_diff 结果
    eval_start_idx = -1
    eval_done = False
    for i in range(len(lines) - 1, -1, -1):
        if '"elo_diff"' in lines[i]:
            eval_done = True
            break
        if "开始 Elo 评估:" in lines[i]:
            eval_start_idx = i
            break
    
    # 如果找到了一个正在进行的评估（有 start 没有 done）
    if eval_start_idx >= 0 and not eval_done:
        eval_state["active"] = True
        eval_state["info"] = lines[eval_start_idx].strip()
        eval_state["m1_wins"] = 0
        eval_state["m2_wins"] = 0
        eval_state["draws"] = 0
        eval_state["games"] = []
        
        for line in lines[eval_start_idx + 1:]:
            em = re.match(
                r'\[Evaluate\] Game (\d+)/(\d+) Opening (\d+) \((M1=\w+)\): (M\d WIN) \((\d+)ms\)',
                line.strip()
            )
            if em:
                game_data = {
                    "current": int(em.group(1)),
                    "total": int(em.group(2)),
                    "opening": int(em.group(3)),
                    "side": em.group(4),
                    "result": em.group(5),
                    "time_ms": int(em.group(6))
                }
                eval_state["current"] = game_data["current"]
                eval_state["total"] = game_data["total"]
                if game_data["result"] == "M1 WIN":
                    eval_state["m1_wins"] += 1
                else:
                    eval_state["m2_wins"] += 1
                eval_state["games"].append(game_data)
        
        print(f"[Monitor] 回扫到正在进行的评估: {eval_state['current']}/{eval_state['total']} "
              f"(M1:{eval_state['m1_wins']} M2:{eval_state['m2_wins']})")


def tail_log():
    """后台 tail -f 日志，实时解析并推送"""
    global live_game
    
    # 等日志文件出现
    while not os.path.exists(LOG_PATH):
        time.sleep(1)
    
    # 启动前先回扫日志尾部，恢复当前评估状态
    _backfill_eval_state()
    
    # tail -f 跟踪新内容（bufsize=1 逐行读取，避免缓冲区阻塞）
    proc = subprocess.Popen(
        ["tail", "-n", "0", "-f", LOG_PATH],
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, bufsize=1
    )
    
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        line = line.strip()
        if not line:
            continue
        
        # 新局开始
        m = re.match(r'^=+ \[(\d+-\d+)\]=+$', line)
        if m:
            with live_lock:
                if live_game["active"] and live_game["moves"]:
                    broadcast_sse("game_end", {
                        "id": live_game["id"],
                        "winner": live_game["winner"],
                        "total_moves": len(live_game["moves"])
                    })
                live_game = {"id": m.group(1), "moves": [], "winner": 0, "opening": "", "active": True}
                broadcast_sse("game_start", {"id": m.group(1)})
            continue
        
        # === 以下事件不依赖 live_game，任何时候都要解析 ===
        
        # 评估开始
        if "开始 Elo 评估:" in line:
            eval_state["active"] = True
            eval_state["m1_wins"] = 0
            eval_state["m2_wins"] = 0
            eval_state["draws"] = 0
            eval_state["current"] = 0
            eval_state["total"] = 0
            eval_state["games"] = []
            eval_state["info"] = line.strip()
            broadcast_sse("eval_start", {"info": line.strip()})
            continue

        # 训练 batch 进度
        tm = re.search(r'\[train\] batch (\d+)/(\d+) loss=([\d.]+)', line)
        if tm:
            broadcast_sse("train_batch", {
                "batch": int(tm.group(1)),
                "total": int(tm.group(2)),
                "loss": float(tm.group(3))
            })
            continue

        # 训练 epoch 完成
        te = re.search(r'episode (\d+) Loss: ([\d.]+) \(value: ([\d.]+), policy: ([\d.]+)\)', line)
        if te:
            broadcast_sse("train_epoch", {
                "episode": int(te.group(1)),
                "loss": float(te.group(2)),
                "value_loss": float(te.group(3)),
                "policy_loss": float(te.group(4))
            })
            continue

        # 训练采样信息
        if "本轮采样" in line:
            broadcast_sse("train_sample", {"info": line.strip()})
            continue

        # 自对弈速度
        sp = re.search(r'自对弈完成.*?(\d+)局.*?(\d+)条.*?([\d.]+)s.*?([\d.]+)条/s', line)
        if sp:
            broadcast_sse("selfplay_done", {
                "games": int(sp.group(1)),
                "records": int(sp.group(2)),
                "time": float(sp.group(3)),
                "speed": float(sp.group(4))
            })
            continue

        # VCT 标注启动: [VCTLabel] START files=N threads=M maxTimeMs=X
        vs = re.search(r'\[VCTLabel\] START files=(\d+) threads=(\d+) maxTimeMs=(\d+)', line)
        if vs:
            broadcast_sse("vct_start", {
                "files": int(vs.group(1)),
                "threads": int(vs.group(2)),
                "max_time_ms": int(vs.group(3))
            })
            continue

        # VCT 单文件进度: [VCTLabel] file=data_X.txt progress=N/M labeled=K changed=C elapsed=Xs
        vp = re.search(r'\[VCTLabel\] file=(\S+) progress=(\d+)/(\d+) labeled=(\d+) changed=(\d+) elapsed=(\d+)s', line)
        if vp:
            broadcast_sse("vct_file_progress", {
                "file": vp.group(1),
                "current": int(vp.group(2)),
                "total": int(vp.group(3)),
                "labeled": int(vp.group(4)),
                "changed": int(vp.group(5)),
                "elapsed": int(vp.group(6))
            })
            continue

        # VCT 全局进度: [VCTLabel] global progress=N/M labeled=K/T changed=C alreadyOne=A
        vg = re.search(r'\[VCTLabel\] global progress=(\d+)/(\d+) labeled=(\d+)/(\d+) changed=(\d+) alreadyOne=(\d+)', line)
        if vg:
            broadcast_sse("vct_global_progress", {
                "files_done": int(vg.group(1)),
                "files_total": int(vg.group(2)),
                "labeled": int(vg.group(3)),
                "samples_total": int(vg.group(4)),
                "changed": int(vg.group(5)),
                "already_one": int(vg.group(6))
            })
            continue

        # VCT 完成: [VCTLabel] FINISH totalFiles=X totalSamples=Y totalLabeled=Z changed=C alreadyOne=A (label=R% change=R%) elapsed=Ws
        vf = re.search(r'\[VCTLabel\] FINISH totalFiles=(\d+) totalSamples=(\d+) totalLabeled=(\d+) changed=(\d+) alreadyOne=(\d+) \(label=([\d.]+)% change=([\d.]+)%\) elapsed=([\d.]+)s', line)
        if vf:
            broadcast_sse("vct_finish", {
                "files": int(vf.group(1)),
                "samples": int(vf.group(2)),
                "labeled": int(vf.group(3)),
                "changed": int(vf.group(4)),
                "already_one": int(vf.group(5)),
                "label_ratio": float(vf.group(6)),
                "change_ratio": float(vf.group(7)),
                "elapsed": float(vf.group(8))
            })
            continue

        # 评估单局结果
        em = re.match(r'\[Evaluate\] Game (\d+)/(\d+) Opening (\d+) \((M1=\w+)\): (M\d WIN) \((\d+)ms\)', line)
        if em:
            game_data = {
                "current": int(em.group(1)),
                "total": int(em.group(2)),
                "opening": int(em.group(3)),
                "side": em.group(4),
                "result": em.group(5),
                "time_ms": int(em.group(6))
            }
            eval_state["current"] = game_data["current"]
            eval_state["total"] = game_data["total"]
            if game_data["result"] == "M1 WIN":
                eval_state["m1_wins"] += 1
            else:
                eval_state["m2_wins"] += 1
            eval_state["games"].append(game_data)
            broadcast_sse("eval_game", game_data)
            continue
        
        # 分开局类型统计
        gm = re.match(r'\[Evaluate\] (Generated|Manual) openings:\s+(\d+)-(\d+)\s+\(([\d.]+)%\)', line)
        if gm:
            otype = "generated" if gm.group(1) == "Generated" else "manual"
            w, l, wr = int(gm.group(2)), int(gm.group(3)), float(gm.group(4))
            eval_state.setdefault("by_type", {})[otype] = {"wins": w, "losses": l, "win_rate": wr}
            broadcast_sse("eval_type", {"type": otype, "wins": w, "losses": l, "win_rate": wr})
            continue

        # Elo 结果
        if '"elo_diff"' in line:
            idx = line.find("{")
            if idx >= 0:
                try:
                    elo = json.loads(line[idx:])
                    eval_state["active"] = False
                    broadcast_sse("elo", elo)
                except json.JSONDecodeError:
                    pass
            continue
        
        # episode 完成
        if '"i_episode"' in line:
            idx = line.find("{")
            if idx >= 0:
                try:
                    ep = json.loads(line[idx:])
                    broadcast_sse("episode", ep)
                except json.JSONDecodeError:
                    pass
            continue
        
        # === 以下事件依赖 live_game ===
        
        with live_lock:
            if not live_game["active"]:
                continue
            gid = live_game["id"]
        
        # 开局信息
        if "Opening pool=" in line:
            info = line.split("]", 1)[-1].strip() if "]" in line else line
            with live_lock:
                live_game["opening"] = info
            broadcast_sse("opening", {"id": gid, "info": info})
        elif "empty board start" in line:
            with live_lock:
                live_game["opening"] = "empty board"
            broadcast_sse("opening", {"id": gid, "info": "empty board"})
        elif "make move" in line:
            # 开局落子
            mm = re.search(r'make move (\d+),(\d+)', line)
            if mm:
                row, col = int(mm.group(1)), int(mm.group(2))
                with live_lock:
                    step = len(live_game["moves"])
                    color = "x" if step % 2 == 0 else "o"
                    move_data = {"color": color, "row": row, "col": col, "rate": 0, "temp": 0, "info": "opening"}
                    live_game["moves"].append(move_data)
                broadcast_sse("move", {"id": gid, "move": move_data, "step": step + 1})
        
        # 正常落子
        m2 = re.match(r'\[\d+-\d+\]\s+(x|o)\s+(\d+),(\d+)\s+rate=([\d.]+)\s+T=([\d.]+)(.*)', line)
        if m2:
            move_data = {
                "color": m2.group(1),
                "row": int(m2.group(2)), "col": int(m2.group(3)),
                "rate": float(m2.group(4)), "temp": float(m2.group(5)),
                "info": m2.group(6).strip()
            }
            with live_lock:
                live_game["moves"].append(move_data)
                step = len(live_game["moves"])
            broadcast_sse("move", {"id": gid, "move": move_data, "step": step})
        
        # 胜者
        if "winner is" in line:
            m3 = re.search(r'winner is (\d)', line)
            if m3:
                winner = int(m3.group(1))
                with live_lock:
                    live_game["winner"] = winner
                    live_game["active"] = False
                broadcast_sse("game_end", {
                    "id": gid, "winner": winner,
                    "total_moves": len(live_game["moves"])
                })


def parse_episodes():
    path = os.path.join(LOG_DIR, "episode.log")
    if not os.path.exists(path):
        return []
    episodes = []
    with open(path, "r", errors="replace") as f:
        for line in f:
            idx = line.find("{")
            if idx >= 0:
                try:
                    episodes.append(json.loads(line[idx:]))
                except json.JSONDecodeError:
                    pass
    return episodes


def parse_elo():
    path = os.path.join(LOG_DIR, "elo.log")
    if not os.path.exists(path):
        return []
    elo_data = []
    with open(path, "r", errors="replace") as f:
        for line in f:
            idx = line.find("{")
            if idx >= 0:
                try:
                    elo_data.append(json.loads(line[idx:]))
                except json.JSONDecodeError:
                    pass
    return elo_data


HTML = r"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>EGO-Zero 训练监控</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; background: #0f0f1a; color: #e0e0e0; overflow: hidden; height: 100vh; }
.header { background: linear-gradient(135deg, #1a1a2e, #16213e); padding: 2px 20px;
           display: flex; align-items: center; justify-content: space-between; border-bottom: 1px solid #333; }
.header h1 { font-size: 16px; color: #e94560; }
.header .status { font-size: 12px; color: #888; }
.live-dot { display: inline-block; width: 7px; height: 7px; background: #4ecdc4; border-radius: 50%;
            animation: pulse 1.5s infinite; margin-right: 5px; }
@keyframes pulse { 0%,100% { opacity: 1; } 50% { opacity: 0.3; } }
.container { display: grid; grid-template-columns: 500px 1fr; gap: 6px; padding: 6px;
            max-width: 1400px; margin: 0 auto; height: calc(100vh - 50px); }
.panel { background: #1a1a2e; border-radius: 6px; padding: 6px 8px; border: 1px solid #2a2a4a; overflow: hidden; }
.panel h2 { font-size: 13px; color: #e94560; margin-bottom: 2px; }
.left-panel { display: flex; flex-direction: column; }
.left-panel .history { flex: 1; min-height: 0; overflow-y: auto; }

/* 手机端响应式 */
@media (max-width: 768px) {
    .container { grid-template-columns: 1fr; height: auto; padding: 4px; gap: 4px; overflow-y: auto; max-height: none; }
    .left-panel { max-height: none; }
    .right-col { min-height: auto; }
    .right-col .panel { min-height: 200px; }
    .header h1 { font-size: 14px; }
    .board-wrap { text-align: center; }
    .board-wrap canvas { max-width: 100%; height: auto !important; }
    .history { max-height: 200px; }
    html, body { overflow-y: auto; height: auto; }
}

.stats-row { display: flex; gap: 6px; margin-bottom: 0; }
.stat-card { background: #16213e; border-radius: 5px; padding: 2px 8px; text-align: center; flex: 1; }
.stat-card .value { font-size: 18px; font-weight: bold; color: #e94560; }
.stat-card .label { font-size: 10px; color: #888; }

.board-wrap { text-align: center; }
.game-header { font-size: 12px; color: #aaa; margin-bottom: 2px; min-height: 14px; }
.move-info-bar { font-size: 12px; color: #aaa; margin-top: 2px; min-height: 16px; line-height: 1.3; }
.winner-banner { font-size: 13px; font-weight: bold; margin-top: 1px; }
.winner-banner.black { color: #fff; }
.winner-banner.white { color: #ccc; }

.right-col { display: flex; flex-direction: column; gap: 6px; min-height: 0; }
.right-col .panel { flex: 1; display: flex; flex-direction: column; min-height: 0; }
.right-col .panel h2 { flex-shrink: 0; }
.chart-wrap { flex: 1; min-height: 0; position: relative; }

.history { overflow-y: auto; font-size: 11px; margin-top: 2px; }
.history-item { padding: 3px 6px; border-bottom: 1px solid #222; cursor: pointer; display: flex; justify-content: space-between; }
.history-item:hover { background: #16213e; }
.history-item .win { color: #4ecdc4; }
.history-item .loss { color: #e94560; }
</style>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4"></script>
<script src="https://cdn.jsdelivr.net/npm/hammerjs@2"></script>
<script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2"></script>
</head>
<body>
<div class="header">
    <h1><span class="live-dot" id="liveDot"></span>EGO-Zero 训练监控</h1>
    <div class="status" id="status">连接中...</div>
</div>

<div class="stats-row" style="padding: 4px 6px 0; max-width: 1400px; margin: 0 auto;">
    <div class="stat-card"><div class="value" id="statGames">-</div><div class="label">总对局</div></div>
    <div class="stat-card"><div class="value" id="statElo">-</div><div class="label">累计 Elo</div></div>
    <div class="stat-card"><div class="value" id="statLoss">-</div><div class="label">Loss</div></div>
    <div class="stat-card"><div class="value" id="statRecord">-</div><div class="label">数据量</div></div>
    <div class="stat-card"><div class="value" id="statSpeed">-</div><div class="label">条/s</div></div>
    <div class="stat-card"><div class="value" id="statLive">-</div><div class="label">当前局</div></div>
</div>

<div class="container">
    <div class="panel left-panel">
        <h2>🎯 实时对局</h2>
        <div class="game-header" id="gameHeader"></div>
        <div class="board-wrap"><canvas id="board" width="480" height="480"></canvas></div>
        <div class="move-info-bar" id="moveInfo"></div>
        <div id="winnerBanner"></div>
        <h2 style="margin-top:4px">📋 最近对局</h2>
        <div class="history" id="history"></div>
    </div>
    <div class="right-col">
        <div class="panel"><h2>📈 Elo 趋势</h2><div class="chart-wrap"><canvas id="eloChart"></canvas></div></div>
        <div class="panel" style="flex:none;height:130px;padding:4px 8px;">
            <h2 style="margin-bottom:2px;">📊 训练进度</h2>
            <div id="trainProgress" style="font-size:11px;color:#aaa;">等待数据...</div>
            <div style="height:1px;background:#2a2a4a;margin:4px 0;"></div>
            <div id="vctProgress" style="display:none;font-size:11px;color:#aaa;">VCT标注: 已废弃</div>
        </div>
        <div class="panel" style="flex: 1.2;">
            <h2>🏆 评估对战 <span id="evalStatus" style="font-size:11px;color:#888;font-weight:normal;"></span></h2>
            <div id="evalProgress" style="margin-bottom:6px;"></div>
            <div id="evalGames" style="max-height:200px;overflow-y:auto;font-size:11px;"></div>
        </div>
    </div>
</div>

<script>
const BOARD_SIZE = 20;
let liveGame = { id: '', moves: [], winner: 0, opening: '' };
let recentGames = [];
let episodes = [], eloData = [];
let eloChart = null;
let cumElo = 0;
let evalState = { active: false, m1Wins: 0, m2Wins: 0, draws: 0, current: 0, total: 0, games: [] };

// 初始加载历史数据
async function loadHistory() {
    const [epRes, eloRes, evalRes] = await Promise.all([
        fetch('/api/episodes'), fetch('/api/elo'), fetch('/api/eval_state')
    ]);
    episodes = await epRes.json();
    eloData = await eloRes.json();
    cumElo = eloData.reduce((s, e) => s + (e.elo_diff || 0), 0);
    const es = await evalRes.json();
    if (es.active || es.games.length > 0) {
        evalState.active = es.active;
        evalState.m1Wins = es.m1_wins;
        evalState.m2Wins = es.m2_wins;
        evalState.draws = es.draws;
        evalState.current = es.current;
        evalState.total = es.total;
        evalState.games = es.games;
        document.getElementById('evalStatus').textContent = es.active ? evalStatusText() : '上次评估已完成';
        updateEvalPanel();
    }
    updateStats();
    updateCharts();
}

function updateStats() {
    const last = episodes.length > 0 ? episodes[episodes.length - 1] : {};
    document.getElementById('statGames').textContent = last.total_games_count || 0;
    document.getElementById('statElo').textContent = '+' + Math.round(cumElo);
    document.getElementById('statLoss').textContent = (last.loss || 0).toFixed(3);
    document.getElementById('statRecord').textContent = last.record_count || 0;
    document.getElementById('statSpeed').textContent = (last.speed || 0).toFixed(0);
    document.getElementById('statLive').textContent = liveGame.id || '-';
}

function drawBoard() {
    const canvas = document.getElementById('board');
    const ctx = canvas.getContext('2d');
    const pad = 18, cell = (canvas.width - pad*2) / (BOARD_SIZE - 1);

    ctx.fillStyle = '#dcb35c';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.strokeStyle = '#8B7355'; ctx.lineWidth = 0.6;
    for (let i = 0; i < BOARD_SIZE; i++) {
        ctx.beginPath(); ctx.moveTo(pad+i*cell, pad); ctx.lineTo(pad+i*cell, pad+(BOARD_SIZE-1)*cell); ctx.stroke();
        ctx.beginPath(); ctx.moveTo(pad, pad+i*cell); ctx.lineTo(pad+(BOARD_SIZE-1)*cell, pad+i*cell); ctx.stroke();
    }
    [3,9,15].forEach(r => [3,9,15].forEach(c => {
        ctx.fillStyle = '#8B7355'; ctx.beginPath();
        ctx.arc(pad+c*cell, pad+r*cell, 2, 0, Math.PI*2); ctx.fill();
    }));

    liveGame.moves.forEach((m, idx) => {
        const x = pad + m.col*cell, y = pad + m.row*cell, r = cell*0.4;
        const isBlack = m.color === 'x';
        ctx.beginPath(); ctx.arc(x+1, y+1, r, 0, Math.PI*2);
        ctx.fillStyle = 'rgba(0,0,0,0.2)'; ctx.fill();

        const grad = ctx.createRadialGradient(x-r*0.3, y-r*0.3, r*0.1, x, y, r);
        if (isBlack) { grad.addColorStop(0,'#555'); grad.addColorStop(1,'#111'); }
        else { grad.addColorStop(0,'#fff'); grad.addColorStop(1,'#ccc'); }
        ctx.beginPath(); ctx.arc(x, y, r, 0, Math.PI*2);
        ctx.fillStyle = grad; ctx.fill();
        ctx.strokeStyle = isBlack ? '#000' : '#999'; ctx.lineWidth = 0.5; ctx.stroke();

        ctx.fillStyle = isBlack ? '#fff' : '#333';
        ctx.font = `bold ${Math.round(cell*0.35)}px sans-serif`;
        ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
        ctx.fillText(String(idx+1), x, y+1);

        if (idx === liveGame.moves.length - 1) {
            ctx.strokeStyle = '#e94560'; ctx.lineWidth = 2.5;
            ctx.beginPath(); ctx.arc(x, y, r+2, 0, Math.PI*2); ctx.stroke();
        }
    });
}

function updateMoveInfo() {
    const moves = liveGame.moves;
    const info = document.getElementById('moveInfo');
    if (moves.length === 0) {
        info.innerHTML = liveGame.opening || '等待落子...';
        return;
    }
    const m = moves[moves.length - 1];
    const color = m.color === 'x' ? '⚫' : '⚪';
    let infoStr = m.info || '';
    if (infoStr.includes('[F]')) {
        infoStr = infoStr.replace('[F]', '<span style="color:#f9ca24;font-weight:bold;">[F]</span>');
    }
    info.innerHTML = `第 ${moves.length} 步 ${color} (${m.row},${m.col}) rate=${m.rate} T=${m.temp} ${infoStr}`;
}

function showWinner(winner, totalMoves) {
    const banner = document.getElementById('winnerBanner');
    if (winner === 1) banner.innerHTML = '<span class="winner-banner black">⚫ 黑方胜 (' + totalMoves + '步)</span>';
    else if (winner === 2) banner.innerHTML = '<span class="winner-banner white">⚪ 白方胜 (' + totalMoves + '步)</span>';
    else banner.innerHTML = '<span class="winner-banner">平局 (' + totalMoves + '步)</span>';
}

function addToHistory(game) {
    // 去重：检查是否已存在相同ID的对局（防止后端重复推送game_end）
    if (recentGames.find(g => g.id === game.id)) return;
    recentGames.unshift(game);
    if (recentGames.length > 50) recentGames.pop();
    const div = document.getElementById('history');
    div.innerHTML = recentGames.map(g => {
        const w = g.winner === 1 ? '<span class="win">黑胜</span>' : (g.winner === 2 ? '<span class="loss">白胜</span>' : '平局');
        return `<div class="history-item"><span>#${g.id}</span><span>${g.totalMoves}步</span>${w}</div>`;
    }).join('');
}

function updateCharts() {
    // 过滤 rollback 标记，计算累计 Elo（不做补偿，忠实记录）
    const eloEntries = eloData.filter(e => e.elo_diff !== undefined && !e.rollback);
    const eLabels = eloEntries.map(e => 'g'+e.total_games);
    const eCum = []; let s = 0;
    eloEntries.forEach(e => { s += e.elo_diff; eCum.push(s); });
    const eDiff = eloEntries.map(e => e.elo_diff);

    if (eloChart) eloChart.destroy();

    const ctx = document.getElementById('eloChart').getContext('2d');

    // 标记被回退撤销的点（rollback_target 之后、rollback 之前的 entry）
    const rolledBackGames = new Set();
    eloData.forEach((e, i) => {
        if (e.rollback && e.rollback_target) {
            for (let j = i - 1; j >= 0; j--) {
                const d = eloData[j];
                if (d.elo_diff !== undefined && !d.rollback) {
                    if (d.total_games > e.rollback_target) {
                        rolledBackGames.add(d.total_games);
                    } else {
                        break;
                    }
                }
            }
        }
    });
    // 映射到当前显示的 eloEntries 索引
    const rolledBack = new Set();
    eloEntries.forEach((e, i) => {
        if (rolledBackGames.has(e.total_games)) rolledBack.add(i);
    });

    // 点颜色：被撤销=黄色，上升=红，下降=绿
    const pointColors = eCum.map((v, i) => {
        if (rolledBack.has(i)) return '#f9ca24';
        if (i === 0) return v >= 0 ? '#e94560' : '#4ecdc4';
        return v >= eCum[i-1] ? '#e94560' : '#4ecdc4';
    });
    // 线段颜色：被撤销=黄色，上升=红，下降=绿
    const segmentColor = (ctx) => {
        if (rolledBack.has(ctx.p1DataIndex) || rolledBack.has(ctx.p0DataIndex)) return '#f9ca24';
        const curr = eCum[ctx.p1DataIndex];
        const prev = eCum[ctx.p0DataIndex];
        return curr >= prev ? '#e94560' : '#4ecdc4';
    };

    eloChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: eLabels,
            datasets: [{
                label: '累计 Elo',
                data: eCum,
                segment: { borderColor: segmentColor },
                pointBackgroundColor: pointColors,
                pointBorderColor: pointColors,
                pointRadius: 2.5,
                pointHoverRadius: 4,
                borderWidth: 2.5,
                tension: 0.1,
                fill: {
                    target: 'origin',
                    above: 'rgba(233,69,96,0.08)',
                    below: 'rgba(78,205,196,0.08)'
                }
            }]
        },
        options: {
            responsive: true, maintainAspectRatio: false,
            plugins: {
                legend: { display: false },
                tooltip: {
                    callbacks: {
                        label: function(ctx) {
                            const i = ctx.dataIndex;
                            const cum = eCum[i];
                            const diff = eDiff[i];
                            const arrow = diff >= 0 ? '▲' : '▼';
                            return `累计: +${cum}  ${arrow} ${diff >= 0 ? '+' : ''}${diff}`;
                        }
                    }
                },
                zoom: {
                    pan: {
                        enabled: window.innerWidth <= 768,
                        mode: 'x',
                    },
                    limits: { x: { minRange: 10 } }
                }
            },
            scales: {
                x: {
                    ticks: { color: '#555', font: { size: 9 }, maxTicksLimit: 15, maxRotation: 45 },
                    min: (window.innerWidth <= 768 && eLabels.length > 20) ? eLabels[eLabels.length - 20] : undefined,
                },
                y: {
                    ticks: { color: '#888', callback: v => '+' + v },
                    grid: { color: '#1a1a2e' }
                }
            }
        }
    });
}

function updateEvalPanel(game) {
    // 进度条
    const pct = evalState.total > 0 ? Math.round(evalState.current / evalState.total * 100) : 0;
    const wr = (evalState.m1Wins + evalState.m2Wins) > 0
        ? (evalState.m1Wins / (evalState.m1Wins + evalState.m2Wins) * 100).toFixed(1) : '-';
    document.getElementById('evalProgress').innerHTML =
        `<div style="display:flex;align-items:center;gap:8px;font-size:12px;">` +
        `<div style="flex:1;background:#16213e;border-radius:4px;height:16px;overflow:hidden;">` +
        `<div style="width:${pct}%;height:100%;background:linear-gradient(90deg,#e94560,#4ecdc4);transition:width 0.3s;"></div></div>` +
        `<span>${evalState.current}/${evalState.total}</span></div>` +
        `<div style="display:flex;gap:12px;margin-top:4px;font-size:12px;">` +
        `<span style="color:#4ecdc4;">M1胜: ${evalState.m1Wins}</span>` +
        `<span style="color:#e94560;">M2胜: ${evalState.m2Wins}</span>` +
        `<span style="color:#888;">胜率: ${wr}%</span></div>` +
        (evalState.byType ? `<div style="display:flex;gap:12px;margin-top:2px;font-size:11px;color:#666;">` +
            (evalState.byType.generated ? `<span>生成: ${evalState.byType.generated.wins}-${evalState.byType.generated.losses} (${evalState.byType.generated.win_rate}%)</span>` : '') +
            (evalState.byType.manual ? `<span>手工: ${evalState.byType.manual.wins}-${evalState.byType.manual.losses} (${evalState.byType.manual.win_rate}%)</span>` : '') +
            `</div>` : '');

    // 对局列表
    const div = document.getElementById('evalGames');
    div.innerHTML = evalState.games.slice(-50).reverse().map(g => {
        const isM1Win = g.result === 'M1 WIN';
        const color = isM1Win ? '#4ecdc4' : '#e94560';
        const icon = isM1Win ? '✅' : '❌';
        return `<div style="padding:2px 4px;border-bottom:1px solid #1a1a2e;display:flex;justify-content:space-between;">` +
            `<span>${icon} #${g.current} Op${g.opening} ${g.side}</span>` +
            `<span style="color:${color}">${g.result}</span>` +
            `<span style="color:#555">${(g.time_ms/1000).toFixed(1)}s</span></div>`;
    }).join('');
    div.scrollTop = 0;
}

function evalStatusText() {
    if (!evalState.active) return '等待中';
    return `进行中 ${evalState.current}/${evalState.total}`;
}

function opts() {
    return { responsive:true, maintainAspectRatio:false, plugins:{legend:{labels:{color:'#aaa',font:{size:10}}}},
        scales:{x:{ticks:{color:'#555',font:{size:9},maxTicksLimit:12,maxRotation:45}},y:{ticks:{color:'#555'},grid:{color:'#1a1a2e'}}} };
}

// SSE 连接
function connectSSE() {
    const es = new EventSource('/sse');
    es.addEventListener('game_start', e => {
        const d = JSON.parse(e.data);
        liveGame = { id: d.id, moves: [], winner: 0, opening: '' };
        document.getElementById('gameHeader').textContent = '对局 #' + d.id;
        document.getElementById('winnerBanner').innerHTML = '';
        document.getElementById('liveDot').style.background = '#4ecdc4';
        updateStats();
        drawBoard();
        updateMoveInfo();
    });
    es.addEventListener('opening', e => {
        const d = JSON.parse(e.data);
        liveGame.opening = d.info;
        document.getElementById('gameHeader').textContent = '对局 #' + d.id + ' | ' + d.info;
    });
    es.addEventListener('move', e => {
        const d = JSON.parse(e.data);
        liveGame.moves.push(d.move);
        drawBoard();
        updateMoveInfo();
    });
    es.addEventListener('game_end', e => {
        const d = JSON.parse(e.data);
        liveGame.winner = d.winner;
        showWinner(d.winner, d.total_moves);
        document.getElementById('liveDot').style.background = '#888';
        addToHistory({ id: d.id, winner: d.winner, totalMoves: d.total_moves });
    });
    es.addEventListener('train_batch', e => {
        const d = JSON.parse(e.data);
        const pct = Math.round(d.batch / d.total * 100);
        const tp = document.getElementById('trainProgress');
        tp.innerHTML =
            `<div style="display:flex;align-items:center;gap:8px;">` +
            `<div style="flex:1;background:#16213e;border-radius:4px;height:14px;overflow:hidden;">` +
            `<div style="width:${pct}%;height:100%;background:#f9ca24;transition:width 0.2s;"></div></div>` +
            `<span style="font-size:11px;">${d.batch}/${d.total}</span></div>` +
            (tp.dataset.info || '');
    });
    es.addEventListener('train_epoch', e => {
        const d = JSON.parse(e.data);
        const tp = document.getElementById('trainProgress');
        const color = v => v > 1.8 ? '#e94560' : v > 1.5 ? '#f9ca24' : '#4ecdc4';
        tp.dataset.info =
            `<div style="margin-top:3px;">ep${d.episode} loss=<b>${d.loss.toFixed(3)}</b> ` +
            `<span style="color:${color(d.value_loss)}">V=${d.value_loss.toFixed(3)}</span> ` +
            `<span style="color:${color(d.policy_loss)}">P=${d.policy_loss.toFixed(3)}</span></div>`;
        tp.innerHTML = tp.innerHTML.replace(/<div style="margin-top:3px;">.*<\/div>$/, '') + tp.dataset.info;
    });
    es.addEventListener('train_sample', e => {
        const d = JSON.parse(e.data);
        const tp = document.getElementById('trainProgress');
        const info = d.info.replace(/.*\]\s*/, '');
        tp.innerHTML = `<div style="color:#888;font-size:11px;">${info}</div>`;
        tp.dataset.info = '';
    });
    es.addEventListener('selfplay_done', e => {
        const d = JSON.parse(e.data);
        const tp = document.getElementById('trainProgress');
        tp.innerHTML = `<div style="font-size:11px;">自对弈 ${d.games}局 ${d.records}条 ${d.speed}条/s (${d.time.toFixed(0)}s)</div>` +
            (tp.dataset.info || '');
    });

    // VCT 标注：独立区域展示，不与训练进度互相覆盖
    es.addEventListener('vct_start', e => {
        const d = JSON.parse(e.data);
        const vp = document.getElementById('vctProgress');
        vp.dataset.active = '1';
        vp.innerHTML =
            `<div style="color:#9b59b6;">🔍 VCT标注 启动 ${d.files}文件 ${d.threads}线程</div>`;
    });
    es.addEventListener('vct_file_progress', e => {
        const d = JSON.parse(e.data);
        const vp = document.getElementById('vctProgress');
        if (vp.dataset.active !== '1') return;
        const pct = Math.round(d.current / d.total * 100);
        const labelRatio = d.current > 0 ? (100 * d.labeled / d.current).toFixed(1) : '0.0';
        const changeRatio = d.current > 0 ? (100 * d.changed / d.current).toFixed(1) : '0.0';
        vp.innerHTML =
            `<div style="display:flex;align-items:center;gap:8px;">` +
            `<div style="flex:1;background:#16213e;border-radius:4px;height:12px;overflow:hidden;">` +
            `<div style="width:${pct}%;height:100%;background:#9b59b6;transition:width 0.2s;"></div></div>` +
            `<span style="font-size:11px;">${d.current}/${d.total}</span></div>` +
            `<div style="color:#9b59b6;margin-top:2px;">🔍 ${d.file} 标注${d.labeled}(${labelRatio}%) <b>覆盖${d.changed}(${changeRatio}%)</b> ${d.elapsed}s</div>`;
    });
    es.addEventListener('vct_global_progress', e => {
        const d = JSON.parse(e.data);
        const vp = document.getElementById('vctProgress');
        if (vp.dataset.active !== '1') return;
        const pct = Math.round(d.files_done / d.files_total * 100);
        const labelRatio = d.samples_total > 0 ? (100 * d.labeled / d.samples_total).toFixed(1) : '0.0';
        const changeRatio = d.samples_total > 0 ? (100 * d.changed / d.samples_total).toFixed(1) : '0.0';
        vp.innerHTML =
            `<div style="display:flex;align-items:center;gap:8px;">` +
            `<div style="flex:1;background:#16213e;border-radius:4px;height:12px;overflow:hidden;">` +
            `<div style="width:${pct}%;height:100%;background:#9b59b6;transition:width 0.2s;"></div></div>` +
            `<span style="font-size:11px;">${d.files_done}/${d.files_total}文件</span></div>` +
            `<div style="color:#9b59b6;margin-top:2px;">🔍 全局 标注${d.labeled}/${d.samples_total}(${labelRatio}%) <b>覆盖${d.changed}(${changeRatio}%)</b></div>`;
    });
    es.addEventListener('vct_finish', e => {
        const d = JSON.parse(e.data);
        const vp = document.getElementById('vctProgress');
        vp.dataset.active = '0';
        vp.innerHTML =
            `<div style="color:#9b59b6;">✓ VCT标注完成 ${d.files}文件 标注${d.labeled}/${d.samples}(${d.label_ratio.toFixed(1)}%) <b>覆盖${d.changed}(${d.change_ratio.toFixed(1)}%)</b> ${d.elapsed.toFixed(1)}s</div>`;
    });

    es.addEventListener('episode', e => {
        const d = JSON.parse(e.data);
        episodes.push(d);
        updateStats();
        updateCharts();
    });
    es.addEventListener('eval_start', e => {
        const d = JSON.parse(e.data);
        evalState = { active: true, m1Wins: 0, m2Wins: 0, draws: 0, current: 0, total: 0, games: [], byType: null };
        document.getElementById('evalStatus').textContent = '开始评估...';
        document.getElementById('evalGames').innerHTML = '<div style="color:#888;padding:8px;">' + d.info + '</div>';
        document.getElementById('evalProgress').innerHTML = '';
    });
    es.addEventListener('eval_game', e => {
        const d = JSON.parse(e.data);
        // 去重：如果已有相同 current 的记录则跳过
        if (evalState.games.length > 0 && evalState.games[evalState.games.length - 1].current >= d.current) return;
        evalState.current = d.current;
        evalState.total = d.total;
        evalState.active = true;
        if (d.result === 'M1 WIN') evalState.m1Wins++;
        else if (d.result === 'M2 WIN') evalState.m2Wins++;
        else evalState.draws++;
        evalState.games.push(d);
        document.getElementById('evalStatus').textContent = evalStatusText();
        updateEvalPanel();
    });
    es.addEventListener('eval_type', e => {
        const d = JSON.parse(e.data);
        if (!evalState.byType) evalState.byType = {};
        evalState.byType[d.type] = { wins: d.wins, losses: d.losses, win_rate: d.win_rate };
        updateEvalPanel();
    });
    es.addEventListener('elo', e => {
        const d = JSON.parse(e.data);
        eloData.push(d);
        cumElo += (d.elo_diff || 0);
        evalState.active = false;
        document.getElementById('evalStatus').textContent =
            `完成 胜率${(d.win_rate*100).toFixed(1)}% Elo${d.elo_diff>=0?'+':''}${d.elo_diff}`;
        updateStats();
        updateCharts();
    });
    es.onerror = () => {
        document.getElementById('status').textContent = '连接断开，重试中...';
        setTimeout(connectSSE, 3000);
    };
    es.onopen = () => {
        document.getElementById('status').textContent = '实时连接中';
    };
}

loadHistory().then(() => {
    drawBoard();
    connectSSE();
});

// 每 60 秒轮询刷新 stats（兜底，防 SSE 漏事件）
setInterval(async () => {
    try {
        const [epRes, eloRes, evalRes] = await Promise.all([
            fetch('/api/episodes'), fetch('/api/elo'), fetch('/api/eval_state')
        ]);
        episodes = await epRes.json();
        const newElo = await eloRes.json();
        const eloChanged = newElo.length !== eloData.length;
        if (eloChanged) {
            eloData = newElo;
            cumElo = eloData.reduce((s, e) => s + (e.elo_diff || 0), 0);
            updateCharts();
        }
        const es = await evalRes.json();
        if (es.active || es.games.length > 0) {
            evalState.active = es.active;
            evalState.m1Wins = es.m1_wins;
            evalState.m2Wins = es.m2_wins;
            evalState.current = es.current;
            evalState.total = es.total;
            evalState.games = es.games;
            document.getElementById('evalStatus').textContent = es.active ? evalStatusText() : '上次评估已完成';
            updateEvalPanel();
        }
        updateStats();
    } catch(e) {}
}, 60000);
</script>
</body>
</html>"""


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/sse":
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Cache-Control", "no-cache")
            self.send_header("Connection", "keep-alive")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()

            q = queue.Queue(maxsize=500)
            with sse_lock:
                sse_clients.append(q)

            try:
                while True:
                    try:
                        msg = q.get(timeout=15)
                        self.wfile.write(msg.encode())
                        self.wfile.flush()
                    except queue.Empty:
                        # keepalive
                        self.wfile.write(": keepalive\n\n".encode())
                        self.wfile.flush()
            except Exception:
                pass
            finally:
                with sse_lock:
                    if q in sse_clients:
                        sse_clients.remove(q)

        elif self.path == "/api/episodes":
            self.send_json(parse_episodes())
        elif self.path == "/api/elo":
            self.send_json(parse_elo())
        elif self.path == "/api/eval_state":
            self.send_json(eval_state)
        elif self.path == "/" or self.path == "/index.html":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML.encode())
        else:
            self.send_error(404)

    def send_json(self, data):
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def log_message(self, format, *args):
        pass


if __name__ == "__main__":
    # 启动日志 tail 线程
    tail_thread = threading.Thread(target=tail_log, daemon=True)
    tail_thread.start()

    server = http.server.ThreadingHTTPServer(("0.0.0.0", PORT), Handler)
    url = f"http://0.0.0.0:{PORT}"
    print(f"训练监控面板启动: {url}")
    print("实时推送模式 (SSE)")
    print("按 Ctrl+C 退出")
    threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n已退出")
