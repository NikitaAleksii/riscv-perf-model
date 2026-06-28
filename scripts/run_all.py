"""
Batch runner.

Discovers all normalized trace files (*.norm) in traces/,
runs the simulator binary on each, and prints a summary table of
instruction statistics across all benchmarks.
"""
import matplotlib.pyplot as plt
from pathlib import Path
import subprocess
import pandas as pd
import json
import sys

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
TRACES_PATH = PROJECT_ROOT / "traces"
SIMULATION_PATH = PROJECT_ROOT / "src" / "build" / "main"
RESULTS_PATH = PROJECT_ROOT / "results"
PREDICTOR = "gshare"
DCACHE_SIZE = "2048"
DCACHE_ASSOC = "8"
DCACHE_LINE = "64"

def run_simulation(trace_path):
    """Run the simulator on a single trace file and return the parsed JSON result, or None on failure."""
    try:
        result = subprocess.run(
            [SIMULATION_PATH, trace_path, RESULTS_PATH, PREDICTOR, DCACHE_SIZE, DCACHE_ASSOC, DCACHE_LINE],
            text=True,
            capture_output=True
        )
    except(FileNotFoundError):
        print(f"File not found {SIMULATION_PATH}", file=sys.stderr)
        exit(1)
    
    if result.returncode != 0:
        print(f"  FAILED: {result.stderr}", file=sys.stderr)
        return None
    
    json_path = RESULTS_PATH / (trace_path.stem + ".json")
    
    with open(json_path) as f:
        return json.load(f)

def flatten_json(json_data):
    """Flatten a simulator JSON result into a single dict, adding per-category percentage columns."""
    flat = {
        # Identifiers
        "trace":   json_data["trace"],
        "config":  json_data.get("config", "default"),
        
        # Raw counts
        "instructions": json_data["instructions"],
        "LOAD":         json_data["categories"]["LOAD"],
        "STORE":        json_data["categories"]["STORE"],
        "BRANCH":       json_data["categories"]["BRANCH"],
        "JAL":          json_data["categories"]["JAL"],
        "JALR":         json_data["categories"]["JALR"],
        "ALU":          json_data["categories"]["ALU"],
        
        # Placeholders
        "cycles":             json_data.get("cycles"),
        "ipc":                json_data.get("ipc"),
        "branch_mispredictions": json_data.get("branch_mispredictions"),
        "branch_mispredictions_rate": json_data.get("branch_mispredictions_rate")
    }
    
    n = flat["instructions"]
    for cat in ["LOAD", "STORE", "BRANCH", "JAL", "JALR", "ALU"]:
        flat[cat + "_pct"] = 100.0 * flat[cat] / n
    return flat

def main():
    """Discover all .norm traces, simulate each, and print a summary table of instruction statistics."""
    files = list(TRACES_PATH.glob('*.norm'))
    
    if not files:
        print(f"Files not found in {TRACES_PATH}", file=sys.stderr)
        sys.exit(1)
    
    runs = []
    for file in files:
        print(f"running {file} ")
        
        data = run_simulation(file)
        if data is None:
            continue
        
        runs.append(flatten_json(data))
        
    if not runs:
        print("No successful runs.", file=sys.stderr)
        sys.exit(1)

    df = pd.DataFrame(runs)
    ax = df.plot(kind='bar', x='trace', y='ipc',
                 title='IPC Across Benchmarks', figsize=(15, 10),
                 legend=False, fontsize=14)
    ax.set_xlabel("Benchmark", fontsize=14)
    ax.set_ylabel("IPC", fontsize=14)
    ax.set_ylim(0, 1)
    ax.tick_params(axis='x', rotation=45)
    plt.tight_layout()
    plt.savefig(RESULTS_PATH / 'ipc_plot.png')
    
    print("=== Summary ===")
    summary_cols = ["trace", "instructions", "LOAD_pct", "STORE_pct",
                    "BRANCH_pct", "JAL_pct", "JALR_pct", "ALU_pct"]
    print(df[summary_cols].to_string(index=False, float_format="%.2f"))
    print(f"\nAggregate: {len(df)} benchmarks, "
          f"{df['instructions'].sum():,} total instructions")
    
if __name__ == "__main__":
    main()