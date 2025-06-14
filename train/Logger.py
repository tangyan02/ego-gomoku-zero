from Utils import getTimeStr


def info(msg):
    print(getTimeStr(), msg)

def infoD(msg, fileName="info.log"):
    log_entry = f"{getTimeStr()} {msg}\n"  # 格式：时间 + 消息 + 换行
    print(log_entry)
    with open(f"log/{fileName}", "a", encoding="utf-8") as f:
        f.write(log_entry)