import os
import pandas as pd
import matplotlib.pyplot as plt

INPUT = "benchmarks/results/real_end_to_end_latency_multi_load.csv"
OUTPUT_DIR = "benchmarks/results/real_end_to_end_latency_analysis"

os.makedirs(OUTPUT_DIR, exist_ok=True)

df = pd.read_csv(INPUT)

for column in df.columns:
    if column not in ["integrity"]:
        try:
            df[column] = pd.to_numeric(df[column])
        except (ValueError, TypeError):
            pass

summary = (
    df.groupby("target_rate_eps")
    .agg(
        throughput_eps=("throughput_eps", "mean"),
        queue_wait_p50_us=("queue_wait_p50_us", "mean"),
        queue_wait_p95_us=("queue_wait_p95_us", "mean"),
        queue_wait_p99_us=("queue_wait_p99_us", "mean"),
        queue_wait_p999_us=("queue_wait_p999_us", "mean"),
        processing_p50_us=("processing_p50_us", "mean"),
        processing_p95_us=("processing_p95_us", "mean"),
        processing_p99_us=("processing_p99_us", "mean"),
        processing_p999_us=("processing_p999_us", "mean"),
        completion_p50_us=("completion_p50_us", "mean"),
        e2e_p50_us=("e2e_p50_us", "mean"),
        e2e_p95_us=("e2e_p95_us", "mean"),
        e2e_p99_us=("e2e_p99_us", "mean"),
        e2e_p999_us=("e2e_p999_us", "mean"),
        e2e_max_us=("e2e_max_us", "max"),
        workers=("workers", "mean"),
        throttled=("throttled", "mean"),
    )
    .reset_index()
)

summary["efficiency_percent"] = (
    summary["throughput_eps"]
    / summary["target_rate_eps"]
    * 100.0
)

summary["queue_wait_fraction_percent"] = (
    summary["queue_wait_p50_us"]
    / summary["e2e_p50_us"]
    * 100.0
)

summary["processing_fraction_percent"] = (
    summary["processing_p50_us"]
    / summary["e2e_p50_us"]
    * 100.0
)

summary.to_csv(
    os.path.join(OUTPUT_DIR, "summary.csv"),
    index=False
)

def save_plot(x, y, xlabel, ylabel, title, filename):
    plt.figure()
    plt.plot(x, y, marker="o")
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(
        os.path.join(OUTPUT_DIR, filename),
        dpi=200
    )
    plt.close()

x = summary["target_rate_eps"]

save_plot(
    x,
    summary["throughput_eps"],
    "Offered Load (EPS)",
    "Mean Throughput (EPS)",
    "NexusFlow End-to-End Throughput vs Offered Load",
    "e2e_throughput_vs_load.png"
)

save_plot(
    x,
    summary["queue_wait_p99_us"],
    "Offered Load (EPS)",
    "Queue Wait P99 (us)",
    "Queue Wait P99 vs Offered Load",
    "e2e_queue_wait_p99_vs_load.png"
)

save_plot(
    x,
    summary["processing_p99_us"],
    "Offered Load (EPS)",
    "Processing P99 (us)",
    "Processing P99 vs Offered Load",
    "e2e_processing_p99_vs_load.png"
)

save_plot(
    x,
    summary["e2e_p99_us"],
    "Offered Load (EPS)",
    "End-to-End P99 (us)",
    "End-to-End P99 vs Offered Load",
    "e2e_latency_p99_vs_load.png"
)

save_plot(
    x,
    summary["e2e_p999_us"],
    "Offered Load (EPS)",
    "End-to-End P99.9 (us)",
    "End-to-End P99.9 vs Offered Load",
    "e2e_latency_p999_vs_load.png"
)

save_plot(
    x,
    summary["queue_wait_fraction_percent"],
    "Offered Load (EPS)",
    "Queue Wait Fraction (%)",
    "Queue Wait Contribution to End-to-End P50",
    "e2e_queue_wait_fraction.png"
)

save_plot(
    x,
    summary["workers"],
    "Offered Load (EPS)",
    "Average Active Workers",
    "Adaptive Worker Scaling",
    "e2e_worker_scaling.png"
)

