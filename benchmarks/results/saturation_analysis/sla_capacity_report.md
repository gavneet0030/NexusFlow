# NexusFlow SLA-Aware Capacity Analysis

## SLA Criteria

- Mean P99 latency <= 100 us
- Mean P99.9 latency <= 1000 us
- Throughput efficiency >= 80%
- Throughput CV <= 5%

## Capacity Table

| Offered Load | Throughput | Efficiency | P99 | P99.9 | CV | Throttled | SLA |
|---:|---:|---:|---:|---:|---:|---:|:---:|
| 10,000 | 9,808.58 | 98.09% | 22.50 us | 55.64 us | 0.44% | 0.00 | PASS |
| 15,000 | 14,623.33 | 97.49% | 21.83 us | 86.27 us | 1.09% | 0.00 | PASS |
| 20,000 | 17,963.65 | 89.82% | 48.04 us | 516.96 us | 12.62% | 0.00 | FAIL |
| 25,000 | 20,367.20 | 81.47% | 53.44 us | 940.74 us | 22.45% | 6.00 | FAIL |
| 30,000 | 25,272.66 | 84.24% | 55.50 us | 455.84 us | 3.84% | 83.00 | PASS |
| 35,000 | 26,186.49 | 74.82% | 52.91 us | 459.81 us | 7.56% | 471.33 | FAIL |
| 40,000 | 29,942.26 | 74.86% | 21.47 us | 153.28 us | 20.65% | 460.33 | FAIL |
| 45,000 | 32,759.22 | 72.80% | 22.00 us | 108.34 us | 1.92% | 2448.00 | FAIL |
| 50,000 | 31,623.92 | 63.25% | 16.40 us | 28.40 us | 4.14% | 3740.67 | FAIL |

## Capacity Decision

Highest measured operating point satisfying all configured criteria: **30,000 EPS**.

Estimated saturation knee: **20,000 EPS**.

Maximum mean throughput observed: **32,759.22 EPS** at **45,000 EPS offered load**.

## Interpretation

The SLA-aware analysis separates three different concepts: the saturation knee, the maximum measured throughput, and the recommended operating point. These values should not be treated as interchangeable.

The saturation knee identifies where additional offered load begins producing disproportionately smaller throughput gains. The maximum measured throughput identifies the highest mean throughput observed during this experiment. The recommended operating point additionally requires acceptable tail latency, throughput efficiency, and run-to-run stability.

Throttling is treated as a backpressure mechanism rather than an integrity failure. All runs used for this analysis passed the benchmark integrity criteria.
