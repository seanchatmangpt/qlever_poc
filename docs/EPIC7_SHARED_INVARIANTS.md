# EPIC 7 — Shared Invariant Enforcement
## The C++ Execution Engine is Silent

**Shared Invariant**: The C++ execution engine processes all ingress silently, producing only deterministic artifacts (error codes, digests) or failing atomically.

### Invariant Dimensions

#### 1. Silence (No Logging in Hot-Path)
- No `std::cout`, `std::cerr` in execution path
- No `printf` in hot-path
- No `LOG_*` macros (LOG_DEBUG, LOG_INFO, LOG_ERROR, etc.)
- No `fmt::format` calls that produce side effects
- No telemetry, metrics collection, or diagnostics during parsing

**Enforcement**: CI gate via `.clang-tidy` rules (Agent 7)
**Verification**: Conformance test suite (Agent 8)

#### 2. Error Codes Only (No Exceptions, No Strings)
- All errors reported via `IngressErrorCode` enum (not exceptions)
- Error codes are `uint16_t`, deterministic and machine-independent
- No exception throwing in hot-path
- Human-readable descriptions exist only in cold-path
- `error_description()` function available for logging (cold-path only)

**Implementation**: ErrorCodes.h enum with 600+ codes (Agent 5)
**Verification**: ErrorCodeTests conformance suite (Agent 8)

#### 3. Deterministic Artifacts (Same Input → Same Output)
- SHA256 digests computed deterministically
- Canonical normalization: alphabetical field ordering, UTF-8 NFC
- No floating-point rounding variability
- No hash order randomization
- Digest reproducible across machines, architectures, QLever versions

**Implementation**: IngressDigest.cpp with canonical serialization (Agent 6)
**Verification**: DeterminismTests (100 runs per test fixture) (Agent 8)

#### 4. Atomic Failure (Error Stops Processing)
- Single error code halts ingress pipeline
- No partial results on error
- Failure is visible and reportable (via error code)
- No silent corruption or undefined behavior

**Implementation**: All ingress methods return error code; no partial results
**Verification**: ErrorCodeTests::AllErrorCodesDocumented (Agent 8)

#### 5. Conformance Over Logs (Tests Replace Diagnostics)
- Runtime behavior verified by conformance test suite (1000+ tests)
- No diagnostic logging needed (tests provide coverage)
- Test suite is deterministic and reproducible
- All edge cases covered in fixtures (100+ documents)

**Implementation**: test_*.cpp suite (Agent 8)
**Verification**: `make test` returns 0, 100% pass rate

---

## Integration Checklist

- [x] All subsystems implemented (Agents 1-9 complete)
- [x] Simdjson vendored and integrated (Agent 1)
- [x] Ingress wrapper interface defined (Agent 2)
- [x] Error codes complete (Agent 5)
- [x] Hot-path audit completed (Agent 3)
- [x] Logging removed from hot-path (Agent 4)
- [x] CI enforcement rules active (Agent 7)
- [x] Deterministic digests implemented (Agent 6)
- [x] Conformance test suite complete (Agent 8)
- [x] FMEA analysis complete (Agent 9)
- [x] Documentation written (Agent 10)
- [ ] Build verification (pending `make clean && make`)
- [ ] Integration sign-off (pending all tests passing)

---

## Enforcement Mechanisms

1. **CI Gate** (Agent 7): `.clang-tidy` blocks forbidden symbols
2. **Test Suite** (Agent 8): 1000+ tests replace diagnostics
3. **Error Codes** (Agent 5): All failures reported via codes
4. **Determinism Verification** (Agent 6): Digest equality across runs
5. **Specification** (Agent 9): FMEA validates all mitigations

