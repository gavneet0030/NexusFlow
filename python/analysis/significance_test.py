from pathlib import Path

import pandas as pd
from scipy.stats import wilcoxon


RESULTS_FILE = Path(
    "benchmarks/results/repeated/repeated_results.csv"
)

OUTPUT_FILE = Path(
    "benchmarks/results/repeated/significance_tests.csv"
)


def main():
    if not RESULTS_FILE.exists():
        print(f"ERROR: Results file not found: {RESULTS_FILE}")
        return

    df = pd.read_csv(RESULTS_FILE)

    required_columns = {
        "mode",
        "throughput_eps",
    }

    missing = required_columns - set(df.columns)

    if missing:
        print(f"ERROR: Missing columns: {sorted(missing)}")
        return

    adaptive = (
        df[df["mode"] == "ADAPTIVE"]
        .sort_values("run")
        ["throughput_eps"]
        .reset_index(drop=True)
    )

    baselines = [
        "FIXED_SINGLE",
        "FIXED_BATCH_32",
        "FIXED_PARALLEL_8",
    ]

    results = []

    print("=" * 100)
    print("NexusFlow Paired Statistical Significance Analysis")
    print("=" * 100)
    print()

    for baseline_name in baselines:
        baseline = (
            df[df["mode"] == baseline_name]
            .sort_values("run")
            ["throughput_eps"]
            .reset_index(drop=True)
        )

        if len(adaptive) != len(baseline):
            print(
                f"ERROR: Run count mismatch for "
                f"ADAPTIVE vs {baseline_name}"
            )
            return

        differences = adaptive - baseline

        statistic, p_value = wilcoxon(
            adaptive,
            baseline,
            alternative="greater",
            zero_method="wilcox",
        )

        median_difference = differences.median()
        mean_difference = differences.mean()

        wins = int((differences > 0).sum())
        losses = int((differences < 0).sum())
        ties = int((differences == 0).sum())

        significant = p_value < 0.05

        print(f"ADAPTIVE vs {baseline_name}")
        print("-" * 60)
        print(f"Mean throughput difference:   {mean_difference:.2f} EPS")
        print(f"Median throughput difference: {median_difference:.2f} EPS")
        print(f"Adaptive wins:                {wins}")
        print(f"Adaptive losses:              {losses}")
        print(f"Ties:                         {ties}")
        print(f"Wilcoxon statistic:           {statistic:.4f}")
        print(f"One-sided p-value:            {p_value:.6f}")
        print(
            "Statistically significant:    "
            + ("YES" if significant else "NO")
        )
        print()

        results.append(
            {
                "comparison": f"ADAPTIVE_vs_{baseline_name}",
                "runs": len(adaptive),
                "mean_difference_eps": mean_difference,
                "median_difference_eps": median_difference,
                "adaptive_wins": wins,
                "adaptive_losses": losses,
                "ties": ties,
                "wilcoxon_statistic": statistic,
                "p_value_one_sided": p_value,
                "alpha": 0.05,
                "statistically_significant": significant,
            }
        )

    output_df = pd.DataFrame(results)

    OUTPUT_FILE.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    output_df.to_csv(
        OUTPUT_FILE,
        index=False,
    )

    print("=" * 100)
    print(f"Results written to: {OUTPUT_FILE}")
    print("=" * 100)


if __name__ == "__main__":
    main()
