import pandas as pd
from pathlib import Path

INPUT = Path("benchmarks/results/saturation_analysis/sla_capacity_analysis.csv")
OUTPUT = Path("benchmarks/results/saturation_analysis/capacity_summary.csv")
REPORT = Path("benchmarks/results/saturation_analysis/capacity_decision.md")

df = pd.read_csv(INPUT)

df = df.sort_values("target_rate_eps").reset_index(drop=True)

# Explicit experimental criteria
P99_LIMIT_US = 100.0
P999_LIMIT_US = 1000.0
EFFICIENCY_LIMIT_PERCENT = 80.0
CV_LIMIT_PERCENT = 5.0

df["satisfies_p99"] = df["p99_mean_us"] <= P99_LIMIT_US
df["satisfies_p999"] = df["p999_mean_us"] <= P999_LIMIT_US
df["satisfies_efficiency"] = (
    df["throughput_efficiency_percent"]
    >= EFFICIENCY_LIMIT_PERCENT
)
df["satisfies_stability"] = (
    df["throughput_cv_percent"]
    <= CV_LIMIT_PERCENT
)

df["satisfies_all_criteria"] = (
    df["satisfies_p99"]
    & df["satisfies_p999"]
    & df["satisfies_efficiency"]
    & df["satisfies_stability"]
)

# Highest fully compliant load
valid = df[df["satisfies_all_criteria"]]

if valid.empty:
    operating_rate = None
else:
    operating_rate = int(
        valid["target_rate_eps"].max()
    )

# Maximum observed mean throughput
peak_index = df["throughput_mean_eps"].idxmax()
peak = df.loc[peak_index]

peak_rate = int(peak["target_rate_eps"])
peak_throughput = float(peak["throughput_mean_eps"])

# First load where efficiency falls below 80%
efficiency_break = df[
    df["throughput_efficiency_percent"]
    < EFFICIENCY_LIMIT_PERCENT
]

if efficiency_break.empty:
    efficiency_break_rate = None
else:
    efficiency_break_rate = int(
        efficiency_break.iloc[0]["target_rate_eps"]
    )

# First load where stability exceeds 5% CV
stability_break = df[
    df["throughput_cv_percent"] > CV_LIMIT_PERCENT
]

if stability_break.empty:
    stability_break_rate = None
else:
    stability_break_rate = int(
        stability_break.iloc[0]["target_rate_eps"]
    )

df.to_csv(
    OUTPUT,
    index=False
)

with open(REPORT, "w", encoding="utf-8") as report:

    report.write(
        "# NexusFlow Capacity Decision\n\n"
    )

    report.write(
        "## Validated Experimental Criteria\n\n"
    )

    report.write(
        f"- P99 latency <= {P99_LIMIT_US:.0f} us\n"
        f"- P99.9 latency <= {P999_LIMIT_US:.0f} us\n"
        f"- Throughput efficiency >= "
        f"{EFFICIENCY_LIMIT_PERCENT:.0f}%\n"
        f"- Throughput CV <= {CV_LIMIT_PERCENT:.0f}%\n\n"
    )

    report.write(
        "## Final Capacity Classification\n\n"
    )

    report.write(
        "| Offered Load | Throughput | Efficiency | "
        "P99 | P99.9 | CV | Classification |\n"
    )

    report.write(
        "|---:|---:|---:|---:|---:|---:|:---|\n"
    )

    for _, row in df.iterrows():

        if row["satisfies_all_criteria"]:
            classification = "RECOMMENDED"
        elif (
            row["throughput_efficiency_percent"]
            >= EFFICIENCY_LIMIT_PERCENT
        ):
            classification = "DEGRADED"
        else:
            classification = "SATURATION"

        report.write(
            f"| {int(row['target_rate_eps']):,} "
            f"| {row['throughput_mean_eps']:,.2f} "
            f"| {row['throughput_efficiency_percent']:.2f}% "
            f"| {row['p99_mean_us']:.2f} us "
            f"| {row['p999_mean_us']:.2f} us "
            f"| {row['throughput_cv_percent']:.2f}% "
            f"| {classification} |\n"
        )

    report.write("\n## Final Decision\n\n")

    if operating_rate is not None:
        report.write(
            f"**Recommended operating point: "
            f"{operating_rate:,} EPS.**\n\n"
        )
    else:
        report.write(
            "**No tested load satisfied all criteria.**\n\n"
        )

    report.write(
        f"**Maximum measured mean throughput: "
        f"{peak_throughput:,.2f} EPS at "
        f"{peak_rate:,} EPS offered load.**\n\n"
    )

    if efficiency_break_rate is not None:
        report.write(
            f"Throughput efficiency first falls below 80% "
            f"at approximately **{efficiency_break_rate:,} EPS**.\n\n"
        )

    if stability_break_rate is not None:
        report.write(
            f"Throughput variability first exceeds 5% CV "
            f"at approximately **{stability_break_rate:,} EPS**.\n\n"
        )

    report.write(
        "## Interpretation\n\n"
        "The recommended operating point is intentionally different "
        "from the maximum throughput point. Maximum throughput "
        "represents the highest observed processing rate, whereas "
        "the recommended operating point requires simultaneous "
        "compliance with latency, efficiency, and stability criteria.\n\n"
        "This distinction prevents the benchmark from presenting "
        "peak throughput as a production-safe operating capacity.\n"
    )

print("Capacity decision finalized.")
print("")

if operating_rate is not None:
    print(
        f"Recommended operating point: "
        f"{operating_rate:,} EPS"
    )

print(
    f"Maximum mean throughput: "
    f"{peak_throughput:,.2f} EPS"
)

print(
    f"Peak load: "
    f"{peak_rate:,} EPS"
)

if efficiency_break_rate is not None:
    print(
        f"Efficiency degradation begins at: "
        f"{efficiency_break_rate:,} EPS"
    )

if stability_break_rate is not None:
    print(
        f"Stability degradation begins at: "
        f"{stability_break_rate:,} EPS"
    )

print("")
print(f"CSV: {OUTPUT}")
print(f"Report: {REPORT}")
