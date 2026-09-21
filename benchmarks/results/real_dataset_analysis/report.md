# NexusFlow Real Dataset Benchmark Analysis

Dataset: UCI Online Retail II
Benchmark configurations: 3

## Summary

| Target EPS | Actual EPS | Accuracy | p50 us | p95 us | p99 us | p99.9 us | Max us | Workers | Integrity |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|:---|
| 1000 | 997.86 | 99.79% | 12.85 | 18.20 | 67.60 | 780.57 | 27216.20 | 1 | PASS |
| 10000 | 9788.49 | 97.88% | 14.80 | 17.60 | 52.50 | 282.71 | 1959.60 | 1 | PASS |
| 50000 | 23115.97 | 46.23% | 12.30 | 16.40 | 45.20 | 461.82 | 31216.90 | 16 | FAIL |

## Best Throughput

- Target: 50000 EPS
- Actual: 23115.97 EPS
- Accuracy: 46.23%

## Best p99 Latency

- Target: 50000 EPS
- p99: 45.20 us

## Best p99.9 Latency

- Target: 10000 EPS
- p99.9: 282.71 us

## Highest Maximum Latency

- Target: 50000 EPS
- Maximum latency: 31216.90 us

## Integrity

1 benchmark configurations failed integrity.

## Interpretation

The benchmark evaluates the real historical event stream through the NexusFlow adaptive processing pipeline.

Throughput should be interpreted together with tail latency. A higher throughput result is not automatically better if p99 or p99.9 latency increases substantially.

The current experiment is a baseline matrix. Repeated runs are required before making statistical claims about adaptive scheduling.
