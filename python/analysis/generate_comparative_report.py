import pandas as pd
from pathlib import Path

BASE = Path("benchmarks/results/comparative_scheduler")
ANALYSIS = BASE / "analysis"

summary = pd.read_csv(ANALYSIS / "summary.csv")
comparison = pd.read_csv(ANALYSIS / "comparison.csv")
adaptive = pd.read_csv(
    ANALYSIS / "adaptive_vs_best_baseline.csv"
)

report = []

report.append("# NexusFlow Comparative Scheduler Experiment")
report.append("")
report.append("## Experiment Overview")
report.append("")
report.append(
    "This experiment compares four scheduling strategies under "
    "identical workloads:"
)
report.append("")
report.append("- FIXED_SINGLE")
report.append("- FIXED_BATCH_32")
report.append("- FIXED_PARALLEL_8")
report.append("- ADAPTIVE")
report.append("")
report.append(
    "The experiment evaluates throughput, processing latency, "
    "tail latency, throughput efficiency, worker scaling, "
    "throttling, and integrity."
)
report.append("")
report.append("## Experimental Configuration")
report.append("")
report.append("| Parameter | Value |")
report.append("|---|---:|")
report.append("| Scheduler modes | 4 |")
report.append("| Offered loads | 10K, 20K, 30K, 40K, 50K EPS |")
report.append("| Repetitions | 3 per mode/load |")
report.append("| Events per run | 10,000 |")
report.append("| Total runs | 60 |")
report.append("| Total events | 600,000 |")
report.append("")
report.append("## Aggregate Results")
report.append("")

table = summary[
    [
        "mode",
        "target_rate_eps",
        "throughput_mean_eps",
        "efficiency_percent",
        "average_latency_mean_us",
        "p99_mean_us",
        "p999_mean_us",
        "max_latency_observed_us",
        "average_workers",
        "total_throttled",
    ]
].copy()

table.columns = [
    "Mode",
    "Load EPS",
    "Mean Throughput EPS",
    "Efficiency %",
    "Avg Latency us",
    "P99 us",
    "P99.9 us",
    "Max Latency us",
    "Avg Workers",
    "Throttled",
]

report.append(table.to_markdown(index=False))
report.append("")

report.append("## Best Strategy by Offered Load")
report.append("")
report.append(comparison.to_markdown(index=False))
report.append("")

report.append("## Adaptive Strategy vs Best Fixed Baseline")
report.append("")
report.append(
    adaptive[
        [
            "target_rate_eps",
            "throughput_mean_eps",
            "best_fixed_throughput_eps",
            "throughput_advantage_percent",
            "p99_mean_us",
            "best_fixed_p99_us",
            "p99_change_percent",
        ]
    ].to_markdown(index=False)
)
report.append("")

report.append("## Key Findings")
report.append("")

best_throughput = summary.loc[
    summary["throughput_mean_eps"].idxmax()
]

report.append(
    f"- Highest measured mean throughput: "
    f"{best_throughput['throughput_mean_eps']:.2f} EPS "
    f"using {best_throughput['mode']} at "
    f"{int(best_throughput['target_rate_eps'])} offered EPS."
)

best_p99 = summary.loc[
    summary["p99_mean_us"].idxmin()
]

report.append(
    f"- Lowest measured mean P99 processing latency: "
    f"{best_p99['p99_mean_us']:.3f} us using "
    f"{best_p99['mode']} at "
    f"{int(best_p99['target_rate_eps'])} offered EPS."
)

adaptive_20k = adaptive[
    adaptive["target_rate_eps"] == 20000
].iloc[0]

report.append(
    f"- At 20K EPS, ADAPTIVE achieved "
    f"{adaptive_20k['throughput_mean_eps']:.2f} EPS, "
    f"which was {adaptive_20k['throughput_advantage_percent']:.2f}% "
    f"above the strongest fixed throughput baseline."
)

adaptive_10k = adaptive[
    adaptive["target_rate_eps"] == 10000
].iloc[0]

report.append(
    f"- At 10K EPS, ADAPTIVE reduced mean P99 latency by "
    f"{abs(adaptive_10k['p99_change_percent']):.2f}% "
    f"relative to the best fixed P99 baseline."
)

adaptive_50k = adaptive[
    adaptive["target_rate_eps"] == 50000
].iloc[0]

report.append(
    f"- At 50K EPS, ADAPTIVE throughput was "
    f"{abs(adaptive_50k['throughput_advantage_percent']):.2f}% "
    f"below the strongest fixed throughput baseline."
)

report.append("")
report.append("## Interpretation")
report.append("")
report.append(
    "The results do not support the claim that ADAPTIVE is "
    "universally superior. Fixed parallel scheduling produced "
    "the highest throughput at several high-load points, while "
    "ADAPTIVE produced strong tail-latency results at lower "
    "loads and demonstrated dynamic worker scaling."
)
report.append("")
report.append(
    "This is an important research result: the adaptive scheduler "
    "shows a latency-oriented trade-off rather than simply "
    "maximizing throughput."
)
report.append("")
report.append(
    "At high offered loads, ADAPTIVE experienced throttling and "
    "lower throughput than FIXED_PARALLEL_8. This indicates that "
    "the current scheduler policy requires further tuning for "
    "high-load saturation conditions."
)
report.append("")
report.append("## Important Measurement Note")
report.append("")
report.append(
    "Processing latency is measured inside the event processor. "
    "It should therefore be interpreted as processing latency, "
    "not full end-to-end queueing latency."
)
report.append("")
report.append(
    "FIXED_BATCH_32 represents queue-drain batching of up to "
    "32 events. Events are still processed individually; this "
    "experiment should not be described as vectorized execution."
)
report.append("")
report.append("## Research Conclusion")
report.append("")
report.append(
    "The comparative experiment successfully establishes a "
    "reproducible baseline across four scheduling policies and "
    "five offered-load levels. The results demonstrate that "
    "scheduler policy materially affects throughput and tail "
    "latency, while also exposing a clear optimization target: "
    "improve ADAPTIVE behavior under high-load saturation without "
    "sacrificing its low-latency characteristics."
)
report.append("")

output = ANALYSIS / "COMPARATIVE_RESEARCH_REPORT.md"

output.write_text(
    "\n".join(report),
    encoding="utf-8"
)

print("")
print("========================================")
print("COMPARATIVE RESEARCH REPORT GENERATED")
print("========================================")
print("")
print(output)
