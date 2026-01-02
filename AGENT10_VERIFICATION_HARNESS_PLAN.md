# AGENT 10: Verification Harness (CLI Orchestration) - Implementation Plan

**Status**: Implementation Phase
**Date**: 2026-01-02
**Branch**: claude/rust-read-cache-verification-TWfE7

---

## Executive Summary

Agent 10 is the final orchestration layer that aggregates all 9 verification subsystems (Agents 1-9) under a single CLI interface. The `qlever-verify` binary exposes three verification modes (contract, regression, full) with configurable time budgets and receipt aggregation.

**Key Constraint**: No agent waits for another. Orchestration is sequential at CLI level but each invocation is independent. Receipts flow to shared storage: `/tmp/qlever-verification-receipts/`.

---

## Architecture: 4-Layer Design

### Layer 1: CLI Interface (src/cli.rs)
**Responsibility**: Parse user input, validate arguments, route to appropriate verification gate.

```
qlever-verify [SUBCOMMAND] [OPTIONS]
  ├─ contract
  │  └─ --parallel N               (default 4, how many tests to run in parallel)
  │
  ├─ regression
  │  ├─ --baseline <file>          (previous commit results, auto-fetch via git)
  │  └─ --tolerance <pct>          (regression threshold, default 5%)
  │
  └─ full
     ├─ --duration-limit <seconds> (safety timeout, default 3600s)
     └─ --skip-nightly             (skip long-running cross-arch tests)
```

**Framework**: `clap` for robust argument parsing with validation.

---

### Layer 2: Gate Execution (src/main.rs + orchestration module)
**Responsibility**: Invoke subsystems in correct order, enforce time budgets, handle failures gracefully.

#### Gate 1: Contract (< 120s, blocking)
1. **Unit Tests** (qlever-artifact-capture, qlever-digest-verifier, qlever-cache-verifier)
   - Run in parallel: `cargo test --lib` in each crate
   - Time budget: 30s

2. **FFI Contract Tests** (qlever-kernel-runner)
   - Verify C↔Rust boundary contracts
   - Time budget: 30s

3. **SIMD Equivalence** (qlever-simd-verifier)
   - AVX-512 vs scalar on current architecture
   - Time budget: 20s

4. **Receipt Schema Validation**
   - Verify all emitted receipts are valid CBOR
   - Time budget: 10s

**Failure Behavior**: Any blocking failure → abort immediately with receipt, exit(1).

#### Gate 2: Regression (< 600s, blocking if threshold exceeded)
1. **Workload Replay** (qlever-replay-verifier)
   - Execute deterministic workload pack
   - Mode: Strict (fail on divergence)
   - Time budget: 180s

2. **Performance Regression** (qlever-regression-verifier)
   - Compare p95 latency, QPS, memory vs baseline
   - Thresholds: 10% latency, 5% QPS, 20% memory
   - Time budget: 180s

3. **Cache Hit Rate** (qlever-cache-verifier)
   - Regression detection on hit rate
   - Threshold: 5% drop
   - Time budget: 60s

4. **Chaos Injection** (qlever-chaos-verifier)
   - Induced fault handling + recovery
   - Time budget: 90s

**Failure Behavior**: Collect all failures, emit receipts, exit code based on `is_blocking`.

#### Gate 3: Full Nightly (< 3600s, advisory)
1. All regression gates (600s)
2. **Cross-Architecture Tests** (qlever-simd-verifier, qlever-epoch-verifier)
   - x86_64 ↔ ARM64 workload equivalence
   - Time budget: 600s (QEMU overhead)
3. **Multi-Machine Reproducibility**
   - Same workload on 2+ machines (requires external setup)
   - Time budget: 900s
4. **Stress Tests** (qlever-chaos-verifier)
   - Long-running stability under load
   - Time budget: 600s

**Failure Behavior**: Advisory; emit receipts but exit(0) regardless.

---

### Layer 3: Receipt Aggregation (src/reporter.rs)
**Responsibility**: Collect all receipts, deduplicate, format output, validate closure.

