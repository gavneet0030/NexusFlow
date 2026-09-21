from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt


PROJECT_ROOT = Path(__file__).resolve().parents[2]
RESULTS_DIR = PROJECT_ROOT / "benchmarks" / "results"
INPUT_FILE = RESULTS_DIR / "end_to_end_thresholds.csv"
OUTPUT_DIR = RESULTS_DIR / "threshold_analysis"

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def main():
    print("===============================================")
    print("NexusFlow Threshold Analysis")
    print("===============================================")
    print()

    if not INPUT_FILE.exists():
        print(f"ERROR: Input file not found: {INPUT_FILE}")
        return

    df = pd.read_csv(INPUT_FILE)

    print(f"Input file: {INPUT_FILE}")
    print(f"Rows loaded: {len(df)}")
    print()

    print("Columns detected:")
    for column in df.columns:
        print(f"  - {column}")
    print()

    required_columns = [
        "micro_batch_size",
        "sla_us",
        "soft_queue_limit",
        "throughput_eps",
        "average_latency_us"
    ]

    missing = [
        column for column in required_columns
        if column not in df.columns
    ]

    if missing:
        print("ERROR: Required columns are missing:")
        for column in missing:
            print(f"  - {column}")
        print()
        print("Analysis stopped.")
        return

    numeric_columns = [
        "micro_batch_size",
        "sla_us",
        "soft_queue_limit",
        "throughput_eps",
        "average_latency_us"
    ]

    for column in numeric_columns:
        df[column] = pd.to_numeric(df[column], errors="coerce")

    df = df.dropna(subset=numeric_columns).copy()

    print(f"Valid rows after cleaning: {len(df)}")
    print()

    # ---------------------------------------------------------
    # Basic validation
    # ---------------------------------------------------------

    if "processed" in df.columns:
        processed = pd.to_numeric(df["processed"], errors="coerce")
        print("Processed-event validation:")
        print(f"  Minimum processed events: {processed.min():.0f}")
        print(f"  Maximum processed events: {processed.max():.0f}")
        print()

    if "integrity" in df.columns:
        print("Integrity results:")
        print(df["integrity"].value_counts(dropna=False))
        print()

    # ---------------------------------------------------------
    # Best configurations
    # ---------------------------------------------------------

    best_throughput = df.sort_values(
        "throughput_eps",
        ascending=False
    ).head(10)

    best_latency = df.sort_values(
        "average_latency_us",
        ascending=True
    ).head(10)

    best_throughput.to_csv(
        OUTPUT_DIR / "top_throughput_configs.csv",
        index=False
    )

    best_latency.to_csv(
        OUTPUT_DIR / "top_latency_configs.csv",
        index=False
    )

    print("Top 10 configurations by throughput:")
    print(
        best_throughput[
            [
                "micro_batch_size",
                "sla_us",
                "soft_queue_limit",
                "throughput_eps",
                "average_latency_us"
            ]
        ].to_string(index=False)
    )
    print()

    print("Top 10 configurations by average latency:")
    print(
        best_latency[
            [
                "micro_batch_size",
                "sla_us",
                "soft_queue_limit",
                "throughput_eps",
                "average_latency_us"
            ]
        ].to_string(index=False)
    )
    print()

    # ---------------------------------------------------------
    # Group analysis by batch size
    # ---------------------------------------------------------

    batch_summary = (
        df.groupby("micro_batch_size")
        .agg(
            mean_throughput_eps=("throughput_eps", "mean"),
            max_throughput_eps=("throughput_eps", "max"),
            mean_latency_us=("average_latency_us", "mean"),
            min_latency_us=("average_latency_us", "min"),
            configurations=("throughput_eps", "count")
        )
        .reset_index()
        .sort_values("mean_throughput_eps", ascending=False)
    )

    batch_summary.to_csv(
        OUTPUT_DIR / "batch_size_summary.csv",
        index=False
    )

    print("Batch-size summary:")
    print(batch_summary.to_string(index=False))
    print()

    # ---------------------------------------------------------
    # Group analysis by SLA
    # ---------------------------------------------------------

    sla_summary = (
        df.groupby("sla_us")
        .agg(
            mean_throughput_eps=("throughput_eps", "mean"),
            max_throughput_eps=("throughput_eps", "max"),
            mean_latency_us=("average_latency_us", "mean"),
            min_latency_us=("average_latency_us", "min"),
            configurations=("throughput_eps", "count")
        )
        .reset_index()
        .sort_values("mean_throughput_eps", ascending=False)
    )

    sla_summary.to_csv(
        OUTPUT_DIR / "sla_summary.csv",
        index=False
    )

    print("SLA summary:")
    print(sla_summary.to_string(index=False))
    print()

    # ---------------------------------------------------------
    # Group analysis by queue threshold
    # ---------------------------------------------------------

    queue_summary = (
        df.groupby("soft_queue_limit")
        .agg(
            mean_throughput_eps=("throughput_eps", "mean"),
            max_throughput_eps=("throughput_eps", "max"),
            mean_latency_us=("average_latency_us", "mean"),
            min_latency_us=("average_latency_us", "min"),
            configurations=("throughput_eps", "count")
        )
        .reset_index()
        .sort_values("mean_throughput_eps", ascending=False)
    )

    queue_summary.to_csv(
        OUTPUT_DIR / "queue_threshold_summary.csv",
        index=False
    )

    print("Queue-threshold summary:")
    print(queue_summary.to_string(index=False))
    print()

    # ---------------------------------------------------------
    # Parameter correlation
    # ---------------------------------------------------------

    correlation_columns = [
        "micro_batch_size",
        "sla_us",
        "soft_queue_limit",
        "throughput_eps",
        "average_latency_us"
    ]

    correlation = df[correlation_columns].corr()

    correlation.to_csv(
        OUTPUT_DIR / "parameter_correlation.csv"
    )

    print("Correlation matrix:")
    print(correlation.to_string())
    print()

    # ---------------------------------------------------------
    # Pareto frontier
    #
    # Objective:
    #   maximize throughput
    #   minimize average latency
    # ---------------------------------------------------------

    pareto_rows = []

    for index, row in df.iterrows():
        dominated = False

        for other_index, other in df.iterrows():
            if index == other_index:
                continue

            other_is_at_least_as_fast = (
                other["throughput_eps"] >= row["throughput_eps"]
            )

            other_is_at_least_as_low_latency = (
                other["average_latency_us"]
                <= row["average_latency_us"]
            )

            other_is_strictly_better = (
                other["throughput_eps"] > row["throughput_eps"]
                or
                other["average_latency_us"]
                < row["average_latency_us"]
            )

            if (
                other_is_at_least_as_fast
                and
                other_is_at_least_as_low_latency
                and
                other_is_strictly_better
            ):
                dominated = True
                break

        if not dominated:
            pareto_rows.append(row)

    pareto_df = pd.DataFrame(pareto_rows)

    if not pareto_df.empty:
        pareto_df = pareto_df.sort_values(
            "throughput_eps",
            ascending=True
        )

    pareto_df.to_csv(
        OUTPUT_DIR / "pareto_frontier.csv",
        index=False
    )

    print("Pareto-optimal configurations:")
    if pareto_df.empty:
        print("  No Pareto configurations found.")
    else:
        print(
            pareto_df[
                [
                    "micro_batch_size",
                    "sla_us",
                    "soft_queue_limit",
                    "throughput_eps",
                    "average_latency_us"
                ]
            ].to_string(index=False)
        )
    print()

    # ---------------------------------------------------------
    # Best configuration
    # ---------------------------------------------------------

    best = df.loc[df["throughput_eps"].idxmax()]

    print("===============================================")
    print("BEST THROUGHPUT CONFIGURATION")
    print("===============================================")
    print(f"Micro-batch size : {best['micro_batch_size']:.0f}")
    print(f"SLA budget       : {best['sla_us']:.0f} us")
    print(f"Queue threshold  : {best['soft_queue_limit']:.0f}")
    print(f"Throughput       : {best['throughput_eps']:.2f} events/sec")
    print(f"Average latency  : {best['average_latency_us']:.2f} us")
    print()

    # ---------------------------------------------------------
    # Charts
    # ---------------------------------------------------------

    plt.figure(figsize=(10, 6))

    for batch_size in sorted(df["micro_batch_size"].unique()):
        subset = df[df["micro_batch_size"] == batch_size]

        plt.scatter(
            subset["average_latency_us"],
            subset["throughput_eps"],
            label=f"Batch {int(batch_size)}"
        )

    plt.xlabel("Average Latency (us)")
    plt.ylabel("Throughput (events/sec)")
    plt.title("NexusFlow Throughput vs Average Latency")
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    plt.savefig(
        OUTPUT_DIR / "throughput_vs_latency.png",
        dpi=200
    )

    plt.close()

    # ---------------------------------------------------------

    plt.figure(figsize=(10, 6))

    batch_means = (
        df.groupby("micro_batch_size")["throughput_eps"]
        .mean()
        .sort_index()
    )

    plt.plot(
        batch_means.index,
        batch_means.values,
        marker="o"
    )

    plt.xlabel("Micro-batch Size")
    plt.ylabel("Mean Throughput (events/sec)")
    plt.title("Mean Throughput by Micro-batch Size")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    plt.savefig(
        OUTPUT_DIR / "throughput_by_batch_size.png",
        dpi=200
    )

    plt.close()

    # ---------------------------------------------------------

    plt.figure(figsize=(10, 6))

    sla_means = (
        df.groupby("sla_us")["throughput_eps"]
        .mean()
        .sort_index()
    )

    plt.plot(
        sla_means.index,
        sla_means.values,
        marker="o"
    )

    plt.xlabel("SLA Budget (us)")
    plt.ylabel("Mean Throughput (events/sec)")
    plt.title("Mean Throughput by SLA Budget")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    plt.savefig(
        OUTPUT_DIR / "throughput_by_sla.png",
        dpi=200
    )

    plt.close()

    # ---------------------------------------------------------
    # Markdown report
    # ---------------------------------------------------------

    report_file = OUTPUT_DIR / "threshold_analysis_report.md"

    with open(report_file, "w", encoding="utf-8") as report:
        report.write("# NexusFlow End-to-End Threshold Analysis\n\n")

        report.write("## Experiment Overview\n\n")
        report.write(
            f"- Configurations analyzed: {len(df)}\n"
        )
        report.write(
            "- Objective: evaluate sensitivity to micro-batch size, "
            "SLA budget, and queue threshold.\n"
        )
        report.write(
            "- Primary performance metric: throughput in events/sec.\n"
        )
        report.write(
            "- Secondary metric: average end-to-end latency in microseconds.\n\n"
        )

        report.write("## Best Throughput Configuration\n\n")
        report.write(
            f"- Micro-batch size: {best['micro_batch_size']:.0f}\n"
        )
        report.write(
            f"- SLA budget: {best['sla_us']:.0f} us\n"
        )
        report.write(
            f"- Queue threshold: {best['soft_queue_limit']:.0f}\n"
        )
        report.write(
            f"- Throughput: {best['throughput_eps']:.2f} events/sec\n"
        )
        report.write(
            f"- Average latency: {best['average_latency_us']:.2f} us\n\n"
        )

        report.write("## Interpretation\n\n")

        best_batch = int(best["micro_batch_size"])

        report.write(
            f"The highest-throughput configuration in this sweep used "
            f"a micro-batch size of {best_batch}, an SLA budget of "
            f"{best['sla_us']:.0f} us, and a queue threshold of "
            f"{best['soft_queue_limit']:.0f}.\n\n"
        )

        report.write(
            "The sweep demonstrates configuration sensitivity, but these "
            "results are single benchmark observations per configuration. "
            "They should not be interpreted as statistically significant "
            "comparisons between configurations without repeated runs.\n\n"
        )

        report.write(
            "The current threshold benchmark records average latency rather "
            "than p50/p95/p99/p99.9/max latency. Tail-latency conclusions "
            "therefore require a follow-up benchmark with percentile "
            "instrumentation.\n\n"
        )

        report.write("## Output Files\n\n")
        report.write(
            "- top_throughput_configs.csv\n"
            "- top_latency_configs.csv\n"
            "- batch_size_summary.csv\n"
            "- sla_summary.csv\n"
            "- queue_threshold_summary.csv\n"
            "- parameter_correlation.csv\n"
            "- pareto_frontier.csv\n"
            "- throughput_vs_latency.png\n"
            "- throughput_by_batch_size.png\n"
            "- throughput_by_sla.png\n"
        )

    print("===============================================")
    print("THRESHOLD ANALYSIS COMPLETED")
    print("===============================================")
    print()
    print(f"Analysis directory: {OUTPUT_DIR}")
    print(f"Report: {report_file}")
    print()


if __name__ == "__main__":
    main()
