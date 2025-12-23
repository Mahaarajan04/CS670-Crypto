#!/usr/bin/env python3
import os
import shutil
import subprocess
from pathlib import Path
import sys
# ===============================================================
# BASELINE CONFIGURATION
# ===============================================================
BASE_N = 64   # items
BASE_M = 64   # users
BASE_K = 64   # features
BASE_Q = 25   # queries

NUM_REPEATS = 1  # times per experiment point

# ===============================================================
# PARAMETER SWEEPS
# ===============================================================
USER_SWEEP    = [10, 50, 100, 150]
ITEM_SWEEP    = [10, 50, 100, 150]
FEATURE_SWEEP = [10, 50, 100, 150]
QUERY_SWEEP   = [10, 200, 400, 600]

# ===============================================================
# WRITER FOR .env FILE
# ===============================================================
def write_env(N, M, K, Q):
    with open(".env", "w") as f:
        f.write(f"N={N}\n")
        f.write(f"M={M}\n")
        f.write(f"K={K}\n")
        f.write(f"Q={Q}\n")


# ===============================================================
# RUN THE FULL DOCKER PIPELINE
# ===============================================================
def run_pipeline():
    subprocess.run(["docker", "container", "prune", "-f"], check=True)
    subprocess.run(["docker-compose", "up", "--build"], check=True)


# ===============================================================
# MOVE data/ FOLDER TO PROPER DESTINATION
# ===============================================================
def store_run(exp_dir, param_value, run_id):
    param_dir = Path(exp_dir) / str(param_value) / f"run{run_id}"
    param_dir.mkdir(parents=True, exist_ok=True)

    shutil.move("data", param_dir / "data")
    Path("data").mkdir(exist_ok=True)


# ===============================================================
# RUN A FULL SWEEP FOR ONE PARAMETER
# ===============================================================
def run_experiment(exp_name, sweep_values, vary_fn):
    """
    exp_name:    subdirectory to store data (e.g., "user_vary_data")
    sweep_values: list of parameter values to vary
    vary_fn:     function(N,M,K,Q,val) -> new (N,M,K,Q)
    """
    print(f"\n=== Running experiment: {exp_name} ===")

    exp_dir = Path(exp_name)
    exp_dir.mkdir(exist_ok=True)

    for val in sweep_values:
        print(f"\n  → Parameter = {val}")

        for run_id in range(1, NUM_REPEATS + 1):
            print(f"     Run {run_id}/{NUM_REPEATS}")

            # Apply parameter change
            N, M, K, Q = vary_fn(BASE_N, BASE_M, BASE_K, BASE_Q, val)

            # Write environment
            write_env(N, M, K, Q)

            # Run full docker pipeline
            run_pipeline()

            # Store results
            store_run(exp_dir, val, run_id)


# ===============================================================
# MAIN: DEFINE ALL FOUR EXPERIMENTS
# ===============================================================
def main():
    global NUM_REPEATS
    if len(sys.argv) == 2:
        NUM_REPEATS = int(sys.argv[1])
        print(f"\nUsing NUM_REPEATS = {NUM_REPEATS}\n")
    else:
        print(f"\nNo repeat count provided → using default NUM_REPEATS = {NUM_REPEATS}\n")


    # -------------------------------
    # 1. User Variation (M)
    # -------------------------------
    run_experiment(
        exp_name="user_vary_data",
        sweep_values=USER_SWEEP,
        vary_fn=lambda N, M, K, Q, v: (N, v, K, Q)
    )

    # -------------------------------
    # 2. Item Variation (N)
    # -------------------------------
    run_experiment(
        exp_name="item_vary_data",
        sweep_values=ITEM_SWEEP,
        vary_fn=lambda N, M, K, Q, v: (v, M, K, Q)
    )

    # -------------------------------
    # 3. Feature Variation (K)
    # -------------------------------
    run_experiment(
        exp_name="feature_vary_data",
        sweep_values=FEATURE_SWEEP,
        vary_fn=lambda N, M, K, Q, v: (N, M, v, Q)
    )

    # -------------------------------
    # 4. Query Variation (Q)
    # -------------------------------
    run_experiment(
        exp_name="query_vary_data",
        sweep_values=QUERY_SWEEP,
        vary_fn=lambda N, M, K, Q, v: (N, M, K, v)
    )

    print("\n=== All experiments complete ===")


if __name__ == "__main__":
    main()
