# NexusFlow Adaptive Scheduler Design

## Purpose

The NexusFlow scheduler dynamically selects an event-processing strategy based on runtime workload conditions.

The scheduler considers:

- Event priority
- Event arrival rate
- Queue depth
- Remaining SLA budget
- CPU utilization
- Available worker capacity

The objective is to balance throughput and latency while preventing excessive queue growth and protecting latency-critical events.

## Processing Modes

### SINGLE

One event is processed at a time.

Characteristics:

- Lowest batching delay
- Suitable for low arrival rates
- Suitable for latency-critical events
- Minimal batching overhead

### MICRO_BATCH

A bounded group of events is collected and processed sequentially.

Characteristics:

- Reduces scheduling overhead
- Improves throughput under sustained load
- Introduces bounded batching delay
- Current default batch size for high-load scheduling: 32

### PARALLEL

Work is distributed across multiple workers.

Characteristics:

- Increases processing concurrency
- Useful when queue pressure is high
- Can increase contention and tail latency
- Worker count is dynamically controlled

## Scheduler Inputs

Let the runtime state be:

    M = (lambda, Q, C, S, P)

where:

- lambda = event arrival rate in events/second
- Q = current queue depth
- C = CPU utilization estimate
- S = remaining SLA budget in microseconds
- P = highest pending event priority

## Decision Function

The scheduler computes:

    D = f(lambda, Q, C, S, P)

where D contains:

- Processing mode
- Batch size
- Target worker count
- SLA bypass flag

Conceptually:

    CRITICAL priority
        |
        v
    SINGLE + SLA bypass
        |
        v
    Latency-critical workload
        |
        v
    SINGLE + additional workers
        |
        v
    High queue / arrival pressure
        |
        v
    MICRO_BATCH
        |
        v
    Low or moderate workload
        |
        v
    SINGLE

## Priority Policy

CRITICAL events receive immediate single-event processing and bypass normal batching decisions.

HIGH priority events use a small micro-batch and increased worker capacity.

NORMAL and LOW priority events are primarily governed by workload pressure and SLA state.

## SLA Policy

The scheduler prioritizes latency when the remaining SLA budget becomes small.

The current implementation uses an SLA-aware latency guard.

Current guard thresholds:

- Remaining SLA <= 250 us: latency guard activated
- Remaining SLA <= 1000 us and queue depth >= 32: latency guard activated

When the guard activates:

- Processing mode becomes SINGLE
- Batch size becomes 1
- SLA bypass is enabled
- At least two workers are requested

## Queue Pressure

Queue depth is treated as a proxy for workload pressure.

Increasing queue depth can cause the scheduler to:

- Increase worker capacity
- Switch toward micro-batching
- Reduce batching when latency becomes critical

## Design Objective

The scheduler is not designed to maximize throughput unconditionally.

The optimization objective is:

    maximize throughput

subject to:

    tail latency <= acceptable SLA target

and:

    queue growth remains bounded

This creates an explicit throughput-versus-tail-latency trade-off.

## Current Limitations

The current implementation uses a simplified CPU utilization signal and does not yet use hardware-level CPU telemetry in the scheduler decision.

The current micro-batching implementation groups events before sequential processing; it does not yet provide fully vectorized batch execution.

The current thresholds are manually specified and should be evaluated through controlled threshold experiments.

## Research Direction

Future scheduler experiments should evaluate:

- Queue-depth thresholds
- Arrival-rate thresholds
- SLA thresholds
- Batch-size thresholds
- Worker-scaling thresholds
- Sensitivity to workload bursts
- Sensitivity to CPU contention

The goal is to determine whether adaptive decisions remain beneficial across workload regimes rather than only one benchmark configuration.
