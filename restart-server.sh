ps -ef|grep Server.py | awk '{print $2}' | xargs kill
ps -ef|grep  ego-gomoku-zero | awk '{print $2}' | xargs kill
nohup python3 -u Server.py > server.log 2>&1 &