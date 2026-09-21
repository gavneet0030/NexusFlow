from pathlib import Path
import pandas as pd
import numpy as np


def main():
    input_file = Path(
        "benchmarks/results/real_dataset_repeated_raw.csv"
    )

    output_dir = Path(
        "benchmarks/results/real_dataset_repeated_analysis"
    )

    output_dir.mkdir(
        parents=True,
        exist_ok=True
    )

    if not input_file.exists():
        print(
            f"ERROR: Input file not found: {input_file}"
        )
        return

    df = pd.read_csv(input_file)

    required_columns = [
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

    missing_columns = [
        column
        for column in required_columns
        if column not in df.columns
    ]

    if missing_columns:
        print("ERROR: Missing required columns:")

        for column in missing_columns:
            print(f"  {column}")

        return

    df["integrity_ok"] = (
        df["integrity_pass"]
        .astype(str)
        .str.upper()
        .eq("PASS")
    )

    rows = []

    for rate, group in df.groupby(
        "target_rate_eps",
        sort=True
    ):
        n = len(group)

        throughput = group["throughput_eps"]
        p99 = group["p99_us"]
        p999 = group["p999_us"]
        max_latency = group["max_us"]

        throughput_mean = throughput.mean()

        throughput_std = (
            throughput.std(ddof=1)
            if n > 1
            else 0.0
        )

        p99_mean = p99.mean()

        p99_std = (
            p99.std(ddof=1)
            if n > 1
            else 0.0
        )

        p999_mean = p999.mean()

        p999_std = (
            p999.std(ddof=1)
            if n > 1
            else 0.0
        )

        max_mean = max_latency.mean()

        max_std = (
            max_latency.std(ddof=1)
            if n > 1
            else 0.0
        )

        throughput_ci = (
            1.96
            * throughput_std
            / np.sqrt(n)
            if n > 1
            else 0.0
        )

        p99_ci = (
            1.96
            * p99_std
            / np.sqrt(n)
            if n > 1
            else 0.0
        )

        p999_ci = (
            1.96
            * p999_std
            / np.sqrt(n)
            if n > 1
            else 0.0
        )

        max_ci = (
            1.96
            * max_std
            / np.sqrt(n)
            if n > 1
            else 0.0
        )

        throughput_cv = (
            throughput_std
            / throughput_mean
            * 100.0
            if throughput_mean != 0
            else 0.0
        )

        rows.append(
            {
                "rate_eps": rate,
                "runs": n,

                "throughput_mean_eps":
                    throughput_mean,

                "throughput_std_eps":
                    throughput_std,

                "throughput_cv_percent":
                    throughput_cv,

                "throughput_ci95_low_eps":
                    throughput_mean
                    - throughput_ci,

                "throughput_ci95_high_eps":
                    throughput_mean
                    + throughput_ci,

                "average_latency_mean_us":
                    group[
                        "average_latency_us"
                    ].mean(),

                "p50_mean_us":
                    group["p50_us"].mean(),

                "p95_mean_us":
                    group["p95_us"].mean(),

                "p99_mean_us":
                    p99_mean,

                "p99_std_us":
                    p99_std,

                "p99_ci95_low_us":
                    p99_mean - p99_ci,

                "p99_ci95_high_us":
                    p99_mean + p99_ci,

                "p999_mean_us":
                    p999_mean,

                "p999_std_us":
                    p999_std,

                "p999_ci95_low_us":
                    p999_mean - p999_ci,

                "p999_ci95_high_us":
                    p999_mean + p999_ci,

                "max_latency_mean_us":
                    max_mean,

                "max_latency_observed_us":
                    max_latency.max(),

                "max_latency_ci95_low_us":
                    max_mean - max_ci,

                "max_latency_ci95_high_us":
                    max_mean + max_ci,

                "average_workers":
                    group["workers"].mean(),

                "maximum_workers":
                    group["workers"].max(),

                "maximum_queue_depth":
                    group["queue_depth"].max(),

                "total_rejected":
                    group["rejected"].sum(),

                "total_throttled":
                    group["throttled"].sum(),

                "integrity_pass_rate_percent":
                    group["integrity_ok"].mean()
                    * 100.0,
            }
        )

    summary = pd.DataFrame(rows)

    summary_file = (
        output_dir / "summary.csv"
    )

    summary.to_csv(
        summary_file,
        index=False
    )

    research_columns = [
        "rate_eps",
        "runs",
        "throughput_mean_eps",
        "throughput_std_eps",
        "throughput_cv_percent",
        "throughput_ci95_low_eps",
        "throughput_ci95_high_eps",
        "average_latency_mean_us",
        "p50_mean_us",
        "p95_mean_us",
        "p99_mean_us",
        "p99_std_us",
        "p99_ci95_low_us",
        "p99_ci95_high_us",
        "p999_mean_us",
        "p999_std_us",
        "p999_ci95_low_us",
        "p999_ci95_high_us",
        "max_latency_mean_us",
        "max_latency_observed_us",
        "average_workers",
        "maximum_workers",
        "maximum_queue_depth",
        "total_rejected",
        "total_throttled",
        "integrity_pass_rate_percent",
    ]

    research_table = summary[
        research_columns
    ]

    research_file = (
        output_dir / "research_table.csv"
    )

    research_table.to_csv(
        research_file,
        index=False
    )

    report_lines = []

    report_lines.append(
        "# NexusFlow Real Dataset Repeated Experiment"
    )

    report_lines.append("")

    report_lines.append(
        "Repeated end-to-end replay experiment "
        "using the processed real-world dataset."
    )

    report_lines.append("")

    report_lines.append(
        f"- Total runs analyzed: {len(df)}"
    )

    report_lines.append(
        f"- Replay rates: "
        f"{df['target_rate_eps'].nunique()}"
    )

    report_lines.append("")

    for _, row in summary.iterrows():

        rate = int(row["rate_eps"])

        report_lines.append(
            f"## {rate:,} EPS"
        )

        report_lines.append("")

        report_lines.append(
            f"- Runs: {int(row['runs'])}"
        )

        report_lines.append(
            f"- Mean throughput: "
            f"{row['throughput_mean_eps']:.2f} EPS"
        )

        report_lines.append(
            f"- Throughput standard deviation: "
            f"{row['throughput_std_eps']:.2f} EPS"
        )

        report_lines.append(
            f"- Throughput CV: "
            f"{row['throughput_cv_percent']:.2f}%"
        )

        report_lines.append(
            f"- Mean p50 latency: "
            f"{row['p50_mean_us']:.3f} us"
        )

        report_lines.append(
            f"- Mean p95 latency: "
            f"{row['p95_mean_us']:.3f} us"
        )

        report_lines.append(
            f"- Mean p99 latency: "
            f"{row['p99_mean_us']:.3f} us"
        )

        report_lines.append(
            f"- Mean p99.9 latency: "
            f"{row['p999_mean_us']:.3f} us"
        )

        report_lines.append(
            f"- Mean maximum latency: "
            f"{row['max_latency_mean_us']:.3f} us"
        )

        report_lines.append(
            f"- Observed maximum latency: "
            f"{row['max_latency_observed_us']:.3f} us"
        )

        report_lines.append(
            f"- Average workers: "
            f"{row['average_workers']:.2f}"
        )

        report_lines.append(
            f"- Maximum workers: "
            f"{int(row['maximum_workers'])}"
        )

        report_lines.append(
            f"- Maximum queue depth: "
            f"{int(row['maximum_queue_depth'])}"
        )

        report_lines.append(
            f"- Rejected events: "
            f"{int(row['total_rejected'])}"
        )

        report_lines.append(
            f"- Throttled events: "
            f"{int(row['total_throttled'])}"
        )

        report_lines.append(
            f"- Integrity pass rate: "
            f"{row['integrity_pass_rate_percent']:.1f}%"
        )

        report_lines.append("")

    report_lines.append(
        "## Statistical Note"
    )

    report_lines.append("")

    report_lines.append(
        "The 95% confidence intervals use a normal "
        "approximation. Five repetitions are available "
        "for each target rate. Future experiments should "
        "use additional repetitions and Student's t "
        "confidence intervals for stronger statistical rigor."
    )

    report_file = (
        output_dir / "report.md"
    )

    report_file.write_text(
        "\n".join(report_lines),
        encoding="utf-8"
    )

    print("")
    print("==============================================")
    print("NexusFlow Real Dataset Statistical Analysis")
    print("==============================================")
    print("")

    print(
        f"Input: {input_file}"
    )

    print(
        f"Runs analyzed: {len(df)}"
    )

    print(
        f"Rates analyzed: "
        f"{df['target_rate_eps'].nunique()}"
    )

    print("")

    display_columns = [
        "rate_eps",
        "throughput_mean_eps",
        "throughput_cv_percent",
        "p99_mean_us",
        "p999_mean_us",
        "max_latency_observed_us",
        "average_workers",
        "maximum_queue_depth",
        "integrity_pass_rate_percent",
    ]

    print(
        summary[
            display_columns
        ].to_string(index=False)
    )

    print("")
    print("Generated files:")
    print(f"  {summary_file}")
    print(f"  {research_file}")
    print(f"  {report_file}")
    print("")
    print("ANALYSIS COMPLETE")


if __name__ == "__main__":
    main()
