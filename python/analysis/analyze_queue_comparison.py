from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = (
    ROOT
    / "benchmarks"
    / "results"
    / "queue_comparison.csv"
)

OUTPUT_DIR = (
    ROOT
    / "benchmarks"
    / "results"
    / "queue_analysis"
)

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


if not INPUT_FILE.exists():
    raise FileNotFoundError(
        f"Queue comparison file not found: {INPUT_FILE}"
    )


df = pd.read_csv(INPUT_FILE)


required_columns = [
    "queue_type",
    "producers",
    "submitted",
    "accepted",
    "processed",
    "throughput_eps",
    "p50_us",
    "p95_us",
    "p99_us",
    "p99_9_us",
    "max_us",
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


df["integrity_pct"] = (
    df["processed"]
    / df["submitted"]
) * 100.0


mutex = (
    df[df["queue_type"] == "MUTEX"]
    .set_index("producers")
)

lock_free = (
    df[df["queue_type"] == "LOCK_FREE"]
    .set_index("producers")
)


comparison_rows = []


for producers in sorted(
    set(mutex.index) & set(lock_free.index)
):
    mutex_row = mutex.loc[producers]
    lock_free_row = lock_free.loc[producers]

    throughput_speedup_pct = (
        (
            lock_free_row["throughput_eps"]
            / mutex_row["throughput_eps"]
        ) - 1.0
    ) * 100.0

    p50_change_pct = (
        (
            lock_free_row["p50_us"]
            / mutex_row["p50_us"]
        ) - 1.0
    ) * 100.0

    p95_change_pct = (
        (
            lock_free_row["p95_us"]
            / mutex_row["p95_us"]
        ) - 1.0
    ) * 100.0

    p99_change_pct = (
        (
            lock_free_row["p99_us"]
            / mutex_row["p99_us"]
        ) - 1.0
    ) * 100.0

    p999_change_pct = (
        (
            lock_free_row["p99_9_us"]
            / mutex_row["p99_9_us"]
        ) - 1.0
    ) * 100.0

    max_change_pct = (
        (
            lock_free_row["max_us"]
            / mutex_row["max_us"]
        ) - 1.0
    ) * 100.0

    comparison_rows.append(
        {
            "producers": producers,
            "mutex_throughput_eps":
                mutex_row["throughput_eps"],
            "lock_free_throughput_eps":
                lock_free_row["throughput_eps"],
            "throughput_improvement_pct":
                throughput_speedup_pct,
            "mutex_p50_us":
                mutex_row["p50_us"],
            "lock_free_p50_us":
                lock_free_row["p50_us"],
            "p50_change_pct":
                p50_change_pct,
            "mutex_p95_us":
                mutex_row["p95_us"],
            "lock_free_p95_us":
                lock_free_row["p95_us"],
            "p95_change_pct":
                p95_change_pct,
            "mutex_p99_us":
                mutex_row["p99_us"],
            "lock_free_p99_us":
                lock_free_row["p99_us"],
            "p99_change_pct":
                p99_change_pct,
            "mutex_p99_9_us":
                mutex_row["p99_9_us"],
            "lock_free_p99_9_us":
                lock_free_row["p99_9_us"],
            "p99_9_change_pct":
                p999_change_pct,
            "mutex_max_us":
                mutex_row["max_us"],
            "lock_free_max_us":
                lock_free_row["max_us"],
            "max_change_pct":
                max_change_pct,
        }
    )


comparison = pd.DataFrame(comparison_rows)


output_csv = (
    OUTPUT_DIR
    / "mutex_vs_lock_free_analysis.csv"
)

comparison.to_csv(
    output_csv,
    index=False
)


plt.figure(figsize=(10, 6))

plt.plot(
    comparison["producers"],
    comparison["mutex_throughput_eps"],
    marker="o",
    label="Mutex"
)

plt.plot(
    comparison["producers"],
    comparison["lock_free_throughput_eps"],
    marker="o",
    label="Lock-Free"
)

plt.xlabel("Number of Producers")
plt.ylabel("Throughput (events/sec)")
plt.title(
    "Mutex vs Lock-Free Queue Throughput"
)

plt.legend()
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR
    / "throughput_comparison.png",
    dpi=200
)

plt.close()


plt.figure(figsize=(10, 6))

plt.plot(
    comparison["producers"],
    comparison["mutex_p99_us"],
    marker="o",
    label="Mutex P99"
)

plt.plot(
    comparison["producers"],
    comparison["lock_free_p99_us"],
    marker="o",
    label="Lock-Free P99"
)

plt.xlabel("Number of Producers")
plt.ylabel("P99 Latency (microseconds)")
plt.title(
    "P99 Latency Under Producer Concurrency"
)

plt.legend()
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR
    / "p99_latency_comparison.png",
    dpi=200
)

plt.close()


plt.figure(figsize=(10, 6))

plt.plot(
    comparison["producers"],
    comparison["p99_9_change_pct"],
    marker="o"
)

plt.axhline(
    0,
    linewidth=1
)

plt.xlabel("Number of Producers")
plt.ylabel("P99.9 Change (%)")
plt.title(
    "Lock-Free vs Mutex P99.9 Latency Change"
)

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR
    / "p999_latency_change.png",
    dpi=200
)

plt.close()


plt.figure(figsize=(10, 6))

plt.plot(
    comparison["producers"],
    comparison["throughput_improvement_pct"],
    marker="o"
)

plt.axhline(
    0,
    linewidth=1
)

plt.xlabel("Number of Producers")
plt.ylabel("Throughput Improvement (%)")
plt.title(
    "Lock-Free Throughput Improvement"
)

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    OUTPUT_DIR
    / "throughput_improvement.png",
    dpi=200
)

plt.close()


print("=" * 72)
print("NexusFlow Mutex vs Lock-Free Queue Analysis")
print("=" * 72)

print()

print(
    comparison[
        [
            "producers",
            "mutex_throughput_eps",
            "lock_free_throughput_eps",
            "throughput_improvement_pct",
            "mutex_p99_us",
            "lock_free_p99_us",
            "p99_change_pct",
        ]
    ].to_string(index=False)
)

print()

best_throughput = comparison.loc[
    comparison["throughput_improvement_pct"].idxmax()
]

best_p99 = comparison.loc[
    comparison["p99_change_pct"].idxmin()
]

print("=" * 72)
print("Key Findings")
print("=" * 72)

print(
    f"Maximum throughput improvement: "
    f"{best_throughput['throughput_improvement_pct']:.2f}% "
    f"at "
    f"{int(best_throughput['producers'])} producers."
)

print(
    f"Best P99 latency change: "
    f"{best_p99['p99_change_pct']:.2f}% "
    f"at "
    f"{int(best_p99['producers'])} producers."
)

print()

print("Generated files:")
print(output_csv)
print(
    OUTPUT_DIR
    / "throughput_comparison.png"
)
print(
    OUTPUT_DIR
    / "p99_latency_comparison.png"
)
print(
    OUTPUT_DIR
    / "p999_latency_change.png"
)
print(
    OUTPUT_DIR
    / "throughput_improvement.png"
)

print()

print("Analysis completed successfully.")
