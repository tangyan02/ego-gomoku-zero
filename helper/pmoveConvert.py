f = open("pmoves8.txt", "r")
lines = f.readlines()

for line in lines:
    if "[" in line:
        line = str.split(line, "[")[1]
        line = str.split(line, "]")[0]
        b = f"game.makeMove(Point({line}));"
        print(b)
