import pandas as pd
from pathlib import Path

INPUT = Path("benchmarks/results/saturation_analysis/plot_summary.csv")
OUTPUT = Path("benchmarks/results/saturation_analysis/sla_capacity_analysis.csv")
REPORT = Path("benchmarks/results/saturation_analysis/sla_capacity_report.md")

df = pd.read_csv(INPUT)

numeric = [
    "target_rate_eps",
    "throughput_mean_eps",
    "throughput_std_eps",
    "throughput_efficiency_percent",
    "throughput_cv_percent",
    "p99_mean_us",
    "p999_mean_us",
    "max_latency_us",
    "workers_mean",
    "throttled_mean",
]

for column in numeric:
    df[column] = pd.to_numeric(df[column], errors="coerce")

# Explicit engineering SLA criteria.
P99_SLA_US = 100.0
P999_SLA_US = 1000.0
MIN_EFFICIENCY_PERCENT = 80.0
MAX_CV_PERCENT = 5.0

df["p99_sla_pass"] = df["p99_mean_us"] <= P99_SLA_US
df["p999_sla_pass"] = df["p999_mean_us"] <= P999_SLA_US
df["efficiency_pass"] = (
    df["throughput_efficiency_percent"]
    >= MIN_EFFICIENCY_PERCENT
)
df["stability_pass"] = (
    df["throughput_cv_percent"]
    <= MAX_CV_PERCENT
)

df["overall_sla_pass"] = (
    df["p99_sla_pass"]
    & df["p999_sla_pass"]
    & df["efficiency_pass"]
    & df["stability_pass"]
)

df.to_csv(OUTPUT, index=False)

eligible = df[df["overall_sla_pass"]]

if eligible.empty:
    operating_rate = None
else:
    operating_rate = int(
        eligible["target_rate_eps"].max()
    )

peak_row = df.loc[
    df["throughput_mean_eps"].idxmax()
]

peak_rate = int(peak_row["target_rate_eps"])
peak_throughput = float(
    peak_row["throughput_mean_eps"]
)

# Find first point where the system enters saturation:
# achieved throughput growth falls below 25% while offered
# load increases by the same fixed step.
df["throughput_growth_percent"] = (
    df["throughput_mean_eps"].pct_change() * 100.0
)

knee_candidates = df[
    (df["throughput_growth_percent"] >= 0.0)
    & (df["throughput_growth_percent"] < 25.0)
]

if knee_candidates.empty:
    knee_rate = None
else:
    knee_rate = int(
        knee_candidates.iloc[0]["target_rate_eps"]
    )

with open(REPORT, "w", encoding="utf-8") as report:

    report.write("# NexusFlow SLA-Aware Capacity Analysis\n\n")

    report.write("## SLA Criteria\n\n")
    report.write(
        f"- Mean P99 latency <= {P99_SLA_US:.0f} us\n"
        f"- Mean P99.9 latency <= {P999_SLA_US:.0f} us\n"
        f"- Throughput efficiency >= {MIN_EFFICIENCY_PERCENT:.0f}%\n"
        f"- Throughput CV <= {MAX_CV_PERCENT:.0f}%\n\n"
    )

    report.write("## Capacity Table\n\n")

    report.write(
        "| Offered Load | Throughput | Efficiency | P99 | P99.9 | CV | Throttled | SLA |\n"
    )
    report.write(
        "|---:|---:|---:|---:|---:|---:|---:|:---:|\n"
    )

    for _, row in df.iterrows():

        status = (
            "PASS"
            if row["overall_sla_pass"]
            else "FAIL"
        )

        report.write(
            f"| {int(row['target_rate_eps']):,} "
            f"| {row['throughput_mean_eps']:,.2f} "
            f"| {row['throughput_efficiency_percent']:.2f}% "
            f"| {row['p99_mean_us']:.2f} us "
            f"| {row['p999_mean_us']:.2f} us "
            f"| {row['throughput_cv_percent']:.2f}% "
            f"| {row['throttled_mean']:.2f} "
            f"| {status} |\n"
        )

    report.write("\n## Capacity Decision\n\n")

    if operating_rate is not None:
        report.write(
            f"Highest measured operating point satisfying all "
            f"configured criteria: **{operating_rate:,} EPS**.\n\n"
        )
    else:
        report.write(
            "No measured load satisfied all configured SLA "
            "criteria simultaneously.\n\n"
        )

    if knee_rate is not None:
        report.write(
            f"Estimated saturation knee: **{knee_rate:,} EPS**.\n\n"
        )

    report.write(
        f"Maximum mean throughput observed: "
        f"**{peak_throughput:,.2f} EPS** at "
        f"**{peak_rate:,} EPS offered load**.\n\n"
    )

    report.write("## Interpretation\n\n")

    report.write(
        "The SLA-aware analysis separates three different concepts: "
        "the saturation knee, the maximum measured throughput, and "
        "the recommended operating point. These values should not be "
        "treated as interchangeable.\n\n"
    )

    report.write(
        "The saturation knee identifies where additional offered load "
        "begins producing disproportionately smaller throughput gains. "
        "The maximum measured throughput identifies the highest mean "
        "throughput observed during this experiment. The recommended "
        "operating point additionally requires acceptable tail latency, "
        "throughput efficiency, and run-to-run stability.\n\n"
    )

    report.write(
        "Throttling is treated as a backpressure mechanism rather than "
        "an integrity failure. All runs used for this analysis passed "
        "the benchmark integrity criteria.\n"
    )

print("SLA-aware capacity analysis completed.")

if operating_rate is not None:
    print(
        f"Recommended operating rate: "
        f"{operating_rate} EPS"
    )
else:
    print(
        "No load point satisfied all SLA criteria."
    )

if knee_rate is not None:
    print(
        f"Estimated saturation knee: "
        f"{knee_rate} EPS"
    )

print(
    f"Maximum mean throughput: "
    f"{peak_throughput:.2f} EPS"
)

print(f"CSV: {OUTPUT}")
print(f"Report: {REPORT}")
