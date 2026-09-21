from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT = Path(__file__).resolve().parents[2]

RESULTS_DIR = ROOT / "benchmarks" / "results"

CSV_FILE = RESULTS_DIR / "worker_scaling.csv"

OUTPUT_DIR = RESULTS_DIR / "plots"

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


# ------------------------------------------------------------
# Load benchmark data
# ------------------------------------------------------------

if not CSV_FILE.exists():

    raise FileNotFoundError(
        f"Benchmark CSV not found: {CSV_FILE}\n"
        "Run the C++ benchmark first."
    )


df = pd.read_csv(CSV_FILE)


required_columns = [
    "workers",
    "events",
    "elapsed_seconds",
    "throughput_events_sec",
    "average_latency_us",
    "p50_us",
    "p95_us",
    "p99_us",
    "p99_9_us",
    "max_latency_us"
]


missing = [
    column
    for column in required_columns
    if column not in df.columns
]


if missing:

    raise ValueError(
        f"Missing columns in benchmark CSV: {missing}"
    )


# ------------------------------------------------------------
# Sort by workers
# ------------------------------------------------------------

df = df.sort_values(
    "workers"
).reset_index(drop=True)


# ------------------------------------------------------------
# Calculate scaling efficiency
# ------------------------------------------------------------

baseline_throughput = df.loc[
    0,
    "throughput_events_sec"
]


df["speedup"] = (
    df["throughput_events_sec"]
    /
    baseline_throughput
)


df["scaling_efficiency_percent"] = (
    df["speedup"]
    /
    df["workers"]
    *
    100.0
)


# ------------------------------------------------------------
# Find best configurations
# ------------------------------------------------------------

best_throughput = df.loc[
    df["throughput_events_sec"].idxmax()
]


best_p99 = df.loc[
    df["p99_us"].idxmin()
]


# ------------------------------------------------------------
# Print report
# ------------------------------------------------------------

print()
print("=" * 72)
print("             NEXUSFLOW BENCHMARK ANALYSIS")
print("=" * 72)

print()

print(
    df[
        [
            "workers",
            "throughput_events_sec",
            "p50_us",
            "p95_us",
            "p99_us",
            "p99_9_us",
            "max_latency_us"
        ]
    ].to_string(
        index=False
    )
)


print()
print("-" * 72)

print(
    f"Best throughput : "
    f"{best_throughput['throughput_events_sec']:,.2f} events/sec "
    f"at {int(best_throughput['workers'])} workers"
)


print(
    f"Best P99        : "
    f"{best_p99['p99_us']:,.3f} us "
    f"at {int(best_p99['workers'])} workers"
)


print("-" * 72)


print()
print("SCALING EFFICIENCY")
print()


print(
    df[
        [
            "workers",
            "speedup",
            "scaling_efficiency_percent"
        ]
    ].to_string(
        index=False
    )
)


# ------------------------------------------------------------
# Save enriched CSV
# ------------------------------------------------------------

analysis_csv = (
    RESULTS_DIR /
    "worker_scaling_analysis.csv"
)


df.to_csv(
    analysis_csv,
    index=False
)


# ------------------------------------------------------------
# Plot 1: Throughput
# ------------------------------------------------------------

plt.figure(
    figsize=(10, 6)
)

plt.plot(
    df["workers"],
    df["throughput_events_sec"],
    marker="o"
)

plt.xlabel(
    "Number of Workers"
)

plt.ylabel(
    "Throughput (events/sec)"
)

plt.title(
    "NexusFlow Throughput vs Worker Count"
)

plt.grid(
    True,
    alpha=0.3
)

plt.xticks(
    df["workers"]
)

plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "throughput_vs_workers.png",
    dpi=200
)

plt.close()


# ------------------------------------------------------------
# Plot 2: Tail Latency
# ------------------------------------------------------------

plt.figure(
    figsize=(10, 6)
)

plt.plot(
    df["workers"],
    df["p50_us"],
    marker="o",
    label="P50"
)

plt.plot(
    df["workers"],
    df["p95_us"],
    marker="o",
    label="P95"
)

plt.plot(
    df["workers"],
    df["p99_us"],
    marker="o",
    label="P99"
)

plt.plot(
    df["workers"],
    df["p99_9_us"],
    marker="o",
    label="P99.9"
)

plt.xlabel(
    "Number of Workers"
)

plt.ylabel(
    "Latency (microseconds)"
)

plt.title(
    "NexusFlow Latency vs Worker Count"
)

plt.legend()

plt.grid(
    True,
    alpha=0.3
)

plt.xticks(
    df["workers"]
)

plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "latency_vs_workers.png",
    dpi=200
)

plt.close()


# ------------------------------------------------------------
# Plot 3: Scaling Efficiency
# ------------------------------------------------------------

plt.figure(
    figsize=(10, 6)
)

plt.plot(
    df["workers"],
    df["scaling_efficiency_percent"],
    marker="o"
)

plt.axhline(
    100.0,
    linestyle="--",
    label="Ideal linear scaling"
)

plt.xlabel(
    "Number of Workers"
)

plt.ylabel(
    "Scaling Efficiency (%)"
)

plt.title(
    "NexusFlow Scaling Efficiency"
)

plt.legend()

plt.grid(
    True,
    alpha=0.3
)

plt.xticks(
    df["workers"]
)

plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "scaling_efficiency.png",
    dpi=200
)

plt.close()


# ------------------------------------------------------------
# Final output
# ------------------------------------------------------------

print()
print("=" * 72)
print("                    ANALYSIS COMPLETE")
print("=" * 72)

print()
print("Generated files:")
print()

print(
    analysis_csv
)

print(
    OUTPUT_DIR / "throughput_vs_workers.png"
)

print(
    OUTPUT_DIR / "latency_vs_workers.png"
)

print(
    OUTPUT_DIR / "scaling_efficiency.png"
)

print()
