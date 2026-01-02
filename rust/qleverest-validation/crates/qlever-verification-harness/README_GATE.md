# qlever-gate: CI/CD Gate Command

**Agent 6 Implementation - EPIC 11 Integration Phase**

## Overview

`qlever-gate` is the single entrypoint for CI/CD pipelines that runs QLever verification, collects artifacts, and emits a deterministic PASS/FAIL verdict.

## Installation

Build the release binary:
```bash
cd qlever-verification
cargo build --release --bin qlever-gate
```

Binary location: `target/release/qlever-gate`

## Usage

```bash
qlever-gate --workload <manifest.json> --env <environment.json> --output <dir/>
```

### Required Arguments

- `--workload <FILE>`: Path to workload pack manifest (CBOR or JSON format)
- `--env <FILE>`: Path to environment configuration (JSON)
- `--output <DIR>`: Output directory for artifacts

### Optional Arguments

- `-v, --verbose`: Enable verbose logging (can be repeated for more detail)
- `--dry-run`: Validate inputs only, skip verification execution

## Exit Codes

- `0`: PASS - All checks passed
- `1`: FAIL - Timeout or error occurred
- `2`: DIVERGENCE - Determinism or correctness failure detected

## Output Format

### stdout: JSON Verdict

```json
{
  "verdict": "PASS",
  "receipt_id": "a1b2c3d4...",
  "artifacts": [
    "/path/to/output/verdict.json",
    "/path/to/output/receipts/receipt-0001.cbor",
    "/path/to/output/environment.json",
    "/path/to/output/repro_manifest.json",
    "/path/to/output/workload_summary.json"
  ],
  "failure_count": 0,
  "divergence_count": 0,
  "duration_ms": 1234,
  "timestamp": "2026-01-02T18:00:00Z"
}
```

### Output Directory Structure

```
output/
├── verdict.json              # Same JSON as stdout
├── receipts/                 # Receipt bundles (CBOR)
│   ├── receipt-0001.cbor
│   └── receipt-0002.cbor
├── environment.json          # Copy of input environment config
├── repro_manifest.json       # Reproduction instructions
└── workload_summary.json     # Workload metadata
```

## Artifact Details

### verdict.json

Final gate verdict with summary metrics.

### receipts/*.cbor

Individual verification receipts in CBOR format. Each receipt contains:
- Failure class
- Digest evidence
- Reproduction command
- Timestamp

### repro_manifest.json

Instructions to reproduce the gate execution:
```json
{
  "workload_path": "path/to/workload.json",
  "environment_path": "path/to/environment.json",
  "repro_command": "qlever-gate --workload ... --env ... --output ...",
  "git_commit": "abc123",
  "timestamp": "2026-01-02T18:00:00Z"
}
```

### workload_summary.json

Metadata about the executed workload:
```json
{
  "workload_id": "deterministic-corpus-v1",
  "query_count": 42,
  "replay_mode": "Strict"
}
```

## Example: CI/CD Integration

```bash
#!/bin/bash
set -e

# Run gate
qlever-gate \
  --workload workloads/production-queries.json \
  --env ci-environment.json \
  --output verification-results/

EXIT_CODE=$?

# Parse verdict
VERDICT=$(jq -r '.verdict' verification-results/verdict.json)

echo "Gate Result: $VERDICT (exit code: $EXIT_CODE)"

# Handle exit codes
case $EXIT_CODE in
  0)
    echo "✅ All checks passed"
    exit 0
    ;;
  2)
    echo "❌ Divergence detected - determinism failure!"
    exit 1
    ;;
  *)
    echo "⚠️  Gate failed"
    exit 1
    ;;
esac
```

## Test Fixtures

Example workload manifest (`test-workload.json`):
```json
{
  "workload_id": "test-workload-v1",
  "query_pack": [
    {
      "query_id": "q1",
      "query_text": "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10",
      "execution_order": 0,
      "expected_result_digest": "0000...0000",
      "expected_cache_behavior": ["MISS"],
      "expected_latency_ms": 100.0
    }
  ],
  "expected_state": {
    "cache_size_bytes": 1024,
    "cache_entries": 1,
    "hit_rate_pct": 50.0,
    "epoch_key": []
  },
  "replay_mode": "Strict"
}
```

Example environment config (`test-environment.json`):
```json
{
  "git_commit": "abc123def456",
  "build_timestamp": "2026-01-02T18:00:00Z",
  "architecture": "x86_64",
  "simd_features": ["AVX2", "AVX512"]
}
```

## Testing

Run unit tests:
```bash
cargo test --bin qlever-gate
```

Run integration tests:
```bash
cargo test --test gate_integration_tests
```

Dry run:
```bash
qlever-gate \
  --workload test-fixtures/test-workload.json \
  --env test-fixtures/test-environment.json \
  --output /tmp/test-output \
  --dry-run
```

## Implementation Notes

- Built on top of `qlever-verification-harness` orchestration layer
- Uses `qlever-replay-verifier` for workload execution
- Collects receipts from all 9 verification subsystems
- Deterministic verdict computation based on failure classes
- BLAKE3 hashing for receipt aggregation

## Related Components

- `qlever-verify`: Interactive verification CLI with subcommands
- `qlever-replay-verifier`: Workload replay subsystem
- `qlever-artifact-capture`: Receipt format and storage

---

**Status**: ✅ COMPLETE
**Agent**: Agent 6
**Date**: 2026-01-02
