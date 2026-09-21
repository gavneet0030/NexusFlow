from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = (
    ROOT
    / "benchmarks"
    / "results"
    / "adaptive_vs_fixed_results.csv"
)

OUTPUT_DIR = (
    ROOT
    / "benchmarks"
    / "results"
    / "analysis"
)

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


def load_results():
    if not INPUT_FILE.exists():
        raise FileNotFoundError(
            f"Benchmark CSV not found: {INPUT_FILE}"
        )

    return pd.read_csv(INPUT_FILE)


def calculate_comparison(df):
    adaptive = df[
        df["mode"] == "ADAPTIVE"
    ].iloc[0]

    fixed_modes = [
        "FIXED_SINGLE",
        "FIXED_BATCH_32",
        "FIXED_PARALLEL_8",
    ]

    rows = []

    for mode in fixed_modes:

        baseline = df[
            df["mode"] == mode
        ].iloc[0]

        throughput_improvement = (
            (
                adaptive["throughput_eps"]
                - baseline["throughput_eps"]
            )
            / baseline["throughput_eps"]
        ) * 100.0

        p99_change = (
            (
                adaptive["p99_us"]
                - baseline["p99_us"]
            )
            / baseline["p99_us"]
        ) * 100.0

        p999_change = (
            (
                adaptive["p999_us"]
                - baseline["p999_us"]
            )
            / baseline["p999_us"]
        ) * 100.0

        rows.append(
            {
                "baseline": mode,
                "adaptive_throughput_eps":
                    adaptive["throughput_eps"],
                "baseline_throughput_eps":
                    baseline["throughput_eps"],
                "throughput_improvement_percent":
                    throughput_improvement,
                "adaptive_p99_us":
                    adaptive["p99_us"],
                "baseline_p99_us":
                    baseline["p99_us"],
                "p99_change_percent":
                    p99_change,
                "adaptive_p999_us":
                    adaptive["p999_us"],
                "baseline_p999_us":
                    baseline["p999_us"],
                "p999_change_percent":
                    p999_change,
            }
        )

    return pd.DataFrame(rows)


def print_summary(df, comparison):
    print()
    print("=" * 72)
    print("NexusFlow Automated Benchmark Analysis")
    print("=" * 72)

    print()
    print("Raw benchmark results")
    print("-" * 72)

    columns = [
        "mode",
        "throughput_eps",
        "p50_us",
        "p95_us",
        "p99_us",
        "p999_us",
        "max_latency_us",
        "processed",
    ]

    print(
        df[columns].to_string(
            index=False
        )
    )

    print()
    print("Adaptive comparison")
    print("-" * 72)

    for _, row in comparison.iterrows():

        print()
        print(
            f"ADAPTIVE vs {row['baseline']}"
        )

        print(
            "Throughput improvement: "
            f"{row['throughput_improvement_percent']:.2f}%"
        )

        print(
            "P99 change: "
            f"{row['p99_change_percent']:.2f}%"
        )

        print(
            "P99.9 change: "
            f"{row['p999_change_percent']:.2f}%"
        )

    print()
    print("=" * 72)


def create_throughput_chart(df):
    plt.figure(figsize=(10, 6))

    plt.bar(
        df["mode"],
        df["throughput_eps"]
    )

    plt.ylabel("Throughput (events/sec)")
    plt.xlabel("Execution Mode")
    plt.title(
        "NexusFlow Throughput Comparison"
    )

    plt.xticks(
        rotation=20,
        ha="right"
    )

    plt.tight_layout()

    output = (
        OUTPUT_DIR
        / "throughput_comparison.png"
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
        df["p99_us"],
        marker="o",
        label="P99"
    )

    plt.plot(
        x,
        df["p999_us"],
        marker="o",
        label="P99.9"
    )

    plt.xticks(
        list(x),
        df["mode"],
        rotation=20,
        ha="right"
    )

    plt.ylabel("Latency (microseconds)")
    plt.xlabel("Execution Mode")
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


def main():
    df = load_results()

    comparison = calculate_comparison(
        df
    )

    print_summary(
        df,
        comparison
    )

    comparison_file = (
        OUTPUT_DIR
        / "adaptive_comparison.csv"
    )

    comparison.to_csv(
        comparison_file,
        index=False
    )

    create_throughput_chart(
        df
    )

    create_tail_latency_chart(
        df
    )

    print()
    print(
        f"Analysis CSV: {comparison_file}"
    )

    print(
        "Throughput chart: "
        f"{OUTPUT_DIR / 'throughput_comparison.png'}"
    )

    print(
        "Tail latency chart: "
        f"{OUTPUT_DIR / 'tail_latency_comparison.png'}"
    )

    print()
    print(
        "Analysis completed successfully."
    )


if __name__ == "__main__":
    main()
