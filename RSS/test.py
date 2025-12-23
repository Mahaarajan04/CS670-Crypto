#!/usr/bin/env python3
import os, random, subprocess, sys

def write_env():
    N = random.randint(1, 200)
    M = random.randint(1, 200)
    K = random.randint(1, 200)
    Q = random.randint(1, 200)
    with open(".env", "w") as f:
        f.write(f"P=3\nT=1\nN={N}\nM={M}\nK={K}\nQ={Q}\n")
    return N, M, K, Q

def run_pipeline():
    # Step 2.1: prune
    subprocess.run(["docker", "container", "prune", "-f"], check=True)
    # Step 2.2: rebuild + run
    subprocess.run(["docker-compose", "up", "--build"], check=True)
    # Step 2.3: compile checker
    compile_res = subprocess.run(
        ["g++","checker.cpp", "-o", "checker"],
    )
    # Step 2.4: run checker
    result = subprocess.run(
        ["./checker"], capture_output=True, text=True
    )
    return result.stdout.strip()

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <num_runs>")
        sys.exit(1)
    num_runs = int(sys.argv[1])

    successes = 0
    for run in range(1, num_runs + 1):
        N, M, K, Q = write_env()
        print(f"[Run {run}/{num_runs}] N={N} M={M} K={K} Q={Q}")

        try:
            output = run_pipeline()
        except subprocess.CalledProcessError as e:
            print(f"Pipeline failed: {e}")
            continue

        if "Simulating queries..." in output and "All queries processed" in output:
            print("Success")
            successes += 1
        else:
            print("Checker failed")
            print("  --- Output ---")
            print(output)

    print(f"\n=== Summary: {successes}/{num_runs} runs successful ===")

if __name__ == "__main__":
    main()
