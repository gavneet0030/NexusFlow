from pathlib import Path
import math

import pandas as pd


ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = (
    ROOT
    / "benchmarks"
    / "results"
    / "repeated"
    / "repeated_results.csv"
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


def mean(values):
    return sum(values) / len(values)


def sample_std(values):
    if len(values) < 2:
        return 0.0

    avg = mean(values)

    variance = sum(
        (value - avg) ** 2
        for value in values
    ) / (len(values) - 1)

    return math.sqrt(variance)


def confidence_interval_95(values):
    n = len(values)

    if n < 2:
        return (
            mean(values),
            mean(values),
        )

    avg = mean(values)
    std = sample_std(values)

    standard_error = (
        std / math.sqrt(n)
    )

    # Conservative t-critical value for n=10.
    # Degrees of freedom = 9.
    t_critical = 2.262

    margin = (
        t_critical
        * standard_error
    )

    return (
        avg - margin,
        avg + margin,
    )


def analyze_mode(group):
    throughput = (
        group["throughput_eps"]
        .dropna()
        .tolist()
    )

    p99 = (
        group["p99_us"]
        .dropna()
        .tolist()
    )

    p999 = (
        group["p999_us"]
        .dropna()
        .tolist()
    )

    max_latency = (
        group["max_latency_us"]
        .dropna()
        .tolist()
    )

    throughput_mean = mean(
        throughput
    )

    throughput_std = sample_std(
        throughput
    )

    ci_low, ci_high = (
        confidence_interval_95(
            throughput
        )
    )

    cv = (
        throughput_std
        / throughput_mean
        * 100.0
        if throughput_mean > 0
        else 0.0
    )

    return {
        "mode": group["mode"].iloc[0],
        "runs": len(group),

        "throughput_mean_eps":
            throughput_mean,

        "throughput_std_eps":
            throughput_std,

        "throughput_ci95_low_eps":
            ci_low,

        "throughput_ci95_high_eps":
            ci_high,

        "throughput_ci95_margin_eps":
            (ci_high - ci_low) / 2.0,

        "throughput_cv_percent":
            cv,

        "p50_mean_us":
            group["p50_us"].mean(),

        "p95_mean_us":
            group["p95_us"].mean(),

        "p99_mean_us":
            mean(p99),

        "p99_std_us":
            sample_std(p99),

        "p999_mean_us":
            mean(p999),

        "p999_std_us":
            sample_std(p999),

        "max_latency_mean_us":
            mean(max_latency),

        "processed_min":
            group["processed"].min(),

        "processed_max":
            group["processed"].max(),
    }


def main():

    if not INPUT_FILE.exists():
        raise FileNotFoundError(
            f"Input file not found: {INPUT_FILE}"
        )

    df = pd.read_csv(
        INPUT_FILE
    )

    required_columns = {
        "run",
        "mode",
        "throughput_eps",
        "p50_us",
        "p95_us",
        "p99_us",
        "p999_us",
        "max_latency_us",
        "processed",
    }

    missing = (
        required_columns
        - set(df.columns)
    )

    if missing:
        raise ValueError(
            "Missing columns: "
            + ", ".join(sorted(missing))
        )

    summary_rows = []

    for _, group in df.groupby(
        "mode",
        sort=False
    ):
        summary_rows.append(
            analyze_mode(group)
        )

    summary = pd.DataFrame(
        summary_rows
    )

    adaptive = summary[
        summary["mode"] == "ADAPTIVE"
    ]

    if adaptive.empty:
        raise ValueError(
            "ADAPTIVE results not found."
        )

    adaptive_throughput = float(
        adaptive.iloc[0][
            "throughput_mean_eps"
        ]
    )

    adaptive_p99 = float(
        adaptive.iloc[0][
            "p99_mean_us"
        ]
    )

    adaptive_p999 = float(
        adaptive.iloc[0][
            "p999_mean_us"
        ]
    )

    summary["adaptive_throughput_delta_percent"] = (
        (
            adaptive_throughput
            - summary["throughput_mean_eps"]
        )
        / summary["throughput_mean_eps"]
        * 100.0
    )

    summary["adaptive_p99_delta_percent"] = (
        (
            adaptive_p99
            - summary["p99_mean_us"]
        )
        / summary["p99_mean_us"]
        * 100.0
    )

    summary["adaptive_p999_delta_percent"] = (
        (
            adaptive_p999
            - summary["p999_mean_us"]
        )
        / summary["p999_mean_us"]
        * 100.0
    )

    output_file = (
        OUTPUT_DIR
        / "statistical_summary.csv"
    )

    summary.to_csv(
        output_file,
        index=False
    )

    print()
    print("=" * 100)
    print(
        "NexusFlow Statistical Benchmark Analysis"
    )
    print("=" * 100)
    print()

    display_columns = [
        "mode",
        "runs",
        "throughput_mean_eps",
        "throughput_std_eps",
        "throughput_ci95_low_eps",
        "throughput_ci95_high_eps",
        "throughput_cv_percent",
        "p99_mean_us",
        "p999_mean_us",
        "processed_min",
        "processed_max",
    ]

    print(
        summary[
            display_columns
        ].to_string(
            index=False,
            float_format=lambda x:
                f"{x:.3f}"
        )
    )

    print()
    print(
        "Adaptive throughput comparison"
    )
    print("-" * 100)

    for _, row in summary.iterrows():

        mode = row["mode"]

        if mode == "ADAPTIVE":
            continue

        print()
        print(
            f"ADAPTIVE vs {mode}"
        )

        print(
            "Throughput difference: "
            f"{row['adaptive_throughput_delta_percent']:.2f}%"
        )

        print(
            "P99 difference: "
            f"{row['adaptive_p99_delta_percent']:.2f}%"
        )

        print(
            "P99.9 difference: "
            f"{row['adaptive_p999_delta_percent']:.2f}%"
        )

    print()
    print("=" * 100)

    print(
        f"Statistical summary written to: "
        f"{output_file}"
    )

    print()
    print(
        "Statistical analysis completed."
    )


if __name__ == "__main__":
    main()