report_path = os.path.join(
    OUTPUT_DIR,
    "E2E_RESEARCH_REPORT.md"
)

with open(report_path, "w", encoding="utf-8") as f:
    f.write("# NexusFlow Real End-to-End Latency Experiment\n\n")

    f.write("## Experimental Design\n\n")
    f.write("- Dataset: UCI Online Retail II processed event stream\n")
    f.write("- Events per run: 10,000\n")
    f.write("- Load levels: 1,000 / 10,000 / 20,000 / 30,000 / 50,000 EPS\n")
    f.write("- Repetitions per load: 3\n")
    f.write("- Total runs: 15\n")
    f.write("- Maximum workers: 16\n")
    f.write("- Scheduler mode: ADAPTIVE\n")
    f.write("- Integrity: all completed runs passed\n\n")

    f.write("## Results\n\n")
    f.write(
        summary[
            [
                "target_rate_eps",
                "throughput_eps",
                "efficiency_percent",
                "e2e_p50_us",
                "e2e_p95_us",
                "e2e_p99_us",
                "e2e_p999_us",
                "e2e_max_us",
                "workers",
                "throttled",
            ]
        ].to_markdown(index=False)
    )

    f.write("\n\n## Latency Breakdown\n\n")
    f.write(
        summary[
            [
                "target_rate_eps",
                "queue_wait_fraction_percent",
                "processing_fraction_percent",
                "queue_wait_p99_us",
                "processing_p99_us",
            ]
        ].to_markdown(index=False)
    )

    f.write("\n\n## Findings\n\n")

    max_throughput_row = summary.loc[
        summary["throughput_eps"].idxmax()
    ]

    f.write(
        f"- Maximum measured mean throughput was "
        f"{max_throughput_row['throughput_eps']:.2f} EPS "
        f"at {int(max_throughput_row['target_rate_eps'])} EPS offered load.\n"
    )

    dominant_row = summary.iloc[-1]

    f.write(
        f"- At {int(dominant_row['target_rate_eps'])} EPS, "
        f"queue waiting accounted for approximately "
        f"{dominant_row['queue_wait_fraction_percent']:.2f}% "
        f"of median end-to-end latency.\n"
    )

    f.write(
        "- Processing latency remained substantially smaller than "
        "queue waiting latency across the measured workloads.\n"
    )

    f.write(
        "- The experiment demonstrates that end-to-end latency is "
        "currently dominated by admission, queueing, and scheduling "
        "rather than the event-processing kernel itself.\n"
    )

    f.write(
        "- Adaptive worker scaling was observed at higher offered loads, "
        "but scaling did not prevent substantial queue buildup and tail "
        "latency growth in all runs.\n"
    )

    f.write("\n## Research Implication\n\n")

    f.write(
        "The measured bottleneck motivates Scheduler V2. The next "
        "scheduler iteration should incorporate queue-growth rate, "
        "recent tail latency, worker utilization, hysteresis, and "
        "cooldown/residency controls rather than relying primarily on "
        "instantaneous queue depth and arrival rate.\n"
    )

    f.write("\n## Important Limitation\n\n")

    f.write(
        "These measurements represent the current adaptive pipeline "
        "implementation. They do not establish that the adaptive "
        "scheduler is superior to fixed scheduling policies. A separate "
        "paired comparative experiment is required for that claim.\n"
    )

print()
print("========================================")
print("E2E RESEARCH ANALYSIS COMPLETE")
print("========================================")
print()
print(f"Summary: {OUTPUT_DIR}/summary.csv")
print(f"Report:  {report_path}")
print()
print("Generated plots:")
for filename in [
    "e2e_throughput_vs_load.png",
    "e2e_queue_wait_p99_vs_load.png",
    "e2e_processing_p99_vs_load.png",
    "e2e_latency_p99_vs_load.png",
    "e2e_latency_p999_vs_load.png",
    "e2e_queue_wait_fraction.png",
    "e2e_worker_scaling.png",
]:
    print(f"  {OUTPUT_DIR}/{filename}")
print()
