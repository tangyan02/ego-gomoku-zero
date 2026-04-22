#!/usr/bin/env python3
"""开局库可视化工具 — 启动后在浏览器中查看所有开局"""

import http.server
import json
import os
import webbrowser
import threading

PORT = 8765
BOARD_SIZE = 20
CENTER = BOARD_SIZE // 2

OPENINGS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "openings")

def parse_openings(filepath):
    """解析开局文件，返回 [{moves: [[row,col],...], source: str}]"""
    openings = []
    if not os.path.exists(filepath):
        return openings
    with open(filepath) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            tokens = line.replace(" ", "").split(",")
            moves = []
            for i in range(0, len(tokens) - 1, 2):
                try:
                    rx, ry = int(tokens[i]), int(tokens[i + 1])
                    moves.append([rx + CENTER, ry + CENTER])
                except (ValueError, IndexError):
                    break
            if moves:
                openings.append(moves)
    return openings

def load_all():
    files = {
        "generated (train)": os.path.join(OPENINGS_DIR, "openings_train.txt"),
        "generated (eval)": os.path.join(OPENINGS_DIR, "openings_eval.txt"),
        "manual": os.path.join(OPENINGS_DIR, "openings_manual.txt"),
    }
    result = {}
    for label, path in files.items():
        ops = parse_openings(path)
        if ops:
            result[label] = ops
    return result

