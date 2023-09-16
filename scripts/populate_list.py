import os
import sys

f = open("../src/list.txt", "w+")
for i in range(int(sys.argv[1])):
    f.write(str(i) + "\n")
f.close()

print("Done")
