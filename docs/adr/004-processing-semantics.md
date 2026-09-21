# ADR-004: Processing Semantics

## Status

Accepted

## Context

Concurrent processing and retry behavior require an explicit event-delivery model.

## Decision

The current NexusFlow benchmark engine treats successful processing as the primary completion criterion and tracks accepted and processed events independently.

The current benchmark does not claim distributed exactly-once semantics.

## Rationale

Exactly-once processing across distributed services requires coordination mechanisms beyond the current benchmark core.

Claiming exactly-once semantics without an implementation and verification strategy would make the research results misleading.

## Consequences

NexusFlow currently focuses on:

- Processing correctness
- Queue integrity
- Accepted-event accounting
- Processed-event accounting
- Failure/recovery behavior

Distributed exactly-once processing remains future work.
