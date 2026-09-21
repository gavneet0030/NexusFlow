# NexusFlow Real End-to-End Latency Experiment

## Experimental Design

- Dataset: UCI Online Retail II processed event stream
- Events per run: 10,000
- Load levels: 1,000 / 10,000 / 20,000 / 30,000 / 50,000 EPS
- Repetitions per load: 3
- Total runs: 15
- Maximum workers: 16
- Scheduler mode: ADAPTIVE
- Integrity: all completed runs passed

## Results

|   target_rate_eps |   throughput_eps |   efficiency_percent |   e2e_p50_us |   e2e_p95_us |   e2e_p99_us |   e2e_p999_us |   e2e_max_us |   workers |   throttled |
|------------------:|-----------------:|---------------------:|-------------:|-------------:|-------------:|--------------:|-------------:|----------:|------------:|
|              1000 |          998.529 |              99.8529 |      299.222 |      2090.66 |     10219.8  |      21718.8  |        53402 |   1       |      0      |
|             10000 |         9898.4   |              98.984  |     2100     |      6030.75 |      8409.49 |       9756.68 |        26908 |   1       |      0      |
|             20000 |        17934.5   |              89.6726 |    21549.8   |     43552.9  |     49469.1  |      51236.2  |       141658 |   2.11111 |      0      |
|             30000 |        17136.9   |              57.123  |    48535.8   |    116378    |    129333    |     131329    |       243043 |   3.44444 |     29.3333 |
|             50000 |        22525.3   |              45.0507 |    72485.8   |    116359    |    127703    |     129560    |       196357 |   2.77778 |    368.667  |

## Latency Breakdown

|   target_rate_eps |   queue_wait_fraction_percent |   processing_fraction_percent |   queue_wait_p99_us |   processing_p99_us |
|------------------:|------------------------------:|------------------------------:|--------------------:|--------------------:|
|              1000 |                       93.9658 |                     4.71593   |            10184.3  |             55.6711 |
|             10000 |                       99.1058 |                     0.698413  |             8391.72 |             38.0011 |
|             20000 |                       99.9018 |                     0.0618723 |            49441.8  |             61.1189 |
|             30000 |                       99.955  |                     0.024724  |           129310    |             84.6744 |
|             50000 |                       99.9712 |                     0.016555  |           127675    |             64.3433 |

## Findings

- Maximum measured mean throughput was 22525.33 EPS at 50000 EPS offered load.
- At 50000 EPS, queue waiting accounted for approximately 99.97% of median end-to-end latency.
- Processing latency remained substantially smaller than queue waiting latency across the measured workloads.
- The experiment demonstrates that end-to-end latency is currently dominated by admission, queueing, and scheduling rather than the event-processing kernel itself.
- Adaptive worker scaling was observed at higher offered loads, but scaling did not prevent substantial queue buildup and tail latency growth in all runs.

## Research Implication

The measured bottleneck motivates Scheduler V2. The next scheduler iteration should incorporate queue-growth rate, recent tail latency, worker utilization, hysteresis, and cooldown/residency controls rather than relying primarily on instantaneous queue depth and arrival rate.

## Important Limitation

These measurements represent the current adaptive pipeline implementation. They do not establish that the adaptive scheduler is superior to fixed scheduling policies. A separate paired comparative experiment is required for that claim.
