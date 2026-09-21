# NexusFlow Real Dataset Repeated Experiment

Repeated end-to-end replay experiment using the processed real-world dataset.

- Total runs analyzed: 20
- Replay rates: 4

## 1,000 EPS

- Runs: 5
- Mean throughput: 997.61 EPS
- Throughput standard deviation: 0.55 EPS
- Throughput CV: 0.05%
- Mean p50 latency: 14.220 us
- Mean p95 latency: 18.260 us
- Mean p99 latency: 74.484 us
- Mean p99.9 latency: 293.230 us
- Mean maximum latency: 5678.740 us
- Observed maximum latency: 14859.000 us
- Average workers: 1.00
- Maximum workers: 1
- Maximum queue depth: 0
- Rejected events: 0
- Throttled events: 0
- Integrity pass rate: 100.0%

## 10,000 EPS

- Runs: 5
- Mean throughput: 9864.50 EPS
- Throughput standard deviation: 51.28 EPS
- Throughput CV: 0.52%
- Mean p50 latency: 14.260 us
- Mean p95 latency: 19.400 us
- Mean p99 latency: 60.023 us
- Mean p99.9 latency: 511.738 us
- Mean maximum latency: 2132.680 us
- Observed maximum latency: 3060.000 us
- Average workers: 1.80
- Maximum workers: 2
- Maximum queue depth: 0
- Rejected events: 0
- Throttled events: 0
- Integrity pass rate: 100.0%

## 50,000 EPS

- Runs: 5
- Mean throughput: 33449.46 EPS
- Throughput standard deviation: 1250.05 EPS
- Throughput CV: 3.74%
- Mean p50 latency: 12.700 us
- Mean p95 latency: 16.240 us
- Mean p99 latency: 21.380 us
- Mean p99.9 latency: 81.283 us
- Mean maximum latency: 7430.780 us
- Observed maximum latency: 15560.900 us
- Average workers: 7.00
- Maximum workers: 16
- Maximum queue depth: 0
- Rejected events: 0
- Throttled events: 13378
- Integrity pass rate: 80.0%

## 100,000 EPS

- Runs: 5
- Mean throughput: 34217.85 EPS
- Throughput standard deviation: 1374.52 EPS
- Throughput CV: 4.02%
- Mean p50 latency: 13.000 us
- Mean p95 latency: 16.200 us
- Mean p99 latency: 20.501 us
- Mean p99.9 latency: 55.032 us
- Mean maximum latency: 15688.020 us
- Observed maximum latency: 27034.000 us
- Average workers: 8.40
- Maximum workers: 16
- Maximum queue depth: 0
- Rejected events: 0
- Throttled events: 19517
- Integrity pass rate: 100.0%

## Statistical Note

The 95% confidence intervals use a normal approximation. Five repetitions are available for each target rate. Future experiments should use additional repetitions and Student's t confidence intervals for stronger statistical rigor.