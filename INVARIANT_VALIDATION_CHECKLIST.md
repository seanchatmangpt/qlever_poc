# Invariant Validation - Implementation Checklist

## VALIDATION REPORT FILES CREATED

Generated files in `/home/user/qlever/`:

1. **INVARIANT_VALIDATION_REPORT.md** (Main comprehensive report)
   - 600+ lines of detailed analysis
   - Component-by-component breakdown
   - Critical findings and recommendations
   - Roadmap to full validation

2. **INVARIANT_VALIDATION_EXECUTIVE_SUMMARY.txt** (Quick reference)
   - One-page visual summary
   - Component status scorecard
   - Critical findings list
   - Immediate action items

3. **INVARIANT_VALIDATION_DETAILED_FINDINGS.md** (Code reference)
   - Exact file paths and line numbers
   - Code snippets for each finding
   - Examples of problems and fixes
   - Template patterns for refactoring

---

## VALIDATION RESULTS SUMMARY

### Current State: MIXED (~30% Invariant-Based, ~70% Execution-Based)

```
PURE VALIDATORS (PASS):
  ✓ validate-receipts.sh
  ✓ verify-seal.sh
  ✓ C++ Unit Tests (static_assert cases)

PARTIAL VALIDATORS (PARTIAL PASS):
  ~ validate-invariants.sh (needs atomicity)
  ~ C++ Unit Tests (runtime cases - good but could improve)

EXECUTORS (NOT VALIDATORS - FAIL):
  ✗ seal-artifacts.sh (should be split)
  ✗ Makefile phases (should separate proof/exec/validate)
  ✗ ServerMain.cpp (should extract invariant layer)

ORCHESTRATORS (NOT VALIDATORS - FAIL):
  ✗ bb80-invariant-validator agent (describes rules, not structure)
  ✗ bb80-parallel-task-coordinator agent (choreography, not invariants)

ASPIRATIONAL (NOT OPERATIONALIZED - FAIL):
  ✗ Benchmark infrastructure (80/20 strategy without formal invariants)
```

---

## KEY METRICS

| Category | Metric | Current | Target |
|----------|--------|---------|--------|
| Pure Validators | Count | 3 (4 if counting verify) | 8+ |
| Branching Logic | In critical path | YES (compiler detection) | NO |
| Atomic Validation | Count | 2/5 scripts | 5/5 scripts |
| Determinism Proof | Status | Unverified | Verified |
| Monoidal Structure | Implemented | NO | YES |
| Explicit Invariants | Count | ~5 (implicit) | 20+ (explicit) |
| Self-Validation | Status | NO | YES |

---

## CRITICAL ISSUES RANKED BY IMPACT

### 1. DETERMINISM CLAIMS ARE UNVERIFIED (HIGH IMPACT)
- **Issue**: Code claims to be "deterministic" but compiler detection is environment-dependent
- **Impact**: Universe digests vary across machines
- **Fix Effort**: Low (add determinism test)
- **Files**: `/home/user/qlever/Makefile` line 34
- **Action**: Require explicit CXX compiler; add determinism test

### 2. MAKEFILE PHASES ARE CHOREOGRAPHY, NOT COMPOSITION (HIGH IMPACT)
- **Issue**: Phases mutate state instead of proving properties
- **Impact**: Cannot replay universe from invariants; debugging is hard
- **Fix Effort**: Medium (refactor each phase)
- **Files**: `/home/user/qlever/Makefile` lines 29-142
- **Action**: Split each phase into proof/exec/validate targets

### 3. AGENTS DESCRIBE EXECUTION, NOT STRUCTURE (HIGH IMPACT)
- **Issue**: Agents use choreography verbs (spawn, synchronize) not invariant specs
- **Impact**: Agents cannot validate themselves; circular dependencies
- **Fix Effort**: Medium (rewrite agent specs)
- **Files**: `/home/user/qlever/.claude/agents/bb80-*.md`
- **Action**: Add formal invariant definitions to agents

### 4. SOURCE CODE IS IMPERATIVE, NOT INVARIANT-DRIVEN (MEDIUM IMPACT)
- **Issue**: ServerMain.cpp has 150+ lines of conditional setup; no invariant proofs
- **Impact**: Invariants are implicit in tests, not explicit in code
- **Fix Effort**: Medium (extract invariant layer)
- **Files**: `/home/user/qlever/src/ServerMain.cpp` lines 29-100+
- **Action**: Create ServerInvariant struct with formal specifications

### 5. seal-artifacts.sh CONFLATES PROOF AND EXECUTION (MEDIUM IMPACT)
- **Issue**: Script validates AND mutates state in same operation
- **Impact**: Cannot run multiple times (not idempotent)
- **Fix Effort**: Low (split into two scripts)
- **Files**: `/home/user/qlever/scripts/seal-artifacts.sh`
- **Action**: Create separate can-seal-artifacts.sh (proof) and seal-artifacts.sh (exec)

