# NexusFlow - Adaptive Capacity Research Report

## 1. Experiment Overview

This experiment evaluates NexusFlow under increasing offered event-arrival rates.

The benchmark measures:

- Offered load
- Achieved throughput
- Throughput efficiency
- P99 processing latency
- Observed worker count
- Throttled events
- Integrity status

## 2. Experimental Results

| Offered Load (EPS) | Throughput (EPS) | Efficiency (%) | P99 (us) | Workers | Throttled | Integrity |
|---:|---:|---:|---:|---:|---:|---|
| 10,000 | 9,879.13 | 98.79 | 19.00 | 2 | 0 | PASS |
| 15,000 | 14,671.26 | 97.81 | 32.00 | 3 | 0 | PASS |
| 20,000 | 19,602.50 | 98.01 | 38.00 | 2 | 0 | PASS |
| 25,000 | 24,001.68 | 96.01 | 30.00 | 7 | 0 | PASS |
| 30,000 | 27,964.66 | 93.22 | 22.00 | 10 | 0 | PASS |
| 35,000 | 31,325.53 | 89.50 | 20.00 | 11 | 314 | PASS |
| 40,000 | 32,030.59 | 80.08 | 19.00 | 12 | 2089 | PASS |
| 45,000 | 31,968.85 | 71.04 | 19.00 | 12 | 3234 | PASS |
| 50,000 | 26,648.46 | 53.30 | 33.00 | 10 | 2389 | PASS |

## 3. Peak Throughput

The highest measured throughput was:

**32,030.59 EPS**

at an offered load of:

**40,000 EPS**

## 4. Capacity Efficiency

The first tested load below the 80% throughput-efficiency threshold was:

**45,000 EPS (71.04% efficiency)**

## 5. Latency Behavior

Lowest measured P99:

**19.00 us**

at **40,000 EPS**.

Highest measured P99:

**38.00 us**

at **20,000 EPS**.

The measured processing P99 does not increase monotonically with offered load.

## 6. Worker Scaling

Maximum observed workers:

**12**

The scheduler dynamically changes worker capacity according to workload conditions.

## 7. Backpressure

Maximum throttled events:

**3234**

Throttling becomes significant in the high-load region, demonstrating overload-control behavior.

## 8. Capacity Regions

### High-Efficiency Region

Approximately 10K-30K EPS.

Efficiency remains above 90% throughout the tested points.

### Saturation Region

Approximately 35K-40K EPS.

Throughput continues increasing while efficiency decreases and throttling begins.

### Overload Region

Approximately 45K-50K EPS.

Throughput no longer increases proportionally with offered load and efficiency declines substantially.

## 9. Capacity Knee

Using the predefined 80% efficiency criterion:

**Capacity knee: 45,000 EPS**

with measured efficiency of **71.04%**.

## 10. Interpretation

The experiment demonstrates:

1. High throughput at moderate offered loads.
2. Dynamic worker scaling.
3. Backpressure activation under heavy load.
4. A measurable throughput saturation point.
5. Reduced efficiency when offered demand exceeds sustainable capacity.

These results validate the functional behavior of the adaptive capacity-control implementation.

They do not establish that the current scheduler is globally optimal.

## 11. Limitations

- Worker utilization is not directly measured by the current runtime scheduler.
- Worker allocation is influenced by multiple scheduler signals.
- Results depend on the benchmark machine and runtime conditions.
- Processing latency and end-to-end latency are different measurements.
- Statistical superiority over fixed scheduling requires separate repeated comparative experiments.
- The experiment does not prove globally optimal scheduler thresholds.

## 12. Reproducibility

The standardized reproducibility pipeline reported:

- Benchmark build: PASS
- Benchmark execution: PASS
- Benchmark integrity: PASS
- CTest: 4/4 PASS
- Standardized pipeline: PASS

## 13. Conclusion

The capacity sweep validates NexusFlow as a functioning adaptive event-processing system with measurable capacity scaling and overload protection.

Measured peak throughput:

**32,030.59 EPS**

at:

**40,000 EPS offered load**

The next research stage should compare adaptive scheduling against fixed single-event, fixed micro-batch, and fixed parallel baselines using repeated trials and statistical analysis of throughput, P99/P99.9 latency, throttling, and SLA violations.