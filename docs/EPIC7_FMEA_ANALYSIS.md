# EPIC 7 — FMEA Analysis
## Failure Mode and Effects Analysis

**Date**: 2026-01-01  
**Specification**: EPIC 7 (SIMD Ingress & Hot-Path Purification)

---

## Failure Mode Analysis Table

| # | Failure Mode | Cause | Effect | Severity | Mitigation | Mitigation Owner | Status |
|---|--------------|-------|--------|----------|-----------|-----------------|--------|
| 1 | Embedded Logging in Hot-Path | LOG_* macro calls, std::cout in execution path | Branch misprediction, cache pollution, non-determinism | CRITICAL | Complete removal + CI enforcement | Agents 4, 7 | ✅ Planned |
| 2 | Multiple Ingress Dialects | Supporting Turtle, N3, SHACL simultaneously | Semantic drift, inconsistent validation | CRITICAL | JSON-LD only; route others to client | Agent 2 | ✅ Planned |
| 3 | Non-Deterministic Digest | Floating-point rounding, hash order variation | Reproduced parses produce different digests | CRITICAL | Canonical normalization, deterministic serialization | Agent 6 | ✅ Planned |
| 4 | Silent Parsing Failures | Missing error code propagation | Undetected corrupted ingress, no observability | CRITICAL | Error code in all return types | Agent 5 | ✅ Planned |
| 5 | Runtime Diagnostics in Hot-Path | Formatting diagnostic strings during execution | Heisenbugs, irreproducible benchmarks | HIGH | Replace with conformance tests + digests | Agents 8, 5 | ✅ Planned |
| 6 | Fallback Path Non-Determinism | Triggering non-SIMD parser inconsistently | Performance cliff, inconsistent results | HIGH | Explicit fallback gates, determinism test | Agent 2 | ✅ Planned |
| 7 | Scope Creep to Dialects | Pressure to support additional formats | Parser explosion, maintenance burden | MEDIUM | Maintain JSON-LD-only posture, specification enforcement | Spec (Agent 9) | ✅ Verified |
| 8 | Cache Invalidation Bugs | EPIC 7 digest changes affecting EPIC 4 cache | Silent cache corruption, incorrect results | HIGH | Cache keys unchanged (separate from digest), integration test | Agent 10 | ✅ Planned |

---

## Mitigation Evidence

### Mitigation 1: Embedded Logging Removal
- **Owner**: Agent 4 (Logging Removal), Agent 7 (CI Enforcement)
- **Proof**: Hot-path audit (Agent 3) + Removal log (Agent 4) + CI rules (Agent 7)
- **Test**: HotPathSilence conformance test (Agent 8)
- **Status**: Pending implementation

### Mitigation 2: JSON-LD-Only Ingestion
- **Owner**: Agent 2 (SIMD Ingress Wrapper)
- **Proof**: Specification § Subsystem 2 (single format requirement)
- **Test**: Multiple dialect rejection tests (Agent 8)
- **Status**: Pending implementation

### Mitigation 3: Deterministic Digests
- **Owner**: Agent 6 (Deterministic Digests)
- **Proof**: IngressDigest implementation + SHA256 canonical normalization
- **Test**: DeterminismTests::SameInputProducesSameDigest (Agent 8, 100 runs)
- **Status**: Pending implementation

### Mitigation 4: Error Code Propagation
- **Owner**: Agent 5 (Error Codes)
- **Proof**: IngressErrorCode enum (complete, 600+ codes)
- **Test**: ErrorCodeTests::AllErrorCodesDocumented (Agent 8)
- **Status**: Pending implementation

### Mitigation 5: Conformance Testing
- **Owner**: Agent 8 (Conformance Testing)
- **Proof**: 1000+ test cases replacing runtime diagnostics
- **Test**: All conformance tests pass (make test)
- **Status**: Pending implementation

### Mitigation 6: Fallback Determinism
- **Owner**: Agent 2 (SIMD Ingress Wrapper)
- **Proof**: Explicit fallback path gates + determinism verification
- **Test**: FallbackDeterminismTest (Agent 8)
- **Status**: Pending implementation

### Mitigation 7: Scope Enforcement
- **Owner**: Specification + Agent 9 (FMEA Validation)
- **Proof**: EPIC 7 specification § JSON-LD-only requirement
- **Test**: Integration checklist (Agent 10) confirms no dialect support
- **Status**: ✅ Verified (specification locked)

### Mitigation 8: Cache Integration Integrity
- **Owner**: Agent 10 (Integration)
- **Proof**: Integration checklist item 12 (no merge conflicts with EPIC 4)
- **Test**: Integration sign-off (Agent 10)
- **Status**: Pending implementation

---

## Risk Summary

| Risk Level | Count | Example |
|-----------|-------|---------|
| CRITICAL | 4 | Logging, Determinism, Silent Failures, Multiple Dialects |
| HIGH | 3 | Runtime Diagnostics, Fallback Non-Determinism, Cache Bugs |
| MEDIUM | 1 | Scope Creep |

**Overall Risk**: Controlled (all mitigations assigned, tracked, testable)

---

## Verification Checklist

- [x] 8 failure modes identified
- [x] Each mode assigned to a subsystem owner
- [ ] All mitigations implemented
- [ ] All evidence collected and verified
- [ ] No unmitigated failure modes remain

**Current Status**: Specification complete, implementation pending

