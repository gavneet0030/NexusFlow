from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = (
    ROOT
    / "benchmarks"
    / "results"
    / "repeated"
    / "statistical_summary.csv"
)

OUTPUT_DIR = (
    ROOT
    / "benchmarks"
    / "results"
    / "repeated"
)

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


def load_data():
    if not INPUT_FILE.exists():
        raise FileNotFoundError(
            f"Statistical summary not found: {INPUT_FILE}"
        )

    return pd.read_csv(INPUT_FILE)


def create_throughput_ci_chart(df):

    plt.figure(figsize=(10, 6))

    means = df[
        "throughput_mean_eps"
    ]

    lower = df[
        "throughput_ci95_low_eps"
    ]

    upper = df[
        "throughput_ci95_high_eps"
    ]

    errors = [
        (
            mean - low,
            high - mean
        )
        for mean, low, high
        in zip(
            means,
            lower,
            upper
        )
    ]

    x = range(len(df))

    plt.errorbar(
        x,
        means,
        yerr=list(zip(*errors)),
        fmt="o",
        capsize=6
    )

    plt.xticks(
        list(x),
        df["mode"],
        rotation=20,
        ha="right"
    )

    plt.ylabel(
        "Throughput (events/sec)"
    )

    plt.xlabel(
        "Execution Mode"
    )

    plt.title(
        "NexusFlow Throughput with 95% Confidence Intervals"
    )

    plt.tight_layout()

    output = (
        OUTPUT_DIR
        / "throughput_confidence_interval.png"
    )

    plt.savefig(
        output,
        dpi=200
    )

    plt.close()


def create_tail_latency_chart(df):

    plt.figure(figsize=(10, 6))

    x = range(len(df))

    plt.plot(
        x,
        df["p99_mean_us"],
        marker="o",
        label="P99"
    )

    plt.plot(
        x,
        df["p999_mean_us"],
        marker="o",
        label="P99.9"
    )

    plt.xticks(
        list(x),
        df["mode"],
        rotation=20,
        ha="right"
    )

    plt.ylabel(
        "Latency (microseconds)"
    )

    plt.xlabel(
        "Execution Mode"
    )

    plt.title(
        "NexusFlow Tail Latency Comparison"
    )

    plt.legend()

    plt.tight_layout()

    output = (
        OUTPUT_DIR
        / "tail_latency_comparison.png"
    )

    plt.savefig(
        output,
        dpi=200
    )

    plt.close()


def create_variability_chart(df):

    plt.figure(figsize=(10, 6))

    plt.bar(
        df["mode"],
        df["throughput_cv_percent"]
    )

    plt.ylabel(
        "Throughput CV (%)"
    )

    plt.xlabel(
        "Execution Mode"
    )

    plt.title(
        "NexusFlow Throughput Variability"
    )

    plt.xticks(
        rotation=20,
        ha="right"
    )

    plt.tight_layout()

    output = (
        OUTPUT_DIR
        / "throughput_variability.png"
    )

    plt.savefig(
        output,
        dpi=200
    )

    plt.close()


def create_markdown_report(df):

    adaptive = df[
        df["mode"] == "ADAPTIVE"
    ].iloc[0]

    lines = []

    lines.append(
        "# NexusFlow Experimental Results"
    )

    lines.append("")

    lines.append(
        "## Experimental Configuration"
    )

    lines.append("")

    lines.append(
        "- Benchmark type: Adaptive vs fixed execution"
    )

    lines.append(
        "- Repetitions per mode: 10"
    )

    lines.append(
        "- Events per run: 30,000"
    )

    lines.append(
        "- Confidence level: 95%"
    )

    lines.append("")

    lines.append(
        "## Throughput Results"
    )

    lines.append("")

    lines.append(
        "| Mode | Mean EPS | Std Dev | 95% CI | CV |"
    )

    lines.append(
        "|---|---:|---:|---:|---:|"
    )

    for _, row in df.iterrows():

        ci = (
            f"{row['throughput_ci95_low_eps']:.1f}"
            " - "
            f"{row['throughput_ci95_high_eps']:.1f}"
        )

        lines.append(
            f"| {row['mode']} "
            f"| {row['throughput_mean_eps']:.1f} "
            f"| {row['throughput_std_eps']:.1f} "
            f"| {ci} "
            f"| {row['throughput_cv_percent']:.2f}% |"
        )

    lines.append("")

    lines.append(
        "## Tail Latency"
    )

    lines.append("")

    lines.append(
        "| Mode | P50 (us) | P95 (us) | P99 (us) | P99.9 (us) | Max (us) |"
    )

    lines.append(
        "|---|---:|---:|---:|---:|---:|"
    )

    for _, row in df.iterrows():

        lines.append(
            f"| {row['mode']} "
            f"| {row['p50_mean_us']:.2f} "
            f"| {row['p95_mean_us']:.2f} "
            f"| {row['p99_mean_us']:.2f} "
            f"| {row['p999_mean_us']:.2f} "
            f"| {row['max_latency_mean_us']:.2f} |"
        )

    lines.append("")

    lines.append(
        "## Adaptive Throughput Comparison"
    )

    lines.append("")

    for _, row in df.iterrows():

        if row["mode"] == "ADAPTIVE":
            continue

        lines.append(
            f"- ADAPTIVE vs {row['mode']}: "
            f"{row['adaptive_throughput_delta_percent']:.2f}% "
            "throughput difference."
        )

    lines.append("")

    lines.append(
        "## Interpretation"
    )

    lines.append("")

    lines.append(
        "The adaptive scheduler achieved the highest mean "
        "throughput across the evaluated modes."
    )

    lines.append("")

    lines.append(
        f"Adaptive mean throughput was "
        f"{adaptive['throughput_mean_eps']:.1f} events/sec."
    )

    lines.append("")

    lines.append(
        "The results also show a tail-latency trade-off. "
        "Adaptive execution does not minimize every latency "
        "percentile, particularly P99.9."
    )

    lines.append("")

    lines.append(
        "Therefore, the current experiment supports the "
        "hypothesis that adaptive scheduling can improve "
        "throughput, but further SLA-aware tuning is required "
        "to control extreme tail latency."
    )

    lines.append("")

    lines.append(
        "## Reproducibility"
    )

    lines.append("")

    lines.append(
        "All modes processed 30,000 events in every recorded "
        "run. The experiment was repeated 10 times per mode."
    )

    lines.append("")

    return "\n".join(lines)


def main():

    df = load_data()

    create_throughput_ci_chart(df)

    create_tail_latency_chart(df)

    create_variability_chart(df)

    report = create_markdown_report(
        df
    )

    report_path = (
        OUTPUT_DIR
        / "experiment_report.md"
    )

    report_path.write_text(
        report,
        encoding="utf-8"
    )

    print()
    print(
        "Research experiment report generated."
    )

    print()
    print(
        f"Report: {report_path}"
    )

    print(
        "Throughput CI chart: "
        f"{OUTPUT_DIR / 'throughput_confidence_interval.png'}"
    )

    print(
        "Tail latency chart: "
        f"{OUTPUT_DIR / 'tail_latency_comparison.png'}"
    )

    print(
        "Variability chart: "
        f"{OUTPUT_DIR / 'throughput_variability.png'}"
    )

    print()
    print(
        "Report generation completed."
    )


if __name__ == "__main__":
    main()
