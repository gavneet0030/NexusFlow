# NexusFlow

Ultra-Low-Latency Intelligent Event Processing Platform.

## Vision

NexusFlow is a high-performance event processing engine designed to
process high-volume event streams while maintaining predictable tail latency.

## Core Idea

Adaptive latency-aware event processing.

The engine dynamically chooses between:

- Single-event processing
- Micro-batching
- Parallel processing

based on:

- Event arrival rate
- Queue depth
- CPU availability
- Event priority
- Latency SLA

## Technology

- C++20
- Python
- Kafka / Redpanda
- Redis
- PostgreSQL
- gRPC
- Docker
- Prometheus
- OpenTelemetry
- GitHub Actions

## Status

?? Under active development.

## Saturation Experiment

NexusFlow has been evaluated using a controlled real-dataset replay workload across 10K–50K events/sec with three repetitions per load point.

Key measured results:

- Recommended operating point: 30K EPS
- Maximum measured mean throughput: 32.76K EPS
- Maximum-throughput offered load: 45K EPS
- Estimated saturation knee: approximately 20K EPS
- 27/27 benchmark runs passed integrity validation

Detailed methodology and results:

`benchmarks/results/saturation_analysis/SATURATION_EXPERIMENT.md`

## Reproducibility

The saturation experiment has been validated with 27 runs across nine offered-load levels. Raw measurements, statistical analysis, SLA classification, validation results, plots, and a SHA-256 experiment manifest are preserved under:

`benchmarks/results/saturation_analysis/`

Reproducibility checklist:

`benchmarks/results/saturation_analysis/REPRODUCIBILITY_CHECKLIST.md`
