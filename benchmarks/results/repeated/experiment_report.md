# NexusFlow Experimental Results

## Experimental Configuration

- Benchmark type: Adaptive vs fixed execution
- Repetitions per mode: 10
- Events per run: 30,000
- Confidence level: 95%

## Throughput Results

| Mode | Mean EPS | Std Dev | 95% CI | CV |
|---|---:|---:|---:|---:|
| FIXED_SINGLE | 28283.4 | 4895.7 | 24781.5 - 31785.3 | 17.31% |
| FIXED_BATCH_32 | 40734.4 | 5349.7 | 36907.7 - 44561.1 | 13.13% |
| FIXED_PARALLEL_8 | 40469.6 | 6785.1 | 35616.2 - 45323.0 | 16.77% |
| ADAPTIVE | 42479.2 | 9199.2 | 35899.0 - 49059.5 | 21.66% |

## Tail Latency

| Mode | P50 (us) | P95 (us) | P99 (us) | P99.9 (us) | Max (us) |
|---|---:|---:|---:|---:|---:|
| FIXED_SINGLE | 11.90 | 14.90 | 36.70 | 249.41 | 17352.80 |
| FIXED_BATCH_32 | 11.80 | 14.50 | 29.70 | 177.40 | 5321.20 |
| FIXED_PARALLEL_8 | 12.00 | 13.90 | 44.00 | 855.15 | 16411.50 |
| ADAPTIVE | 12.00 | 15.30 | 40.20 | 604.92 | 21651.50 |

## Adaptive Throughput Comparison

- ADAPTIVE vs FIXED_SINGLE: 50.19% throughput difference.
- ADAPTIVE vs FIXED_BATCH_32: 4.28% throughput difference.
- ADAPTIVE vs FIXED_PARALLEL_8: 4.97% throughput difference.

## Statistical Interpretation

The adaptive scheduler achieved the highest mean throughput across the evaluated modes at 42,479.2 events/sec.

Compared with FIXED_SINGLE, adaptive execution improved mean throughput by 50.19%. A paired one-sided Wilcoxon signed-rank test found this improvement to be statistically significant (p = 0.000977, alpha = 0.05), with Adaptive winning all 10 paired runs.

Compared with FIXED_BATCH_32, adaptive execution showed a 4.28% higher mean throughput. However, the difference was not statistically significant across the 10 paired runs (p = 0.246094).

Compared with FIXED_PARALLEL_8, adaptive execution showed a 4.97% higher mean throughput. This difference was also not statistically significant (p = 0.161133).

These results therefore support a statistically significant throughput advantage over fixed single-event processing, while the advantages over the stronger fixed batching and parallel baselines should be described as observed performance improvements rather than statistically established gains.

## Tail-Latency Trade-off

Adaptive execution does not minimize every latency percentile.

FIXED_BATCH_32 achieved the lowest mean P99.9 latency at 177.40 us, compared with 604.92 us for ADAPTIVE.

However, ADAPTIVE reduced mean P99.9 latency by 29.26% relative to FIXED_PARALLEL_8, while also achieving higher mean throughput.

This demonstrates a measurable throughput-versus-tail-latency trade-off rather than universal dominance by a single scheduling strategy.

Extreme tail latency remains an optimization target for future scheduler improvements.

## Formal Significance Tests

| Comparison | Mean Throughput Difference | Adaptive Wins | Adaptive Losses | One-sided p-value | Significant at alpha=0.05 |
|---|---:|---:|---:|---:|---|
| ADAPTIVE vs FIXED_SINGLE | +14195.85 EPS | 10 | 0 | 0.000977 | YES |
| ADAPTIVE vs FIXED_BATCH_32 | +1744.86 EPS | 6 | 4 | 0.246094 | NO |
| ADAPTIVE vs FIXED_PARALLEL_8 | +2009.64 EPS | 7 | 3 | 0.161133 | NO |

The significance analysis uses a paired one-sided Wilcoxon signed-rank test over the 10 repeated runs, testing whether ADAPTIVE has higher throughput than each fixed baseline.

## Reproducibility

All modes processed 30,000 events in every recorded run. The experiment was repeated 10 times per mode.
