# NexusFlow Saturation Sweep — Final Findings

## Experimental Configuration

- Real dataset event replay
- 10,000 events per run
- 9 offered-load levels
- 3 repetitions per level
- 27 total runs
- Adaptive scheduling enabled
- Maximum worker pool: 16
- Integrity filtering: PASS runs

## Aggregated Results

| Offered Load | Mean Throughput | Efficiency | P99 | P99.9 | Max Latency | CV | Workers | Throttled |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 10,000 | 9,808.58 | 98.09% | 22.50 us | 55.64 us | 369.40 us | 0.44% | 1.67 | 0.00 |
| 15,000 | 14,623.33 | 97.49% | 21.83 us | 86.27 us | 945.00 us | 1.09% | 2.33 | 0.00 |
| 20,000 | 17,963.65 | 89.82% | 48.04 us | 516.96 us | 49259.60 us | 12.62% | 1.33 | 0.00 |
| 25,000 | 20,367.20 | 81.47% | 53.44 us | 940.74 us | 39023.80 us | 22.45% | 5.67 | 6.00 |
| 30,000 | 25,272.66 | 84.24% | 55.50 us | 455.84 us | 8970.10 us | 3.84% | 5.33 | 83.00 |
| 35,000 | 26,186.49 | 74.82% | 52.91 us | 459.81 us | 30418.60 us | 7.56% | 7.00 | 471.33 |
| 40,000 | 29,942.26 | 74.86% | 21.47 us | 153.28 us | 19390.30 us | 20.65% | 13.33 | 460.33 |
| 45,000 | 32,759.22 | 72.80% | 22.00 us | 108.34 us | 10371.20 us | 1.92% | 8.67 | 2448.00 |
| 50,000 | 31,623.92 | 63.25% | 16.40 us | 28.40 us | 4553.00 us | 4.14% | 5.33 | 3740.67 |

## Capacity Findings

- Estimated saturation knee: **20,000 EPS**.
- Peak measured mean throughput: **32,759.22 EPS** at **45,000 EPS offered load**.
- Recommended operating point under the configured criteria: **30,000 EPS**.
- Highest load with throughput CV <= 5%: **50,000 EPS**.

## Engineering Interpretation

The throughput curve demonstrates that NexusFlow initially tracks the offered event rate closely. As offered load increases, achieved throughput grows more slowly and eventually approaches a practical processing ceiling. This region represents the saturation regime.

The increasing throttling observed at higher offered loads is consistent with active backpressure rather than data loss, because the recorded runs retained full submission, acceptance, processing, and queue-drain integrity.

Tail latency must be interpreted separately from throughput. P99.9 and maximum latency exhibit larger variability than central latency percentiles at several load levels. This indicates that operating-system scheduling, contention, runtime variability, or transient queueing can dominate rare tail events even when median and P99 latency remain low.

## Research Conclusion

The saturation sweep provides empirical evidence for an adaptive event-processing capacity boundary. Rather than reporting a single maximum throughput number, the experiment characterizes the relationship between offered load, achieved throughput, worker scaling, throttling, and latency tails. This provides a stronger systems-performance characterization for NexusFlow.
