#!/usr/bin/env python3
import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

# ===========================================================
# GLOBAL FLAG — ENABLE/DISABLE ERROR BARS
# ===========================================================
USE_ERROR_BARS = True


# -----------------------------------------------------------
# Helper: load µs values from file
# -----------------------------------------------------------
def load_times(path):
    if not path.exists():
        return None
    try:
        return np.loadtxt(path).astype(float)
    except:
        return None


# -----------------------------------------------------------
# Compute stats for one role directory
# -----------------------------------------------------------
def compute_role_stats(data_dir, role):
    d = Path(data_dir)
    def f(name): return load_times(d / f"times_role{role}_{name}.txt")

    # Load timing files
    t_delta   = f("t_delta")
    t_vjdelta = f("t_vjdelta")
    t_Uwrite  = f("t_Uwrite")

    t_uidelta = f("t_uidelta")
    t_Vwrite  = f("t_Vwrite")

    t_pre     = f("preprocess")
    t_online  = f("online")

    num_q = len(t_delta) if t_delta is not None else 0
    to_s = lambda arr: arr / 1e6 if arr is not None else None

    # USER update = delta + vjdelta + Uwrite
    if t_delta is not None:
        user_vec = to_s(t_delta + t_vjdelta + t_Uwrite)
    else:
        user_vec = None

    # ITEM update = delta + uidelta + Vwrite
    if t_delta is not None:
        item_vec = to_s(t_delta + t_uidelta + t_Vwrite)
    else:
        item_vec = None

    preprocess_total = to_s(np.sum(t_pre)) if t_pre is not None else None
    online_total     = to_s(np.sum(t_online)) if t_online is not None else None
    online_avg       = online_total / num_q if (online_total and num_q > 0) else None

    return {
        "user_update": np.mean(user_vec) if user_vec is not None else None,
        "user_update_std": np.std(user_vec) if user_vec is not None else None,
        "item_update": np.mean(item_vec) if item_vec is not None else None,
        "item_update_std": np.std(item_vec) if item_vec is not None else None,
        "preprocess_total": preprocess_total,
        "online_total": online_total,
        "online_per_query": online_avg,
    }


# -----------------------------------------------------------
# Compute P2 stats
# -----------------------------------------------------------
def compute_p2_stats(data_dir):
    p = Path(data_dir) / "times_p2_da_generation.txt"
    arr = load_times(p)
    return np.sum(arr) / 1e6 if arr is not None else None


# -----------------------------------------------------------
# Summaries for one experiment directory
# -----------------------------------------------------------
def summarize_experiment(expdir):
    expdir = Path(expdir)
    params = sorted([int(x.name) for x in expdir.iterdir() if x.is_dir()])
    rows = []

    for param in params:
        runs = sorted([(expdir / str(param) / d) for d in os.listdir(expdir / str(param))
                       if (expdir / str(param) / d).is_dir()])

        role0, role1, p2 = [], [], []
        for r in runs:
            data_path = r / "data"
            if not data_path.exists():
                continue
            role0.append(compute_role_stats(data_path, 0))
            role1.append(compute_role_stats(data_path, 1))
            p2.append(compute_p2_stats(data_path))

        if not role0:
            continue

        df0 = pd.DataFrame(role0)
        df1 = pd.DataFrame(role1)
        p2arr = np.array(p2)

        rows.append({
            "param": param,
            "Uupdate_P0": df0["user_update"].mean(),
            "Uupdate_P0_std": df0["user_update_std"].mean(),
            "Uupdate_P1": df1["user_update"].mean(),
            "Uupdate_P1_std": df1["user_update_std"].mean(),

            "Vupdate_P0": df0["item_update"].mean(),
            "Vupdate_P0_std": df0["item_update_std"].mean(),
            "Vupdate_P1": df1["item_update"].mean(),
            "Vupdate_P1_std": df1["item_update_std"].mean(),

            "preprocess_P0": df0["preprocess_total"].mean(),
            "preprocess_P1": df1["preprocess_total"].mean(),

            "online_P0": df0["online_total"].mean(),
            "online_P1": df1["online_total"].mean(),

            "online_avg_P0": df0["online_per_query"].mean(),
            "online_avg_P1": df1["online_per_query"].mean(),

            "p2_total": p2arr.mean(),
        })

    return pd.DataFrame(rows)


# -----------------------------------------------------------
# Combined plot: user + item update in same figure
# -----------------------------------------------------------
def plot_combined(df, expname, role):
    out = Path("analysis_plots")
    out.mkdir(exist_ok=True)

    # Pick columns based on party
    u_mean = f"Uupdate_{role}"
    u_std  = f"Uupdate_{role}_std"
    v_mean = f"Vupdate_{role}"
    v_std  = f"Vupdate_{role}_std"

    plt.figure(figsize=(7,5))

    # USER update line
    if USE_ERROR_BARS:
        plt.errorbar(df["param"], df[u_mean], yerr=df[u_std],
                     fmt="-o", label="User Update", capsize=4)
    else:
        plt.plot(df["param"], df[u_mean], "-o", label="User Update")

    # ITEM update line
    if USE_ERROR_BARS:
        plt.errorbar(df["param"], df[v_mean], yerr=df[v_std],
                     fmt="-o", label="Item Update", capsize=4)
    else:
        plt.plot(df["param"], df[v_mean], "-o", label="Item Update")

    plt.xlabel(expname)
    plt.ylabel("Time (seconds)")
    plt.title(f"{expname} — User vs Item Update ({role})")
    plt.grid()
    plt.legend()
    plt.tight_layout()
    plt.savefig(out / f"{expname}_{role}_combined.png", dpi=200)
    plt.close()


# -----------------------------------------------------------
# Plot all experiments
# -----------------------------------------------------------
def plot_experiment(df, expname):
    plot_combined(df, expname, "P0")
    plot_combined(df, expname, "P1")


# -----------------------------------------------------------
# MAIN
# -----------------------------------------------------------
def main():
    experiments = {
        "vary_users": "user_vary_data",
        "vary_items": "item_vary_data",
        "vary_features": "feature_vary_data",
        "vary_queries": "query_vary_data",
    }

    outdir = Path("analysis_results")
    outdir.mkdir(exist_ok=True)

    for expname, dirname in experiments.items():
        print(f"\n=== Analyzing {expname} ===")

        df = summarize_experiment(dirname)
        df.to_csv(outdir / f"{expname}_summary.csv", index=False)

        plot_experiment(df, expname)

    print("\nAnalysis complete.")


if __name__ == "__main__":
    main()
