from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = (
    ROOT
    / "benchmarks"
    / "results"
    / "resource_utilization.csv"
)

OUTPUT_DIR = (
    ROOT
    / "benchmarks"
    / "results"
    / "resource_analysis"
)

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


if not INPUT_FILE.exists():
    raise FileNotFoundError(
        f"Resource benchmark file not found: {INPUT_FILE}"
    )


df = pd.read_csv(INPUT_FILE)


required_columns = [
    "workers",
    "submitted",
    "processed",
    "throughput_eps",
    "cpu_percent",
    "peak_memory_mb",
]


missing = [
    column
    for column in required_columns
    if column not in df.columns
]


if missing:
    raise ValueError(
        f"Missing required columns: {missing}"
    )


df = df.sort_values("workers").reset_index(drop=True)


baseline_throughput = (
    df.iloc[0]["throughput_eps"]
)


df["speedup"] = (
    df["throughput_eps"]
    / baseline_throughput
)


df["scaling_efficiency_pct"] = (
    df["speedup"]
    / df["workers"]
) * 100.0


df["throughput_per_cpu_pct"] = (
    df["throughput_eps"]
    / df["cpu_percent"].replace(0, pd.NA)
)


df["memory_per_processed_event_kb"] = (
    df["peak_memory_mb"] * 1024
    / df["processed"].replace(0, pd.NA)
)


output_csv = (
    OUTPUT_DIR
    / "resource_scaling_analysis.csv"
)

df.to_csv(
    output_csv,
    index=False
)


# Throughput and speedup
plt.figure(figsize=(10, 6))

plt.plot(
    df["workers"],
    df["throughput_eps"],
    marker="o",
    label="Throughput"
)

plt.xlabel("Worker Count")
plt.ylabel("Throughput (events/sec)")
plt.title(
    "NexusFlow Throughput Scaling"
)

plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "throughput_scaling.png",
    dpi=200
)

plt.close()


# Speedup
plt.figure(figsize=(10, 6))

plt.plot(
    df["workers"],
    df["speedup"],
    marker="o",
    label="Measured Speedup"
)

plt.plot(
    df["workers"],
    df["workers"],
    marker="o",
    label="Ideal Linear Speedup"
)

plt.xlabel("Worker Count")
plt.ylabel("Speedup")
plt.title(
    "Measured vs Ideal Worker Scaling"
)

plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "speedup_vs_ideal.png",
    dpi=200
)

plt.close()


# Scaling efficiency
plt.figure(figsize=(10, 6))

plt.plot(
    df["workers"],
    df["scaling_efficiency_pct"],
    marker="o"
)

plt.axhline(
    100,
    linewidth=1
)

plt.xlabel("Worker Count")
plt.ylabel("Scaling Efficiency (%)")
plt.title(
    "NexusFlow Worker Scaling Efficiency"
)

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "scaling_efficiency.png",
    dpi=200
)

plt.close()


# CPU utilization
plt.figure(figsize=(10, 6))

plt.plot(
    df["workers"],
    df["cpu_percent"],
    marker="o"
)

plt.xlabel("Worker Count")
plt.ylabel("CPU Utilization (%)")
plt.title(
    "CPU Utilization vs Worker Count"
)

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "cpu_utilization.png",
    dpi=200
)

plt.close()


# Memory
plt.figure(figsize=(10, 6))

plt.plot(
    df["workers"],
    df["peak_memory_mb"],
    marker="o"
)

plt.xlabel("Worker Count")
plt.ylabel("Peak Memory (MB)")
plt.title(
    "Peak Memory vs Worker Count"
)

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "memory_scaling.png",
    dpi=200
)

plt.close()


# Throughput efficiency
plt.figure(figsize=(10, 6))

plt.plot(
    df["workers"],
    df["throughput_per_cpu_pct"],
    marker="o"
)

plt.xlabel("Worker Count")
plt.ylabel(
    "Throughput per CPU Percentage Point"
)

plt.title(
    "Throughput Efficiency Relative to CPU"
)

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "throughput_cpu_efficiency.png",
    dpi=200
)

plt.close()


print("=" * 72)
print("NexusFlow Resource Utilization Analysis")
print("=" * 72)

print()

display_columns = [
    "workers",
    "throughput_eps",
    "speedup",
    "scaling_efficiency_pct",
    "cpu_percent",
    "peak_memory_mb",
    "throughput_per_cpu_pct",
]

print(
    df[display_columns].to_string(
        index=False
    )
)

print()

best_scaling = df.loc[
    df["scaling_efficiency_pct"].idxmax()
]

best_throughput = df.loc[
    df["throughput_eps"].idxmax()
]

lowest_memory = df.loc[
    df["peak_memory_mb"].idxmin()
]

print("=" * 72)
print("Key Findings")
print("=" * 72)

print(
    f"Best scaling efficiency: "
    f"{best_scaling['scaling_efficiency_pct']:.2f}% "
    f"at "
    f"{int(best_scaling['workers'])} workers."
)

print(
    f"Highest throughput: "
    f"{best_throughput['throughput_eps']:.2f} events/sec "
    f"at "
    f"{int(best_throughput['workers'])} workers."
)

print(
    f"Lowest measured peak memory: "
    f"{lowest_memory['peak_memory_mb']:.2f} MB "
    f"at "
    f"{int(lowest_memory['workers'])} workers."
)

print()

print("=" * 72)
print("Generated Files")
print("=" * 72)

print(output_csv)
print(
    OUTPUT_DIR / "throughput_scaling.png"
)
print(
    OUTPUT_DIR / "speedup_vs_ideal.png"
)
print(
    OUTPUT_DIR / "scaling_efficiency.png"
)
print(
    OUTPUT_DIR / "cpu_utilization.png"
)
print(
    OUTPUT_DIR / "memory_scaling.png"
)
print(
    OUTPUT_DIR / "throughput_cpu_efficiency.png"
)

print()

print("Analysis completed successfully.")
