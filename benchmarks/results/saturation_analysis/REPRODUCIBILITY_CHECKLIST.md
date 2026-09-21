# NexusFlow Experiment Reproducibility Checklist

## Saturation Sweep

### Dataset

- [x] Real dataset replay
- [x] UCI Online Retail II
- [x] Processed event CSV generated
- [x] Timestamped event replay

### Workload

- [x] 10,000 events per run
- [x] 10K EPS
- [x] 15K EPS
- [x] 20K EPS
- [x] 25K EPS
- [x] 30K EPS
- [x] 35K EPS
- [x] 40K EPS
- [x] 45K EPS
- [x] 50K EPS
- [x] Three repetitions per load
- [x] 27 total runs
- [x] 270,000 total events

### Measurements

- [x] Throughput
- [x] Average latency
- [x] P50 latency
- [x] P95 latency
- [x] P99 latency
- [x] P99.9 latency
- [x] Maximum latency
- [x] Worker count
- [x] Queue depth
- [x] Throttling
- [x] Integrity validation

### Statistical Analysis

- [x] Mean throughput
- [x] Throughput standard deviation
- [x] Throughput coefficient of variation
- [x] Throughput efficiency
- [x] Tail-latency analysis
- [x] SLA classification
- [x] Saturation-knee estimation
- [x] Operating-point selection

### Validation

- [x] 27/27 runs completed
- [x] 27/27 integrity PASS
- [x] 27/27 submitted events processed
- [x] 27/27 runs had zero rejected events
- [x] 27/27 runs drained the queue
- [x] No missing metric values
- [x] No negative metric values
- [x] Unique run IDs
- [x] Expected load levels
- [x] Correct repetition count

### Final Results

- Recommended operating point: 30,000 EPS
- Estimated saturation knee: approximately 20,000 EPS
- Maximum measured mean throughput: 32,759.22 EPS
- Maximum-throughput offered load: 45,000 EPS
- Efficiency degradation begins: 35,000 EPS
- Stability degradation begins: 20,000 EPS

### Reproducibility Artifacts

- [x] Raw benchmark CSV
- [x] Aggregated summary
- [x] SLA capacity analysis
- [x] Capacity decision report
- [x] Experiment validation report
- [x] Experiment manifest
- [x] SHA-256 hashes
- [x] Saturation plots
- [x] Experiment documentation

## Artifact Directory

`benchmarks/results/saturation_analysis/`

## Raw Dataset Result

`benchmarks/results/saturation_sweep_raw.csv`

## Main Documentation

`benchmarks/results/saturation_analysis/SATURATION_EXPERIMENT.md`

## Capacity Decision

`benchmarks/results/saturation_analysis/capacity_decision.md`

## Validation

`benchmarks/results/saturation_analysis/experiment_validation.md`

## Reproducibility Manifest

`benchmarks/results/saturation_analysis/experiment_manifest.json`

## Research Status

**SATURATION EXPERIMENT VALIDATED**

The experiment is now frozen as a reproducible benchmark artifact.

Future scheduler changes should create a new experiment identifier and must not overwrite the existing raw results.
