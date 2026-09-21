from pathlib import Path


REPORT_FILE = Path(
    "benchmarks/results/repeated/experiment_report.md"
)


def main():
    if not REPORT_FILE.exists():
        print(f"ERROR: Report not found: {REPORT_FILE}")
        return

    report = REPORT_FILE.read_text(
        encoding="utf-8"
    )

    old_interpretation = """## Interpretation

The adaptive scheduler achieved the highest mean throughput across the evaluated modes.

Adaptive mean throughput was 42479.2 events/sec.

The results also show a tail-latency trade-off. Adaptive execution does not minimize every latency percentile, particularly P99.9.

Therefore, the current experiment supports the hypothesis that adaptive scheduling can improve throughput, but further SLA-aware tuning is required to control extreme tail latency.
"""

    new_interpretation = """## Statistical Interpretation

The adaptive scheduler achieved the highest mean throughput across the evaluated modes at 42,479.2 events/sec.

Compared with FIXED_SINGLE, adaptive execution improved mean throughput by 50.19%. A paired one-sided Wilcoxon signed-rank test found this improvement to be statistically significant (p = 0.000977, alpha = 0.05), with Adaptive winning all 10 paired runs.

Compared with FIXED_BATCH_32, adaptive execution showed a 4.28% higher mean throughput. However, the difference was not statistically significant across the 10 paired runs (p = 0.246094).

Compared with FIXED_PARALLEL_8, adaptive execution showed a 4.97% higher mean throughput. This difference was also not statistically significant (p = 0.161133).

These results therefore support a statistically significant throughput advantage over fixed single-event processing, while the advantages over the stronger fixed batching and parallel baselines should be described as observed performance improvements rather than statistically established gains.

## Tail-Latency Trade-off

Adaptive execution does not minimize every latency percentile.

FIXED_BATCH_32 achieved the lowest mean P99.9 latency at 177.40 us, compared with 604.92 us for ADAPTIVE.

However, ADAPTIVE reduced mean P99.9 latency by 29.26% relative to FIXED_PARALLEL_8, while also achieving higher mean throughput.

This demonstrates a measurable throughput-versus-tail-latency trade-off rather than universal dominance by a single scheduling strategy.

Extreme tail latency remains an optimization target for future scheduler improvements.
"""

    if old_interpretation not in report:
        print(
            "ERROR: Expected interpretation section was not found."
        )
        print(
            "The report was not modified."
        )
        return

    report = report.replace(
        old_interpretation,
        new_interpretation,
        1
    )

    significance_section = """## Formal Significance Tests

| Comparison | Mean Throughput Difference | Adaptive Wins | Adaptive Losses | One-sided p-value | Significant at alpha=0.05 |
|---|---:|---:|---:|---:|---|
| ADAPTIVE vs FIXED_SINGLE | +14195.85 EPS | 10 | 0 | 0.000977 | YES |
| ADAPTIVE vs FIXED_BATCH_32 | +1744.86 EPS | 6 | 4 | 0.246094 | NO |
| ADAPTIVE vs FIXED_PARALLEL_8 | +2009.64 EPS | 7 | 3 | 0.161133 | NO |

The significance analysis uses a paired one-sided Wilcoxon signed-rank test over the 10 repeated runs, testing whether ADAPTIVE has higher throughput than each fixed baseline.

"""

    marker = "## Reproducibility"

    if "## Formal Significance Tests" not in report:
        if marker not in report:
            print(
                "ERROR: Reproducibility section was not found."
            )
            return

        report = report.replace(
            marker,
            significance_section + marker,
            1
        )

    REPORT_FILE.write_text(
        report,
        encoding="utf-8"
    )

    print("=" * 80)
    print("Research experiment report updated successfully.")
    print("=" * 80)
    print(f"Report: {REPORT_FILE}")
    print("Statistical interpretation: UPDATED")
    print("Formal significance tests: ADDED")
    print("=" * 80)


if __name__ == "__main__":
    main()
