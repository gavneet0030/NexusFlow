from pathlib import Path
import csv

import matplotlib.pyplot as plt

root = Path(r"C:\Users\PC\NexusFlow")
analysis = root / "benchmarks" / "results" / "analysis"
csv_path = analysis / "capacity_sweep_analysis.csv"

rows = []

with csv_path.open("r", encoding="utf-8-sig", newline="") as f:
    for row in csv.DictReader(f):
        rows.append({
            "load": float(row["LoadEPS"]),
            "throughput": float(row["ThroughputEPS"]),
            "efficiency": float(row["EfficiencyPct"]),
            "p99": float(row["P99us"]),
            "workers": int(row["Workers"]),
            "throttled": int(row["Throttled"]),
        })

rows.sort(key=lambda x: x["load"])

load = [r["load"] for r in rows]
throughput = [r["throughput"] for r in rows]
efficiency = [r["efficiency"] for r in rows]
p99 = [r["p99"] for r in rows]
workers = [r["workers"] for r in rows]
throttled = [r["throttled"] for r in rows]

# 1. Throughput and offered load
plt.figure(figsize=(9, 5.5))
plt.plot(load, load, marker="o", label="Offered Load")
plt.plot(load, throughput, marker="o", label="Achieved Throughput")
plt.xlabel("Event Rate (EPS)")
plt.ylabel("Events Per Second")
plt.title("NexusFlow Capacity: Offered Load vs Throughput")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(analysis / "capacity_offered_vs_throughput.png", dpi=180)
plt.close()

# 2. Throughput efficiency
plt.figure(figsize=(9, 5.5))
plt.plot(load, efficiency, marker="o")
plt.axhline(80.0, linestyle="--", label="80% Efficiency Threshold")
plt.xlabel("Offered Load (EPS)")
plt.ylabel("Throughput Efficiency (%)")
plt.title("NexusFlow Capacity Efficiency")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(analysis / "capacity_efficiency.png", dpi=180)
plt.close()

# 3. P99 latency
plt.figure(figsize=(9, 5.5))
plt.plot(load, p99, marker="o")
plt.xlabel("Offered Load (EPS)")
plt.ylabel("P99 Processing Latency (us)")
plt.title("NexusFlow P99 Processing Latency")
plt.grid(True)
plt.tight_layout()
plt.savefig(analysis / "capacity_p99_latency.png", dpi=180)
plt.close()

# 4. Worker scaling
plt.figure(figsize=(9, 5.5))
plt.plot(load, workers, marker="o")
plt.xlabel("Offered Load (EPS)")
plt.ylabel("Observed Workers")
plt.title("NexusFlow Adaptive Worker Scaling")
plt.grid(True)
plt.tight_layout()
plt.savefig(analysis / "capacity_worker_scaling.png", dpi=180)
plt.close()

# 5. Throttling
plt.figure(figsize=(9, 5.5))
plt.plot(load, throttled, marker="o")
plt.xlabel("Offered Load (EPS)")
plt.ylabel("Throttled Events")
plt.title("NexusFlow Backpressure and Throttling")
plt.grid(True)
plt.tight_layout()
plt.savefig(analysis / "capacity_throttling.png", dpi=180)
plt.close()

# 6. Combined research figure: throughput efficiency
fig = plt.figure(figsize=(10, 6))
ax1 = fig.add_subplot(111)

ax1.plot(load, throughput, marker="o")
ax1.set_xlabel("Offered Load (EPS)")
ax1.set_ylabel("Throughput (EPS)")
ax1.grid(True)

ax2 = ax1.twinx()
ax2.plot(load, efficiency, marker="s", linestyle="--")
ax2.set_ylabel("Efficiency (%)")

plt.title("NexusFlow Adaptive Capacity Profile")
fig.tight_layout()
fig.savefig(analysis / "nexusflow_capacity_profile.png", dpi=200)
plt.close()

print("============================================")
print("NEXUSFLOW: CAPACITY VISUALIZATION")
print("============================================")
print("")
print(f"Records: {len(rows)}")
print("Plots generated: 6")
print("")
print("Generated files:")

for name in [
    "capacity_offered_vs_throughput.png",
    "capacity_efficiency.png",
    "capacity_p99_latency.png",
    "capacity_worker_scaling.png",
    "capacity_throttling.png",
    "nexusflow_capacity_profile.png",
]:
    print(analysis / name)

print("")
print("VISUALIZATION: PASS")
print("============================================")
