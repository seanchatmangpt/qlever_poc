# Agent 8: SIMD Verifier - Implementation Plan

**Status**: Independent implementation on claude/rust-read-cache-verification-TWfE7
**Scope**: EPIC 11 Subsystem 8 (SIMD Equivalence Verification)
**Goal**: Verify SIMD equivalence across architectures (x86_64 AVX-512 ↔ ARM64 NEON ↔ scalar modes)

---

## Strategy (Single-Pass Compilation)

### 1. **SIMD Equivalence Formula** (Part I D1, formalized)
   - **Input**: Workload pack W, two architectures A1, A2, execution mode
   - **Invariant**: `run_workload(W, A1, Strict) == run_workload(W, A2, Strict)`
   - **Output**: Binary equivalence (identical BLAKE3 digests) or fail-closed with receipt
   - **Tolerance**: ZERO bytes divergence (bit-level exactness required)

### 2. **Architecture Comparison**
   - **x86_64**: AVX-512F, AVX2, scalar fallback detection
   - **ARM64**: NEON, scalar fallback detection
   - **Validation**: CPU feature detection via CPUID (x86) / HWCAP (ARM)
   - **Mode representation**: Enum(Avx512, Avx2, Neon, Scalar)

### 3. **SIMD Modes Definition**
   - **AVX-512F**: 512-bit SIMD operations, ZMM registers (Intel Skylake+, AMD Milan+)
   - **AVX2**: 256-bit SIMD operations, YMM registers (Haswell+, Zen+)
   - **NEON**: 128-bit SIMD operations, Q registers (ARM Cortex-A7+)
   - **Scalar**: Fallback; no SIMD (all architectures)
   - **Detection logic**: Check CPU flags, emit diagnostic if mismatch detected

### 4. **Cross-Architecture Comparison Logic**
   - **Baseline capture**: Execute on x86_64, record result digest + cache decision log
   - **Replay verification**: Execute same workload on ARM64 (native or QEMU)
   - **Digest comparison**: BLAKE3(result_bytes || cache_log) must match byte-for-byte
   - **Failure classification**: If divergence → emit receipt class `ArchitectureDivergence`
   - **Inheritance**: EPIC 10.3 SIMD test suite validates AVX-512 ≈ NEON ≈ scalar; EPIC 11 validates this holds across machine boundaries

### 5. **Implementation Modules**
   - **`lib.rs`**: Main API `verify_simd_equivalence()` + types
   - **`cross_architecture.rs`**: `CrossArchComparison` struct, digest comparison, CPU feature detection
   - **`simd_modes.rs`**: `SimdMode` enum, CPU flag parsing, architecture inference
   - **Dependencies**: `qlever-digest-verifier` (BLAKE3), `qlever-artifact-capture` (receipts)

### 6. **Test Coverage**
   - **Unit tests**: CPU detection, mode parsing, digest formatting
   - **Integration tests**: Compile SIMD test vectors on both x86 (AVX-512) and scalar modes
   - **Cross-arch tests**: (Conditional) If dual-arch hardware or QEMU available, verify x86→ARM consistency
   - **Failure-closed**: All divergences emit receipts, no silent pass

### 7. **Acceptance Criteria (Binary Checklist)**
   - ✓ Crate builds: `cargo build`
   - ✓ `verify_simd_equivalence()` function present and callable
   - ✓ Test "SimdAvx512VsScalar" passes or fails-closed with receipt
   - ✓ Test "CrossArchitectureX86VsArm" passes (or skipped on single-arch, logged as advisory)
   - ✓ SIMD equivalence inherited from EPIC 10.3 (reference previous completion)
   - ✓ All tests pass: `cargo test`
   - ✓ Code compiles with zero warnings (2021 edition)

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Digest-based equivalence | BLAKE3 enforces exact bit-level equality; no floating-point tolerance |
| Zero tolerance (0 bytes divergence) | Per spec Invariant D1; SIMD correctness non-negotiable |
| Fail-closed on divergence | Receipt class `ArchitectureDivergence` blocks further execution |
| CPU detection via CPUID/HWCAP | Deterministic, no fallible assumptions about hardware |
| Cross-arch tests conditional | QEMU adds 5x overhead; test only on capable hardware; log advisory if skipped |

---

## Files to Create/Modify

| Path | Status | Purpose |
|------|--------|---------|
| `/qlever-verification/qlever-simd-verifier/src/lib.rs` | CREATE | Main API, types, entry point |
| `/qlever-verification/qlever-simd-verifier/src/cross_architecture.rs` | CREATE | x86_64 ↔ ARM64 comparison logic |
| `/qlever-verification/qlever-simd-verifier/src/simd_modes.rs` | CREATE | SimdMode enum, CPU feature detection |
| `/qlever-verification/qlever-simd-verifier/tests/simd_equivalence_tests.rs` | CREATE | AVX-512 vs scalar tests |
| `/qlever-verification/qlever-simd-verifier/tests/cross_arch_tests.rs` | CREATE | Cross-architecture tests |
| `/qlever-verification/qlever-simd-verifier/Cargo.toml` | EXISTS | Dependencies (serde, blake3, thiserror) |

---

## Implementation Order

1. **`simd_modes.rs`**: CPU feature detection, mode enum, parsing logic
2. **`lib.rs`**: Main API types, `verify_simd_equivalence()` scaffold
3. **`cross_architecture.rs`**: Digest comparison, architecture inference
4. **`simd_equivalence_tests.rs`**: Unit tests, simple vectors
5. **`cross_arch_tests.rs`**: Integration tests (conditional on dual-arch)
6. **Build & validate**: `cargo test`, zero failures

---

## Success Metric

**All tests pass** + **zero warnings** + **binary checklist 100%** = COMPLETION

**Failure path**: Any test fails → emit receipt (class `SimdScalarMismatch` or `ArchitectureDivergence`) and continue to next test; never silent abort.

---

**Agent**: 8 (SIMD Verifier)
**Start Date**: 2026-01-02
**Expected Duration**: Single-pass (no iteration)
**Closure Trigger**: All 5 acceptance criteria met + all tests passing
