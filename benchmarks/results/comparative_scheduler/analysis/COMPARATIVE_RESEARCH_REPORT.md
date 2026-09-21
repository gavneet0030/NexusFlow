# NexusFlow Comparative Scheduler Experiment

## Experiment Overview

This experiment compares four scheduling strategies under identical workloads:

- FIXED_SINGLE
- FIXED_BATCH_32
- FIXED_PARALLEL_8
- ADAPTIVE

The experiment evaluates throughput, processing latency, tail latency, throughput efficiency, worker scaling, throttling, and integrity.

## Experimental Configuration

| Parameter | Value |
|---|---:|
| Scheduler modes | 4 |
| Offered loads | 10K, 20K, 30K, 40K, 50K EPS |
| Repetitions | 3 per mode/load |
| Events per run | 10,000 |
| Total runs | 60 |
| Total events | 600,000 |

## Aggregate Results

| Mode             |   Load EPS |   Mean Throughput EPS |   Efficiency % |   Avg Latency us |   P99 us |   P99.9 us |   Max Latency us |   Avg Workers |   Throttled |
|:-----------------|-----------:|----------------------:|---------------:|-----------------:|---------:|-----------:|-----------------:|--------------:|------------:|
| ADAPTIVE         |      10000 |               9813.57 |        98.1357 |          14.9998 |  22.3677 |    92.3422 |           1646.9 |       2       |           0 |
| ADAPTIVE         |      20000 |              19444.2  |        97.2211 |          15.1424 |  25.6687 |    93.2773 |            860.8 |       2.66667 |           0 |
| ADAPTIVE         |      30000 |              27789.2  |        92.6306 |          15.3499 |  26.5007 |   133.304  |           7114.7 |       6.66667 |           0 |
| ADAPTIVE         |      40000 |              31124.9  |        77.8124 |          14.369  |  27.07   |   142.638  |           1488.7 |       6.66667 |        4634 |
| ADAPTIVE         |      50000 |              30789.1  |        61.5781 |          15.269  |  38.3007 |   269.648  |           4951.2 |       6.66667 |        4738 |
| FIXED_BATCH_32   |      10000 |               9826.82 |        98.2682 |          15.2143 |  41.7017 |   209.943  |           1363   |       1       |           0 |
| FIXED_BATCH_32   |      20000 |              19269.5  |        96.3475 |          15.1815 |  42.1713 |   205.513  |           1193   |       1       |           0 |
| FIXED_BATCH_32   |      30000 |              28050.5  |        93.5015 |          15.5331 |  31.267  |   175.346  |           2099   |       1       |           0 |
| FIXED_BATCH_32   |      40000 |              33553.8  |        83.8845 |          15.1117 |  25.6013 |   145.87   |            961.2 |       1       |           0 |
| FIXED_BATCH_32   |      50000 |              34450.1  |        68.9003 |          14.9858 |  23.834  |   114.455  |           2832.6 |       1       |         178 |
| FIXED_PARALLEL_8 |      10000 |               9917.49 |        99.1749 |          13.5186 |  25.235  |   180.605  |           1901.6 |       8       |           0 |
| FIXED_PARALLEL_8 |      20000 |              19265.9  |        96.3296 |          13.8949 |  27.4017 |   223.701  |           3147.3 |       8       |           0 |
| FIXED_PARALLEL_8 |      30000 |              29318.9  |        97.7297 |          13.6221 |  24.3027 |   162.224  |           3292.2 |       8       |           0 |
| FIXED_PARALLEL_8 |      40000 |              37555.6  |        93.8891 |          15.7676 |  26.869  |   604.981  |          16006.9 |       8       |           0 |
| FIXED_PARALLEL_8 |      50000 |              37149    |        74.298  |          13.9982 |  23.2003 |   173.31   |           5870.5 |       8       |           0 |
| FIXED_SINGLE     |      10000 |               9833.05 |        98.3305 |          16.9299 |  65.8443 |   618.458  |           5359.4 |       1       |           0 |
| FIXED_SINGLE     |      20000 |              19178.5  |        95.8924 |          16.5666 |  42.309  |   381.548  |           6481   |       1       |           0 |
| FIXED_SINGLE     |      30000 |              25962.6  |        86.5421 |          15.457  |  23.6013 |   125.92   |           6245.8 |       1       |           0 |
| FIXED_SINGLE     |      40000 |              24517.5  |        61.2937 |          15.2122 |  33.9357 |   212.327  |          20675.7 |       1       |        2239 |
| FIXED_SINGLE     |      50000 |              25779.3  |        51.5586 |          15.1321 |  26.801  |   124.935  |           1589.6 |       1       |        7005 |

