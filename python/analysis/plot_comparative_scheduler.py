import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

BASE = Path("benchmarks/results/comparative_scheduler")
ANALYSIS = BASE / "analysis"

df = pd.read_csv(ANALYSIS / "summary.csv")

# Throughput
plt.figure()
for mode in df["mode"].unique():
    x = df[df["mode"] == mode]
    plt.plot(
        x["target_rate_eps"],
        x["throughput_mean_eps"],
        marker="o",
        label=mode
    )

plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Mean Throughput (events/sec)")
plt.title("Scheduler Throughput vs Offered Load")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(ANALYSIS / "throughput_comparison.png", dpi=200)
plt.close()

# P99 latency
plt.figure()
for mode in df["mode"].unique():
    x = df[df["mode"] == mode]
    plt.plot(
        x["target_rate_eps"],
        x["p99_mean_us"],
        marker="o",
        label=mode
    )

plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Mean P99 Processing Latency (us)")
plt.title("P99 Processing Latency vs Offered Load")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(ANALYSIS / "p99_comparison.png", dpi=200)
plt.close()

# P99.9 latency
plt.figure()
for mode in df["mode"].unique():
    x = df[df["mode"] == mode]
    plt.plot(
        x["target_rate_eps"],
        x["p999_mean_us"],
        marker="o",
        label=mode
    )

plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Mean P99.9 Processing Latency (us)")
plt.title("P99.9 Processing Latency vs Offered Load")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(ANALYSIS / "p999_comparison.png", dpi=200)
plt.close()

# Efficiency
plt.figure()
for mode in df["mode"].unique():
    x = df[df["mode"] == mode]
    plt.plot(
        x["target_rate_eps"],
        x["efficiency_percent"],
        marker="o",
        label=mode
    )

plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Throughput Efficiency (%)")
plt.title("Throughput Efficiency vs Offered Load")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(ANALYSIS / "efficiency_comparison.png", dpi=200)
plt.close()

# Worker utilization
plt.figure()
for mode in df["mode"].unique():
    x = df[df["mode"] == mode]
    plt.plot(
        x["target_rate_eps"],
        x["average_workers"],
        marker="o",
        label=mode
    )

plt.xlabel("Offered Load (events/sec)")
plt.ylabel("Average Workers")
plt.title("Worker Scaling vs Offered Load")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(ANALYSIS / "worker_scaling_comparison.png", dpi=200)
plt.close()

print("")
print("========================================")
print("COMPARATIVE VISUALIZATION COMPLETE")
print("========================================")
print("")
print(f"  {ANALYSIS / 'throughput_comparison.png'}")
print(f"  {ANALYSIS / 'p99_comparison.png'}")
print(f"  {ANALYSIS / 'p999_comparison.png'}")
print(f"  {ANALYSIS / 'efficiency_comparison.png'}")
print(f"  {ANALYSIS / 'worker_scaling_comparison.png'}")
