# NexusFlow Saturation Sweep Report

## Experimental Setup

- Dataset: UCI Online Retail II
- Events per run: 10,000
- Offered rates: 10K–50K EPS
- Repetitions per rate: 3
- Scheduler: AdaptiveScheduler
- Maximum workers: 16
- Integrity filtering: PASS runs only

## Aggregated Results

|   target_rate_eps |   throughput_mean_eps |   throughput_std_eps |   throughput_cv_percent |   throughput_efficiency_percent |   p99_mean_us |   p999_mean_us |   max_latency_observed_us |   workers_mean |   throttled_mean |
|------------------:|----------------------:|---------------------:|------------------------:|--------------------------------:|--------------:|---------------:|--------------------------:|---------------:|-----------------:|
|             10000 |               9808.58 |              42.9563 |                0.437946 |                         98.0858 |       22.5    |        55.6366 |                     369.4 |        1.66667 |            0     |
|             15000 |              14623.3  |             159.643  |                1.0917   |                         97.4889 |       21.834  |        86.2697 |                     945   |        2.33333 |            0     |
|             20000 |              17963.7  |            2267.65   |               12.6235   |                         89.8183 |       48.0397 |       516.962  |                   49259.6 |        1.33333 |            0     |
|             25000 |              20367.2  |            4571.88   |               22.4473   |                         81.4688 |       53.4373 |       940.743  |                   39023.8 |        5.66667 |            6     |
|             30000 |              25272.7  |             969.574  |                3.83645  |                         84.2422 |       55.5023 |       455.842  |                    8970.1 |        5.33333 |           83     |
|             35000 |              26186.5  |            1978.85   |                7.55676  |                         74.8185 |       52.905  |       459.806  |                   30418.6 |        7       |          471.333 |
|             40000 |              29942.3  |            6183.46   |               20.6513   |                         74.8557 |       21.4683 |       153.279  |                   19390.3 |       13.3333  |          460.333 |
|             45000 |              32759.2  |             630.048  |                1.92327  |                         72.7983 |       22.001  |       108.336  |                   10371.2 |        8.66667 |         2448     |
|             50000 |              31623.9  |            1310.29   |                4.14334  |                         63.2478 |       16.4    |        28.4007 |                    4553   |        5.33333 |         3740.67  |

## Saturation Analysis

Estimated saturation knee: approximately 20,000 EPS.

Recommended operating point under the configured criteria: approximately 30,000 EPS.

## Interpretation

The saturation sweep measures how achieved throughput changes as offered event rate increases. A flattening throughput curve indicates that the processing pipeline is approaching its practical capacity. Increasing throttling at higher offered rates indicates that backpressure is becoming active.

Tail latency should be evaluated separately from average latency because rare scheduling, operating-system, or contention effects can produce substantially larger maximum latency values.
