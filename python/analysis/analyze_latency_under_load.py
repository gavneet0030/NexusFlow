from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = ROOT / "benchmarks" / "results" / "latency_under_load.csv"
OUTPUT_DIR = ROOT / "benchmarks" / "results" / "latency_analysis"

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


if not INPUT_FILE.exists():
    raise FileNotFoundError(
        f"Benchmark result file not found: {INPUT_FILE}"
    )


df = pd.read_csv(INPUT_FILE)


required_columns = [
    "scenario",
    "submitted",
    "accepted",
    "rejected",
    "processed",
    "throughput_eps",
    "p50_us",
    "p95_us",
    "p99_us",
    "p99_9_us",
    "max_us",
]


missing_columns = [
    column for column in required_columns
    if column not in df.columns
]


if missing_columns:
    raise ValueError(
        f"Missing required columns: {missing_columns}"
    )


df["rejection_rate_pct"] = (
    df["rejected"] / df["submitted"]
) * 100.0


df["acceptance_rate_pct"] = (
    df["accepted"] / df["submitted"]
) * 100.0


df["processing_integrity_pct"] = (
    df["processed"] / df["accepted"].replace(0, pd.NA)
) * 100.0


baseline_throughput = df.iloc[0]["throughput_eps"]

df["throughput_change_vs_low_pct"] = (
    (df["throughput_eps"] / baseline_throughput) - 1.0
) * 100.0


analysis_file = OUTPUT_DIR / "latency_under_load_analysis.csv"

df.to_csv(
    analysis_file,
    index=False
)


plt.figure(figsize=(10, 6))

plt.plot(
    df["scenario"],
    df["throughput_eps"],
    marker="o"
)

plt.xlabel("Workload Scenario")
plt.ylabel("Throughput (events/sec)")
plt.title("NexusFlow Throughput Under Increasing Load")
plt.xticks(rotation=25)
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "throughput_under_load.png",
    dpi=200
)

plt.close()


plt.figure(figsize=(10, 6))

plt.plot(
    df["scenario"],
    df["p50_us"],
    marker="o",
    label="P50"
)

plt.plot(
    df["scenario"],
    df["p95_us"],
    marker="o",
    label="P95"
)

plt.plot(
    df["scenario"],
    df["p99_us"],
    marker="o",
    label="P99"
)

plt.plot(
    df["scenario"],
    df["p99_9_us"],
    marker="o",
    label="P99.9"
)

plt.xlabel("Workload Scenario")
plt.ylabel("Latency (microseconds)")
plt.title("NexusFlow Tail Latency Under Increasing Load")
plt.xticks(rotation=25)
plt.legend()
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "latency_percentiles_under_load.png",
    dpi=200
)

plt.close()


plt.figure(figsize=(10, 6))

plt.plot(
    df["scenario"],
    df["max_us"],
    marker="o"
)

plt.xlabel("Workload Scenario")
plt.ylabel("Maximum Latency (microseconds)")
plt.title("NexusFlow Maximum Latency Under Load")
plt.xticks(rotation=25)
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "maximum_latency_under_load.png",
    dpi=200
)

plt.close()


plt.figure(figsize=(10, 6))

plt.plot(
    df["scenario"],
    df["rejection_rate_pct"],
    marker="o"
)

plt.xlabel("Workload Scenario")
plt.ylabel("Rejected Events (%)")
plt.title("NexusFlow Backpressure Rejection Rate")
plt.xticks(rotation=25)
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "rejection_rate_under_load.png",
    dpi=200
)

plt.close()


print("=" * 70)
print("NexusFlow Latency Under Load Analysis")
print("=" * 70)

print()

print(
    df[
        [
            "scenario",
            "throughput_eps",
            "p50_us",
            "p95_us",
            "p99_us",
            "p99_9_us",
            "max_us",
            "rejection_rate_pct",
        ]
    ].to_string(index=False)
)

print()

print("=" * 70)
print("Key Findings")
print("=" * 70)

highest_throughput = df.loc[
    df["throughput_eps"].idxmax()
]

highest_p99 = df.loc[
    df["p99_us"].idxmax()
]

highest_rejection = df.loc[
    df["rejection_rate_pct"].idxmax()
]

print(
    f"Highest throughput: "
    f"{highest_throughput['scenario']} "
    f"({highest_throughput['throughput_eps']:.2f} events/sec)"
)

print(
    f"Highest P99 latency: "
    f"{highest_p99['scenario']} "
    f"({highest_p99['p99_us']:.2f} us)"
)

print(
    f"Highest rejection rate: "
    f"{highest_rejection['scenario']} "
    f"({highest_rejection['rejection_rate_pct']:.2f}%)"
)

print()

print("=" * 70)
print("Generated Files")
print("=" * 70)

print(analysis_file)
print(OUTPUT_DIR / "throughput_under_load.png")
print(OUTPUT_DIR / "latency_percentiles_under_load.png")
print(OUTPUT_DIR / "maximum_latency_under_load.png")
print(OUTPUT_DIR / "rejection_rate_under_load.png")

print()

print("Analysis completed successfully.")
