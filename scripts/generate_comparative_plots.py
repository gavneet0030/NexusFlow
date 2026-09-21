import csv
import statistics
from pathlib import Path

import matplotlib.pyplot as plt


ROOT = Path(r"C:\Users\PC\NexusFlow")
INPUT = ROOT / "benchmarks" / "results" / "comparative_scheduler" / "comparative_raw.csv"
OUT = ROOT / "benchmarks" / "results" / "comparative_scheduler" / "plots"

OUT.mkdir(parents=True, exist_ok=True)


MODES = [
    "FIXED_SINGLE",
    "FIXED_BATCH_32",
    "FIXED_PARALLEL_8",
    "ADAPTIVE",
]

LABELS = {
    "FIXED_SINGLE": "Fixed Single",
    "FIXED_BATCH_32": "Fixed Batch 32",
    "FIXED_PARALLEL_8": "Fixed Parallel 8",
    "ADAPTIVE": "Adaptive",
}


def avg(values):
    return statistics.mean(values)


def sd(values):
    return statistics.stdev(values) if len(values) > 1 else 0.0


def load_data():
    with INPUT.open("r", encoding="utf-8-sig", newline="") as f:
        rows = list(csv.DictReader(f))

    if not rows:
        raise RuntimeError("comparative_raw.csv contains no rows.")

    required = [
        "mode",
        "target_rate_eps",
        "throughput_eps",
        "p99_processing_latency_us",
        "p999_processing_latency_us",
        "max_processing_latency_us",
        "workers",
        "throttled",
    ]

    missing = [c for c in required if c not in rows[0]]

    if missing:
        raise RuntimeError(
            "Missing required columns: " + ", ".join(missing)
        )

    return rows


def aggregate(rows):
    groups = {}

    for row in rows:
        mode = row["mode"]
        rate = int(float(row["target_rate_eps"]))

        key = (mode, rate)

        if key not in groups:
            groups[key] = {
                "throughput": [],
                "p99": [],
                "p999": [],
                "max_latency": [],
                "workers": [],
                "throttled": [],
            }

        groups[key]["throughput"].append(
            float(row["throughput_eps"])
        )

        groups[key]["p99"].append(
            float(row["p99_processing_latency_us"])
        )

        groups[key]["p999"].append(
            float(row["p999_processing_latency_us"])
        )

        groups[key]["max_latency"].append(
            float(row["max_processing_latency_us"])
        )

        groups[key]["workers"].append(
            float(row["workers"])
        )

        groups[key]["throttled"].append(
            float(row["throttled"])
        )

    result = {}

    for key, values in groups.items():
        result[key] = {
            "throughput_mean": avg(values["throughput"]),
            "throughput_std": sd(values["throughput"]),

            "p99_mean": avg(values["p99"]),
            "p99_std": sd(values["p99"]),

            "p999_mean": avg(values["p999"]),
            "p999_std": sd(values["p999"]),

            "max_latency_mean": avg(values["max_latency"]),
            "max_latency_std": sd(values["max_latency"]),

            "workers_mean": avg(values["workers"]),
            "workers_std": sd(values["workers"]),

            "throttled_mean": avg(values["throttled"]),
            "throttled_std": sd(values["throttled"]),
        }

    return result


def get_points(data, mode, value_key, std_key):
    points = []

    for (current_mode, rate), values in data.items():
        if current_mode == mode:
            points.append(
                (
                    rate,
                    values[value_key],
                    values[std_key],
                )
            )

    return sorted(points)


def plot_metric(
    data,
    filename,
    title,
    ylabel,
    value_key,
    std_key,
):
    plt.figure(figsize=(10, 6))

    for mode in MODES:
        points = get_points(
            data,
            mode,
            value_key,
            std_key,
        )

        if not points:
            continue

        x = [p[0] for p in points]
        y = [p[1] for p in points]
        error = [p[2] for p in points]

        plt.errorbar(
            x,
            y,
            yerr=error,
            marker="o",
            linewidth=2,
            capsize=4,
            label=LABELS[mode],
        )

    plt.title(title)
    plt.xlabel("Offered Load (events/s)")
    plt.ylabel(ylabel)
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    plt.savefig(
        OUT / filename,
        dpi=200,
        bbox_inches="tight",
    )

    plt.close()


def write_aggregate_csv(data):
    output = (
        ROOT
        / "benchmarks"
        / "results"
        / "comparative_scheduler"
        / "comparative_aggregate.csv"
    )

    fields = [
        "mode",
        "target_rate_eps",
        "throughput_mean",
        "throughput_std",
        "p99_mean",
        "p99_std",
        "p999_mean",
        "p999_std",
        "max_latency_mean",
        "max_latency_std",
        "workers_mean",
        "workers_std",
        "throttled_mean",
        "throttled_std",
    ]

    with output.open(
        "w",
        encoding="utf-8",
        newline="",
    ) as f:
        writer = csv.DictWriter(
            f,
            fieldnames=fields,
        )

        writer.writeheader()

        for (mode, rate), values in sorted(
            data.items(),
            key=lambda item: (
                MODES.index(item[0][0]),
                item[0][1],
            ),
        ):
            row = {
                "mode": mode,
                "target_rate_eps": rate,
            }

            row.update(values)
            writer.writerow(row)

    return output


