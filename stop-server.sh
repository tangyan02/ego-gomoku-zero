ps -ef|grep Server.py | awk '{print $2}' | xargs kill
ps -ef|grep  ego-gomoku-zero | awk '{print $2}' | xargs kill