HTML = """<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>开局库查看器</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
       background: #1a1a2e; color: #e0e0e0; padding: 20px; }
h1 { text-align: center; margin-bottom: 10px; color: #e94560; font-size: 22px; }
.stats { text-align: center; margin-bottom: 15px; color: #888; font-size: 14px; }
.controls { display: flex; justify-content: center; gap: 10px; margin-bottom: 15px; flex-wrap: wrap; }
.controls button, .controls select {
    padding: 6px 16px; border: 1px solid #444; border-radius: 6px;
    background: #16213e; color: #e0e0e0; cursor: pointer; font-size: 14px;
}
.controls button:hover { background: #e94560; border-color: #e94560; }
.controls button.active { background: #e94560; border-color: #e94560; }
.board-container { display: flex; justify-content: center; }
canvas { border-radius: 8px; box-shadow: 0 4px 20px rgba(0,0,0,0.5); }
.info { text-align: center; margin-top: 10px; color: #aaa; font-size: 14px; }
.nav { display: flex; justify-content: center; gap: 15px; margin-top: 12px; align-items: center; }
.nav button { padding: 8px 20px; border: 1px solid #444; border-radius: 6px;
              background: #16213e; color: #e0e0e0; cursor: pointer; font-size: 16px; }
.nav button:hover { background: #e94560; }
.nav span { font-size: 16px; min-width: 120px; text-align: center; }
</style>
</head>
<body>
<h1>🎯 开局库查看器</h1>
<div class="stats" id="stats"></div>
<div class="controls" id="controls"></div>
<div class="board-container"><canvas id="board" width="580" height="580"></canvas></div>
<div class="info" id="info"></div>
<div class="nav">
  <button onclick="prev()">◀ 上一个</button>
  <span id="counter">-</span>
  <button onclick="next()">下一个 ▶</button>
</div>
<script>
let allData = {};
let currentPool = '';
let currentIdx = 0;
let pools = [];

async function init() {
    const res = await fetch('/api/openings');
    allData = await res.json();
    pools = Object.keys(allData);
    if (!pools.length) { document.getElementById('stats').textContent = '未找到开局文件'; return; }

    // stats
    let statsText = pools.map(p => p + ': ' + allData[p].length + '个').join('  |  ');
    let total = pools.reduce((s, p) => s + allData[p].length, 0);
    document.getElementById('stats').textContent = '共 ' + total + ' 个开局  ( ' + statsText + ' )';

    // buttons
    const ctrl = document.getElementById('controls');
    pools.forEach(p => {
        const btn = document.createElement('button');
        btn.textContent = p + ' (' + allData[p].length + ')';
        btn.onclick = () => selectPool(p);
        btn.id = 'btn-' + p;
        ctrl.appendChild(btn);
    });

    selectPool(pools[0]);
}

function selectPool(name) {
    currentPool = name;
    currentIdx = 0;
    pools.forEach(p => {
        const b = document.getElementById('btn-' + p);
        if (b) b.className = (p === name) ? 'active' : '';
    });
    draw();
}

function prev() { if (!currentPool) return; currentIdx = Math.max(0, currentIdx - 1); draw(); }
function next() {
    if (!currentPool) return;
    currentIdx = Math.min(allData[currentPool].length - 1, currentIdx + 1);
    draw();
}

function draw() {
    const moves = allData[currentPool][currentIdx];
    const canvas = document.getElementById('board');
    const ctx = canvas.getContext('2d');
    const size = 20;
    const pad = 25;
    const cell = (canvas.width - pad * 2) / (size - 1);

    // background
    ctx.fillStyle = '#dcb35c';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    // grid
    ctx.strokeStyle = '#8B7355';
    ctx.lineWidth = 0.8;
    for (let i = 0; i < size; i++) {
        ctx.beginPath();
        ctx.moveTo(pad + i * cell, pad);
        ctx.lineTo(pad + i * cell, pad + (size-1) * cell);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(pad, pad + i * cell);
        ctx.lineTo(pad + (size-1) * cell, pad + i * cell);
        ctx.stroke();
    }

    // star points
    ctx.fillStyle = '#8B7355';
    [3, 9, 15].forEach(r => [3, 9, 15].forEach(c => {
        if (r < size && c < size) {
            ctx.beginPath();
            ctx.arc(pad + c * cell, pad + r * cell, 2.5, 0, Math.PI * 2);
            ctx.fill();
        }
    }));

    // stones
    moves.forEach((m, idx) => {
        const row = m[0], col = m[1];
        const x = pad + col * cell;
        const y = pad + row * cell;
        const r = cell * 0.42;
        const isBlack = (idx % 2 === 0);

        // shadow
        ctx.beginPath();
        ctx.arc(x + 1.5, y + 1.5, r, 0, Math.PI * 2);
        ctx.fillStyle = 'rgba(0,0,0,0.3)';
        ctx.fill();

        // stone
        const grad = ctx.createRadialGradient(x - r*0.3, y - r*0.3, r*0.1, x, y, r);
        if (isBlack) {
            grad.addColorStop(0, '#555');
            grad.addColorStop(1, '#111');
        } else {
            grad.addColorStop(0, '#fff');
            grad.addColorStop(1, '#ccc');
        }
        ctx.beginPath();
        ctx.arc(x, y, r, 0, Math.PI * 2);
        ctx.fillStyle = grad;
        ctx.fill();
        ctx.strokeStyle = isBlack ? '#000' : '#999';
        ctx.lineWidth = 0.5;
        ctx.stroke();

        // move number
        ctx.fillStyle = isBlack ? '#fff' : '#333';
        ctx.font = 'bold ' + Math.round(cell * 0.38) + 'px sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(String(idx + 1), x, y + 1);
    });

    // coordinate labels
    ctx.fillStyle = '#8B7355';
    ctx.font = '10px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    for (let i = 0; i < size; i++) {
        ctx.fillText(String(i), pad + i * cell, pad + (size-1) * cell + 8);
    }
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    for (let i = 0; i < size; i++) {
        ctx.fillText(String(i), pad - 8, pad + i * cell);
    }

    document.getElementById('counter').textContent = (currentIdx + 1) + ' / ' + allData[currentPool].length;
    document.getElementById('info').textContent =
        currentPool + ' #' + (currentIdx + 1) + '  |  ' + moves.length + ' 步  |  ' +
        (moves.length % 2 === 0 ? '轮到黑方' : '轮到白方');
}

document.addEventListener('keydown', e => {
    if (e.key === 'ArrowLeft') prev();
    else if (e.key === 'ArrowRight') next();
});

init();
</script>
</body>
</html>"""

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/api/openings':
            data = load_all()
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(data).encode())
        elif self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(HTML.encode())
        else:
            self.send_error(404)

    def log_message(self, format, *args):
        pass  # suppress logs

if __name__ == '__main__':
    server = http.server.HTTPServer(('127.0.0.1', PORT), Handler)
    url = f'http://127.0.0.1:{PORT}'
    print(f'开局库查看器启动: {url}')
    print('按 Ctrl+C 退出')
    threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\n已退出')
