import os
import pandas as pd

INPUT = "benchmarks/results/real_end_to_end_latency_multi_load.csv"
OUTPUT_DIR = "benchmarks/results/real_end_to_end_latency_analysis"

os.makedirs(OUTPUT_DIR, exist_ok=True)

df = pd.read_csv(INPUT)

numeric_columns = [
    "target_rate_eps",
    "throughput_eps",
    "queue_wait_p50_us",
    "queue_wait_p95_us",
    "queue_wait_p99_us",
    "queue_wait_p999_us",
    "queue_wait_max_us",
    "processing_p50_us",
    "processing_p95_us",
    "processing_p99_us",
    "processing_p999_us",
    "processing_max_us",
    "completion_p50_us",
    "completion_p95_us",
    "completion_p99_us",
    "completion_p999_us",
    "completion_max_us",
    "e2e_p50_us",
    "e2e_p95_us",
    "e2e_p99_us",
    "e2e_p999_us",
    "e2e_max_us",
    "workers",
    "queue_depth",
    "throttled",
]

for column in numeric_columns:
    if column in df.columns:
        df[column] = pd.to_numeric(df[column], errors="coerce")

summary = (
    df.groupby("target_rate_eps")
    .agg(
        throughput_mean_eps=("throughput_eps", "mean"),
        throughput_std_eps=("throughput_eps", "std"),
        queue_wait_p50_mean_us=("queue_wait_p50_us", "mean"),
        queue_wait_p95_mean_us=("queue_wait_p95_us", "mean"),
        queue_wait_p99_mean_us=("queue_wait_p99_us", "mean"),
        queue_wait_p999_mean_us=("queue_wait_p999_us", "mean"),
        processing_p50_mean_us=("processing_p50_us", "mean"),
        processing_p95_mean_us=("processing_p95_us", "mean"),
        processing_p99_mean_us=("processing_p99_us", "mean"),
        processing_p999_mean_us=("processing_p999_us", "mean"),
        e2e_p50_mean_us=("e2e_p50_us", "mean"),
        e2e_p95_mean_us=("e2e_p95_us", "mean"),
        e2e_p99_mean_us=("e2e_p99_us", "mean"),
        e2e_p999_mean_us=("e2e_p999_us", "mean"),
        e2e_max_observed_us=("e2e_max_us", "max"),
        average_workers=("workers", "mean"),
        maximum_queue_depth=("queue_depth", "max"),
        average_throttled=("throttled", "mean"),
    )
    .reset_index()
)

summary["efficiency_percent"] = (
    summary["throughput_mean_eps"]
    / summary["target_rate_eps"]
    * 100.0
)

summary["throughput_cv_percent"] = (
    summary["throughput_std_eps"]
    / summary["throughput_mean_eps"]
    * 100.0
)

summary["queue_wait_fraction_percent"] = (
    summary["queue_wait_p50_mean_us"]
    / summary["e2e_p50_mean_us"]
    * 100.0
)

summary["processing_fraction_percent"] = (
    summary["processing_p50_mean_us"]
    / summary["e2e_p50_mean_us"]
    * 100.0
)

summary["sla_p99_pass"] = summary["e2e_p99_mean_us"] <= 100.0
summary["sla_p999_pass"] = summary["e2e_p999_mean_us"] <= 1000.0
summary["efficiency_pass"] = summary["efficiency_percent"] >= 80.0
summary["stability_pass"] = summary["throughput_cv_percent"] <= 5.0

summary["overall_sla_pass"] = (
    summary["sla_p99_pass"]
    & summary["sla_p999_pass"]
    & summary["efficiency_pass"]
    & summary["stability_pass"]
)

summary_path = os.path.join(OUTPUT_DIR, "summary.csv")
summary.to_csv(summary_path, index=False)

print()
print("========================================")
print("NEXUSFLOW REAL E2E ANALYSIS")
print("========================================")
print()
print(f"Runs analyzed: {len(df)}")
print(f"Load levels: {df['target_rate_eps'].nunique()}")
print()

display_columns = [
    "target_rate_eps",
    "throughput_mean_eps",
    "efficiency_percent",
    "e2e_p50_mean_us",
    "e2e_p95_mean_us",
    "e2e_p99_mean_us",
    "e2e_p999_mean_us",
    "e2e_max_observed_us",
    "average_workers",
    "average_throttled",
]

print(summary[display_columns].to_string(index=False))

print()
print("Queue-wait dominance:")
print(
    summary[
        [
            "target_rate_eps",
            "queue_wait_fraction_percent",
            "processing_fraction_percent",
        ]
    ].to_string(index=False)
)

print()
print("SLA assessment:")
print(
    summary[
        [
            "target_rate_eps",
            "sla_p99_pass",
            "sla_p999_pass",
            "efficiency_pass",
            "stability_pass",
            "overall_sla_pass",
        ]
    ].to_string(index=False)
)

print()
print(f"Summary written to: {summary_path}")
print()
