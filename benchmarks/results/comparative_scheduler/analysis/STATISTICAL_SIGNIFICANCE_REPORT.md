# Statistical Significance Analysis

Paired Wilcoxon signed-rank tests compare ADAPTIVE against each fixed scheduling policy at every offered-load level.

The pairing is based on identical target load and repetition number.

Significance threshold: alpha = 0.05.

## Throughput

| mode             |   target_rate_eps |   n_pairs |   adaptive_mean_throughput_eps |   fixed_mean_throughput_eps |   throughput_difference_eps |   wilcoxon_statistic |   p_value | significant_alpha_0.05   |
|:-----------------|------------------:|----------:|-------------------------------:|----------------------------:|----------------------------:|---------------------:|----------:|:-------------------------|
| FIXED_SINGLE     |             10000 |         3 |                        9813.57 |                     9833.05 |                    -19.4805 |                    2 |      0.75 | False                    |
| FIXED_SINGLE     |             20000 |         3 |                       19444.2  |                    19178.5  |                    265.74   |                    1 |      0.5  | False                    |
| FIXED_SINGLE     |             30000 |         3 |                       27789.2  |                    25962.6  |                   1826.56   |                    0 |      0.25 | False                    |
| FIXED_SINGLE     |             40000 |         3 |                       31124.9  |                    24517.5  |                   6607.45   |                    0 |      0.25 | False                    |
| FIXED_SINGLE     |             50000 |         3 |                       30789.1  |                    25779.3  |                   5009.76   |                    0 |      0.25 | False                    |
| FIXED_BATCH_32   |             10000 |         3 |                        9813.57 |                     9826.82 |                    -13.2483 |                    2 |      0.75 | False                    |
| FIXED_BATCH_32   |             20000 |         3 |                       19444.2  |                    19269.5  |                    174.72   |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             30000 |         3 |                       27789.2  |                    28050.5  |                   -261.276  |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             40000 |         3 |                       31124.9  |                    33553.8  |                  -2428.84   |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             50000 |         3 |                       30789.1  |                    34450.1  |                  -3661.08   |                    0 |      0.25 | False                    |
| FIXED_PARALLEL_8 |             10000 |         3 |                        9813.57 |                     9917.49 |                   -103.918  |                    0 |      0.25 | False                    |
| FIXED_PARALLEL_8 |             20000 |         3 |                       19444.2  |                    19265.9  |                    178.306  |                    0 |      0.25 | False                    |
| FIXED_PARALLEL_8 |             30000 |         3 |                       27789.2  |                    29318.9  |                  -1529.72   |                    0 |      0.25 | False                    |
| FIXED_PARALLEL_8 |             40000 |         3 |                       31124.9  |                    37555.6  |                  -6430.7    |                    0 |      0.25 | False                    |
| FIXED_PARALLEL_8 |             50000 |         3 |                       30789.1  |                    37149    |                  -6359.94   |                    0 |      0.25 | False                    |

## P99 Processing Latency