### 6. BENCHMARK INVARIANTS NOT OPERATIONALIZED (MEDIUM IMPACT)
- **Issue**: 80/20 strategy is narrative, not formal specification
- **Impact**: Benchmarks are aspiration, not proof
- **Fix Effort**: Low (add formal bounds)
- **Files**: `/home/user/qlever/benchmark/80-20_THESIS_STRATEGY.md`
- **Action**: Define BENCHMARK_INVARIANT_1, BENCHMARK_INVARIANT_2, etc.

---

## BLOCKERS TO FULL INVARIANT VALIDATION

### Blocker 1: No Self-Referential Invariant Validation
**Description**: The invariant validator cannot validate itself
**Current State**: Agents describe external validation rules only
**Required**: Add invariant about invariant validators themselves
**Blocking**: Cannot prove invariant set is complete without self-validation
**Fix**: Add SELF_REFERENTIAL_INVARIANT to agent specs

### Blocker 2: Monoidal Composition Not Formally Defined
**Description**: Code claims "monoidal composition" but never defines monoid
**Current State**: Phases are sequential executors, not monoidal operations
**Required**: Define domain, operation, identity, associativity proofs
**Blocking**: Cannot prove single-pass feasibility without monoid structure
**Fix**: Formally define monoid for universe construction

### Blocker 3: Determinism Not Verified
**Description**: "Deterministic" claimed but no proof provided
**Current State**: Environment-dependent compiler detection exists
**Required**: Add test that proves bit-identical reproducibility
**Blocking**: Cannot claim deterministic receipts without verification
**Fix**: Add scripts/validate-determinism.sh that runs make twice and compares digests

### Blocker 4: Implicit Sequential Dependencies
**Description**: Phase dependencies are hidden in Makefile make rules
**Current State**: universe → phase-a → phase-b → ... implicit
**Required**: Make dependencies explicit; document what each phase requires/produces
**Blocking**: Cannot prove phases are independent or necessary
**Fix**: Add explicit requirements and artifacts documentation for each phase

### Blocker 5: Branching Logic in Critical Path
**Description**: Runtime environment branches in phase-a (compiler detection)
**Current State**: if clang_available; then use clang; else use g++
**Required**: Require explicit compiler specification
**Blocking**: Cannot guarantee determinism with environment-dependent branches
**Fix**: Change to: require CXX variable; fail if not specified

---

## QUICK WINS (Low Effort, High Impact)

### Quick Win 1: Add Determinism Test
**Effort**: 30 minutes
**Impact**: Proves "deterministic" claim is true or false
**File to create**: `/home/user/qlever/scripts/validate-determinism.sh`

```bash
#!/bin/bash
make clean >/dev/null 2>&1
make universe >/dev/null 2>&1
DIGEST1=$(sha256sum .artifacts/manifest.sha256 | awk '{print $1}')

make clean >/dev/null 2>&1
make universe >/dev/null 2>&1
DIGEST2=$(sha256sum .artifacts/manifest.sha256 | awk '{print $1}')

if [ "$DIGEST1" = "$DIGEST2" ]; then
    echo "DETERMINISM_VERIFIED"
    exit 0
else
    echo "DETERMINISM_FAILED: $DIGEST1 != $DIGEST2"
    exit 1
fi
```

### Quick Win 2: Make validate-invariants.sh Atomic
**Effort**: 15 minutes
**Impact**: Eliminates partial validation states
**File to modify**: `/home/user/qlever/scripts/validate-invariants.sh`

**Change**:
- Remove individual check_condition() calls
- Collect all checks into single result variable
- Report once at end (all pass or all fail)

### Quick Win 3: Document Invariants Explicitly
**Effort**: 1 hour
**Impact**: Makes implicit invariants visible for validation
**File to create**: `/home/user/qlever/INVARIANT_SPECIFICATION.md`

```markdown
# Invariant Specification for QLever Universe

## Invariant 1: Makefile Structure
- Must have "universe" target as single entry point
- Must have "phase-a" through "phase-f" targets
- Each phase target must have set -e semantics

## Invariant 2: Artifact Immutability
- All artifacts in .artifacts/ are reproducible
- All artifacts are sealed (manifest with permissions 444)
- All artifacts can be re-verified with verify-seal.sh

## Invariant 3: Deterministic Receipts
- Build produces bit-identical artifacts across runs
- Benchmarks have deterministic digests
- Determinism is verified by validate-determinism.sh
```

### Quick Win 4: Separate Validators from Executors
**Effort**: 2 hours
**Impact**: Clear separation of concerns; enables reuse
**Files to reorganize**:
```
scripts/
  ├── validators/
  │   ├── validate-invariants.sh
  │   ├── validate-receipts.sh
  │   ├── validate-determinism.sh
  │   └── verify-seal.sh
  ├── executors/
  │   └── seal-artifacts.sh
```

---

## IMPLEMENTATION ROADMAP

