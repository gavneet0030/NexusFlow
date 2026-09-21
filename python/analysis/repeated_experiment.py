from pathlib import Path
import subprocess
import re
import pandas as pd


ROOT = Path(__file__).resolve().parents[2]

BINARY = (
    ROOT
    / "build"
    / "Debug"
    / "adaptive_vs_fixed_benchmark.exe"
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

RUNS = 10


def parse_output(output, run_number):
    pattern = re.compile(
        r"^\s*(FIXED_SINGLE|FIXED_BATCH_32|FIXED_PARALLEL_8|ADAPTIVE)"
        r"\s+"
        r"([\d.]+)\s+"
        r"([\d.]+)\s+"
        r"([\d.]+)\s+"
        r"([\d.]+)\s+"
        r"([\d.]+)\s+"
        r"([\d.]+)\s+"
        r"(\d+)",
        re.MULTILINE,
    )

    rows = []

    for match in pattern.finditer(output):

        rows.append(
            {
                "run": run_number,
                "mode": match.group(1),
                "throughput_eps": float(match.group(2)),
                "p50_us": float(match.group(3)),
                "p95_us": float(match.group(4)),
                "p99_us": float(match.group(5)),
                "p999_us": float(match.group(6)),
                "max_latency_us": float(match.group(7)),
                "processed": int(match.group(8)),
            }
        )

    return rows


def main():

    if not BINARY.exists():
        raise FileNotFoundError(
            f"Benchmark binary not found: {BINARY}"
        )

    all_rows = []

    print("=" * 72)
    print("NexusFlow Repeated Benchmark Experiment")
    print("=" * 72)
    print()

    for run_number in range(1, RUNS + 1):

        print(
            f"Running experiment "
            f"{run_number}/{RUNS}..."
        )

        completed = subprocess.run(
            [str(BINARY)],
            capture_output=True,
            text=True,
            cwd=ROOT,
        )

        output = (
            completed.stdout
            + "\n"
            + completed.stderr
        )

        rows = parse_output(
            output,
            run_number,
        )

        if len(rows) != 4:

            print(
                "Warning: expected 4 benchmark modes, "
                f"found {len(rows)}."
            )

        all_rows.extend(rows)

        print(
            f"  Parsed {len(rows)} modes."
        )

    if not all_rows:
        raise RuntimeError(
            "No benchmark results were parsed."
        )

    df = pd.DataFrame(
        all_rows
    )

    raw_output = (
        OUTPUT_DIR
        / "repeated_results.csv"
    )

    df.to_csv(
        raw_output,
        index=False,
    )

    summary = (
        df
        .groupby("mode")
        .agg(
            throughput_mean_eps=(
                "throughput_eps",
                "mean",
            ),
            throughput_std_eps=(
                "throughput_eps",
                "std",
            ),
            p50_mean_us=(
                "p50_us",
                "mean",
            ),
            p95_mean_us=(
                "p95_us",
                "mean",
            ),
            p99_mean_us=(
                "p99_us",
                "mean",
            ),
            p999_mean_us=(
                "p999_us",
                "mean",
            ),
            max_latency_mean_us=(
                "max_latency_us",
                "mean",
            ),
            processed_min=(
                "processed",
                "min",
            ),
            processed_max=(
                "processed",
                "max",
            ),
            runs=(
                "run",
                "count",
            ),
        )
        .reset_index()
    )

    summary["throughput_cv_percent"] = (
        summary["throughput_std_eps"]
        / summary["throughput_mean_eps"]
        * 100.0
    )

    summary_file = (
        OUTPUT_DIR
        / "repeated_summary.csv"
    )

    summary.to_csv(
        summary_file,
        index=False,
    )

    print()
    print("=" * 72)
    print("Repeated Experiment Summary")
    print("=" * 72)
    print()

    display_columns = [
        "mode",
        "throughput_mean_eps",
        "throughput_std_eps",
        "throughput_cv_percent",
        "p99_mean_us",
        "p999_mean_us",
        "max_latency_mean_us",
        "processed_min",
        "processed_max",
        "runs",
    ]

    print(
        summary[
            display_columns
        ].to_string(
            index=False
        )
    )

    print()
    print(
        f"Raw results: {raw_output}"
    )

    print(
        f"Summary: {summary_file}"
    )

    print()
    print(
        "Repeated experiment completed."
    )


if __name__ == "__main__":
    main()
