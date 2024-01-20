import subprocess


def slefPlayInCpp():
    # 执行可执行程序
    process = subprocess.Popen('./build/ego-gomoku-zero', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # 持续打印输出
    for line in process.stdout:
        print(line, end='')

    # 等待命令执行完成
    process.wait()

    # 检查命令的返回码
    if process.returncode == 0:
        print("Command execution successful.")
    else:
        print(f"Command execution failed with return code: {process.returncode}")


slefPlayInCpp()
