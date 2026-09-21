# NexusFlow Comparative Statistical Report

## Experimental Design

- Modes: FIXED_SINGLE, FIXED_BATCH_32, FIXED_PARALLEL_8, ADAPTIVE
- Target loads: 10K, 20K, 30K, 40K, 50K EPS
- Repetitions: 3 per mode/load combination
- Events per run: 10,000
- Statistical test: Wilcoxon signed-rank test
- Significance threshold: alpha = 0.05

## Aggregate Results

| Load | Mode | Mean Throughput EPS | Mean P99 us | Mean P99.9 us | Mean Workers | Mean Throttled |
|---:|---|---:|---:|---:|---:|---:|
| 10,000 | ADAPTIVE | 9,888.48 | 130.92 | 1,282.26 | 3.00 | 0.00 |
| 20,000 | ADAPTIVE | 18,070.78 | 62.47 | 1,205.95 | 7.00 | 1.00 |
| 30,000 | ADAPTIVE | 22,060.39 | 56.80 | 594.17 | 9.00 | 408.67 |
| 40,000 | ADAPTIVE | 19,538.63 | 61.37 | 954.40 | 8.00 | 386.00 |
| 50,000 | ADAPTIVE | 22,134.99 | 66.77 | 743.05 | 9.00 | 416.00 |
| 10,000 | FIXED_BATCH_32 | 9,655.62 | 112.14 | 1,038.70 | 1.00 | 0.00 |
| 20,000 | FIXED_BATCH_32 | 18,416.26 | 91.44 | 1,045.42 | 1.00 | 0.00 |
| 30,000 | FIXED_BATCH_32 | 21,646.49 | 98.98 | 1,095.70 | 1.00 | 2.33 |
| 40,000 | FIXED_BATCH_32 | 22,692.90 | 80.20 | 686.69 | 1.00 | 298.67 |
| 50,000 | FIXED_BATCH_32 | 22,320.32 | 91.88 | 843.45 | 1.00 | 195.67 |
| 10,000 | FIXED_PARALLEL_8 | 9,956.76 | 157.31 | 1,622.26 | 8.00 | 0.00 |
| 20,000 | FIXED_PARALLEL_8 | 19,625.27 | 88.78 | 1,691.92 | 8.00 | 0.00 |
| 30,000 | FIXED_PARALLEL_8 | 26,460.21 | 64.47 | 1,426.71 | 8.00 | 0.00 |
| 40,000 | FIXED_PARALLEL_8 | 26,781.97 | 72.41 | 1,099.64 | 8.00 | 0.00 |
| 50,000 | FIXED_PARALLEL_8 | 27,475.69 | 87.97 | 1,493.67 | 8.00 | 0.00 |
| 10,000 | FIXED_SINGLE | 9,888.94 | 104.31 | 811.98 | 1.00 | 0.00 |
| 20,000 | FIXED_SINGLE | 15,905.14 | 60.84 | 527.70 | 1.00 | 36.33 |
| 30,000 | FIXED_SINGLE | 17,881.10 | 64.30 | 412.23 | 1.00 | 921.00 |
| 40,000 | FIXED_SINGLE | 17,209.42 | 63.37 | 652.76 | 1.00 | 949.33 |
| 50,000 | FIXED_SINGLE | 17,247.69 | 76.97 | 599.27 | 1.00 | 1215.33 |

## ADAPTIVE vs Fixed Baselines

| Load | Baseline | Throughput Delta | P99 Delta | Throughput p | P99 p |
|---:|---|---:|---:|---:|---:|
| 10,000 | FIXED_SINGLE | -0.00% | +25.51% | 1.000000 | 0.250000 |
| 10,000 | FIXED_BATCH_32 | +2.41% | +16.74% | 0.750000 | 0.500000 |
| 10,000 | FIXED_PARALLEL_8 | -0.69% | -16.78% | 0.250000 | 0.500000 |
| 20,000 | FIXED_SINGLE | +13.62% | +2.68% | 0.750000 | 0.750000 |
| 20,000 | FIXED_BATCH_32 | -1.88% | -31.68% | 0.750000 | 0.250000 |
| 20,000 | FIXED_PARALLEL_8 | -7.92% | -29.63% | 0.250000 | 0.250000 |
| 30,000 | FIXED_SINGLE | +23.37% | -11.66% | 0.250000 | 1.000000 |
| 30,000 | FIXED_BATCH_32 | +1.91% | -42.61% | 1.000000 | 0.250000 |
| 30,000 | FIXED_PARALLEL_8 | -16.63% | -11.90% | 0.250000 | 0.750000 |
| 40,000 | FIXED_SINGLE | +13.53% | -3.16% | 0.250000 | 0.750000 |
| 40,000 | FIXED_BATCH_32 | -13.90% | -23.48% | 0.250000 | 0.250000 |
| 40,000 | FIXED_PARALLEL_8 | -27.05% | -15.24% | 0.250000 | 0.750000 |
| 50,000 | FIXED_SINGLE | +28.34% | -13.25% | 0.250000 | 0.500000 |
| 50,000 | FIXED_BATCH_32 | -0.83% | -27.32% | 0.750000 | 0.250000 |
| 50,000 | FIXED_PARALLEL_8 | -19.44% | -24.10% | 0.250000 | 0.250000 |

## Statistical Interpretation

No comparison reached p < 0.05. The experiment therefore does not provide sufficient statistical evidence to claim ADAPTIVE superiority over the fixed baselines. Observed differences should be treated as descriptive rather than statistically established effects.

## Reproducibility

The analysis uses the existing comparative_raw.csv without generating additional benchmark runs.
