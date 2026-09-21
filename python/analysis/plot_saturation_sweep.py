import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

INPUT = Path("benchmarks/results/saturation_sweep_raw.csv")
OUTPUT = Path("benchmarks/results/saturation_analysis")

OUTPUT.mkdir(parents=True, exist_ok=True)

df = pd.read_csv(INPUT)

df = df[df["integrity_pass"] == "PASS"].copy()

numeric_columns = [
    "target_rate_eps",
    "throughput_eps",
    "p50_us",
    "p95_us",
    "p99_us",
    "p999_us",
    "max_us",
    "workers",
    "throttled",
]

for column in numeric_columns:
    df[column] = pd.to_numeric(df[column], errors="coerce")

summary = (
    df.groupby("target_rate_eps")
    .agg(
        throughput_mean_eps=("throughput_eps", "mean"),
        throughput_std_eps=("throughput_eps", "std"),
        p50_mean_us=("p50_us", "mean"),
        p95_mean_us=("p95_us", "mean"),
        p99_mean_us=("p99_us", "mean"),
        p999_mean_us=("p999_us", "mean"),
        max_latency_us=("max_us", "max"),
        workers_mean=("workers", "mean"),
        throttled_mean=("throttled", "mean"),
    )
    .reset_index()
)

summary["throughput_efficiency_percent"] = (
    summary["throughput_mean_eps"]
    / summary["target_rate_eps"]
    * 100.0
)

summary["throughput_cv_percent"] = (
    summary["throughput_std_eps"]
    / summary["throughput_mean_eps"]
    * 100.0
)

summary.to_csv(
    OUTPUT / "plot_summary.csv",
    index=False
)

x = summary["target_rate_eps"]


def save_plot(filename, title, ylabel):
    plt.title(title)
    plt.xlabel("Offered Load (events/sec)")
    plt.ylabel(ylabel)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(OUTPUT / filename, dpi=200)
    plt.close()


# 1. Throughput vs offered load
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["throughput_mean_eps"],
    marker="o",
    linewidth=2
)
plt.axvline(
    20000,
    linestyle="--",
    linewidth=1.5,
    label="Estimated saturation knee: 20K EPS"
)
plt.axvline(
    30000,
    linestyle="--",
    linewidth=1.5,
    label="Recommended operating point: 30K EPS"
)
plt.legend()
save_plot(
    "throughput_vs_load.png",
    "NexusFlow Throughput Saturation",
    "Achieved Throughput (events/sec)"
)


# 2. Throughput efficiency
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["throughput_efficiency_percent"],
    marker="o",
    linewidth=2
)
plt.axhline(
    80,
    linestyle="--",
    linewidth=1.5,
    label="80% efficiency"
)
plt.legend()
save_plot(
    "throughput_efficiency.png",
    "Throughput Efficiency vs Offered Load",
    "Throughput Efficiency (%)"
)


# 3. Latency percentiles
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["p50_mean_us"],
    marker="o",
    label="P50"
)
plt.plot(
    x,
    summary["p95_mean_us"],
    marker="o",
    label="P95"
)
plt.plot(
    x,
    summary["p99_mean_us"],
    marker="o",
    label="P99"
)
plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Latency (microseconds)")
plt.title("NexusFlow Latency Percentiles")
plt.legend()
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    OUTPUT / "latency_percentiles.png",
    dpi=200
)
plt.close()


# 4. P99.9 latency
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["p999_mean_us"],
    marker="o",
    linewidth=2
)
plt.xlabel("Offered Load (events/sec)")
plt.ylabel("P99.9 Latency (microseconds)")
plt.title("NexusFlow Tail Latency: P99.9")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    OUTPUT / "p999_latency.png",
    dpi=200
)
plt.close()


# 5. Maximum latency
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["max_latency_us"],
    marker="o",
    linewidth=2
)
plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Maximum Latency (microseconds)")
plt.title("NexusFlow Maximum Observed Latency")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    OUTPUT / "maximum_latency.png",
    dpi=200
)
plt.close()


# 6. Throughput variability
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["throughput_cv_percent"],
    marker="o",
    linewidth=2
)
plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Coefficient of Variation (%)")
plt.title("Throughput Variability")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    OUTPUT / "throughput_variability.png",
    dpi=200
)
plt.close()


# 7. Worker scaling
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["workers_mean"],
    marker="o",
    linewidth=2
)
plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Average Active Workers")
plt.title("Adaptive Worker Scaling")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    OUTPUT / "worker_scaling.png",
    dpi=200
)
plt.close()


# 8. Throttling
plt.figure(figsize=(10, 6))
plt.plot(
    x,
    summary["throttled_mean"],
    marker="o",
    linewidth=2
)
plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Average Throttled Events")
plt.title("Backpressure and Throttling vs Offered Load")
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig(
    OUTPUT / "throttling_vs_load.png",
    dpi=200
)
plt.close()


print("Saturation plots generated successfully.")
print(f"Output directory: {OUTPUT}")
print("")
print("Generated files:")

for path in sorted(OUTPUT.glob("*.png")):
    print(path.name)

print("plot_summary.csv")
