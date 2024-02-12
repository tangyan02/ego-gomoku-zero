f = open("actions.txt", "r")
lines = f.readlines()

for line in lines:

    a = line.split("is")[1].split("on")[0].strip()
    b = f"game.makeMove(Point({a}));"
    print(b)
