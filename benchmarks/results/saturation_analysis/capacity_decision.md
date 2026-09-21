# NexusFlow Capacity Decision

## Validated Experimental Criteria

- P99 latency <= 100 us
- P99.9 latency <= 1000 us
- Throughput efficiency >= 80%
- Throughput CV <= 5%

## Final Capacity Classification

| Offered Load | Throughput | Efficiency | P99 | P99.9 | CV | Classification |
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

## Final Decision

**Recommended operating point: 30,000 EPS.**

**Maximum measured mean throughput: 32,759.22 EPS at 45,000 EPS offered load.**

Throughput efficiency first falls below 80% at approximately **35,000 EPS**.

Throughput variability first exceeds 5% CV at approximately **20,000 EPS**.

## Interpretation

The recommended operating point is intentionally different from the maximum throughput point. Maximum throughput represents the highest observed processing rate, whereas the recommended operating point requires simultaneous compliance with latency, efficiency, and stability criteria.

This distinction prevents the benchmark from presenting peak throughput as a production-safe operating capacity.