### PHASE 1: IMMEDIATE (This Week)
- [ ] Create INVARIANT_SPECIFICATION.md (document implicit invariants)
- [ ] Add validate-determinism.sh (prove determinism or fail fast)
- [ ] Make validate-invariants.sh atomic (all checks pass/fail together)
- [ ] Require explicit CXX compiler (remove if-else branch)

**Effort**: 4-6 hours
**Benefit**: Eliminates 3 critical blockers; provides foundation for phase 2

### PHASE 2: SHORT-TERM (Next 2 Weeks)
- [ ] Refactor Makefile phases (split proof/exec/validate)
- [ ] Split seal-artifacts.sh into two operations
- [ ] Define monoid explicitly for universe construction
- [ ] Add ServerInvariant struct to ServerMain.cpp
- [ ] Operationalize benchmark invariants with hard bounds

**Effort**: 10-15 hours
**Benefit**: Full separation of concerns; enables reusability

### PHASE 3: ONGOING (Continuous)
- [ ] Refactor agents to describe invariants (not choreography)
- [ ] Add property-based tests to expand coverage
- [ ] Make state reconstruction testable and visible
- [ ] Create invariant validation CI/CD checks
- [ ] Document design decisions in invariant catalog

**Effort**: Open-ended
**Benefit**: Complete invariant-driven architecture

---

## SUCCESS CRITERIA

### Criterion 1: All Pure Validators Identified and Documented
**Current**: 3 identified (validate-receipts.sh, verify-seal.sh, unit tests)
**Target**: 8+ pure validators across codebase
**Verification**: Can list each validator, its proof, and reusability

### Criterion 2: Determinism Verified
**Current**: Claimed but unverified
**Target**: validate-determinism.sh passes (bit-identical across runs)
**Verification**: Run make universe twice, compare manifests

### Criterion 3: No Branching in Critical Path
**Current**: YES (compiler detection in phase-a)
**Target**: NO branching in critical paths
**Verification**: Grep for "if/else" in critical phase scripts

### Criterion 4: All Invariants Explicit
**Current**: ~5 implicit invariants spread across files
**Target**: 20+ explicit invariants in INVARIANT_SPECIFICATION.md
**Verification**: Can trace each invariant to specific code

### Criterion 5: Proof/Execution/Validation Separated
**Current**: Mixed (seal-artifacts.sh, Makefile phases)
**Target**: All operations split into three targets
**Verification**: Each component has -proof, -exec, -validate variants

### Criterion 6: Monoidal Structure Proven
**Current**: Claimed in skill; not implemented
**Target**: Formal proof that phases form monoid
**Verification**: Can write mathematical proof in INVARIANT_SPECIFICATION.md

---

## VALIDATION SCORECARD (Current)

```
Component                           Status  Grade  Action
──────────────────────────────────────────────────────────
Orchestration Agents                FAIL    D      Reframe
Validation Scripts (overall)         MIXED   C      Fix atomicity
Build System (Makefile)              FAIL    D      Split phases
C++ Tests                            PASS    B      Expand coverage
Source Code Architecture             FAIL    D      Extract invariants
Benchmark Infrastructure             FAIL    D      Operationalize
Determinism Proofs                   FAIL    F      Add test
Monoidal Composition                 FAIL    F      Formalize
Pure Validators Count                MIXED   C      Add more
──────────────────────────────────────────────────────────
OVERALL                              MIXED   C+     See roadmap
```

---

## REFERENCE: PURE VALIDATOR TEMPLATE

All pure validators should follow this pattern:

```bash
# Pure existence validator template
validate_something() {
    local input="$1"

    # Check 1: Required input exists
    [[ -f "${input}" ]] || return 1

    # Check 2: Input satisfies invariant 1
    # ... check invariant 1 ...
    [[ condition1 ]] || return 1

    # Check 3: Input satisfies invariant 2
    # ... check invariant 2 ...
    [[ condition2 ]] || return 1

    # All invariants satisfied
    return 0
}

# Usage:
if validate_something /path/to/artifact; then
    echo "INVARIANT_HOLDS"
else
    echo "INVARIANT_VIOLATED"
fi
```

**Key Properties**:
- No file mutations (read-only)
- Atomic result (0 or 1)
- Deterministic and repeatable
- Composable with other validators
- Can be used in CI/CD checks

---

## CONCLUSION

The QLever codebase has **excellent atoms** (pure validators, deterministic tests) but **lacks coherence** between atoms. The path to full invariant validation requires:

1. **Making determinism verifiable** (quick win: validate-determinism.sh)
2. **Separating proof/execution/validation** (medium effort: refactor phases)
3. **Documenting invariants explicitly** (low effort, high value)
4. **Formalizing monoidal composition** (medium effort, high value)
5. **Achieving self-referential validation** (high effort, foundational)

**Recommendation**: Focus on Phase 1 quick wins first (4-6 hours). They unblock everything else.

