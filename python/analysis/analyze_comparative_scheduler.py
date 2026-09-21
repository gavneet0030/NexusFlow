import pandas as pd
from pathlib import Path

INPUT = Path("benchmarks/results/comparative_scheduler/comparative_raw.csv")
OUTPUT_DIR = Path("benchmarks/results/comparative_scheduler/analysis")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

df = pd.read_csv(INPUT)

df.columns = df.columns.str.strip().str.lower()

# Map actual CSV columns to analysis names
rename_map = {
    "average_processing_latency_us": "average_latency_us",
    "p99_processing_latency_us": "p99_us",
    "p999_processing_latency_us": "p999_us",
    "max_processing_latency_us": "max_us",
}

df = df.rename(columns=rename_map)

required = [
    "mode",
    "target_rate_eps",
    "throughput_eps",
    "average_latency_us",
    "p99_us",
    "p999_us",
    "max_us",
    "workers",
    "throttled",
    "integrity_pass",
]

missing = [c for c in required if c not in df.columns]

if missing:
    print("ERROR: Missing columns:")
    for col in missing:
        print(f"  {col}")
    raise SystemExit(1)

numeric_cols = [
    "target_rate_eps",
    "throughput_eps",
    "average_latency_us",
    "p99_us",
    "p999_us",
    "max_us",
    "workers",
    "throttled",
]

for col in numeric_cols:
    df[col] = pd.to_numeric(df[col], errors="coerce")

df["integrity_pass"] = (
    df["integrity_pass"]
    .astype(str)
    .str.strip()
    .str.upper()
    .map({
        "PASS": 1,
        "TRUE": 1,
        "1": 1,
        "FAIL": 0,
        "FALSE": 0,
        "0": 0,
    })
)

summary = (
    df.groupby(["mode", "target_rate_eps"])
    .agg(
        throughput_mean_eps=("throughput_eps", "mean"),
        throughput_std_eps=("throughput_eps", "std"),
        throughput_cv_percent=(
            "throughput_eps",
            lambda x: (
                x.std(ddof=1) / x.mean() * 100
                if x.mean() != 0
                else 0
            ),
        ),
        average_latency_mean_us=("average_latency_us", "mean"),
        p99_mean_us=("p99_us", "mean"),
        p999_mean_us=("p999_us", "mean"),
        max_latency_mean_us=("max_us", "mean"),
        max_latency_observed_us=("max_us", "max"),
        average_workers=("workers", "mean"),
        total_throttled=("throttled", "sum"),
        integrity_pass_rate_percent=("integrity_pass", "mean"),
        runs=("throughput_eps", "count"),
    )
    .reset_index()
)

summary["integrity_pass_rate_percent"] *= 100

summary["efficiency_percent"] = (
    summary["throughput_mean_eps"]
    / summary["target_rate_eps"]
    * 100
)

summary.to_csv(
    OUTPUT_DIR / "summary.csv",
    index=False
)

# Best mode for every offered load
comparison = []

for rate in sorted(df["target_rate_eps"].unique()):

    subset = summary[
        summary["target_rate_eps"] == rate
    ]

    best_throughput = subset.loc[
        subset["throughput_mean_eps"].idxmax()
    ]

    best_p99 = subset.loc[
        subset["p99_mean_us"].idxmin()
    ]

    best_p999 = subset.loc[
        subset["p999_mean_us"].idxmin()
    ]

    comparison.append({
        "target_rate_eps": rate,
        "best_throughput_mode": best_throughput["mode"],
        "best_throughput_eps": best_throughput["throughput_mean_eps"],
        "best_p99_mode": best_p99["mode"],
        "best_p99_us": best_p99["p99_mean_us"],
        "best_p999_mode": best_p999["mode"],
        "best_p999_us": best_p999["p999_mean_us"],
    })

comparison_df = pd.DataFrame(comparison)

comparison_df.to_csv(
    OUTPUT_DIR / "comparison.csv",
    index=False
)

# Adaptive versus strongest fixed baseline
adaptive = summary[
    summary["mode"] == "ADAPTIVE"
].copy()

fixed = summary[
    summary["mode"] != "ADAPTIVE"
]

baseline = (
    fixed
    .groupby("target_rate_eps")
    .agg(
        best_fixed_throughput_eps=(
            "throughput_mean_eps",
            "max"
        ),
        best_fixed_p99_us=(
            "p99_mean_us",
            "min"
        ),
        best_fixed_p999_us=(
            "p999_mean_us",
            "min"
        ),
    )
    .reset_index()
)

adaptive_comparison = adaptive.merge(
    baseline,
    on="target_rate_eps",
    how="left"
)

adaptive_comparison["throughput_advantage_percent"] = (
    (
        adaptive_comparison["throughput_mean_eps"]
        - adaptive_comparison["best_fixed_throughput_eps"]
    )
    / adaptive_comparison["best_fixed_throughput_eps"]
    * 100
)

adaptive_comparison["p99_change_percent"] = (
    (
        adaptive_comparison["p99_mean_us"]
        - adaptive_comparison["best_fixed_p99_us"]
    )
    / adaptive_comparison["best_fixed_p99_us"]
    * 100
)

adaptive_comparison.to_csv(
    OUTPUT_DIR / "adaptive_vs_best_baseline.csv",
    index=False
)

print("")
print("========================================")
print("COMPARATIVE SCHEDULER ANALYSIS")
print("========================================")
print("")

print(f"Runs analyzed: {len(df)}")
print(f"Modes: {df['mode'].nunique()}")
print(f"Load levels: {df['target_rate_eps'].nunique()}")

print("")
print("MEAN RESULTS")
print("----------------------------------------")

print(
    summary[
        [
            "mode",
            "target_rate_eps",
            "throughput_mean_eps",
            "efficiency_percent",
            "average_latency_mean_us",
            "p99_mean_us",
            "p999_mean_us",
            "max_latency_observed_us",
            "average_workers",
            "total_throttled",
            "integrity_pass_rate_percent",
        ]
    ].to_string(index=False)
)

print("")
print("========================================")
print("BEST MODE BY LOAD")
print("========================================")

print(
    comparison_df.to_string(index=False)
)

print("")
print("========================================")
print("ADAPTIVE VS BEST FIXED BASELINE")
print("========================================")

print(
    adaptive_comparison[
        [
            "target_rate_eps",
            "throughput_mean_eps",
            "best_fixed_throughput_eps",
            "throughput_advantage_percent",
            "p99_mean_us",
            "best_fixed_p99_us",
            "p99_change_percent",
        ]
    ].to_string(index=False)
)

print("")
print("========================================")
print("ANALYSIS COMPLETE")
print("========================================")

print("")
print("Generated files:")
print(f"  {OUTPUT_DIR / 'summary.csv'}")
print(f"  {OUTPUT_DIR / 'comparison.csv'}")
print(f"  {OUTPUT_DIR / 'adaptive_vs_best_baseline.csv'}")
