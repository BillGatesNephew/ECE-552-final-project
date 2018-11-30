import subprocess

configs = ["-bconf naive -issue:aggressive false"]
benches = ["anagram.ss words < anagram.in > OUT","cc1.ss -O 1stmt.i","go.ss 50 9 2stone9.in > OUT"]

for bench in benches:
    for config in configs:
        cmd="./hydra"+" "+config+" "+bench
        output=subprocess.run(cmd, stderr=subprocess.PIPE,shell=True,).stderr.decode('utf-8')
        for line in output.split("\n"):
            if "sim_cycle" in line:
                print(cmd+"\n"+line + "\n\n\n")