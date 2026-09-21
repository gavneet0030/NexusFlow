# NexusFlow Saturation Experiment

## Objective

Measure how NexusFlow behaves as the offered event arrival rate increases and identify:

1. The saturation knee
2. The maximum observed throughput
3. The recommended operating point
4. The onset of throughput degradation
5. The onset of run-to-run instability

## Dataset

UCI Online Retail II converted into timestamped NexusFlow events.

The dataset is replayed through the NexusFlow event pipeline rather than processed directly as a static CSV workload.

## Workload

| Parameter | Value |
|---|---:|
| Events per run | 10,000 |
| Offered rates | 10K–50K EPS |
| Load points | 9 |
| Repetitions | 3 |
| Total runs | 27 |
| Total events | 270,000 |
| Maximum workers | 16 |
| Scheduler | Adaptive |
| Integrity | PASS |

## SLA Criteria

| Metric | Threshold |
|---|---:|
| Mean P99 latency | <= 100 us |
| Mean P99.9 latency | <= 1,000 us |
| Throughput efficiency | >= 80% |
| Throughput CV | <= 5% |

A load point is classified as `RECOMMENDED` only when all four criteria are satisfied.

## Results

| Offered Load | Mean Throughput | Efficiency | P99 | P99.9 | CV | Classification |
|---:|---:|---:|---:|---:|---:|:---|
| 10,000 | 9,808.58 | 98.09% | 22.50 us | 55.64 us | 0.44% | RECOMMENDED |
| 15,000 | 14,623.33 | 97.49% | 21.83 us | 86.27 us | 1.09% | RECOMMENDED |
| 20,000 | 17,963.65 | 89.82% | 48.04 us | 516.96 us | 12.62% | DEGRADED |
| 25,000 | 20,367.20 | 81.47% | 53.44 us | 940.74 us | 22.45% | DEGRADED |
| 30,000 | 25,272.66 | 84.24% | 55.50 us | 455.84 us | 3.84% | RECOMMENDED |
| 35,000 | 26,186.49 | 74.82% | 52.91 us | 459.81 us | 7.56% | SATURATION |
| 40,000 | 29,942.26 | 74.86% | 21.47 us | 153.28 us | 20.65% | SATURATION |
| 45,000 | 32,759.22 | 72.80% | 22.00 us | 108.34 us | 1.92% | SATURATION |
| 50,000 | 31,623.92 | 63.25% | 16.40 us | 28.40 us | 4.14% | SATURATION |

## Key Findings

### Recommended operating point

**30,000 EPS**

This is the highest tested load that simultaneously satisfies:

- P99 latency requirement
- P99.9 latency requirement
- throughput efficiency requirement
- throughput stability requirement

### Maximum measured throughput

**32,759.22 EPS**

Observed at an offered load of:

**45,000 EPS**

This is a measured peak, not the recommended operating capacity.

### Saturation knee

The analysis identifies approximately:

**20,000 EPS**

as the saturation knee.

Beyond this region, throughput growth becomes less proportional to the increase in offered load and variability increases.

### Efficiency degradation

Throughput efficiency first falls below 80% at:

**35,000 EPS**

### Stability degradation

Throughput coefficient of variation first exceeds 5% at:

**20,000 EPS**

## Engineering Interpretation

NexusFlow initially tracks the offered arrival rate closely.

At higher offered loads, the adaptive worker pool increases concurrency and the backpressure mechanism begins throttling events. The system continues to preserve processing integrity while limiting overload.

The experiment demonstrates why maximum throughput and safe operating capacity are different engineering quantities.

The 45K EPS point produces the highest measured mean throughput, but its efficiency is only 72.80%. Therefore it should not be presented as the recommended production operating point.

The 30K EPS point provides a better balance between throughput, latency, efficiency, and stability.

## Important Limitation

This experiment evaluates the current single-host NexusFlow pipeline.

It does not yet establish:

- distributed Kafka throughput
- multi-node scaling
- network saturation
- cross-process worker scaling
- Kubernetes scaling
- cloud performance
- comparison against external systems

Those belong to subsequent distributed and comparative experiments.

## Reproducibility

Raw results:

`benchmarks/results/saturation_sweep_raw.csv`

Aggregated analysis:

`benchmarks/results/saturation_analysis/plot_summary.csv`

SLA analysis:

`benchmarks/results/saturation_analysis/sla_capacity_analysis.csv`

Capacity decision:

`benchmarks/results/saturation_analysis/capacity_decision.md`

Plots:

`benchmarks/results/saturation_analysis/`

The experiment can be reproduced by rebuilding the benchmark and executing:

`build/Debug/saturation_sweep_benchmark.exe`

## Research Claim

The saturation experiment supports the following claim:

> NexusFlow exhibits a measurable transition from near-linear throughput scaling to a saturation regime as offered event rate increases, while adaptive scheduling and backpressure maintain event-processing integrity under overload.

This claim is limited to the tested workload, hardware, software configuration, and experimental conditions.
