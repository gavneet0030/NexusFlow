from pathlib import Path
import pandas as pd


INPUT = Path("benchmarks/results/scheduler_thresholds.csv")
OUTPUT_DIR = Path("benchmarks/results/threshold_analysis")

OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def main():
    if not INPUT.exists():
        print(f"ERROR: Input file not found: {INPUT}")
        return 1

    df = pd.read_csv(INPUT)

    required_columns = {
        "case_id",
        "micro_batch_size",
        "sla_us",
        "soft_queue_limit",
        "scenario",
        "queue_depth",
        "arrival_rate_eps",
        "mode",
        "decision_batch_size",
        "target_workers",
        "sla_bypass",
    }

    missing = required_columns - set(df.columns)

    if missing:
        print(f"ERROR: Missing columns: {sorted(missing)}")
        return 1

    expected_rows = 256

    if len(df) != expected_rows:
        print(
            f"ERROR: Expected {expected_rows} rows, "
            f"found {len(df)}."
        )
        return 1

    print("============================================")
    print("NexusFlow Scheduler Threshold Analysis")
    print("============================================")

    print(f"Input rows: {len(df)}")
    print(f"Configurations: {df['case_id'].nunique()}")
    print(f"Scenarios: {df['scenario'].nunique()}")

    summary = (
        df.groupby(
            [
                "case_id",
                "micro_batch_size",
                "sla_us",
                "soft_queue_limit",
            ]
        )
        .agg(
            scenarios=("scenario", "count"),
            unique_modes=("mode", "nunique"),
            parallel_decisions=(
                "mode",
                lambda x: (x == "PARALLEL").sum(),
            ),
            micro_batch_decisions=(
                "mode",
                lambda x: (x == "MICRO_BATCH").sum(),
            ),
            single_decisions=(
                "mode",
                lambda x: (x == "SINGLE").sum(),
            ),
            average_workers=("target_workers", "mean"),
            max_workers=("target_workers", "max"),
            sla_bypass_decisions=("sla_bypass", "sum"),
        )
        .reset_index()
    )

    # Prefer configurations that demonstrate all three
    # execution modes across the controlled scenarios.
    summary["mode_coverage_score"] = (
        summary["unique_modes"] / 3.0
    )

    # Reward adaptive mode diversity while penalizing
    # configurations that bypass batching too frequently.
    summary["selection_score"] = (
        summary["mode_coverage_score"] * 100.0
        + summary["parallel_decisions"] * 5.0
        + summary["micro_batch_decisions"] * 2.0
        - summary["sla_bypass_decisions"] * 3.0
    )

    ranking = summary.sort_values(
        [
            "selection_score",
            "unique_modes",
            "parallel_decisions",
        ],
        ascending=False,
    ).reset_index(drop=True)

    ranking.insert(0, "rank", range(1, len(ranking) + 1))

    ranking_path = (
        OUTPUT_DIR / "scheduler_configuration_ranking.csv"
    )

    ranking.to_csv(
        ranking_path,
        index=False,
    )

    mode_summary = (
        df.groupby(
            [
                "micro_batch_size",
                "sla_us",
                "soft_queue_limit",
                "mode",
            ]
        )
        .size()
        .reset_index(name="decision_count")
    )

    mode_summary_path = (
        OUTPUT_DIR / "mode_decision_summary.csv"
    )

    mode_summary.to_csv(
        mode_summary_path,
        index=False,
    )

    scenario_summary = (
        df.groupby(
            [
                "scenario",
                "mode",
            ]
        )
        .size()
        .reset_index(name="decision_count")
    )

    scenario_summary_path = (
        OUTPUT_DIR / "scenario_mode_summary.csv"
    )

    scenario_summary.to_csv(
        scenario_summary_path,
        index=False,
    )

    best = ranking.iloc[0]

    best_path = OUTPUT_DIR / "best_scheduler_configuration.txt"

    with best_path.open("w", encoding="utf-8") as file:
        file.write(
            "NexusFlow Best Scheduler Configuration Candidate\n"
        )
        file.write("================================================\n")
        file.write(
            f"Rank: {int(best['rank'])}\n"
        )
        file.write(
            f"Case ID: {int(best['case_id'])}\n"
        )
        file.write(
            f"Micro-batch size: "
            f"{int(best['micro_batch_size'])}\n"
        )
        file.write(
            f"SLA budget: {int(best['sla_us'])} us\n"
        )
        file.write(
            f"Soft queue limit: "
            f"{int(best['soft_queue_limit'])}\n"
        )
        file.write(
            f"Mode coverage: "
            f"{int(best['unique_modes'])}/3\n"
        )
        file.write(
            f"Selection score: "
            f"{best['selection_score']:.2f}\n"
        )

    print("")
    print("Top 10 configurations:")
    print(
        ranking[
            [
                "rank",
                "case_id",
                "micro_batch_size",
                "sla_us",
                "soft_queue_limit",
                "unique_modes",
                "parallel_decisions",
                "micro_batch_decisions",
                "single_decisions",
                "selection_score",
            ]
        ]
        .head(10)
        .to_string(index=False)
    )

    print("")
    print("============================================")
    print("Analysis completed successfully.")
    print("============================================")

    print(f"Ranking: {ranking_path}")
    print(f"Mode summary: {mode_summary_path}")
    print(f"Scenario summary: {scenario_summary_path}")
    print(f"Best candidate: {best_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
