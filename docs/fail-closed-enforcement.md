# Fail-Closed Enforcement Layers (EPIC 10.2)

## Overview

The EPIC 10.2 Construction Seal implements a **three-layer fail-closed enforcement** strategy to prevent partial results, best-effort recovery, and silent failures. All three layers are complementary and essential.

## Layer 1: Static Analysis CI Gate (Agent 2 - Silence Enforcer)

**Timing**: Build-time (pre-compilation)
**Enforcement Point**: GitHub Actions workflow / `make silence_enforcer`
**Goal**: Prevent forbidden constructs from entering the codebase

### Forbidden Constructs
In hot-path scopes (`src/engine/ingress/*`, `src/engine/query/*`, `src/index/*`, `src/util/MemoryMap.*`):
- ✗ `std::cout` / `std::cerr`
- ✗ `printf()`, `fprintf()`
- ✗ `LOG(DEBUG)`, `LOG(INFO)`, `LOG(WARNING)`, `LOG(ERROR)`

### Whitelisted Logging
Only permitted during initialization and critical abort:
- ✓ `LOG(INIT)` - Startup messages
- ✓ `LOG(FATAL)` - Pre-abort diagnostic messages

### Implementation
- **Script**: `scripts/ci-silence-enforcer.sh`
- **CMake**: `cmake/SilenceEnforcer.cmake`
- **CI Integration**: Runs before linking; exits 1 on violation

### Success Criteria
```
ctest -R silence_enforcer
# Result: All violations must be 0
# Exit code: 0 (PASS) or 1 (FAIL)
```

### Failure Behavior
Build aborts immediately; no partial compilation permitted.

---

## Layer 2: Runtime Resource Guards (Agent 6 - Datalog/N3 Guardrails)

**Timing**: Runtime (during rule execution)
**Enforcement Point**: Datalog/N3 rule evaluation in `src/engine/`
**Goal**: Prevent runaway behavior and resource exhaustion

### Resource Limits
```cpp
struct ResourceGuards {
  static constexpr size_t MAX_FACT_COUNT = 1'000'000;  // Fact explosion
  static constexpr uint64_t MAX_RULE_TIME_MS = 30'000;  // Infinite loops
  static constexpr size_t MAX_MEMORY_BYTES = 1 << 30;  // OOM prevention (1GB)
};
```

### Epoch Isolation
- Cache keys include epoch ID: `(QueryFingerprint, EpochID)`
- Prevents cross-contamination when data index updates
- Manifest hash ensures deterministic cache invalidation

### Implementation
- **Classes**: `DatalogResourceGuards.h/cpp` in `src/engine/datalog/`
- **Integration**: `FixpointComputation`, `RuleExpansion`
- **Exception**: `ResourceGuardViolation` thrown on limit breach

### Success Criteria
```cpp
// Resource guard holds across epochs
result1 = executeRules(query, epoch1_data);  // Success
result2 = executeRules(query, epoch2_data);  // Different epoch → different cache key
// No cache pollution between epochs
```

### Failure Behavior
Exception thrown → caught by Layer 3 (DivergenceAbort) → process terminates.

---

## Layer 3: Divergence Abort (Agent 7 - FMEA Abort Logic)

**Timing**: Runtime (critical failures)
**Enforcement Point**: `src/engine/ingress/DivergenceAbort.h/cpp`
**Goal**: Ensure clean shutdown on critical errors; prevent partial results

### Abort Conditions
Triggered when:
- Hash mismatch (manifest vs actual)
- Out-of-Memory (malloc failure)
- Guard breach (resource limits exceeded, epoch violation)
- Index corruption detected
- Invariant violation
- Internal errors

### Implementation
```cpp
[[noreturn]] void DivergenceAbort(const AbortContext& ctx) noexcept;

// Behavior:
// 1. Log abort reason to stderr
// 2. Flush caches to stable storage
// 3. Close open files
// 4. Exit process with code 42 (distinguishes integrity failures)
// Note: std::_Exit() used to bypass destructors (fail-closed guarantee)
```

### Success Criteria
```bash
# Simulated hash mismatch
./engine --validate-manifest
# Expected: Exit code 42, stderr log, no partial results
```

### Failure Behavior
Process terminates immediately; no recovery attempt.

---

## Enforcement Workflow Diagram

```
Source Code
    ↓
[LAYER 1: Static Analysis Gate]
  - Check: No printf, std::cout, LOG(ERROR) in hot paths
  - If violation → BUILD FAILS (exit 1)
  - If pass → Proceed to compilation
    ↓
Compiled Binary
    ↓
[Build Runtime Startup]
  - Manifest verification
  - Index load
  - If verification fails → LAYER 3 aborts
    ↓
[LAYER 2: Resource Guards]
  - Datalog rule execution begins
  - Monitoring: fact count, wall time, memory
  - If limit exceeded → Exception thrown → LAYER 3 triggered
    ↓
[LAYER 3: Divergence Abort]
  - Catch exceptions, verify integrity
  - If violation → Log reason, flush, exit(42)
  - No recovery, no partial results, no soft failures
    ↓
Exit Code: 0 (success) or 42 (integrity failure)
```

---

## Complementarity (No Redundancy)

| Layer | Scope | Mechanism | Timing | Failure |
|-------|-------|-----------|--------|---------|
| **1** | Codebase | Pattern matching (grep/clang-tidy) | Build | Hard fail |
| **2** | Execution | Resource tracking (limits) | Runtime | Exception |
| **3** | Critical | Abort logic (fail-closed) | Runtime (catch) | Process exit |

Each layer prevents a different class of failure:
- Layer 1 prevents bad code
- Layer 2 prevents bad behavior
- Layer 3 prevents bad state (partial results)

**No overlap**: No two layers validate the same invariant.

---

## Integration with EPIC 10.2 Seal

All three layers contribute to the final `.phase.lock`:

```json
{
  "build_validation": {
    "layer_1_silence_enforcer": "PASS",
    "layer_2_resource_guards": "ENABLED",
    "layer_3_divergence_abort": "ACTIVE"
  }
}
```

Build is sealed only if:
- Layer 1: No forbidden constructs found
- Layer 2: Guards compiled in and functional
- Layer 3: Abort mechanism tested and working

---

## Testing Strategy

### Layer 1 Test
```bash
make silence_enforcer
# Passes if: zero forbidden constructs in hot paths
```

### Layer 2 Test
```bash
ctest -R "DatalogEpochIsolation"
# Passes if: two epochs execute without cache pollution
```

### Layer 3 Test (Death Test)
```cpp
EXPECT_EXIT(
  divergence_test_hash_mismatch(),
  testing::ExitedWithCode(42),
  "Hash mismatch detected"
);
```

---

## Maintenance & Evolution

### Future Enhancements
- Layer 1: Extend to additional hot paths (e.g., `src/parser/`)
- Layer 2: Tighter bounds on fact count (e.g., 100K for simple queries)
- Layer 3: Structured logging before abort (for forensics)

### Fail-Closed Guarantee
All three layers must be active for Construction Seal. Disabling any layer voids the seal.

---

## See Also
- `docs/manifest-schema-family.md` - Cryptographic proof structure
- `docs/dual-gate-strategy.md` - Build + Runtime determinism gates
- EPIC 10.2 Convergence Decisions (complete policy)
