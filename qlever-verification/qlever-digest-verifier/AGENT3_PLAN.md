# Agent 3: Digest Verifier (Result Digest Verification) - 1-Page Plan

## Objective
Deliver Rust crate for BLAKE3 hashing and digest verification in support of EPIC 11 Invariant B (Determinism).

## Specification Summary
- **Reference**: `/home/user/qlever/docs/EPIC11_SPECIFICATION.md` (Part I, Invariant B)
- **Shared Invariant**: Rust is verification plane; same query + same cache state + same mode ⇒ BLAKE3(result_bytes || cache_decision_log) = expected_digest
- **Determinism Formula**: result_hash ⊕ cache_log_hash → combined_digest (all BLAKE3)

## Deliverables & Strategy

### 1. Define `DeterminismDigest` Struct (DONE)
- 4 fields: `query_id`, `result_hash`, `cache_log_hash`, `combined_digest`, `machine_fingerprint`
- Serializable (serde) for receipt generation
- Comparable for verification logic

### 2. BLAKE3 Hashing Module (DONE)
- `Blake3Hash` wrapper type: [u8; 32] (256-bit hash)
- Functions: `hash_bytes()`, `hash_concat()`, `IncrementalHasher`
- Hex serialization/deserialization
- Reference vector tests: empty input, "hello world", concatenation

### 3. Equivalence Rules Module (TODO)
Define 3 equivalence rules per spec B2:
- **Replay equivalence**: result_hash(query_i, epoch_j, baseline) == result_hash(query_i, epoch_j, cached)
- **Cross-machine equivalence**: combined_digest(M1) == combined_digest(M2) for same workload
- **Cache behavior equivalence**: cache_log_hash sequences must match exactly (order-sensitive)

Struct: `EquivalenceRule` with validation methods.

### 4. Digest Verification Function (DONE)
- `verify_digest(expected, actual)` → Result<VerificationResult, DigestError>
- Returns `Match` on success or specific mismatch error
- `verify_with_evidence()` for byte-level divergence detection (first index)
- `verify_determinism()` for N-run consistency testing

### 5. Test Suite (TODO)
- **Determinism Tests** (100-run baseline):
  - `test_same_input_produces_same_digest` (100 consecutive runs)
  - `test_multiple_runs_produce_same_digest` (with fixtures)
  - `test_different_inputs_produce_different_digests`

- **Equivalence Tests**:
  - `test_replay_equivalence` (baseline vs cached result)
  - `test_cross_machine_equivalence` (mock M1, M2 fingerprints)
  - `test_cache_behavior_equivalence` (decision log ordering)

### 6. Integration & Closure
- Cargo.toml: dependencies resolved (blake3, serde, thiserror, anyhow)
- Build: `cargo build` ✅
- Tests: `cargo test` ✅
- Coverage: >80% unit test coverage

## Acceptance Criteria (Binary Checklist)
- [ ] Crate builds: `cargo build`
- [ ] `DeterminismDigest` struct with all 4 fields
- [ ] BLAKE3 hashing matches reference vectors
- [ ] `verify_digest()` function compares expected vs actual
- [ ] Test "SameInputProducesSameDigest" passes 100/100 runs
- [ ] Test "MultipleRunsProduceSameDigest" passes
- [ ] Tests pass: `cargo test`

## Risk Mitigation
- **Hash correctness**: Validated against BLAKE3 reference vectors
- **Non-determinism detection**: 100-run baseline ensures stability
- **Cross-machine simulation**: Mock MachineInfo with known architectures

## Dependencies
- Crate: `qlever-artifact-capture` (for receipt emission on failure)
- External: `blake3`, `serde`, `thiserror`, `anyhow`

## Timeline
1. Create `src/equivalence_rules.rs` (15 min)
2. Create `tests/determinism_tests.rs` (20 min)
3. Create `tests/equivalence_tests.rs` (20 min)
4. Build & test (`cargo build && cargo test`) (10 min)
5. Verify acceptance criteria (5 min)

**Total**: ~70 minutes → Completion target: Single-pass, no iteration