```rust
pub struct VerificationReport {
    pub gate_name: String,                      // "contract", "regression", "full"
    pub total_time_ms: u64,
    pub gate_result: GateResult,               // PASS, FAIL, TIMEOUT, PARTIAL
    pub receipts_count: usize,
    pub blocking_failures_count: usize,
    pub advisory_failures_count: usize,
    pub receipts: Vec<VerificationReceipt>,
}

pub enum GateResult {
    PASS,                      // All tests passed
    FAIL,                      // Blocking test failed
    TIMEOUT,                   // Gate exceeded time budget
    PARTIAL,                   // Some failures, some passes
}
```

**Output Formats**:
1. **JSON** (default, machine-readable): Emitted to stdout + `/tmp/qlever-verification-receipts/report-<timestamp>.json`
2. **Summary** (human-readable): Emitted to stdout with `--human-readable` flag

**Aggregation Rules**:
- Deduplicate receipts by (timestamp, failure_class, query_id)
- Summarize by failure class
- Report total time and exit code

---

### Layer 4: Integration Tests (tests/integration/)
**Responsibility**: Validate CLI behavior, receipt flow, error handling.

#### Test 1: cli_tests.rs
- CLI argument validation
- Help text generation
- Error handling for invalid args

#### Test 2: e2e_tests.rs
- Full workflow: contract → regression → full
- Receipt emission + aggregation
- Time budget enforcement
- Exit code correctness

---

## Implementation Order

1. **src/cli.rs** (30min)
   - Define CliArgs struct with subcommands
   - Argument validation
   - Help text

2. **src/reporter.rs** (45min)
   - VerificationReport struct
   - Receipt aggregation logic
   - JSON output formatter

3. **src/main.rs** (1h 15min)
   - Gate execution loop
   - Time budget tracking
   - Subsystem invocation orchestration
   - Error handling

4. **tests/integration/cli_tests.rs** (30min)
   - Argument parsing tests
   - Help/version tests

5. **tests/integration/e2e_tests.rs** (1h)
   - Full gate execution
   - Receipt verification
   - Time budget validation

---

## Dependencies & Assumptions

**Assumptions**:
- All 9 subsystems (Agents 1-9) have completed their implementations
- Each subsystem has a public API that can be called independently
- Receipt storage path `/tmp/qlever-verification-receipts/` is writable
- Git integration (for baseline fetch) is available

**No Blocking Dependencies**:
- Agent 10 can be implemented in parallel with Agents 1-9
- Receipt aggregation is built on subsystem APIs, not their internal state

---

## Acceptance Criteria Checklist

- [ ] Cargo.toml configured with all 9 subsystems as dependencies
- [ ] `cargo build --release` succeeds, produces `qlever-verify` binary
- [ ] `qlever-verify contract` runs in <120s, emits receipts
- [ ] `qlever-verify regression` runs in <600s, enforces thresholds
- [ ] `qlever-verify full` runs in <3600s (or skips long tests)
- [ ] All receipts written to `/tmp/qlever-verification-receipts/`
- [ ] JSON report generated with correct schema
- [ ] Exit code: 0 if all blocking gates pass, 1 if any blocking gate fails
- [ ] `cargo test --all` passes in harness crate
- [ ] Integration tests validate CLI + receipt flow end-to-end

---

## Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| Subsystems not ready | Mock subsystem APIs; gate execution is sequential and can skip unavailable subsystems |
| Receipt aggregation collisions | Deduplicate by (timestamp, failure_class, query_id); document collision handling |
| Time budget overrun | Implement cancellation tokens; abort non-critical gates if budget exceeded |
| Receipt directory permissions | Ensure `/tmp/qlever-verification-receipts/` is created with permissive mode (0o777) |

---

## Summary

Agent 10 is a thin orchestration layer that:
1. **Parses** user intent (contract/regression/full)
2. **Invokes** subsystems in deterministic order
3. **Aggregates** receipts without modifying them
4. **Reports** results in standardized format
5. **Enforces** time budgets and blocking semantics

**No rework. Single pass. Deterministic output.**
