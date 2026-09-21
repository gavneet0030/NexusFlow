import pandas as pd
from pathlib import Path

INPUT = Path("benchmarks/results/saturation_sweep_raw.csv")
OUTPUT = Path("benchmarks/results/saturation_analysis/experiment_validation.md")

df = pd.read_csv(INPUT)

expected_rates = [
    10000,
    15000,
    20000,
    25000,
    30000,
    35000,
    40000,
    45000,
    50000,
]

expected_repetitions = 3
expected_runs = len(expected_rates) * expected_repetitions

checks = []

# Check total run count
checks.append(
    (
        "Total run count",
        len(df) == expected_runs,
        f"{len(df)} / {expected_runs}"
    )
)

# Check run IDs
unique_run_ids = df["run_id"].nunique()

checks.append(
    (
        "Unique run IDs",
        unique_run_ids == expected_runs,
        f"{unique_run_ids} unique IDs"
    )
)

# Check expected rates
actual_rates = sorted(
    df["target_rate_eps"].unique().tolist()
)

checks.append(
    (
        "Expected load levels",
        actual_rates == expected_rates,
        f"{actual_rates}"
    )
)

# Check repetitions per rate
repetition_counts = (
    df.groupby("target_rate_eps")
    .size()
)

repetitions_ok = all(
    repetition_counts.get(rate, 0)
    == expected_repetitions
    for rate in expected_rates
)

checks.append(
    (
        "Three repetitions per rate",
        repetitions_ok,
        str(repetition_counts.to_dict())
    )
)

# Integrity
integrity_pass = (
    df["integrity_pass"]
    .astype(str)
    .str.upper()
    .eq("PASS")
)

checks.append(
    (
        "Integrity validation",
        bool(integrity_pass.all()),
        f"{integrity_pass.sum()} / {len(df)} PASS"
    )
)

# Processed events must equal submitted events
processing_complete = (
    df["processed"] == df["submitted"]
)

checks.append(
    (
        "All submitted events processed",
        bool(processing_complete.all()),
        f"{processing_complete.sum()} / {len(df)} PASS"
    )
)

# Rejected events
no_rejections = (
    df["rejected"] == 0
)

checks.append(
    (
        "No rejected events",
        bool(no_rejections.all()),
        f"{no_rejections.sum()} / {len(df)} PASS"
    )
)

# Queue drained
queue_drained = (
    df["queue_depth"] == 0
)

checks.append(
    (
        "Queue drained",
        bool(queue_drained.all()),
        f"{queue_drained.sum()} / {len(df)} PASS"
    )
)

# Numeric validity
metric_columns = [
    "throughput_eps",
    "average_latency_us",
    "p50_us",
    "p95_us",
    "p99_us",
    "p999_us",
    "max_us",
]

numeric_valid = True

for column in metric_columns:
    values = pd.to_numeric(
        df[column],
        errors="coerce"
    )

    if values.isna().any():
        numeric_valid = False

    if (values < 0).any():
        numeric_valid = False

checks.append(
    (
        "Latency and throughput metrics valid",
        numeric_valid,
        "No missing or negative metric values"
    )
)

# Throughput must be positive
positive_throughput = (
    df["throughput_eps"] > 0
)

checks.append(
    (
        "Positive throughput",
        bool(positive_throughput.all()),
        f"{positive_throughput.sum()} / {len(df)} PASS"
    )
)

# Generate report
all_pass = all(
    check[1]
    for check in checks
)

with open(
    OUTPUT,
    "w",
    encoding="utf-8"
) as report:

    report.write(
        "# NexusFlow Saturation Experiment Validation\n\n"
    )

    report.write(
        "## Validation Checks\n\n"
    )

    report.write(
        "| Check | Status | Evidence |\n"
    )

    report.write(
        "|---|:---:|---|\n"
    )

    for name, passed, evidence in checks:

        status = (
            "PASS"
            if passed
            else "FAIL"
        )

        report.write(
            f"| {name} | {status} | {evidence} |\n"
        )

    report.write("\n")

    if all_pass:
        report.write(
            "## Overall Result\n\n"
            "**PASS — the saturation experiment is internally "
            "consistent and reproducible from the recorded CSV.**\n"
        )
    else:
        report.write(
            "## Overall Result\n\n"
            "**FAIL — one or more validation checks require "
            "investigation before using the dataset as a final "
            "research result.**\n"
        )

print("Experiment validation completed.")

for name, passed, evidence in checks:
    status = "PASS" if passed else "FAIL"
    print(f"{status}: {name} — {evidence}")

print("")

if all_pass:
    print("OVERALL: PASS")
else:
    print("OVERALL: FAIL")

print(f"Validation report: {OUTPUT}")
