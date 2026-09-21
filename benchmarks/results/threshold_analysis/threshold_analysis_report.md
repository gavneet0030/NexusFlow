# NexusFlow End-to-End Threshold Analysis

## Experiment Overview

- Configurations analyzed: 64
- Objective: evaluate sensitivity to micro-batch size, SLA budget, and queue threshold.
- Primary performance metric: throughput in events/sec.
- Secondary metric: average end-to-end latency in microseconds.

## Best Throughput Configuration

- Micro-batch size: 30000
- SLA budget: 0 us
- Queue threshold: 443
- Throughput: 476.31 events/sec
- Average latency: 10840.60 us

## Interpretation

The highest-throughput configuration in this sweep used a micro-batch size of 30000, an SLA budget of 0 us, and a queue threshold of 443.

The sweep demonstrates configuration sensitivity, but these results are single benchmark observations per configuration. They should not be interpreted as statistically significant comparisons between configurations without repeated runs.

The current threshold benchmark records average latency rather than p50/p95/p99/p99.9/max latency. Tail-latency conclusions therefore require a follow-up benchmark with percentile instrumentation.

## Output Files

- top_throughput_configs.csv
- top_latency_configs.csv
- batch_size_summary.csv
- sla_summary.csv
- queue_threshold_summary.csv
- parameter_correlation.csv
- pareto_frontier.csv
- throughput_vs_latency.png
- throughput_by_batch_size.png
- throughput_by_sla.png
