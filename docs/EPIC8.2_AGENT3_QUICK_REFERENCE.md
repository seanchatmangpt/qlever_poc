# EPIC 8.2 Agent 3: Recovery Collapse Enforcer - Quick Reference

**TL;DR**: Recovery = multiple execution paths after failure. ILLEGAL under EPIC 8. Single abort path only (exit 1).

---

## What is RECOVERY?

**DEFINITION**: Execution continues after failure without restarting from Phase A

**EXAMPLES**:
```cpp
// ILLEGAL: Exception recovery
try { operation(); }
catch (...) { log(); /* continues */ }

// ILLEGAL: Optional fallback
result = optional.value_or(default);

// LEGAL: Immediate abort
if (failed) { exit(1); }
```

---

## 6 Recovery Patterns (ALL ILLEGAL)

| Pattern | Example | Violations | Fix |
|---------|---------|------------|-----|
| Exception Catching | `catch {...}` | 370 | Replace with `exit(1)` |
| Optional Fallbacks | `.value_or()` | 40 | Use `.value()` (throws) |
| Retry Loops | `for (i=0; i<3; i++)` | 0 | Single attempt only |
| Config Defaults | `value_or(default)` | 49 | Make config mandatory |
| Algorithm Fallback | `if (fast) ... else ...` | 70 | Single algorithm |
| State Rollback | `rollback()` | 31 | No transactions |
| **TOTAL** | | **560** | |

---

## Collapse Rules (Pattern → Impossibility)

### CR-1: No Exception Recovery
```cpp
// BEFORE (2 paths)
try { op(); } catch (...) { log(); }

// AFTER (1 path)
try { op(); } catch (...) { exit(1); }
```

### CR-2: No Optional Fallbacks
```cpp
// BEFORE (2 paths)
x = opt.value_or(default);

// AFTER (1 path)
x = opt.value(); // throws if empty
```

### CR-3: No Retries
```cpp
// BEFORE (N paths)
for (i=0; i<MAX; i++) { if (try()) break; }

// AFTER (1 path)
if (!try()) exit(1);
```

### CR-4: No Config Defaults
```cpp
// BEFORE (2 paths)
cfg = exists ? load() : defaults();

// AFTER (1 path)
if (!exists) exit(1);
cfg = load();
```

### CR-5: No Algorithm Fallbacks
```cpp
// BEFORE (2 paths)
result = fast ? optimized() : slow();

// AFTER (1 path)
if (!prerequisites) exit(1);
result = optimized();
```

### CR-6: No Rollback
```cpp
// BEFORE (2 paths)
try { modify(); commit(); }
catch { rollback(); }

// AFTER (1 path)
if (!preconditions) exit(1);
modify(); // atomic
```

---

## How to Check for Violations

### Run Static Analysis
```bash
bash scripts/check_recovery_patterns.sh
# Expected output: 560 violations (current state)
# Target output: 0 violations (after Agents 4-10)
```

### Check Specific Pattern
```bash
# Exception catching
grep -r "catch\s*(" src/ | grep -v "exit\|terminate"

# Optional fallbacks
grep -r "\.value_or(" src/ benchmark/

# Config defaults
grep -r "value_or\|default" src/util/ConfigManager/
```

---

## How to Install Git Hook

```bash
# Copy hook to git directory
cp scripts/git-pre-commit-recovery-check.sh .git/hooks/pre-commit

# Make executable
chmod +x .git/hooks/pre-commit

# Test
git add <file>
git commit -m "test"
# Hook will reject commits with recovery patterns
```

---

## Integration with EPIC 8 Phases

### Phase D Enhancement
Add to `/home/user/qlever/Makefile`:
```makefile
phase-d: phase-c
	@echo "PHASE_D: Rule & constraint enforcement" >&2
	@bash scripts/check_recovery_patterns.sh || exit 1
```

**Effect**: Construction fails if recovery patterns detected

---

## Priority Fixes

### Critical (Agent 4)
- **File**: `src/util/ExceptionHandling.h`
- **Functions**: `ignoreExceptionIfThrows`, `ThrowInDestructorIfSafe`
- **Action**: DELETE (370 call sites must be refactored)

### High (Agent 5)
- **File**: `benchmark/JoinAlgorithmBenchmark.cpp`
- **Lines**: 613, 1125, 1560
- **Action**: Replace `.value_or()` with `.value()`

### High (Agent 7)
- **File**: `src/util/ConfigManager/ConfigManager.cpp`
- **Action**: Remove all default values; make config mandatory

### Medium (Agent 8)
- **File**: `src/engine/SpatialJoinAlgorithms.cpp`
- **Action**: Consolidate to single join algorithm

---

## Theoretical Foundation (One-Liner)

**Graph**: Single path START → PHASE_A → ... → SEAL → EXIT_SUCCESS; all failures → EXIT_FAILURE

**Monoid**: FAILURE is annihilator; no transformation FAILURE → SUCCESS permitted

**Type**: Result<T,E> (sum type) → Required<T> (identity type)

---

## Agent 4-10 Workflow

```
Wave 1 (Parallel):
  Agent 4: Exception handlers (370 violations)
  Agent 5: Optional fallbacks (40 violations)
  Agent 7: Config defaults (49 violations)
  Agent 8: Algorithm fallbacks (70 violations)

Wave 2 (Sequential):
  Agent 9: Integrate enforcement (PHASE D, git hook)

Wave 3 (Sequential):
  Agent 10: Final verification (0 violations)
```

**Total Time**: ~70 minutes
**Parallelism**: 70%

---

## Key Files

| File | Purpose |
|------|---------|
| `docs/EPIC8.2_AGENT3_RECOVERY_COLLAPSE_ENFORCER.md` | Full specification (1,100+ lines) |
| `docs/EPIC8.2_AGENT3_SUMMARY_REPORT.md` | Executive summary |
| `docs/EPIC8.2_AGENT3_QUICK_REFERENCE.md` | This file (quick lookup) |
| `scripts/check_recovery_patterns.sh` | Automated violation detection |
| `scripts/git-pre-commit-recovery-check.sh` | Git commit validation |
| `.artifacts/recovery_analysis/*.txt` | Violation reports (6 files) |

---

## Common Questions

**Q: Why is recovery illegal?**
A: EPIC 8 requires fail-closed semantics. Multiple execution paths violate deterministic construction.

**Q: What about destructors?**
A: Use `terminateIfThrows` (calls exit(1)). Never use `ignoreExceptionIfThrows` or `ThrowInDestructorIfSafe`.

**Q: What about optional configuration?**
A: All configuration must be explicit. No defaults. Freeze in Phase A.

**Q: Can I retry network operations?**
A: No. Single attempt only. If network fails, construction fails (exit 1).

**Q: What about tests?**
A: Tests may use recovery patterns. EPIC 8 applies to construction (Phases A-F) only.

**Q: How do I handle errors?**
A: Log error message to stderr, then exit(1). No continuation.

---

## Status

**Current Violations**: 560
**Target Violations**: 0
**Compliance**: 0% → 100% (after Agents 4-10)

**Next Action**: Launch Agents 4, 5, 7, 8 in parallel

---

**Document**: Quick Reference
**Version**: 1.0
**Date**: 2026-01-01
**Agent**: 3 of 10 (EPIC 8.2)
