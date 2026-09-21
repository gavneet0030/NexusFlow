# ADR-002: Bounded MPMC Queue

## Status

Accepted

## Context

Multiple producers and consumers may access the NexusFlow event queue concurrently.

The queue must prevent unbounded memory growth and provide explicit overload behavior.

## Decision

Use a bounded multi-producer, multi-consumer queue as the primary event-buffering mechanism.

## Rationale

A bounded queue provides:

- Explicit capacity
- Backpressure
- Controlled memory usage
- Concurrent producer/consumer support
- Measurable overload behavior

The project also contains a lock-free MPMC implementation for performance comparison.

## Consequences

When capacity is exhausted, the system must expose the resulting behavior through acceptance, rejection, and throttling metrics.

Queue behavior therefore becomes part of the benchmark methodology rather than an invisible implementation detail.
