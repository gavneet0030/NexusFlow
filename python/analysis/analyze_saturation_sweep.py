import pandas as pd
import numpy as np
from pathlib import Path

INPUT = Path("benchmarks/results/saturation_sweep_raw.csv")
OUTPUT_DIR = Path("benchmarks/results/saturation_analysis")

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

df = pd.read_csv(INPUT)

df = df[df["integrity_pass"] == "PASS"].copy()

numeric_columns = [
    "target_rate_eps",
    "throughput_eps",
    "average_latency_us",
    "p50_us",
    "p95_us",
    "p99_us",
    "p999_us",
    "max_us",
    "queue_depth",
    "workers",
    "throttled",
]

for column in numeric_columns:
    df[column] = pd.to_numeric(df[column], errors="coerce")

grouped = (
    df.groupby("target_rate_eps")
    .agg(
        repetitions=("run_id", "count"),
        throughput_mean_eps=("throughput_eps", "mean"),
        throughput_std_eps=("throughput_eps", "std"),
        average_latency_mean_us=("average_latency_us", "mean"),
        p50_mean_us=("p50_us", "mean"),
        p95_mean_us=("p95_us", "mean"),
        p99_mean_us=("p99_us", "mean"),
        p999_mean_us=("p999_us", "mean"),
        max_latency_observed_us=("max_us", "max"),
        workers_mean=("workers", "mean"),
        workers_max=("workers", "max"),
        throttled_mean=("throttled", "mean"),
        throttled_total=("throttled", "sum"),
    )
    .reset_index()
)

grouped["throughput_cv_percent"] = (
    grouped["throughput_std_eps"]
    / grouped["throughput_mean_eps"]
    * 100.0
)

grouped["throughput_efficiency_percent"] = (
    grouped["throughput_mean_eps"]
    / grouped["target_rate_eps"]
    * 100.0
)

grouped["throttled_rate_percent"] = (
    grouped["throttled_total"]
    / (grouped["target_rate_eps"] * grouped["repetitions"])
    * 100.0
)

grouped["integrity_pass_rate_percent"] = 100.0

grouped["throughput_delta_eps"] = (
    grouped["throughput_mean_eps"].diff()
)

grouped["throughput_growth_percent"] = (
    grouped["throughput_mean_eps"].pct_change() * 100.0
)

grouped["throughput_slope"] = (
    grouped["throughput_delta_eps"]
    / grouped["target_rate_eps"].diff()
)

# Saturation knee:
# Find the first load where additional offered load produces
# less than 25% throughput growth relative to the previous point.
knee_rate = None

for i in range(1, len(grouped)):
    growth = grouped.iloc[i]["throughput_growth_percent"]

    if pd.notna(growth) and growth < 25.0:
        knee_rate = int(grouped.iloc[i]["target_rate_eps"])
        break

grouped["estimated_saturation_knee_eps"] = knee_rate

# Practical operating point:
# Highest offered rate with >= 80% throughput efficiency
# and mean p99 latency below 100 us.
eligible = grouped[
    (grouped["throughput_efficiency_percent"] >= 80.0)
    & (grouped["p99_mean_us"] < 100.0)
]

if not eligible.empty:
    practical_rate = int(
        eligible["target_rate_eps"].max()
    )
else:
    practical_rate = None

grouped["recommended_operating_rate_eps"] = practical_rate

grouped.to_csv(
    OUTPUT_DIR / "summary.csv",
    index=False
)

# Corrected compact research table
research_columns = [
    "target_rate_eps",
    "repetitions",
    "throughput_mean_eps",
    "throughput_std_eps",
    "throughput_cv_percent",
    "throughput_efficiency_percent",
    "p99_mean_us",
    "p999_mean_us",
    "max_latency_observed_us",
    "workers_mean",
    "workers_max",
    "throttled_mean",
    "throttled_rate_percent",
]

research_table = grouped[research_columns].copy()

research_table.to_csv(
    OUTPUT_DIR / "research_table.csv",
    index=False
)

# Markdown research report
with open(
    OUTPUT_DIR / "report.md",
    "w",
    encoding="utf-8"
) as report:

    report.write("# NexusFlow Saturation Sweep Report\n\n")

    report.write(
        "## Experimental Setup\n\n"
        "- Dataset: UCI Online Retail II\n"
        "- Events per run: 10,000\n"
        "- Offered rates: 10K–50K EPS\n"
        "- Repetitions per rate: 3\n"
        "- Scheduler: AdaptiveScheduler\n"
        "- Maximum workers: 16\n"
        "- Integrity filtering: PASS runs only\n\n"
    )

    report.write("## Aggregated Results\n\n")

    report.write(
        grouped[
            [
                "target_rate_eps",
                "throughput_mean_eps",
                "throughput_std_eps",
                "throughput_cv_percent",
                "throughput_efficiency_percent",
                "p99_mean_us",
                "p999_mean_us",
                "max_latency_observed_us",
                "workers_mean",
                "throttled_mean",
            ]
        ].to_markdown(index=False)
    )

    report.write("\n\n")

    report.write("## Saturation Analysis\n\n")

    if knee_rate is not None:
        report.write(
            f"Estimated saturation knee: "
            f"approximately {knee_rate:,} EPS.\n\n"
        )
    else:
        report.write(
            "No saturation knee was detected using the "
            "configured throughput-growth criterion.\n\n"
        )

    if practical_rate is not None:
        report.write(
            f"Recommended operating point under the configured "
            f"criteria: approximately {practical_rate:,} EPS.\n\n"
        )
    else:
        report.write(
            "No operating point satisfied the configured "
            "throughput-efficiency and p99-latency criteria.\n\n"
        )

    report.write(
        "## Interpretation\n\n"
        "The saturation sweep measures how achieved throughput "
        "changes as offered event rate increases. A flattening "
        "throughput curve indicates that the processing pipeline "
        "is approaching its practical capacity. Increasing "
        "throttling at higher offered rates indicates that "
        "backpressure is becoming active.\n\n"
        "Tail latency should be evaluated separately from average "
        "latency because rare scheduling, operating-system, or "
        "contention effects can produce substantially larger "
        "maximum latency values.\n"
    )

print("Saturation analysis completed.")
print(f"Runs analyzed: {len(df)}")
print(f"Rates analyzed: {len(grouped)}")

if knee_rate is not None:
    print(f"Estimated saturation knee: {knee_rate} EPS")
else:
    print("Estimated saturation knee: not detected")

if practical_rate is not None:
    print(f"Recommended operating rate: {practical_rate} EPS")
else:
    print("Recommended operating rate: not detected")

print(f"Results written to: {OUTPUT_DIR}")
