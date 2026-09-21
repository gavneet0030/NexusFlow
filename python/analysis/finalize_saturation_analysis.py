import pandas as pd
from pathlib import Path

INPUT = Path("benchmarks/results/saturation_analysis/plot_summary.csv")
OUTPUT = Path("benchmarks/results/saturation_analysis/final_saturation_report.md")

df = pd.read_csv(INPUT)

df["target_rate_eps"] = pd.to_numeric(df["target_rate_eps"])
df["throughput_mean_eps"] = pd.to_numeric(df["throughput_mean_eps"])
df["throughput_efficiency_percent"] = pd.to_numeric(
    df["throughput_efficiency_percent"]
)
df["throughput_cv_percent"] = pd.to_numeric(
    df["throughput_cv_percent"]
)
df["p99_mean_us"] = pd.to_numeric(df["p99_mean_us"])
df["p999_mean_us"] = pd.to_numeric(df["p999_mean_us"])
df["max_latency_us"] = pd.to_numeric(df["max_latency_us"])
df["workers_mean"] = pd.to_numeric(df["workers_mean"])
df["throttled_mean"] = pd.to_numeric(df["throttled_mean"])

df["throughput_growth_percent"] = (
    df["throughput_mean_eps"].pct_change() * 100.0
)

df["efficiency_drop_percent"] = (
    -df["throughput_efficiency_percent"].diff()
)

# Identify the first point where throughput growth
# becomes substantially smaller than the offered-load growth.
knee_candidates = df[
    (df["throughput_growth_percent"] >= 0) &
    (df["throughput_growth_percent"] < 25)
]

if not knee_candidates.empty:
    knee_rate = int(
        knee_candidates.iloc[0]["target_rate_eps"]
    )
else:
    knee_rate = None

# Practical operating point:
# highest rate with at least 80% efficiency
# and mean P99 below 100 us.
eligible = df[
    (df["throughput_efficiency_percent"] >= 80.0) &
    (df["p99_mean_us"] < 100.0)
]

if not eligible.empty:
    operating_rate = int(
        eligible["target_rate_eps"].max()
    )
else:
    operating_rate = None

peak_row = df.loc[
    df["throughput_mean_eps"].idxmax()
]

peak_rate = int(peak_row["target_rate_eps"])
peak_throughput = float(peak_row["throughput_mean_eps"])

# Stability point:
# choose the highest rate whose throughput CV remains <= 5%.
stable = df[
    df["throughput_cv_percent"] <= 5.0
]

if not stable.empty:
    stable_rate = int(
        stable["target_rate_eps"].max()
    )
else:
    stable_rate = None

with open(OUTPUT, "w", encoding="utf-8") as report:

    report.write("# NexusFlow Saturation Sweep — Final Findings\n\n")

    report.write("## Experimental Configuration\n\n")
    report.write(
        "- Real dataset event replay\n"
        "- 10,000 events per run\n"
        "- 9 offered-load levels\n"
        "- 3 repetitions per level\n"
        "- 27 total runs\n"
        "- Adaptive scheduling enabled\n"
        "- Maximum worker pool: 16\n"
        "- Integrity filtering: PASS runs\n\n"
    )

    report.write("## Aggregated Results\n\n")

    report.write(
        "| Offered Load | Mean Throughput | Efficiency | P99 | P99.9 | Max Latency | CV | Workers | Throttled |\n"
    )
    report.write(
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n"
    )

    for _, row in df.iterrows():

        report.write(
            f"| {int(row['target_rate_eps']):,} "
            f"| {row['throughput_mean_eps']:,.2f} "
            f"| {row['throughput_efficiency_percent']:.2f}% "
            f"| {row['p99_mean_us']:.2f} us "
            f"| {row['p999_mean_us']:.2f} us "
            f"| {row['max_latency_us']:.2f} us "
            f"| {row['throughput_cv_percent']:.2f}% "
            f"| {row['workers_mean']:.2f} "
            f"| {row['throttled_mean']:.2f} |\n"
        )

    report.write("\n## Capacity Findings\n\n")

    if knee_rate is not None:
        report.write(
            f"- Estimated saturation knee: **{knee_rate:,} EPS**.\n"
        )

    report.write(
        f"- Peak measured mean throughput: "
        f"**{peak_throughput:,.2f} EPS** "
        f"at **{peak_rate:,} EPS offered load**.\n"
    )

    if operating_rate is not None:
        report.write(
            f"- Recommended operating point under the "
            f"configured criteria: **{operating_rate:,} EPS**.\n"
        )

    if stable_rate is not None:
        report.write(
            f"- Highest load with throughput CV <= 5%: "
            f"**{stable_rate:,} EPS**.\n"
        )

    report.write("\n## Engineering Interpretation\n\n")

    report.write(
        "The throughput curve demonstrates that NexusFlow initially "
        "tracks the offered event rate closely. As offered load "
        "increases, achieved throughput grows more slowly and "
        "eventually approaches a practical processing ceiling. "
        "This region represents the saturation regime.\n\n"
    )

    report.write(
        "The increasing throttling observed at higher offered loads "
        "is consistent with active backpressure rather than data loss, "
        "because the recorded runs retained full submission, acceptance, "
        "processing, and queue-drain integrity.\n\n"
    )

    report.write(
        "Tail latency must be interpreted separately from throughput. "
        "P99.9 and maximum latency exhibit larger variability than "
        "central latency percentiles at several load levels. This "
        "indicates that operating-system scheduling, contention, "
        "runtime variability, or transient queueing can dominate "
        "rare tail events even when median and P99 latency remain low.\n\n"
    )

    report.write("## Research Conclusion\n\n")

    report.write(
        "The saturation sweep provides empirical evidence for an "
        "adaptive event-processing capacity boundary. Rather than "
        "reporting a single maximum throughput number, the experiment "
        "characterizes the relationship between offered load, achieved "
        "throughput, worker scaling, throttling, and latency tails. "
        "This provides a stronger systems-performance characterization "
        "for NexusFlow.\n"
    )

print("Final saturation report generated.")
print(f"Peak throughput: {peak_throughput:.2f} EPS")
print(f"Peak throughput load: {peak_rate} EPS")

if knee_rate is not None:
    print(f"Estimated saturation knee: {knee_rate} EPS")

if operating_rate is not None:
    print(f"Recommended operating point: {operating_rate} EPS")

if stable_rate is not None:
    print(f"Highest CV <= 5% load: {stable_rate} EPS")

print(f"Report: {OUTPUT}")
