# ADR-003: Adaptive Scheduling

## Status

Accepted

## Context

A single processing strategy is unlikely to be optimal across all workload regimes.

Single-event processing minimizes batching delay but can reduce throughput.

Large batches improve throughput but can increase waiting time.

High parallelism can increase concurrency but may increase contention and tail latency.

## Decision

NexusFlow uses a runtime adaptive scheduler that selects among:

- SINGLE
- MICRO_BATCH
- PARALLEL

The decision is based on workload and latency state.

## Rationale

The scheduler allows the system to react to changing conditions rather than using one static execution strategy.

## Experimental Hypothesis

Adaptive scheduling should provide higher throughput than fixed single-event processing and may provide a better throughput-latency trade-off than fixed parallelism.

The hypothesis is evaluated empirically rather than assumed to be true.

## Consequences

The scheduler introduces:

- Additional decision overhead
- More complex runtime behavior
- More parameters to tune
- A need for repeated statistical experiments

These costs are justified only if measurable performance benefits are demonstrated.
