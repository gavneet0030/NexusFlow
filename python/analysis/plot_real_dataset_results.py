from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


INPUT = Path(
    "benchmarks/results/real_dataset_repeated_raw.csv"
)

OUTPUT = Path(
    "benchmarks/results/real_dataset_repeated_analysis"
)

OUTPUT.mkdir(
    parents=True,
    exist_ok=True
)


def save_plot(filename):
    plt.tight_layout()
    plt.savefig(
        OUTPUT / filename,
        dpi=220,
        bbox_inches="tight"
    )
    plt.close()


def main():

    if not INPUT.exists():
        print(
            f"ERROR: Input file not found: {INPUT}"
        )
        return

    df = pd.read_csv(INPUT)

    df["integrity_ok"] = (
        df["integrity_pass"]
        .astype(str)
        .str.upper()
        .eq("PASS")
    )

    # Exclude the historical pre-fix 50K run
    # from corrected integrity statistics.
    corrected_df = df[
        ~(
            (df["target_rate_eps"] == 50000)
            & (df["run_id"] == 11)
        )
    ].copy()

    summary = (
        corrected_df
        .groupby("target_rate_eps")
        .agg(
            throughput_mean_eps=(
                "throughput_eps",
                "mean"
            ),
            throughput_std_eps=(
                "throughput_eps",
                "std"
            ),
            p50_mean_us=(
                "p50_us",
                "mean"
            ),
            p95_mean_us=(
                "p95_us",
                "mean"
            ),
            p99_mean_us=(
                "p99_us",
                "mean"
            ),
            p999_mean_us=(
                "p999_us",
                "mean"
            ),
            max_latency_mean_us=(
                "max_us",
                "mean"
            ),
            max_latency_observed_us=(
                "max_us",
                "max"
            ),
            average_workers=(
                "workers",
                "mean"
            ),
            maximum_workers=(
                "workers",
                "max"
            ),
            integrity_pass_rate=(
                "integrity_ok",
                "mean"
            )
        )
        .reset_index()
    )

    # --------------------------------------------------------
    # 1. Throughput vs offered load
    # --------------------------------------------------------

    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["target_rate_eps"],
        summary["throughput_mean_eps"],
        marker="o"
    )

    plt.plot(
        summary["target_rate_eps"],
        summary["target_rate_eps"],
        linestyle="--"
    )

    plt.xlabel(
        "Offered Load (events/sec)"
    )

    plt.ylabel(
        "Achieved Throughput (events/sec)"
    )

    plt.title(
        "NexusFlow Throughput vs Offered Load"
    )

    plt.xscale("log")
    plt.grid(True, alpha=0.3)

    save_plot(
        "throughput_vs_load.png"
    )

    # --------------------------------------------------------
    # 2. Throughput efficiency
    # --------------------------------------------------------

    efficiency = (
        summary["throughput_mean_eps"]
        / summary["target_rate_eps"]
        * 100.0
    )

    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["target_rate_eps"],
        efficiency,
        marker="o"
    )

    plt.xlabel(
        "Offered Load (events/sec)"
    )

    plt.ylabel(
        "Throughput Efficiency (%)"
    )

    plt.title(
        "NexusFlow Throughput Efficiency"
    )

    plt.xscale("log")
    plt.grid(True, alpha=0.3)

    save_plot(
        "throughput_efficiency.png"
    )

    # --------------------------------------------------------
    # 3. Percentile latency
    # --------------------------------------------------------

    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["target_rate_eps"],
        summary["p50_mean_us"],
        marker="o",
        label="p50"
    )

    plt.plot(
        summary["target_rate_eps"],
        summary["p95_mean_us"],
        marker="o",
        label="p95"
    )

    plt.plot(
        summary["target_rate_eps"],
        summary["p99_mean_us"],
        marker="o",
        label="p99"
    )

    plt.xlabel(
        "Offered Load (events/sec)"
    )

    plt.ylabel(
        "Latency (microseconds)"
    )

    plt.title(
        "NexusFlow Latency Percentiles"
    )

    plt.xscale("log")
    plt.yscale("log")

    plt.legend()
    plt.grid(True, alpha=0.3)

    save_plot(
        "p50_p95_p99_latency.png"
    )

    # --------------------------------------------------------
    # 4. p99.9 latency
    # --------------------------------------------------------

    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["target_rate_eps"],
        summary["p999_mean_us"],
        marker="o"
    )

    plt.xlabel(
        "Offered Load (events/sec)"
    )

    plt.ylabel(
        "p99.9 Latency (microseconds)"
    )

    plt.title(
        "NexusFlow Tail Latency at p99.9"
    )

    plt.xscale("log")
    plt.yscale("log")

    plt.grid(True, alpha=0.3)

    save_plot(
        "p999_latency.png"
    )

    # --------------------------------------------------------
    # 5. Maximum latency
    # --------------------------------------------------------

    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["target_rate_eps"],
        summary["max_latency_mean_us"],
        marker="o",
        label="Mean maximum"
    )

    plt.plot(
        summary["target_rate_eps"],
        summary["max_latency_observed_us"],
        marker="x",
        linestyle="--",
        label="Observed maximum"
    )

    plt.xlabel(
        "Offered Load (events/sec)"
    )

    plt.ylabel(
        "Maximum Latency (microseconds)"
    )

    plt.title(
        "NexusFlow Maximum Latency"
    )

    plt.xscale("log")
    plt.yscale("log")

    plt.legend()
    plt.grid(True, alpha=0.3)

    save_plot(
        "maximum_latency.png"
    )

    # --------------------------------------------------------
    # 6. Throughput variability
    # --------------------------------------------------------

    summary["throughput_cv_percent"] = (
        summary["throughput_std_eps"]
        / summary["throughput_mean_eps"]
        * 100.0
    )

    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["target_rate_eps"],
        summary["throughput_cv_percent"],
        marker="o"
    )

    plt.xlabel(
        "Offered Load (events/sec)"
    )

    plt.ylabel(
        "Throughput CV (%)"
    )

    plt.title(
        "NexusFlow Throughput Variability"
    )

    plt.xscale("log")
    plt.grid(True, alpha=0.3)

    save_plot(
        "throughput_variability.png"
    )

    # --------------------------------------------------------
    # 7. Worker scaling
    # --------------------------------------------------------

    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["target_rate_eps"],
        summary["average_workers"],
        marker="o",
        label="Average workers"
    )

    plt.plot(
        summary["target_rate_eps"],
        summary["maximum_workers"],
        marker="x",
        linestyle="--",
        label="Maximum workers"
    )

    plt.xlabel(
        "Offered Load (events/sec)"
    )

    plt.ylabel(
        "Worker Count"
    )

    plt.title(
        "NexusFlow Adaptive Worker Scaling"
    )

    plt.xscale("log")

    plt.legend()
    plt.grid(True, alpha=0.3)

    save_plot(
        "worker_scaling.png"
    )

    # --------------------------------------------------------
    # Save corrected summary
    # --------------------------------------------------------

    summary.to_csv(
        OUTPUT / "corrected_summary.csv",
        index=False
    )

    print("")
    print("==============================================")
    print("NexusFlow Research Visualization")
    print("==============================================")
    print("")
    print(
        "Historical pre-fix 50K run excluded "
        "from corrected integrity statistics."
    )
    print("")
    print("Generated figures:")

    files = [
        "throughput_vs_load.png",
        "throughput_efficiency.png",
        "p50_p95_p99_latency.png",
        "p999_latency.png",
        "maximum_latency.png",
        "throughput_variability.png",
        "worker_scaling.png",
        "corrected_summary.csv",
    ]

    for filename in files:
        print(
            f"  {OUTPUT / filename}"
        )

    print("")
    print("VISUALIZATION COMPLETE")


if __name__ == "__main__":
    main()