def combined_profile(data):
    fig, axes = plt.subplots(
        2,
        3,
        figsize=(17, 9),
    )

    charts = [
        (
            "throughput_mean",
            "throughput_std",
            "Throughput",
            "Throughput (events/s)",
        ),
        (
            "p99_mean",
            "p99_std",
            "P99 Processing Latency",
            "P99 Latency (us)",
        ),
        (
            "p999_mean",
            "p999_std",
            "P99.9 Processing Latency",
            "P99.9 Latency (us)",
        ),
        (
            "max_latency_mean",
            "max_latency_std",
            "Maximum Processing Latency",
            "Maximum Latency (us)",
        ),
        (
            "workers_mean",
            "workers_std",
            "Worker Scaling",
            "Workers",
        ),
        (
            "throttled_mean",
            "throttled_std",
            "Backpressure / Throttling",
            "Throttled Events",
        ),
    ]

    for ax, chart in zip(axes.flat, charts):
        value_key = chart[0]
        std_key = chart[1]
        title = chart[2]
        ylabel = chart[3]

        for mode in MODES:
            points = get_points(
                data,
                mode,
                value_key,
                std_key,
            )

            if not points:
                continue

            x = [p[0] for p in points]
            y = [p[1] for p in points]
            error = [p[2] for p in points]

            ax.errorbar(
                x,
                y,
                yerr=error,
                marker="o",
                linewidth=1.8,
                capsize=3,
                label=LABELS[mode],
            )

        ax.set_title(title)
        ax.set_xlabel("Offered Load (events/s)")
        ax.set_ylabel(ylabel)
        ax.grid(True, alpha=0.3)

    handles, labels = axes[0, 0].get_legend_handles_labels()

    fig.legend(
        handles,
        labels,
        loc="upper center",
        ncol=4,
        bbox_to_anchor=(0.5, 0.995),
    )

    fig.suptitle(
        "NexusFlow Comparative Scheduler Profile",
        fontsize=16,
        y=0.96,
    )

    plt.tight_layout(
        rect=[0, 0, 1, 0.93]
    )

    plt.savefig(
        OUT / "nexusflow_comparative_profile.png",
        dpi=200,
        bbox_inches="tight",
    )

    plt.close()


def main():
    print("============================================")
    print("NEXUSFLOW: COMPARATIVE VISUALIZATION")
    print("============================================")
    print()

    rows = load_data()

    print("Loaded rows:", len(rows))

    data = aggregate(rows)

    print("Aggregated conditions:", len(data))
    print()

    write_aggregate_csv(data)

    plot_metric(
        data,
        "comparative_throughput_vs_load.png",
        "Throughput vs Offered Load",
        "Throughput (events/s)",
        "throughput_mean",
        "throughput_std",
    )

    plot_metric(
        data,
        "comparative_p99_latency_vs_load.png",
        "P99 Processing Latency vs Offered Load",
        "P99 Processing Latency (us)",
        "p99_mean",
        "p99_std",
    )

    plot_metric(
        data,
        "comparative_p999_latency_vs_load.png",
        "P99.9 Processing Latency vs Offered Load",
        "P99.9 Processing Latency (us)",
        "p999_mean",
        "p999_std",
    )

    plot_metric(
        data,
        "comparative_max_latency_vs_load.png",
        "Maximum Processing Latency vs Offered Load",
        "Maximum Processing Latency (us)",
        "max_latency_mean",
        "max_latency_std",
    )

    plot_metric(
        data,
        "comparative_worker_scaling_vs_load.png",
        "Worker Scaling vs Offered Load",
        "Worker Count",
        "workers_mean",
        "workers_std",
    )

    plot_metric(
        data,
        "comparative_throttling_vs_load.png",
        "Throttling vs Offered Load",
        "Throttled Events",
        "throttled_mean",
        "throttled_std",
    )

    combined_profile(data)

    expected = [
        "comparative_throughput_vs_load.png",
        "comparative_p99_latency_vs_load.png",
        "comparative_p999_latency_vs_load.png",
        "comparative_max_latency_vs_load.png",
        "comparative_worker_scaling_vs_load.png",
        "comparative_throttling_vs_load.png",
        "nexusflow_comparative_profile.png",
    ]

    missing = [
        name
        for name in expected
        if not (OUT / name).exists()
    ]

    print()

    if missing:
        print("COMPARATIVE VISUALIZATION: FAIL")
        print("Missing:")
        for name in missing:
            print("  " + name)
        raise RuntimeError("Visualization generation failed.")

    print("COMPARATIVE VISUALIZATION: PASS")
    print()
    print("Generated:")

    for name in expected:
        print("  " + name)

    print()
    print("Output directory:")
    print(str(OUT))


if __name__ == "__main__":
    main()
