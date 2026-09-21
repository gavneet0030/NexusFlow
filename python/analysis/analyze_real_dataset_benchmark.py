from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt


INPUT = Path(
    "benchmarks/results/real_dataset_benchmark_matrix.csv"
)

OUTPUT_DIR = Path(
    "benchmarks/results/real_dataset_analysis"
)

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


def require_columns(dataframe):
    required = [
        "target_rate_eps",
        "event_count",
        "submitted",
        "accepted",
        "processed",
        "rejected",
        "throttled",
        "elapsed_seconds",
        "throughput_eps",
        "average_latency_us",
        "p50_latency_us",
        "p95_latency_us",
        "p99_latency_us",
        "p999_latency_us",
        "max_latency_us",
        "final_queue_depth",
        "active_workers",
        "checksum",
        "integrity_pass",
    ]

    missing = [
        column
        for column in required
        if column not in dataframe.columns
    ]

    if missing:
        raise ValueError(
            "Missing required columns: "
            + ", ".join(missing)
        )


def main():
    if not INPUT.exists():
        raise FileNotFoundError(
            f"Benchmark result file not found: {INPUT}"
        )

    data = pd.read_csv(INPUT)

    require_columns(data)

    data["integrity_pass"] = (
        data["integrity_pass"]
        .astype(str)
        .str.upper()
        .eq("PASS")
    )

    data["rate_accuracy_percent"] = (
        data["throughput_eps"]
        / data["target_rate_eps"]
        * 100.0
    )

    data["rate_error_percent"] = (
        data["rate_accuracy_percent"] - 100.0
    )

    data["throughput_gap_eps"] = (
        data["target_rate_eps"]
        - data["throughput_eps"]
    )

    data["tail_amplification_p99"] = (
        data["p99_latency_us"]
        / data["p50_latency_us"]
    )

    data["tail_amplification_p999"] = (
        data["p999_latency_us"]
        / data["p50_latency_us"]
    )

    data["max_to_p99_ratio"] = (
        data["max_latency_us"]
        / data["p99_latency_us"]
    )

    data.to_csv(
        OUTPUT_DIR / "detailed_results.csv",
        index=False
    )

    summary_columns = [
        "target_rate_eps",
        "event_count",
        "throughput_eps",
        "rate_accuracy_percent",
        "rate_error_percent",
        "average_latency_us",
        "p50_latency_us",
        "p95_latency_us",
        "p99_latency_us",
        "p999_latency_us",
        "max_latency_us",
        "final_queue_depth",
        "active_workers",
        "rejected",
        "throttled",
        "integrity_pass",
    ]

    summary = data[summary_columns]

    summary.to_csv(
        OUTPUT_DIR / "summary.csv",
        index=False
    )

    best_throughput = data.loc[
        data["throughput_eps"].idxmax()
    ]

    best_p99 = data.loc[
        data["p99_latency_us"].idxmin()
    ]

    best_p999 = data.loc[
        data["p999_latency_us"].idxmin()
    ]

    highest_max = data.loc[
        data["max_latency_us"].idxmax()
    ]

    report = []

    report.append(
        "# NexusFlow Real Dataset Benchmark Analysis"
    )

    report.append("")

    report.append(
        "Dataset: UCI Online Retail II"
    )

    report.append(
        f"Benchmark configurations: {len(data)}"
    )

    report.append("")

    report.append("## Summary")

    report.append("")

    report.append(
        "| Target EPS | Actual EPS | Accuracy | "
        "p50 us | p95 us | p99 us | p99.9 us | Max us | Workers | Integrity |"
    )

    report.append(
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|:---|"
    )

    for _, row in data.iterrows():

        report.append(
            f"| {int(row['target_rate_eps'])} "
            f"| {row['throughput_eps']:.2f} "
            f"| {row['rate_accuracy_percent']:.2f}% "
            f"| {row['p50_latency_us']:.2f} "
            f"| {row['p95_latency_us']:.2f} "
            f"| {row['p99_latency_us']:.2f} "
            f"| {row['p999_latency_us']:.2f} "
            f"| {row['max_latency_us']:.2f} "
            f"| {int(row['active_workers'])} "
            f"| {'PASS' if row['integrity_pass'] else 'FAIL'} |"
        )

    report.append("")

    report.append("## Best Throughput")

    report.append("")

    report.append(
        f"- Target: {int(best_throughput['target_rate_eps'])} EPS"
    )

    report.append(
        f"- Actual: {best_throughput['throughput_eps']:.2f} EPS"
    )

    report.append(
        f"- Accuracy: "
        f"{best_throughput['rate_accuracy_percent']:.2f}%"
    )

    report.append("")

    report.append("## Best p99 Latency")

    report.append("")

    report.append(
        f"- Target: {int(best_p99['target_rate_eps'])} EPS"
    )

    report.append(
        f"- p99: {best_p99['p99_latency_us']:.2f} us"
    )

    report.append("")

    report.append("## Best p99.9 Latency")

    report.append("")

    report.append(
        f"- Target: {int(best_p999['target_rate_eps'])} EPS"
    )

    report.append(
        f"- p99.9: {best_p999['p999_latency_us']:.2f} us"
    )

    report.append("")

    report.append("## Highest Maximum Latency")

    report.append("")

    report.append(
        f"- Target: {int(highest_max['target_rate_eps'])} EPS"
    )

    report.append(
        f"- Maximum latency: "
        f"{highest_max['max_latency_us']:.2f} us"
    )

    report.append("")

    report.append("## Integrity")

    report.append("")

    if bool(data["integrity_pass"].all()):
        report.append(
            "All benchmark configurations passed integrity validation."
        )
    else:
        failed = data.loc[
            ~data["integrity_pass"]
        ]

        report.append(
            f"{len(failed)} benchmark configurations failed integrity."
        )

    report.append("")

    report.append("## Interpretation")

    report.append("")

    report.append(
        "The benchmark evaluates the real historical event stream "
        "through the NexusFlow adaptive processing pipeline."
    )

    report.append("")

    report.append(
        "Throughput should be interpreted together with tail latency. "
        "A higher throughput result is not automatically better if "
        "p99 or p99.9 latency increases substantially."
    )

    report.append("")

    report.append(
        "The current experiment is a baseline matrix. "
        "Repeated runs are required before making statistical claims "
        "about adaptive scheduling."
    )

    report.append("")

    (OUTPUT_DIR / "report.md").write_text(
        "\n".join(report),
        encoding="utf-8"
    )

    # Throughput vs target rate.
    plt.figure(figsize=(9, 5))

    plt.plot(
        data["target_rate_eps"],
        data["throughput_eps"],
        marker="o",
    )

    plt.xlabel("Target Rate (events/sec)")
    plt.ylabel("Actual Throughput (events/sec)")
    plt.title("NexusFlow Throughput vs Target Rate")
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    plt.savefig(
        OUTPUT_DIR / "throughput_vs_target.png",
        dpi=160
    )

    plt.close()

    # Tail latency.
    plt.figure(figsize=(9, 5))

    plt.plot(
        data["target_rate_eps"],
        data["p99_latency_us"],
        marker="o",
        label="p99",
    )

    plt.plot(
        data["target_rate_eps"],
        data["p999_latency_us"],
        marker="o",
        label="p99.9",
    )

    plt.plot(
        data["target_rate_eps"],
        data["max_latency_us"],
        marker="o",
        label="max",
    )

    plt.xlabel("Target Rate (events/sec)")
    plt.ylabel("Latency (microseconds)")
    plt.title("NexusFlow Tail Latency vs Target Rate")
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    plt.savefig(
        OUTPUT_DIR / "tail_latency_vs_target.png",
        dpi=160
    )

    plt.close()

    print("========================================")
    print("REAL DATASET ANALYSIS COMPLETE")
    print("========================================")
    print()

    print(f"Input: {INPUT}")
    print(f"Configurations: {len(data)}")
    print()

    print(
        f"Best throughput: "
        f"{best_throughput['throughput_eps']:.2f} EPS "
        f"at {int(best_throughput['target_rate_eps'])} EPS target"
    )

    print(
        f"Best p99: "
        f"{best_p99['p99_latency_us']:.2f} us "
        f"at {int(best_p99['target_rate_eps'])} EPS target"
    )

    print(
        f"Best p99.9: "
        f"{best_p999['p999_latency_us']:.2f} us "
        f"at {int(best_p999['target_rate_eps'])} EPS target"
    )

    print()

    print("Integrity:")
    print(
        "PASS"
        if data["integrity_pass"].all()
        else "FAIL"
    )

    print()

    print("Output directory:")
    print(OUTPUT_DIR)


if __name__ == "__main__":
    main()
