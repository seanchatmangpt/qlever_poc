# EPIC 10.1: Workload Replay Result Artifact

**Agent**: Agent 8 - Workload Replay & Fail-Closed Divergence Abort
**Date**: 2026-01-02
**Spec Lock**: EPIC 10.1 Sections 3.6, 3.7, 6.2, 6.4

## Purpose

This document defines the **Workload Replay Result Artifact**, a deterministic, machine-readable record of workload replay execution. The artifact proves:

1. **Deterministic Replay**: Same workload produces identical execution envelopes
2. **Fail-Closed Divergence Abort**: Envelope mismatches trigger immediate abort with no partial results
3. **Structured Error Reporting**: All divergences reported via machine-readable error codes (not prose)
4. **Envelope Verification**: Every replayed query includes envelope digest comparison

## Artifact Structure

### Replay Run Artifact

A **Replay Run** captures the complete execution of a workload replay session:

```json
{
  "@context": "https://qlever.cs.uni-freiburg.de/readplane/v1",
  "@type": "ReplayRun",
  "@version": "v1.0",
  "run_id": 1001,
  "source_manifest_id": "workload-20260102-prod",
  "source_manifest_digest": "sha256:aabbccdd...",
  "replay_epoch": {
    "epoch_id": 42,
    "epoch_manifest_sha256": "sha256:11223344..."
  },
  "replay_hostname": "replay-node-01",
  "overall_status": "SUCCESS|DIVERGENCE|ABORT|ERROR",
  "aborted_at_sequence_id": 0,
  "results": [ ... ],
  "stats": {
    "@type": "ReplayRunStats",
    "total_records": 100,
    "successful_replays": 100,
    "divergent_replays": 0,
    "aborted_replays": 0,
    "error_replays": 0,
    "cross_epoch_reproducible": 5,
    "total_execution_time_ns": 5000000000
  },
  "start_timestamp_ns": 1735862400000000000,
  "end_timestamp_ns": 1735862405000000000
}
```

### Replay Result (Per Query)

Each replayed query produces a **ReplayResult**:

```json
{
  "@type": "ReplayResult",
  "@version": "v1.0",
  "replay_run_id": 1001,
  "workload_record_id": 42,
  "replayed_on_epoch": {
    "epoch_id": 42,
    "epoch_manifest_sha256": "sha256:11223344..."
  },
  "replayed_on_hostname": "replay-node-01",
  "execution_status": "SUCCESS",
  "execution_digest": {
    "@type": "ExecutionDigest",
    "query_fingerprint_sha256": "sha256:aabbcc...",
    "plan_hash": "sha256:ddeeff...",
    "resource_signature": "sha256:112233...",
    "result_length_hash": "sha256:445566...",
    "result_shape_hash": "sha256:778899...",
    "digest_hash": "sha256:final..."
  },
  "expected_digest": "sha256:final...",
  "digest_matches": true,
  "is_cross_epoch_reproducible": false,
  "execution_duration_ns": 50000000,
  "trace_events": [ ... ],
  "divergence_artifact": null
}
```

### Divergence Artifact (On Failure)

When divergence is detected, a **DivergenceArtifact** is created:

```json
{
  "@type": "DivergenceArtifact",
  "@severity": "FAIL_CLOSED",
  "query_fingerprint_sha256": "sha256:aabbcc...",
  "workload_record_sequence_id": 42,
  "expected_digest": "sha256:expected...",
  "actual_digest": "sha256:actual...",
  "expected_plan_hash": "sha256:plan1...",
  "actual_plan_hash": "sha256:plan2...",
  "expected_result_shape_hash": "sha256:shape1...",
  "actual_result_shape_hash": "sha256:shape2...",
  "expected_resource_signature": "sha256:rsrc1...",
  "actual_resource_signature": "sha256:rsrc2...",
  "trace_events": [ ... ],
  "detection_timestamp_ns": 1735862402500000000
}
```

### Structured Divergence Error

Fail-closed abort produces a **StructuredDivergenceError**:

```json
{
  "@type": "StructuredDivergenceError",
  "@severity": "FAIL_CLOSED",
  "error_code": 11,
  "error_code_name": "ENVELOPE_PLAN_HASH_MISMATCH",
  "workload_record_sequence_id": 42,
  "replay_run_id": 1001,
  "expected_digest": "sha256:expected...",
  "actual_digest": "sha256:actual...",
  "envelope_diff": {
    "@type": "EnvelopeDiff",
    "is_identical": false,
    "classification": "PLAN_DIVERGENCE",
    "plan_changed": true,
    "resource_envelope_changed": false,
    "result_shape_changed": false,
    "differing_components": [
      {
        "component": "plan_hash",
        "before": "sha256:plan1...",
        "after": "sha256:plan2..."
      }
    ]
  },
  "partial_results_emitted": false,
  "abort_was_immediate": true,
  "detection_timestamp_ns": 1735862402500000000
}
```

