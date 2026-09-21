# NexusFlow Scheduler Decision Model

## State Variables

The scheduler observes the following runtime variables:

| Symbol | Variable | Unit |
|---|---|---|
| lambda | Arrival rate | events/sec |
| Q | Queue depth | events |
| C | CPU utilization | fraction |
| S | Remaining SLA budget | microseconds |
| P | Highest priority | categorical |

## Output Variables

The scheduler produces:

| Output | Meaning |
|---|---|
| Mode | SINGLE, MICRO_BATCH, or PARALLEL |
| Batch size | Number of events grouped for processing |
| Target workers | Desired active worker count |
| SLA bypass | Whether latency protection overrides batching |

## Decision Priority

The scheduler evaluates decisions in priority order:

1. CRITICAL event protection
2. Latency-critical protection
3. SLA tail-latency guard
4. Queue and arrival pressure
5. Normal workload policy

Higher-priority rules can override lower-priority throughput optimizations.

## Decision Table

| Condition | Mode | Batch | Workers | SLA Bypass |
|---|---|---:|---:|---|
| CRITICAL priority | SINGLE | 1 | >= 4 | Yes |
| HIGH priority | MICRO_BATCH | 4 | >= 2 | No |
| Latency-critical | SINGLE | 1 | >= 2 | Yes |
| Tail guard active | SINGLE | 1 | >= 2 | Yes |
| High workload pressure | MICRO_BATCH / PARALLEL | Adaptive | Adaptive | No |
| Low workload | SINGLE | 1 | 1 | No |

## Formal Optimization View

Let:

    T = throughput
    L99 = P99 latency
    L999 = P99.9 latency
    Q = queue depth

The scheduler attempts to maximize:

    T

while maintaining:

    L99 and L999 within acceptable limits

and:

    Q < Q_hard

The system therefore represents a constrained optimization problem rather than a pure throughput optimization.

## Threshold Experiment

The next scheduler experiment should vary the decision thresholds systematically.

Candidate dimensions:

- Queue threshold
- Arrival-rate threshold
- SLA threshold
- Batch size
- Worker target

For each configuration, record:

- Mean throughput
- P50
- P95
- P99
- P99.9
- Maximum latency
- Queue depth
- Processed events
- Rejected events
- Throttled events

The selected configuration should be based on measured performance rather than arbitrary threshold selection.

## Important Experimental Rule

The benchmark workload, event count, compiler configuration, machine, and measurement methodology must remain constant while scheduler thresholds are changed.

Only scheduler parameters should vary.

This isolates the effect of the scheduling policy.
