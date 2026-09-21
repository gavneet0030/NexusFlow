# NexusFlow Comparative Scheduler Dashboard

## Experiment Status

| Metric | Value |
|---|---:|
| Total benchmark runs | 60 |
| Scheduling modes | 4 |
| Offered-load levels | 5 |
| Repetitions per configuration | 3 |
| Events per run | 10,000 |
| Total processed events | 600,000 |
| Integrity pass rate | 100% |

## Best Throughput by Load

|   target_rate_eps | best_throughput_mode   |   best_throughput_eps |   best_efficiency_percent |   best_p99_us |   best_p999_us |
|------------------:|:-----------------------|----------------------:|--------------------------:|--------------:|---------------:|
|             10000 | FIXED_PARALLEL_8       |               9917.49 |                   99.1749 |       25.235  |       180.605  |
|             20000 | ADAPTIVE               |              19444.2  |                   97.2211 |       25.6687 |        93.2773 |
|             30000 | FIXED_PARALLEL_8       |              29318.9  |                   97.7297 |       24.3027 |       162.224  |
|             40000 | FIXED_PARALLEL_8       |              37555.6  |                   93.8891 |       26.869  |       604.981  |
|             50000 | FIXED_PARALLEL_8       |              37149    |                   74.298  |       23.2003 |       173.31   |

## Adaptive Scheduler

- Peak measured adaptive throughput: 31124.94 EPS at 40000 offered EPS.
- Lowest adaptive mean P99: 22.368 us at 10000 offered EPS.
- Lowest adaptive mean P99.9: 92.342 us at 10000 offered EPS.

## Statistical Testing

| Metric | Significant comparisons | Total comparisons |
|---|---:|---:|
| Throughput | 0 | 15 |
| P99 | 0 | 15 |
| P99.9 | 0 | 15 |

## Research Interpretation

The experiment demonstrates that no single scheduling policy dominates every workload. FIXED_PARALLEL_8 provides the strongest high-load throughput, while ADAPTIVE provides competitive throughput and strong processing-latency behavior at lower loads while dynamically changing worker count.

The Wilcoxon tests do not show statistically significant differences at alpha = 0.05 for the current three-repetition sample. This should be interpreted as insufficient evidence for a statistically significant difference, not evidence of equivalence.

The next research optimization target is therefore high-load adaptive scheduling: improve worker-target selection and overload control so that ADAPTIVE approaches FIXED_PARALLEL_8 throughput while preserving its latency characteristics.

## Important Limitation

The current latency measurement represents processing latency inside the event processor. It does not measure complete end-to-end latency including queue waiting time.

The current statistical sample contains only three paired repetitions per configuration. More repetitions should be used for a stronger statistical conclusion.