## Error Code Classification

### Machine-Readable Error Codes

All divergence errors are classified via **DivergenceErrorCode** enum:

| Error Code | Hex Value | Name | Description |
|------------|-----------|------|-------------|
| `SUCCESS` | `0x0000` | No error | Replay succeeded, envelopes match |
| `ENVELOPE_DIGEST_MISMATCH` | `0x1001` | Generic digest mismatch | Overall digest differs |
| `ENVELOPE_PLAN_HASH_MISMATCH` | `0x1002` | Plan divergence | Plan hash differs (non-deterministic planning) |
| `ENVELOPE_RESOURCE_SIG_MISMATCH` | `0x1003` | Resource envelope divergence | Resource signature differs (cache state) |
| `ENVELOPE_RESULT_SHAPE_MISMATCH` | `0x1004` | Result shape divergence | Result structure differs |
| `ENVELOPE_RESULT_LENGTH_MISMATCH` | `0x1005` | Result length divergence | Result size differs |
| `ENVELOPE_MULTIPLE_MISMATCHES` | `0x1006` | Multiple divergences | Multiple components differ |
| `QUERY_FINGERPRINT_MISMATCH` | `0x2001` | Query mismatch | Different query executed (should never happen) |
| `REPLAY_INVALID_CONFIGURATION` | `0x3001` | Config error | Invalid replay configuration |
| `REPLAY_MISSING_CONTEXT` | `0x3002` | Context error | Required execution context missing |
| `REPLAY_QUERY_PARSE_FAILED` | `0x3003` | Parse error | SPARQL parsing failed |
| `REPLAY_QUERY_EXECUTION_FAILED` | `0x3004` | Execution error | Query execution failed |
| `REPLAY_DIGEST_COMPUTATION_FAILED` | `0x3005` | Digest error | Digest computation failed |
| `ABORT_PARTIAL_RESULTS_DETECTED` | `0x4001` | Fail-closed violation | Partial results emitted before abort |
| `ABORT_DEFERRED_ABORT_DETECTED` | `0x4002` | Fail-closed violation | Abort was deferred (not immediate) |
| `ABORT_HANDLER_FAILURE` | `0x4003` | Handler error | Abort handler itself failed |

### Exit Code Mapping

Error codes map to deterministic process exit codes:

- `SUCCESS` → Exit code 0
- `ENVELOPE_DIGEST_MISMATCH` → Exit code 10
- `ENVELOPE_PLAN_HASH_MISMATCH` → Exit code 11
- `ENVELOPE_RESOURCE_SIG_MISMATCH` → Exit code 12
- `ENVELOPE_RESULT_SHAPE_MISMATCH` → Exit code 13
- `ENVELOPE_RESULT_LENGTH_MISMATCH` → Exit code 14
- `ENVELOPE_MULTIPLE_MISMATCHES` → Exit code 15
- `QUERY_FINGERPRINT_MISMATCH` → Exit code 20
- `REPLAY_*` → Exit codes 30-34
- `ABORT_*` → Exit codes 40-42

## Fail-Closed Semantics

### Invariants

1. **No Partial Results**: If divergence detected, no results from that query or subsequent queries are emitted
2. **Immediate Abort**: Divergence triggers immediate abort (no continuation)
3. **Structured Errors Only**: All errors reported via machine-readable codes (no prose)
4. **Atomic Visibility**: Replay results written atomically (all-or-nothing)

### Validation Checks

Every replay result must satisfy:

```cpp
bool isValidFailClosed(const StructuredDivergenceError& error) {
  return !error.partial_results_emitted && error.abort_was_immediate;
}
```

### Abort Pathway

```
1. Digest Comparison
   ↓
2. Envelope Mismatch Detected
   ↓
3. Classify Divergence (→ DivergenceErrorCode)
   ↓
4. Create StructuredDivergenceError
   ↓
5. Verify Fail-Closed (no partial results, immediate abort)
   ↓
6. Throw DivergenceAbortException
   ↓
7. Halt Replay (no further queries executed)
```

## Envelope Identity Verification

### Determinism Proof

For each replayed query:

1. **Capture Phase** (original execution):
   - Compute `ExecutionDigest` from query execution
   - Store digest in `WorkloadRecord.fingerprint_sha256`

2. **Replay Phase**:
   - Re-execute same query
   - Compute new `ExecutionDigest`
   - Compare: `actual_digest.digest_hash == expected_digest`

3. **Verification**:
   - If digests match → **SUCCESS** (determinism validated)
   - If digests differ → **DIVERGENCE** (fail-closed abort)

### Envelope Components

Each `ExecutionDigest` includes:

- `query_fingerprint_sha256` - Query identity (from EPIC 3)
- `plan_hash` - Execution plan structure
- `resource_signature` - Cache/memory behavior
- `result_length_hash` - Result size
- `result_shape_hash` - Result structure
- `digest_hash` - Combined hash of all components

