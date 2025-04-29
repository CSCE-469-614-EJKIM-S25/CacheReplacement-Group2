import os
import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

# Use relative path to outputs directory
RESULTS_DIR = "./outputs/term_project/"
os.makedirs(RESULTS_DIR, exist_ok=True)

# Load CSV from results directory
csv_path = os.path.join(RESULTS_DIR, "benchmark_results.csv")
df = pd.read_csv(csv_path)

# Benchmarks to consider
included_benchmarks = [
    "bzip2", "gcc", "mcf", "hmmer", "xalancbmk",
    "milc", "cactusADM", "leslie3d", "namd", "calculix",
    "blackscholes", "bodytrack", "fluidanimate", "streamcluster", "swaptions",
    "sjeng", "libquantum", "soplex", "lbm",
    "canneal", "x264"
]

def benchmark_matches(name):
    return any(approved in name for approved in included_benchmarks)
df = df[df["Benchmark"].apply(benchmark_matches)]

def simplify_name(name):
    for approved in included_benchmarks:
        if approved in name:
            return approved
    return name

df["Benchmark"] = df["Benchmark"].apply(simplify_name)

benchmarks = sorted(df["Benchmark"].unique())
policies = sorted(df["Policy"].unique())

def plot_and_save_metric(metric, ylabel, title, filename, ylim=None):
    pivot = df.pivot(index="Benchmark", columns="Policy", values=metric).loc[benchmarks]

    pivot.plot(kind="bar", figsize=(10, 8), width=0.8)
    plt.title(title, fontsize=16)
    plt.ylabel(ylabel, fontsize=14)
    plt.xlabel("Benchmark", fontsize=14)
    plt.xticks(rotation=45, ha="right", fontsize=12)
    plt.yticks(fontsize=12)
    plt.grid(axis="y", linestyle="--", alpha=0.7)
    if ylim:
        plt.ylim(ylim)
    plt.tight_layout()

    if metric == "IPC":
        plt.legend(
            loc="upper center",
            bbox_to_anchor=(0.5, 1.01),
            ncol=7,
            fontsize=10,
            title_fontsize=11,
            frameon=False
        )
    else:
        plt.legend(title="Policy", fontsize=12, title_fontsize=13)

    output_path = os.path.join(RESULTS_DIR, filename)
    plt.savefig(output_path, bbox_inches="tight")
    plt.close()

def plot_filtered_metric(metric, ylabel, title, filename, threshold=1.0):
    eligible_benchmarks = (
        df[df[metric] < threshold]["Benchmark"]
        .unique()
        .tolist()
    )

    filtered_df = df[df["Benchmark"].isin(eligible_benchmarks)]

    if not eligible_benchmarks:
        print(f"No benchmarks with any policy having {metric} < {threshold}")
        return

    pivot = filtered_df.pivot(index="Benchmark", columns="Policy", values=metric).loc[sorted(eligible_benchmarks)]

    fig, ax = plt.subplots(figsize=(10, 8))
    pivot.plot(kind="bar", ax=ax, width=0.8)
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.set_xlabel(f"Benchmark (if any {metric} < {threshold})")
    ax.grid(axis="y", linestyle="--", alpha=0.7)
    ax.tick_params(axis='x', rotation=45)
    ax.legend(title="Policy", fontsize=10, title_fontsize=11)

    inset_ax = inset_axes(ax, width="20%", height="20%", loc="center left", borderpad=7)
    zoom_benchmarks = ["blackscholes", "swaptions"]
    pivot_zoom = pivot.loc[zoom_benchmarks]

    pivot_zoom.plot(kind="bar", ax=inset_ax, width=0.8, legend=False)
    inset_ax.set_ylim(0, 0.01)
    inset_ax.set_title("Zoom: blackscholes & swaptions", fontsize=9)
    inset_ax.set_xticklabels(zoom_benchmarks, rotation=30, ha='right')
    inset_ax.tick_params(axis='y', labelsize=7)
    inset_ax.tick_params(axis='x', labelsize=8)

    fig.tight_layout()
    output_path = os.path.join(RESULTS_DIR, filename)
    plt.savefig(output_path, bbox_inches="tight")
    plt.close()

plot_and_save_metric("IPC", "Instructions Per Cycle", "IPC by Benchmark and Policy", "ipc_plot.png")
plot_and_save_metric("MPKI", "Misses Per 1000 Instructions", "MPKI by Benchmark and Policy", "mpki_plot.png")
plot_filtered_metric("MPKI", "Misses Per 1000 Instructions", "Low MPKI Benchmarks", "mpki_filtered.png")
plot_and_save_metric("Cycles", "Total Cycles", "Cycles by Benchmark and Policy", "cycles_plot.png")

print("All graphs saved to ./outputs/term_project/")
