import hashlib
import json
import platform
from datetime import datetime, timezone
from pathlib import Path

RAW = Path("benchmarks/results/saturation_sweep_raw.csv")
SUMMARY = Path("benchmarks/results/saturation_analysis/capacity_summary.csv")
REPORT = Path("benchmarks/results/saturation_analysis/capacity_decision.md")
MANIFEST = Path("benchmarks/results/saturation_analysis/experiment_manifest.json")


def sha256(path):
    digest = hashlib.sha256()

    with open(path, "rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)

    return digest.hexdigest()


manifest = {
    "experiment": {
        "name": "NexusFlow Saturation Sweep",
        "version": "1.0",
        "status": "validated",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
    },

    "workload": {
        "dataset": "UCI Online Retail II",
        "events_per_run": 10000,
        "offered_rates_eps": [
            10000,
            15000,
            20000,
            25000,
            30000,
            35000,
            40000,
            45000,
            50000,
        ],
        "repetitions_per_rate": 3,
        "total_runs": 27,
        "total_events": 270000,
    },

    "system": {
        "scheduler": "AdaptiveScheduler",
        "maximum_workers": 16,
        "queue_capacity": 4096,
        "platform": platform.platform(),
        "python_version": platform.python_version(),
    },

    "sla": {
        "mean_p99_latency_us": 100,
        "mean_p999_latency_us": 1000,
        "minimum_throughput_efficiency_percent": 80,
        "maximum_throughput_cv_percent": 5,
    },

    "validated_results": {
        "integrity_pass_rate_percent": 100,
        "recommended_operating_point_eps": 30000,
        "estimated_saturation_knee_eps": 20000,
        "maximum_mean_throughput_eps": 32759.22,
        "maximum_throughput_offered_load_eps": 45000,
        "efficiency_degradation_begins_eps": 35000,
        "stability_degradation_begins_eps": 20000,
    },

    "artifacts": {
        "raw_results": str(RAW),
        "capacity_summary": str(SUMMARY),
        "capacity_report": str(REPORT),
    },

    "sha256": {
        "raw_results": sha256(RAW),
        "capacity_summary": sha256(SUMMARY),
        "capacity_report": sha256(REPORT),
    },
}

with open(
    MANIFEST,
    "w",
    encoding="utf-8"
) as file:

    json.dump(
        manifest,
        file,
        indent=4
    )

print("Experiment manifest created.")
print("")
print(f"Raw CSV SHA-256: {manifest['sha256']['raw_results']}")
print(
    f"Summary CSV SHA-256: "
    f"{manifest['sha256']['capacity_summary']}"
)
print(
    f"Capacity report SHA-256: "
    f"{manifest['sha256']['capacity_report']}"
)
print("")
print(f"Manifest: {MANIFEST}")