**Determinism Property**: Same query on same data → same `digest_hash`

## Example: Successful Replay

```json
{
  "@type": "ReplayRun",
  "run_id": 1001,
  "overall_status": "SUCCESS",
  "stats": {
    "total_records": 100,
    "successful_replays": 100,
    "divergent_replays": 0
  },
  "results": [
    {
      "workload_record_id": 1,
      "execution_status": "SUCCESS",
      "digest_matches": true,
      "expected_digest": "sha256:abc...",
      "execution_digest": {
        "digest_hash": "sha256:abc..."
      }
    },
    ...
  ]
}
```

**Proof**: All 100 queries replayed with `digest_matches: true` → determinism validated

## Example: Divergence Abort

```json
{
  "@type": "ReplayRun",
  "run_id": 1002,
  "overall_status": "DIVERGENCE",
  "aborted_at_sequence_id": 42,
  "stats": {
    "total_records": 100,
    "successful_replays": 41,
    "divergent_replays": 1,
    "aborted_replays": 58
  },
  "results": [
    { "workload_record_id": 1, "execution_status": "SUCCESS", "digest_matches": true },
    ...
    { "workload_record_id": 41, "execution_status": "SUCCESS", "digest_matches": true },
    {
      "workload_record_id": 42,
      "execution_status": "DIVERGENCE",
      "digest_matches": false,
      "expected_digest": "sha256:abc...",
      "execution_digest": {
        "digest_hash": "sha256:xyz..."
      },
      "divergence_artifact": {
        "@type": "DivergenceArtifact",
        "@severity": "FAIL_CLOSED",
        "actual_plan_hash": "sha256:different...",
        "expected_plan_hash": "sha256:original..."
      }
    }
    // No records 43-100 (fail-closed abort)
  ]
}
```

**Proof**: Divergence at record 42 → immediate abort → no records 43-100 executed → fail-closed validated

## Deliberate Perturbation Tests

### Test 1: Plan Hash Perturbation

**Setup**: Replay with modified query planner to produce different plan

**Expected Result**:
- Error Code: `ENVELOPE_PLAN_HASH_MISMATCH` (0x1002)
- Abort: Immediate, no partial results
- Exit Code: 11

### Test 2: Resource Signature Perturbation

**Setup**: Replay with different cache state

**Expected Result**:
- Error Code: `ENVELOPE_RESOURCE_SIG_MISMATCH` (0x1003)
- Abort: Immediate, no partial results
- Exit Code: 12

### Test 3: Result Shape Perturbation

**Setup**: Replay with modified data producing different result structure

**Expected Result**:
- Error Code: `ENVELOPE_RESULT_SHAPE_MISMATCH` (0x1004)
- Abort: Immediate, no partial results
- Exit Code: 13

## Integration Points

### With Other Agents

- **Agent 1 (Envelope)**: Uses `PerformanceEnvelope` for aggregate metrics
- **Agent 4 (Result Digest)**: Uses `ExecutionDigest` for envelope comparison
- **Agent 3 (Epoch Identity)**: Validates `EpochKey` consistency across replay
- **Agent 10 (Regression Gates)**: Uses replay results for baseline comparison

### Artifact Storage

Replay result artifacts are stored as JSON-LD files:

```
/path/to/artifacts/
  replay-runs/
    replay-run-1001.jsonld      # Complete ReplayRun
    replay-run-1002.jsonld      # Divergence case
  divergence-artifacts/
    divergence-1002-seq42.jsonld # Detailed divergence info
  structured-errors/
    error-1002-seq42.jsonld     # Machine-readable error
```

## Compliance Matrix

| Requirement | Implementation | Validation |
|-------------|----------------|------------|
| Section 3.7: Workload replay required | `WorkloadReplayEngine` | `WorkloadReplayFailClosedTest` |
| Section 3.6: Fail-closed on divergence | `DivergenceAbortHandler` | Tests 2-5, 6-7 |
| Section 6.4: Deliberate perturbation tests | Perturbation test helpers | Tests 2-5 |
| Section 6.2: Determinism artifact | `ReplayResult` with digest comparison | Test 1 |
| No partial results on divergence | `partial_results_emitted = false` | Test 6 |
| Immediate abort (no deferred) | `abort_was_immediate = true` | Test 7 |
| Structured error codes (not prose) | `DivergenceErrorCode` enum | Test 8 |
| Machine-readable reporting | JSON-LD serialization | Test 8 |

## Conclusion

This artifact specification ensures:

1. **Deterministic Replay**: Proven via envelope digest matching
2. **Fail-Closed Abort**: Enforced via structured error handling with no partial results
3. **Machine-Readable Errors**: All errors classified via enums, serialized to JSON-LD
4. **Regression Detection**: Baseline comparison enabled via replay result storage

**Status**: EPIC 10.1 Agent 8 deliverable complete.