## Best Strategy by Offered Load

|   target_rate_eps | best_throughput_mode   |   best_throughput_eps | best_p99_mode    |   best_p99_us | best_p999_mode   |   best_p999_us |
|------------------:|:-----------------------|----------------------:|:-----------------|--------------:|:-----------------|---------------:|
|             10000 | FIXED_PARALLEL_8       |               9917.49 | ADAPTIVE         |       22.3677 | ADAPTIVE         |        92.3422 |
|             20000 | ADAPTIVE               |              19444.2  | ADAPTIVE         |       25.6687 | ADAPTIVE         |        93.2773 |
|             30000 | FIXED_PARALLEL_8       |              29318.9  | FIXED_SINGLE     |       23.6013 | FIXED_SINGLE     |       125.92   |
|             40000 | FIXED_PARALLEL_8       |              37555.6  | FIXED_BATCH_32   |       25.6013 | ADAPTIVE         |       142.638  |
|             50000 | FIXED_PARALLEL_8       |              37149    | FIXED_PARALLEL_8 |       23.2003 | FIXED_BATCH_32   |       114.455  |

## Adaptive Strategy vs Best Fixed Baseline

|   target_rate_eps |   throughput_mean_eps |   best_fixed_throughput_eps |   throughput_advantage_percent |   p99_mean_us |   best_fixed_p99_us |   p99_change_percent |
|------------------:|----------------------:|----------------------------:|-------------------------------:|--------------:|--------------------:|---------------------:|
|             10000 |               9813.57 |                     9917.49 |                      -1.04782  |       22.3677 |             25.235  |            -11.3625  |
|             20000 |              19444.2  |                    19269.5  |                       0.906716 |       25.6687 |             27.4017 |             -6.32443 |
|             30000 |              27789.2  |                    29318.9  |                      -5.21751  |       26.5007 |             23.6013 |             12.2846  |
|             40000 |              31124.9  |                    37555.6  |                     -17.1231   |       27.07   |             25.6013 |              5.73668 |
|             50000 |              30789.1  |                    37149    |                     -17.1201   |       38.3007 |             23.2003 |             65.0867  |

## Key Findings

- Highest measured mean throughput: 37555.64 EPS using FIXED_PARALLEL_8 at 40000 offered EPS.
- Lowest measured mean P99 processing latency: 22.368 us using ADAPTIVE at 10000 offered EPS.
- At 20K EPS, ADAPTIVE achieved 19444.22 EPS, which was 0.91% above the strongest fixed throughput baseline.
- At 10K EPS, ADAPTIVE reduced mean P99 latency by 11.36% relative to the best fixed P99 baseline.
- At 50K EPS, ADAPTIVE throughput was 17.12% below the strongest fixed throughput baseline.

## Interpretation

The results do not support the claim that ADAPTIVE is universally superior. Fixed parallel scheduling produced the highest throughput at several high-load points, while ADAPTIVE produced strong tail-latency results at lower loads and demonstrated dynamic worker scaling.

This is an important research result: the adaptive scheduler shows a latency-oriented trade-off rather than simply maximizing throughput.

At high offered loads, ADAPTIVE experienced throttling and lower throughput than FIXED_PARALLEL_8. This indicates that the current scheduler policy requires further tuning for high-load saturation conditions.

## Important Measurement Note

Processing latency is measured inside the event processor. It should therefore be interpreted as processing latency, not full end-to-end queueing latency.

FIXED_BATCH_32 represents queue-drain batching of up to 32 events. Events are still processed individually; this experiment should not be described as vectorized execution.

## Research Conclusion

The comparative experiment successfully establishes a reproducible baseline across four scheduling policies and five offered-load levels. The results demonstrate that scheduler policy materially affects throughput and tail latency, while also exposing a clear optimization target: improve ADAPTIVE behavior under high-load saturation without sacrificing its low-latency characteristics.
