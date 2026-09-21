from pathlib import Path
import math
import pandas as pd


INPUT = Path(
    "benchmarks/results/real_dataset_repeated_raw.csv"
)

OUTPUT_DIR = Path(
    "benchmarks/results/real_dataset_repeated_analysis"
)

OUTPUT_DIR.mkdir(
    parents=True,
    exist_ok=True
)


def mean_confidence_interval(series):
    values = series.dropna().astype(float)

    n = len(values)

    if n == 0:
        return 0.0, 0.0, 0.0, 0.0

    mean = values.mean()

    if n == 1:
        return mean, 0.0, mean, mean

    std = values.std(ddof=1)

    # 95% normal approximation.
    # For n=5 this is intentionally conservative enough
    # for an engineering baseline; later we can use the
    # exact Student-t critical value.
    margin = 1.96 * std / math.sqrt(n)

    return (
        mean,
        std,
        mean - margin,
        mean + margin,
    )


def coefficient_of_variation(series):
    values = series.dropna().astype(float)

    if len(values) < 2:
        return 0.0

    mean = values.mean()

    if mean == 0:
        return 0.0

    return (
        values.std(ddof=1)
        / mean
        * 100.0
    )


def main():

    if not INPUT.exists():
        raise FileNotFoundError(
            f"Input file not found: {INPUT}"
        )

    data = pd.read_csv(INPUT)

    required = [
        "run_id",
        "target_rate_eps",
        "throughput_eps",
        "average_latency_us",
        "p50_us",
        "p95_us",
        "p99_us",
        "p999_us",
        "max_us",
        "queue_depth",
        "workers",
        "rejected",
        "throttled",
        "integrity_pass",
    ]

    missing = [
        column
        for column in required
        if column not in data.columns
    ]

    if missing:
        raise ValueError(
            "Missing columns: "
            + ", ".join(missing)
        )

    data["integrity_pass"] = (
        data["integrity_pass"]
        .astype(str)
        .str.upper()
        .eq("PASS")
    )

    rows = []

    for rate, group in data.groupby(
        "target_rate_eps"
    ):

        throughput_mean, throughput_std, throughput_low, throughput_high = (
            mean_confidence_interval(
                group["throughput_eps"]
            )
        )

        p99_mean, p99_std, p99_low, p99_high = (
            mean_confidence_interval(
                group["p99_us"]
            )
        )

        p999_mean, p999_std, p999_low, p999_high = (
            mean_confidence_interval(
                group["p999_us"]
            )
        )

        max_mean, max_std, max_low, max_high = (
            mean_confidence_interval(
                group["max_us"]
            )
        )

        p50_mean = group["p50_us"].mean()
        p95_mean = group["p95_us"].mean()

        rows.append({
            "target_rate_eps": rate,
            "runs": len(group),

            "throughput_mean_eps":
                throughput_mean,

            "throughput_std_eps":
                throughput_std,

            "throughput_cv_percent":
                coefficient_of_variation(
                    group["throughput_eps"]
                ),

            "throughput_ci95_low":
                throughput_low,

            "throughput_ci95_high":
                throughput_high,

            "p50_mean_us":
                p50_mean,

            "p95_mean_us":
                p95_mean,

            "p99_mean_us":
                p99_mean,

            "p99_std_us":
                p99_std,

            "p99_ci95_low_us":
                p99_low,

            "p99_ci95_high_us":
                p99_high,

            "p999_mean_us":
                p999_mean,

            "p999_std_us":
                p999_std,

            "p999_ci95_low_us":
                p999_low,

            "p999_ci95_high_us":
                p999_high,

            "max_mean_us":
                max_mean,

            "max_std_us":
                max_std,

            "max_ci95_low_us":
                max_low,

            "max_ci95_high_us":
                max_high,

            "max_observed_us":
                group["max_us"].max(),

            "average_workers":
                group["workers"].mean(),

            "maximum_queue_depth":
                group["queue_depth"].max(),

            "total_rejected":
                group["rejected"].sum(),

            "total_throttled":
                group["throttled"].sum(),

            "integrity_pass_rate_percent":
                group["integrity_pass"].mean() * 100.0,
        })

    summary = pd.DataFrame(rows)

    summary = summary.sort_values(
        "target_rate_eps"
    )

    summary.to_csv(
        OUTPUT_DIR / "summary.csv",
        index=False
    )

    # Save a compact research table.
    research_columns = [
        "target_rate_eps",
        "runs",
        "throughput_mean_eps",
        "throughput_std_eps",
        "throughput_cv_percent",
        "throughput_ci95_low",
        "throughput_ci95_high",
        "p50_mean_us",
        "p95_mean_us",
        "p99_mean_us",
        "p99_ci95_low_us",
        "p99_ci95_high_us",
        "p999_mean_us",
        "p999_ci95_low_us",
        "p999_ci95_high_us",
        "max_mean_us",
        "max_observed_us",
        "average_workers",
        "maximum_queue_depth",
        "total_rejected",
        "total_throttled",
        "integrity_pass_rate_percent",
    ]

    summary[research_columns].to_csv(
        OUTPUT_DIR / "research_table.csv",
        index=False
    )

    report = []

    report.append(
        "# NexusFlow Repeated Real Dataset Experiment"
    )

    report.append("")

    report.append(
        "Workload: UCI Online Retail II"
    )

    report.append(
        f"Total completed runs: {len(data)}"
    )

    report.append("")

    report.append("## Results")

    report.append("")

    report.append(
        "| Target EPS | Mean EPS | Std EPS | CV | "
        "p50 us | p95 us | p99 us | p99.9 us | "
        "Max us | Workers | Integrity |"
    )

    report.append(
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|:---|"
    )

    for _, row in summary.iterrows():

        report.append(
            f"| {int(row['target_rate_eps'])} "
            f"| {row['throughput_mean_eps']:.2f} "
            f"| {row['throughput_std_eps']:.2f} "
            f"| {row['throughput_cv_percent']:.2f}% "
            f"| {row['p50_mean_us']:.2f} "
            f"| {row['p95_mean_us']:.2f} "
            f"| {row['p99_mean_us']:.2f} "
            f"| {row['p999_mean_us']:.2f} "
            f"| {row['max_observed_us']:.2f} "
            f"| {row['average_workers']:.2f} "
            f"| {row['integrity_pass_rate_percent']:.0f}% |"
        )

    report.append("")

    report.append("## Statistical Interpretation")

    report.append("")

    report.append(
        "Throughput confidence intervals describe "
        "run-to-run variability under each target load."
    )

    report.append("")

    report.append(
        "Tail latency is reported independently because "
        "average latency can hide rare high-latency events."
    )

    report.append("")

    report.append(
        "These results are observational baseline measurements. "
        "They do not establish that adaptive scheduling is superior "
        "until fixed scheduling baselines are measured under the "
        "same workload and experimental conditions."
    )

    report.append("")

    report.append("## Integrity")

    report.append("")

    if data["integrity_pass"].all():
        report.append(
            "All completed runs passed integrity validation."
        )
    else:
        failed = int(
            (~data["integrity_pass"]).sum()
        )

        report.append(
            f"{failed} runs failed integrity validation."
        )

    report.append("")

    report.append("## Files")

    report.append("")

    report.append(
        "- raw results: "
        "benchmarks/results/real_dataset_repeated_raw.csv"
    )

    report.append(
        "- statistical summary: "
        "benchmarks/results/real_dataset_repeated_analysis/summary.csv"
    )

    report.append(
        "- research table: "
        "benchmarks/results/real_dataset_repeated_analysis/research_table.csv"
    )

    (OUTPUT_DIR / "report.md").write_text(
        "\n".join(report),
        encoding="utf-8"
    )

    print("========================================")
    print("STATISTICAL ANALYSIS COMPLETE")
    print("========================================")
    print()

    print(
        f"Completed runs: {len(data)}"
    )

    print()

    for _, row in summary.iterrows():

        print(
            f"{int(row['target_rate_eps']):>7} EPS | "
            f"mean={row['throughput_mean_eps']:.2f} | "
            f"std={row['throughput_std_eps']:.2f} | "
            f"CV={row['throughput_cv_percent']:.2f}% | "
            f"p99={row['p99_mean_us']:.2f} us | "
            f"p99.9={row['p999_mean_us']:.2f} us"
        )

    print()
    print("Integrity:")
    print(
        "PASS"
        if data["integrity_pass"].all()
        else "FAIL"
    )

    print()
    print(
        "Output:"
    )

    print(
        OUTPUT_DIR
    )


if __name__ == "__main__":
    main()
