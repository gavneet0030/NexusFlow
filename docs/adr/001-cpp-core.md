# ADR-001: C++ for the Performance-Critical Core

## Status

Accepted

## Context

NexusFlow contains a latency-sensitive event-processing path where predictable execution cost, explicit memory management, and concurrency control are important.

## Decision

Use modern C++ for the core event-processing engine.

Python is used for:

- Experiment orchestration
- Data analysis
- Statistical analysis
- Visualization
- Report generation

## Rationale

C++ provides:

- Explicit control over memory
- Native multithreading primitives
- Atomic operations
- Low-level queue implementation
- Predictable performance characteristics
- Direct access to platform-level facilities

Python remains appropriate for the research and analysis layer.

## Consequences

The project requires maintaining two technical layers:

- C++ performance engine
- Python experimentation and analysis environment

This increases development complexity but provides a realistic separation between execution and analysis.
