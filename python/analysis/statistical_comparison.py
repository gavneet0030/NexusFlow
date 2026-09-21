import pandas as pd
from pathlib import Path
from scipy.stats import wilcoxon

BASE = Path("benchmarks/results/comparative_scheduler")
OUTPUT = BASE / "analysis"

df = pd.read_csv(BASE / "comparative_raw.csv")

throughput_results = []
p99_results = []
p999_results = []

fixed_modes = [
    "FIXED_SINGLE",
    "FIXED_BATCH_32",
    "FIXED_PARALLEL_8",
]

adaptive = df[df["mode"] == "ADAPTIVE"].copy()

for mode in fixed_modes:

    fixed = df[df["mode"] == mode].copy()

    merged = adaptive.merge(
        fixed,
        on=["target_rate_eps", "repetition"],
        suffixes=("_adaptive", "_fixed")
    )

    for rate in sorted(merged["target_rate_eps"].unique()):

        subset = merged[
            merged["target_rate_eps"] == rate
        ]

        if len(subset) < 2:
            continue

        adaptive_throughput = subset[
            "throughput_eps_adaptive"
        ].values

        fixed_throughput = subset[
            "throughput_eps_fixed"
        ].values

        adaptive_p99 = subset[
            "p99_processing_latency_us_adaptive"
        ].values

        fixed_p99 = subset[
            "p99_processing_latency_us_fixed"
        ].values

        adaptive_p999 = subset[
            "p999_processing_latency_us_adaptive"
        ].values

        fixed_p999 = subset[
            "p999_processing_latency_us_fixed"
        ].values

        try:
            throughput_stat, throughput_p = wilcoxon(
                adaptive_throughput,
                fixed_throughput,
                alternative="two-sided"
            )

            p99_stat, p99_p = wilcoxon(
                adaptive_p99,
                fixed_p99,
                alternative="two-sided"
            )

            p999_stat, p999_p = wilcoxon(
                adaptive_p999,
                fixed_p999,
                alternative="two-sided"
            )

        except ValueError:
            throughput_stat = throughput_p = float("nan")
            p99_stat = p99_p = float("nan")
            p999_stat = p999_p = float("nan")

        throughput_results.append({
            "mode": mode,
            "target_rate_eps": rate,
            "n_pairs": len(subset),
            "adaptive_mean_throughput_eps": adaptive_throughput.mean(),
            "fixed_mean_throughput_eps": fixed_throughput.mean(),
            "throughput_difference_eps": (
                adaptive_throughput.mean()
                - fixed_throughput.mean()
            ),
            "wilcoxon_statistic": throughput_stat,
            "p_value": throughput_p,
            "significant_alpha_0.05": (
                throughput_p < 0.05
                if pd.notna(throughput_p)
                else False
            ),
        })

        p99_results.append({
            "mode": mode,
            "target_rate_eps": rate,
            "n_pairs": len(subset),
            "adaptive_mean_p99_us": adaptive_p99.mean(),
            "fixed_mean_p99_us": fixed_p99.mean(),
            "p99_difference_us": (
                adaptive_p99.mean()
                - fixed_p99.mean()
            ),
            "wilcoxon_statistic": p99_stat,
            "p_value": p99_p,
            "significant_alpha_0.05": (
                p99_p < 0.05
                if pd.notna(p99_p)
                else False
            ),
        })

        p999_results.append({
            "mode": mode,
            "target_rate_eps": rate,
            "n_pairs": len(subset),
            "adaptive_mean_p999_us": adaptive_p999.mean(),
            "fixed_mean_p999_us": fixed_p999.mean(),
            "p999_difference_us": (
                adaptive_p999.mean()
                - fixed_p999.mean()
            ),
            "wilcoxon_statistic": p999_stat,
            "p_value": p999_p,
            "significant_alpha_0.05": (
                p999_p < 0.05
                if pd.notna(p999_p)
                else False
            ),
        })

throughput_df = pd.DataFrame(throughput_results)
p99_df = pd.DataFrame(p99_results)
p999_df = pd.DataFrame(p999_results)

throughput_df.to_csv(
    OUTPUT / "wilcoxon_throughput.csv",
    index=False
)

p99_df.to_csv(
    OUTPUT / "wilcoxon_p99.csv",
    index=False
)

p999_df.to_csv(
    OUTPUT / "wilcoxon_p999.csv",
    index=False
)

report = []

report.append("# Statistical Significance Analysis")
report.append("")
report.append(
    "Paired Wilcoxon signed-rank tests compare ADAPTIVE against "
    "each fixed scheduling policy at every offered-load level."
)
report.append("")
report.append(
    "The pairing is based on identical target load and repetition "
    "number."
)
report.append("")
report.append("Significance threshold: alpha = 0.05.")
report.append("")

report.append("## Throughput")
report.append("")
report.append(
    throughput_df.to_markdown(index=False)
    if not throughput_df.empty
    else "No throughput comparisons available."
)
report.append("")

report.append("## P99 Processing Latency")
report.append("")
report.append(
    p99_df.to_markdown(index=False)
    if not p99_df.empty
    else "No P99 comparisons available."
)
report.append("")

report.append("## P99.9 Processing Latency")
report.append("")
report.append(
    p999_df.to_markdown(index=False)
    if not p999_df.empty
    else "No P99.9 comparisons available."
)
report.append("")

report.append("## Interpretation")
report.append("")
report.append(
    "A p-value below 0.05 indicates statistically significant "
    "evidence of a difference between ADAPTIVE and the corresponding "
    "fixed policy for that metric and offered-load level."
)
report.append("")
report.append(
    "A non-significant result does not establish that the two "
    "strategies are equivalent; it indicates that this experiment "
    "does not provide sufficient statistical evidence of a difference."
)
report.append("")

output = OUTPUT / "STATISTICAL_SIGNIFICANCE_REPORT.md"

output.write_text(
    "\n".join(report),
    encoding="utf-8"
)

print("")
print("========================================")
print("STATISTICAL SIGNIFICANCE ANALYSIS")
print("========================================")
print("")

print("THROUGHPUT")
print("----------------------------------------")
print(
    throughput_df.to_string(index=False)
    if not throughput_df.empty
    else "No results."
)

print("")
print("P99")
print("----------------------------------------")
print(
    p99_df.to_string(index=False)
    if not p99_df.empty
    else "No results."
)

print("")
print("P99.9")
print("----------------------------------------")
print(
    p999_df.to_string(index=False)
    if not p999_df.empty
    else "No results."
)

print("")
print("========================================")
print("STATISTICAL ANALYSIS COMPLETE")
print("========================================")
print("")
print(output)
