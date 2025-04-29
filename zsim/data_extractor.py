import os
import re
import pandas as pd

# the path must point to the location of your main zsim directory
# this is the same directory that your outputs, src, etc folders are located
#ROOT_DIR = r"C:\Users\ryano\Desktop\TAMU\Classes\Senior\ECEN469\data"

# Specify paths to read in data and output processed data.
RESULTS_DIR = "./outputs/term_project/"
DATA_DIR = "./outputs/term_project/"

# keywords to look for
CORE_METRICS = ["cycles", "cCycles", "instrs"]
L3_METRICS = ["mGETS", "mGETXIM", "mGETXSM"]

def parse_zsim_out(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    core_data = {k: [] for k in CORE_METRICS}
    l3_data = {k: [] for k in L3_METRICS}
    in_l3 = False

    for line in lines:
        line = line.strip()

        # Start parsing after we hit an l3-* line
        if line.startswith("l3-"):
            in_l3 = True
            continue
        if in_l3 and line == "":
            in_l3 = False
            continue

        # Parse core metrics
        if not in_l3:
            for metric in CORE_METRICS:
                if line.startswith(f"{metric}:"):
                    match = re.search(rf"{metric}:\s*(\d+)", line)
                    if match:
                        core_data[metric].append(int(match.group(1)))

        # Parse L3 metrics
        if in_l3:
            for metric in L3_METRICS:
                if line.startswith(f"{metric}:"):
                    match = re.search(rf"{metric}:\s*(\d+)", line)
                    if match:
                        l3_data[metric].append(int(match.group(1)))

    # Summarize data
    summed_core_data = {k: sum(v) for k, v in core_data.items()}
    summed_l3_data = {k: sum(v) for k, v in l3_data.items()}

    instrs = summed_core_data.get("instrs", 0)
    cycles = summed_core_data.get("cycles", 0)
    total_misses = (
        summed_l3_data.get("mGETS", 0) +
        summed_l3_data.get("mGETXIM", 0) +
        summed_l3_data.get("mGETXSM", 0)
    )

    mpki = (total_misses / instrs * 1000) if instrs else 0
    ipc = instrs / cycles if cycles else 0

    return cycles, instrs, ipc, mpki, summed_l3_data

# Loop through all benchmark directories
results = []

for policy in os.listdir(DATA_DIR):
    policy_path = os.path.join(DATA_DIR, policy)
    if not os.path.isdir(policy_path):
        continue

    for benchmark in os.listdir(policy_path):
        benchmark_path = os.path.join(policy_path, benchmark)
        zsim_out = os.path.join(benchmark_path, "zsim.out")

        if os.path.isfile(zsim_out):
            try:
                # create column headers
                cycles, instrs, ipc, mpki, summed_l3_data = parse_zsim_out(zsim_out)
                results.append({
                    "Policy": policy,
                    "Benchmark": benchmark,
                    "Cycles": cycles,
                    "Instructions": instrs,
                    "IPC": ipc,
                    "MPKI": mpki,
                    "mGETS": summed_l3_data.get("mGETS", 0),
                    "mGETXIM": summed_l3_data.get("mGETXIM", 0),
                    "mGETXSM": summed_l3_data.get("mGETXSM", 0)
                })
            except Exception as e:
                print(f"Error parsing {zsim_out}: {e}")

# Ensure the results directory exists
os.makedirs(RESULTS_DIR, exist_ok=True)

# Export to CSV
output_path = os.path.join(RESULTS_DIR, "benchmark_results.csv")
df = pd.DataFrame(results)
df.to_csv(output_path, index=False)
print(f"CSV written: {output_path}")

