from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = (
    ROOT
    / "benchmarks"
    / "results"
    / "fault_recovery.csv"
)

OUTPUT_DIR = (
    ROOT
    / "benchmarks"
    / "results"
    / "fault_analysis"
)

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


if not INPUT_FILE.exists():
    raise FileNotFoundError(
        f"Fault recovery file not found: {INPUT_FILE}"
    )


df = pd.read_csv(INPUT_FILE)


required_columns = [
    "submitted",
    "processed_before_failure",
    "processed_after_recovery",
    "processed_total",
    "remaining",
    "failure_detection_ms",
    "recovery_time_ms",
    "total_time_ms",
    "integrity",
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


df["pre_failure_processing_pct"] = (
    df["processed_before_failure"]
    / df["submitted"]
) * 100.0


df["post_recovery_processing_pct"] = (
    df["processed_after_recovery"]
    / df["submitted"]
) * 100.0


df["recovery_overhead_pct"] = (
    df["recovery_time_ms"]
    / df["total_time_ms"]
) * 100.0


df["recovery_throughput_eps"] = (
    df["processed_after_recovery"]
    / (df["recovery_time_ms"] / 1000.0)
)


df["integrity_passed"] = (
    df["integrity"].astype(str).str.upper()
    == "PASS"
)


output_file = (
    OUTPUT_DIR
    / "fault_recovery_analysis.csv"
)

df.to_csv(
    output_file,
    index=False
)


# Recovery timeline
labels = [
    "Before Failure",
    "After Recovery",
]

values = [
    float(df.iloc[0]["processed_before_failure"]),
    float(df.iloc[0]["processed_after_recovery"]),
]


plt.figure(figsize=(9, 6))

plt.bar(
    labels,
    values
)

plt.xlabel("Processing Stage")
plt.ylabel("Events")
plt.title(
    "NexusFlow Fault Recovery Processing"
)

plt.grid(
    axis="y",
    alpha=0.3
)

plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "fault_recovery_processing.png",
    dpi=200
)

plt.close()


# Recovery timing
timing_labels = [
    "Failure Detection",
    "Recovery",
    "Total Benchmark",
]

timing_values = [
    float(df.iloc[0]["failure_detection_ms"]),
    float(df.iloc[0]["recovery_time_ms"]),
    float(df.iloc[0]["total_time_ms"]),
]


plt.figure(figsize=(9, 6))

plt.bar(
    timing_labels,
    timing_values
)

plt.xlabel("Timing Category")
plt.ylabel("Time (ms)")
plt.title(
    "NexusFlow Fault Recovery Timing"
)

plt.grid(
    axis="y",
    alpha=0.3
)

plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "fault_recovery_timing.png",
    dpi=200
)

plt.close()


# Recovery throughput
plt.figure(figsize=(8, 6))

plt.bar(
    ["Recovery"],
    [float(df.iloc[0]["recovery_throughput_eps"])]
)

plt.ylabel("Throughput (events/sec)")
plt.title(
    "NexusFlow Recovery Throughput"
)

plt.grid(
    axis="y",
    alpha=0.3
)

plt.tight_layout()

plt.savefig(
    OUTPUT_DIR / "recovery_throughput.png",
    dpi=200
)

plt.close()


row = df.iloc[0]

print("=" * 72)
print("NexusFlow Fault Recovery Analysis")
print("=" * 72)

print()

print(
    f"Submitted events:          "
    f"{int(row['submitted'])}"
)

print(
    f"Processed before failure:  "
    f"{int(row['processed_before_failure'])}"
)

print(
    f"Processed after recovery:  "
    f"{int(row['processed_after_recovery'])}"
)

print(
    f"Final processed:           "
    f"{int(row['processed_total'])}"
)

print(
    f"Remaining queue:           "
    f"{int(row['remaining'])}"
)

print()

print(
    f"Failure detection time:    "
    f"{row['failure_detection_ms']:.3f} ms"
)

print(
    f"Recovery time:             "
    f"{row['recovery_time_ms']:.3f} ms"
)

print(
    f"Total benchmark time:      "
    f"{row['total_time_ms']:.3f} ms"
)

print(
    f"Recovery overhead:         "
    f"{row['recovery_overhead_pct']:.2f}%"
)

print(
    f"Recovery throughput:       "
    f"{row['recovery_throughput_eps']:.2f} events/sec"
)

print()

print("=" * 72)
print("Integrity")
print("=" * 72)

print(
    f"Integrity result: "
    f"{row['integrity']}"
)

print(
    f"All events recovered: "
    f"{int(row['processed_total']) == int(row['submitted'])}"
)

print(
    f"Queue completely drained: "
    f"{int(row['remaining']) == 0}"
)

print()

print("=" * 72)
print("Generated Files")
print("=" * 72)

print(output_file)

print(
    OUTPUT_DIR
    / "fault_recovery_processing.png"
)

print(
    OUTPUT_DIR
    / "fault_recovery_timing.png"
)

print(
    OUTPUT_DIR
    / "recovery_throughput.png"
)

print()

print("Analysis completed successfully.")