| mode             |   target_rate_eps |   n_pairs |   adaptive_mean_p99_us |   fixed_mean_p99_us |   p99_difference_us |   wilcoxon_statistic |   p_value | significant_alpha_0.05   |
|:-----------------|------------------:|----------:|-----------------------:|--------------------:|--------------------:|---------------------:|----------:|:-------------------------|
| FIXED_SINGLE     |             10000 |         3 |                22.3677 |             65.8443 |           -43.4767  |                    0 |      0.25 | False                    |
| FIXED_SINGLE     |             20000 |         3 |                25.6687 |             42.309  |           -16.6403  |                    1 |      0.5  | False                    |
| FIXED_SINGLE     |             30000 |         3 |                26.5007 |             23.6013 |             2.89933 |                    2 |      0.75 | False                    |
| FIXED_SINGLE     |             40000 |         3 |                27.07   |             33.9357 |            -6.86567 |                    1 |      0.5  | False                    |
| FIXED_SINGLE     |             50000 |         3 |                38.3007 |             26.801  |            11.4997  |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             10000 |         3 |                22.3677 |             41.7017 |           -19.334   |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             20000 |         3 |                25.6687 |             42.1713 |           -16.5027  |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             30000 |         3 |                26.5007 |             31.267  |            -4.76633 |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             40000 |         3 |                27.07   |             25.6013 |             1.46867 |                    3 |      1    | False                    |
| FIXED_BATCH_32   |             50000 |         3 |                38.3007 |             23.834  |            14.4667  |                    1 |      0.5  | False                    |
| FIXED_PARALLEL_8 |             10000 |         3 |                22.3677 |             25.235  |            -2.86733 |                    1 |      0.5  | False                    |
| FIXED_PARALLEL_8 |             20000 |         3 |                25.6687 |             27.4017 |            -1.733   |                    3 |      1    | False                    |
| FIXED_PARALLEL_8 |             30000 |         3 |                26.5007 |             24.3027 |             2.198   |                    1 |      0.5  | False                    |
| FIXED_PARALLEL_8 |             40000 |         3 |                27.07   |             26.869  |             0.201   |                    3 |      1    | False                    |
| FIXED_PARALLEL_8 |             50000 |         3 |                38.3007 |             23.2003 |            15.1003  |                    1 |      0.5  | False                    |

## P99.9 Processing Latency

| mode             |   target_rate_eps |   n_pairs |   adaptive_mean_p999_us |   fixed_mean_p999_us |   p999_difference_us |   wilcoxon_statistic |   p_value | significant_alpha_0.05   |
|:-----------------|------------------:|----------:|------------------------:|---------------------:|---------------------:|---------------------:|----------:|:-------------------------|
| FIXED_SINGLE     |             10000 |         3 |                 92.3422 |              618.458 |           -526.115   |                    1 |      0.5  | False                    |
| FIXED_SINGLE     |             20000 |         3 |                 93.2773 |              381.548 |           -288.271   |                    0 |      0.25 | False                    |
| FIXED_SINGLE     |             30000 |         3 |                133.304  |              125.92  |              7.38353 |                    2 |      0.75 | False                    |
| FIXED_SINGLE     |             40000 |         3 |                142.638  |              212.327 |            -69.6889  |                    2 |      0.75 | False                    |
| FIXED_SINGLE     |             50000 |         3 |                269.648  |              124.935 |            144.713   |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             10000 |         3 |                 92.3422 |              209.943 |           -117.6     |                    1 |      0.5  | False                    |
| FIXED_BATCH_32   |             20000 |         3 |                 93.2773 |              205.513 |           -112.236   |                    0 |      0.25 | False                    |
| FIXED_BATCH_32   |             30000 |         3 |                133.304  |              175.346 |            -42.0425  |                    2 |      0.75 | False                    |
| FIXED_BATCH_32   |             40000 |         3 |                142.638  |              145.87  |             -3.23247 |                    3 |      1    | False                    |
| FIXED_BATCH_32   |             50000 |         3 |                269.648  |              114.455 |            155.194   |                    1 |      0.5  | False                    |
| FIXED_PARALLEL_8 |             10000 |         3 |                 92.3422 |              180.605 |            -88.2632  |                    1 |      0.5  | False                    |
| FIXED_PARALLEL_8 |             20000 |         3 |                 93.2773 |              223.701 |           -130.423   |                    0 |      0.25 | False                    |
| FIXED_PARALLEL_8 |             30000 |         3 |                133.304  |              162.224 |            -28.9197  |                    1 |      0.5  | False                    |
| FIXED_PARALLEL_8 |             40000 |         3 |                142.638  |              604.981 |           -462.344   |                    0 |      0.25 | False                    |
| FIXED_PARALLEL_8 |             50000 |         3 |                269.648  |              173.31  |             96.3384  |                    1 |      0.5  | False                    |

## Interpretation

A p-value below 0.05 indicates statistically significant evidence of a difference between ADAPTIVE and the corresponding fixed policy for that metric and offered-load level.

A non-significant result does not establish that the two strategies are equivalent; it indicates that this experiment does not provide sufficient statistical evidence of a difference.
