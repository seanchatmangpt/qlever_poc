# Invariant Validation - Detailed Findings with Code References

## File: /home/user/qlever/INVARIANT_VALIDATION_DETAILED_FINDINGS.md

---

## FINDING 1: PURE VALIDATORS EXIST AND CAN BE REUSED

### Finding 1.1: validate-receipts.sh is a Pure Existence Validator

**File**: `/home/user/qlever/scripts/validate-receipts.sh`
**Lines**: 1-68
**Status**: PASS - Pure validator with atomic semantics

**Key Code Section** (lines 30-64):
```bash
# Atomic validation: fail on first mismatch
while IFS= read -r line || [[ -n "$line" ]]; do
    # Skip empty lines and comments
    [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue

    # Parse line: expected_hash filepath
    read -r expected_hash filepath <<< "$line"

    # Validate parsing
    if [[ -z "$expected_hash" || -z "$filepath" ]]; then
        echo "VALIDATE_RECEIPTS_ERROR: invalid manifest entry: $line" >&2
        exit 1
    fi

    # Verify file exists
    if [[ ! -f "$filepath" ]]; then
        echo "VALIDATE_RECEIPTS_ERROR: artifact not found: $filepath" >&2
        exit 1
    fi

    # Compute current digest
    current_hash=$(sha256sum "$filepath" 2>/dev/null | awk '{print $1}')

    # Atomic comparison: fail immediately on mismatch
    if [[ "$expected_hash" != "$current_hash" ]]; then
        echo "VALIDATE_RECEIPTS_ERROR: digest mismatch: $filepath" >&2
        exit 1
    fi
done < "$manifest_file"

# All validations passed
exit 0
```

**Invariant Proven**: "All entries in manifest satisfy: computed_hash == expected_hash"
**Why It's Pure**:
- No file mutations (read-only operations only)
- Atomic failure (line 58: fail on first mismatch)
- Deterministic output (exit 0 or 1)
- No side effects beyond exit code

**Can Be Reframed As**: "Does a manifest exist where ALL digests are valid?"
**Reusability**: YES - Use for ANY manifest validation

---

### Finding 1.2: verify-seal.sh is a Pure Integrity Validator

**File**: `/home/user/qlever/scripts/verify-seal.sh`
**Lines**: 1-299
**Status**: PASS - Pure validator with comprehensive checks

**Key Verification Functions** (lines 106-165):

```bash
# Verify artifact digest matches manifest
verify_artifact_digest() {
    local artifacts_dir="$1"
    local rel_path="$2"
    local expected_digest="$3"
    local full_path="${artifacts_dir}/${rel_path}"
    local computed_digest

    # Compute current digest
    computed_digest=$(sha256sum "${full_path}" | cut -d' ' -f1)

    # Compare with manifest
    if [[ "${computed_digest}" != "${expected_digest}" ]]; then
        log_mismatch "DIGEST_MISMATCH" "${rel_path}: expected ${expected_digest}, got ${computed_digest}"
        ((MISMATCH_COUNT++))
        return 1
    fi

    return 0
}

# Verify artifact permissions match manifest
verify_artifact_permissions() {
    local artifacts_dir="$1"
    local rel_path="$2"
    local expected_perms="$3"
    local full_path="${artifacts_dir}/${rel_path}"
    local actual_perms

    # Get actual permissions
    actual_perms=$(stat -c '%a' "${full_path}")

    # Compare with manifest
    if [[ "${actual_perms}" != "${expected_perms}" ]]; then
        log_mismatch "PERMISSION_MISMATCH" "${rel_path}: expected ${expected_perms}, got ${actual_perms}"
        return 1
    fi

    return 0
}
```

**Main Verification Loop** (lines 220-260):
```bash
while IFS= read -r line; do
    ((line_num++))

    # Skip comments and empty lines
    [[ "${line}" =~ ^# ]] && continue
    [[ -z "${line}" ]] && continue

    # Skip directory entries (checked separately later)
    [[ "${line}" =~ ^DIR ]] && continue

    # Parse manifest line: SHA256 FILENAME FILEMODE FILESIZE
    local sha256 rel_path filemode filesize
    read -r sha256 rel_path filemode filesize <<< "${line}"

    ((total_files++))

    # Verify artifact exists
    if ! verify_artifact_exists "${artifacts_dir}" "${rel_path}"; then
        continue
    fi

    # Verify digest (primary integrity check)
    if ! verify_artifact_digest "${artifacts_dir}" "${rel_path}" "${sha256}"; then
        continue
    fi

    # Verify permissions
    if ! verify_artifact_permissions "${artifacts_dir}" "${rel_path}" "${filemode}"; then
        continue
    fi

    # Verify size
    if ! verify_artifact_size "${artifacts_dir}" "${rel_path}" "${filesize}"; then
        continue
    fi

    # All checks passed for this artifact
    ((verified_files++))

done < <(grep -v '^#' "${manifest_file}" | grep -v '^DIR ' | grep -v '^$')
```

**Invariants Proven**:
- "All artifacts exist"
- "All artifacts have correct digests"
- "All artifacts have correct permissions"
- "All artifacts have correct sizes"
- "No extra files beyond manifest"

**Why It's Pure**:
- No mutations (read-only verification)
- Atomic semantics (collects all violations, exits once)
- Deterministic (same manifest + state → same result)
- Replayable (can verify same seal multiple times)

**Reusability**: YES - Use for ANY sealed state verification

---

### Finding 1.3: C++ Unit Tests Use Pure Compile-Time Proofs

**File**: `/home/user/qlever/test/BitUtilsTest.cpp`
**Lines**: 1-82
**Status**: PASS - Compile-time invariants with runtime verification

**Compile-Time Invariants** (lines 14-27):
```cpp
TEST(BitUtils, bitMaskForLowerBits) {
  static_assert(bitMaskForLowerBits(0) == 0);
  static_assert(bitMaskForLowerBits(1) == 1);
  static_assert(bitMaskForLowerBits(2) == 3);

  for (size_t i = 0; i < 64; ++i) {
    auto expected = static_cast<uint64_t>(std::pow(2, i)) - 1;
    ASSERT_EQ(bitMaskForLowerBits(i), expected);
  }
  ASSERT_EQ(bitMaskForLowerBits(64), std::numeric_limits<uint64_t>::max());

  for (size_t i = 65; i < 2048; ++i) {
    ASSERT_THROW(bitMaskForLowerBits(i), std::out_of_range);
  }
}
```

**Why It's Pure**:
- static_assert (lines 15-17): Proves invariant at compile-time (PURE)
- No side effects during compilation
- ASSERT_EQ (lines 19-27): Executable proof with deterministic data

**Invariants**:
- `bitMaskForLowerBits(i) == 2^i - 1` for all `i in [0, 64]`
- Exception thrown for `i > 64`

**Reusability**: YES - As formal specification and behavioral contract

---

### Finding 1.4: Makefile verify Target Can Be Pure Validator

**File**: `/home/user/qlever/Makefile`
**Lines**: 175-178
**Status**: PARTIAL (Could be pure)

**Current Implementation**:
```makefile
verify:
    @test -f $(PHASE_MANIFEST) || (echo "FATAL: Manifest not found"; exit 1)
    @sha256sum -c $(PHASE_MANIFEST) >/dev/null 2>&1 || (echo "FATAL: Artifact verification failed"; exit 1)
    @echo "Verification passed" >&2
```

**Why It's Mostly Pure**:
- Reads manifest file (no mutations)
- Validates checksums (read-only verification)
- Returns 0/1 based on validation result
- Deterministic and repeatable

**Can Be Reframed**: "Does completed phase satisfy integrity invariants?"
**Reusability**: YES - Verify any phase completion

---

## FINDING 2: VALIDATION SCRIPTS NEED ATOMICITY

### Finding 2.1: validate-invariants.sh Uses Sequential Condition Checking

**File**: `/home/user/qlever/scripts/validate-invariants.sh`
**Lines**: 1-323
**Status**: PARTIAL PASS - Good structure, needs atomicity

**Problem Pattern** (lines 73-86):
```bash
# Helper function to check condition and update report
check_condition() {
    local condition_name="$1"
    local check_result="$2"

    if [ "$check_result" -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $condition_name" >&2
        echo "[PASS] $condition_name" >> "${VALIDATION_REPORT}"
    else
        echo -e "${RED}[FAIL]${NC} $condition_name" >&2
        echo "[FAIL] $condition_name" >> "${VALIDATION_REPORT}"
        EXIT_CODE=1
    fi
}
```

**Issue**: Each check:
1. Writes to report file (mutation)
2. Reports individually
3. Creates partial state (some checks pass, some fail)

**Better Approach**:
```bash
# Collect all results; report atomically
collect_all_checks() {
    local results=0

    # Check 1
    test -f "${PROJECT_ROOT}/Makefile" || results=1

    # Check 2
    grep -q "^universe:" "${PROJECT_ROOT}/Makefile" || results=1

    # Check 3
    # ... all other checks ...

    # Atomic report: all pass or all fail
    return $results
}

# Single atomic exit
if collect_all_checks; then
    echo "INVARIANTS_SATISFIED"
    exit 0
else
    echo "INVARIANTS_VIOLATED"
    exit 1
fi
```

**Impact**: Current design allows partial validation states (some checks pass, others fail).
**Needed Change**: Make validation atomic (all checks pass or all fail).

---

### Finding 2.2: seal-artifacts.sh is an Executor, Not a Validator

**File**: `/home/user/qlever/scripts/seal-artifacts.sh`
**Lines**: 1-193
**Status**: FAIL - Should be split into proof + execution

**Problematic Pattern** (lines 64-146):
```bash
# Generate SHA-256 manifest for all artifacts
generate_manifest() {
    local artifacts_dir="$1"
    local manifest_path="$2"
    local temp_manifest

    # Use temporary file for atomic writes
    temp_manifest="${manifest_path}.tmp.$$"

    # Atomically fail if unable to write to temp file
    if ! touch "${temp_manifest}" 2>/dev/null; then
        error_exit "Cannot write to manifest directory: ${MANIFEST_DIR}" 1
        return 1
    fi

    # Generate checksums in sorted order for determinism
    {
        printf "# Artifact Seal Manifest\n"
        printf "# Directory: %s\n" "${artifacts_dir}"
        printf "# Generated: %s\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        printf "# Format: <SHA256_DIGEST> <FILENAME> <FILEMODE> <FILESIZE>\n"
        printf "\n"

        # Find all files, compute hashes, and record permissions
        find "${artifacts_dir}" -type f -print0 | sort -z | while IFS= read -r -d '' file; do
            # ... compute digests ...
        done

    } > "${temp_manifest}"

    # Atomic move: rename temp file to final location
    if ! mv "${temp_manifest}" "${manifest_path}" 2>/dev/null; then
        rm -f "${temp_manifest}" 2>/dev/null || true
        error_exit "Cannot atomically commit manifest to: ${manifest_path}" 1
        return 1
    fi

    # Set manifest permissions to immutable read-only (444)
    if ! chmod 444 "${manifest_path}" 2>/dev/null; then
        rm -f "${manifest_path}" 2>/dev/null || true
        error_exit "Cannot set manifest to read-only (444): ${manifest_path}" 1
        return 1
    fi
```

**Issues**:
1. Creates files (mutation)
2. Changes permissions (mutation)
3. Cannot be run multiple times (not idempotent)
4. Mixes validation with execution

**How to Split**:

```bash
# PART 1: Existence proof (pure - no mutations)
can_seal_artifacts() {
    local artifacts_dir="$1"
    test -d "${artifacts_dir}" || return 1
    test -r "${artifacts_dir}" || return 1
    # Check if all expected files exist
    find "${artifacts_dir}" -type f >/dev/null || return 1
    return 0
}

# PART 2: Execution (mutation)
seal_artifacts_now() {
    local artifacts_dir="$1"
    local manifest_path="$2"
    local temp_manifest="${manifest_path}.tmp.$$"

    # ... perform mutations ...
    # ... write manifest ...
    # ... set permissions ...

    return 0
}

# PART 3: Validation (pure - verify seal exists)
verify_seal_exists() {
    local manifest_path="$1"
    test -f "${manifest_path}" || return 1
    local perms=$(stat -c '%a' "${manifest_path}")
    [[ "${perms}" == "444" ]] || return 1
    return 0
}
```

**Impact**: Current design conflates proof-checking with state mutation.
**Needed Change**: Separate into three operations (proof/exec/validate).

---

## FINDING 3: MAKEFILE PHASES ARE EXECUTION, NOT INVARIANT PROOF

### Finding 3.1: Phase A Has Environment-Dependent Branching

**File**: `/home/user/qlever/Makefile`
**Lines**: 23-44
**Status**: FAIL - Not deterministic due to runtime detection

**Problematic Code** (lines 32-44):
```makefile
phase-a: ensure-build-dir
    @mkdir -p $(ARTIFACTS_DIR)
    @echo "PHASE_A: Toolchain sealing" >&2
    @$(SHELL) -c '\
        set -e; \
        if command -v clang++ >/dev/null 2>&1; then CXX=$$(command -v clang++); else CXX=$$(command -v g++); fi; \
        test -n "$$CXX" || exit 1; \
        CXXID=$$($$CXX -v 2>&1 | head -1); \
        echo "$$CXXID" > $(COMPILER_ID_FILE); \
        SOURCE_DATE_EPOCH=$$(git log -1 --format=%ct); \
        NORMALIZED_FLAGS="-std=c++20 -O3 -DNDEBUG"; \
        echo "NORMALIZED_CXXFLAGS=\"$$NORMALIZED_FLAGS\"" > $(ARTIFACTS_DIR)/flags.env; \
        echo "CXX=\"$$CXX\"" >> $(ARTIFACTS_DIR)/flags.env; \
        echo "SOURCE_DATE_EPOCH=\"$$SOURCE_DATE_EPOCH\"" >> $(ARTIFACTS_DIR)/flags.env; \
        exit 0 \
    '
```

**Issues**:
1. **Line 34**: `if command -v clang++` - Runtime branch based on environment
2. **Output varies**: Different machines will output different compiler IDs
3. **Not idempotent**: CXXID changes if compiler changes
4. **Source of non-determinism**: Compiler choice is environment-dependent

**Example Failure**:
```
Machine 1: CXX=/usr/bin/clang++ (Apple Clang version)
Machine 2: CXX=/usr/bin/g++ (GNU g++)
Result: Different COMPILER_ID_FILE contents → Different universe digests
```

**How to Fix** (make deterministic):
```makefile
# OPTION 1: Require explicit compiler
phase-a: ensure-build-dir
    @test -n "$(CXX)" || (echo "FATAL: CXX not set"; exit 1)
    @$(CXX) --version >/dev/null || (echo "FATAL: $(CXX) not found"; exit 1)
    @echo "$(CXX) --version" > $(COMPILER_ID_FILE)
    # ... rest of phase-a ...

# OPTION 2: Verify reproducibility
phase-a-verify: phase-a
    @CXXID=$$($(CXX) -v 2>&1 | head -1); \
     test "$$CXXID" = "$$(cat $(COMPILER_ID_FILE))" || exit 1
```

**Impact**: Current design produces different outputs on different machines.
**Needed Change**: Require explicit compiler specification, verify determinism.

---

### Finding 3.2: Phase E Claims Determinism But Doesn't Define Bounds

**File**: `/home/user/qlever/Makefile`
**Lines**: 106-120
**Status**: FAIL - "Deterministic" claim is unverified

**Current Code** (lines 112-120):
```makefile
phase-e: phase-d
    @echo "PHASE_E: Deterministic benchmarks" >&2
    @$(SHELL) -c '\
        set -e; \
        cd $(BUILD_DIR); \
        ctest --rerun-failed --output-on-failure >/dev/null 2>&1 || exit 1; \
        test -f CTestTestfile.cmake || exit 1; \
        exit 0 \
    '
```

**Issues**:
1. Claims "Deterministic Benchmarks" (line 113)
2. But just runs `ctest` with no variance bounds defined
3. No specification of what "deterministic" means
4. No proof that results are bit-identical across runs

**What "Deterministic" Should Mean**:
```
BENCHMARK_INVARIANT:
  For the same input data and environment:
    - Test result digests are bit-identical
    - Test result digests are immutable
    - Test variance is bounded (< 5%)
```

**How to Verify Determinism**:
```bash
#!/bin/bash
# Run phase-e twice
make phase-d
make phase-e
DIGEST1=$(sha256sum .artifacts/benchmarks.txt | awk '{print $1}')

make clean
make phase-d
make phase-e
DIGEST2=$(sha256sum .artifacts/benchmarks.txt | awk '{print $1}')

if [ "$DIGEST1" = "$DIGEST2" ]; then
    echo "PHASE_E_DETERMINISTIC"
else
    echo "PHASE_E_NOT_DETERMINISTIC"
    exit 1
fi
```

**Impact**: Current design claims determinism without proof.
**Needed Change**: Add determinism verification test.

---

### Finding 3.3: Phase F Artifact Sealing May Be Incomplete

**File**: `/home/user/qlever/Makefile`
**Lines**: 122-137
**Status**: PARTIAL - Good structure, completeness uncertain

**Current Code** (lines 128-137):
```makefile
phase-f: phase-e
    @echo "PHASE_F: Artifact sealing" >&2
    @$(SHELL) -c '\
        set -e; \
        cd $(BUILD_DIR); \
        find . -type f -executable -o -name "*.a" -o -name "*.so" 2>/dev/null | sort | xargs -I {} sh -c "test -f {} && sha256sum {} || true" > $(PHASE_MANIFEST) 2>/dev/null || true; \
        test -s $(PHASE_MANIFEST) || exit 1; \
        chmod 444 $(PHASE_MANIFEST); \
        exit 0 \
    '
```

**Issues**:
1. **find with -o operators**: May miss some artifacts
2. **Complex xargs condition**: `sh -c "test -f {} && sha256sum {} || true"`
3. **Completeness assumption**: Assumes find catches all important artifacts
4. **No validation**: Doesn't verify all expected artifacts are included

**Better Approach**:
```makefile
phase-f: phase-e
    @echo "PHASE_F: Artifact sealing" >&2
    @$(SHELL) -c '\
        set -e; \
        cd $(BUILD_DIR); \
        \
        # List expected artifacts
        EXPECTED="src/qlever src/queryCanonical/QueryCanonicalTest"; \
        \
        # Generate manifest
        {
            echo "# Sealed artifacts from phase-f"; \
            echo "# Generated: $$(date -u +%Y-%m-%dT%H:%M:%SZ)"; \
            find . -type f -executable -o -name "*.a" -o -name "*.so" | \
              sort | \
              while read f; do sha256sum "$$f"; done; \
        } > $(PHASE_MANIFEST); \
        \
        # Verify expected artifacts are present
        for exp in $$EXPECTED; do \
            grep -q "$$exp" $(PHASE_MANIFEST) || exit 1; \
        done; \
        \
        chmod 444 $(PHASE_MANIFEST); \
        exit 0 \
    '
```

**Impact**: Current design may seal incomplete artifact set.
**Needed Change**: Explicitly verify expected artifacts are included.

---

## FINDING 4: SOURCE CODE IS IMPERATIVE, NOT INVARIANT-DRIVEN

### Finding 4.1: ServerMain.cpp Has 150+ Lines of Conditional Setup

**File**: `/home/user/qlever/src/ServerMain.cpp`
**Lines**: 1-150
**Status**: FAIL - Imperative, no explicit invariant proofs

**Sample Conditional Logic** (lines 29-100):
```cpp
int main(int argc, char** argv) {
  setRuntimeParameter<&RuntimeParameters::stripColumns_>(true);
  qlever::version::copyVersionInfo();
  setlocale(LC_CTYPE, "");

  std::locale loc;
  ad_utility::ReadableNumberFacet facet(1);
  std::locale locWithNumberGrouping(loc, &facet);
  ad_utility::Log::imbue(locWithNumberGrouping);

  // Init variables that may or may not be
  // filled / set depending on the options.
  using ad_utility::NonNegative;

  std::string indexBasename;
  std::string accessToken;
  bool noAccessCheck = false;
  bool text = false;
  unsigned short port;
  NonNegative numSimultaneousQueries = 1;
  bool noPatterns;
  bool onlyPsoAndPosPermutations;
  bool persistUpdates;

  ad_utility::MemorySize memoryMaxSize;

  ad_utility::ParameterToProgramOptionFactory optionFactory{
      &globalRuntimeParameters};

  po::options_description options("Options for ServerMain");
  auto add = [&options](auto&&... args) {
    options.add_options()(AD_FWD(args)...);
  };
  add("help,h", "Produce this help message.");
  // ... many more option additions ...
  add("port,p", po::value<unsigned short>(&port)->required(),
      "The port on which HTTP requests are served (required).");
  add("access-token,a", po::value<std::string>(&accessToken)->default_value(""),
      "Access token for restricted API calls (default: no access).");
  add("no-access-check,n",
      po::bool_switch(&noAccessCheck)->default_value(false),
      "If set to true, no access-token check is performed for restricted API "
      "calls (default: false).");
  // ... more options ...
}
```

**Problems**:
1. Mutation-heavy: Creates mutable objects throughout
2. Conditional initialization: Options can be in many states
3. No invariant proofs: What MUST be true after initialization?
4. No self-documenting structure: Invariants are implicit

**Example Invariant Violation Risk**:
```cpp
// What if both are set?
add("no-access-check,n", ...);  // noAccessCheck = true
add("access-token,a", ...);     // accessToken = "secret"

// Code doesn't prove this invariant:
// INVARIANT: if noAccessCheck, then accessToken is ignored
```

**How to Refactor**:
```cpp
// Extract invariants explicitly
struct ServerInvariant {
    // INVARIANT: Port must be in valid range
    static constexpr unsigned short MIN_PORT = 1;
    static constexpr unsigned short MAX_PORT = 65535;
    static bool isValidPort(unsigned short p) {
        return p >= MIN_PORT && p <= MAX_PORT;
    }

    // INVARIANT: If noAccessCheck, accessToken is irrelevant
    static bool accessControlInvariant(
        bool noAccessCheck,
        const std::string& accessToken) {
        if (noAccessCheck) return true;  // accessToken ignored
        return !accessToken.empty();      // accessToken required
    }

    // INVARIANT: Memory limit must be positive
    static bool isValidMemoryLimit(size_t limit) {
        return limit > 0;
    }
};

// Then verify invariants during initialization
void initializeServer(const Options& opts) {
    // Check invariants BEFORE creating server
    VERIFY(ServerInvariant::isValidPort(opts.port));
    VERIFY(ServerInvariant::accessControlInvariant(
        opts.noAccessCheck,
        opts.accessToken));
    VERIFY(ServerInvariant::isValidMemoryLimit(
        opts.memoryMaxSize));

    // Now create server (mutation is allowed after invariants checked)
    Server server(opts);
}
```

**Impact**: Current design makes invariants implicit in code behavior.
**Needed Change**: Extract explicit invariant specifications.

---

## FINDING 5: BENCHMARK INVARIANTS ARE NOT OPERATIONALIZED

### Finding 5.1: 80/20 Strategy is Aspirational, Not Formal

**File**: `/home/user/qlever/benchmark/80-20_THESIS_STRATEGY.md`
**Lines**: 1-200
**Status**: FAIL - Meta-strategy without operational invariants

**Claimed Principle** (lines 4-8):
```markdown
This document outlines the **Pareto-optimized** benchmarking strategy for your
CONSTRUCT performance PhD thesis. By focusing on the **20% of benchmarks that
generate 80% of thesis impact**, you maximize research value with minimal
implementation overhead.
```

**Problem**: No formal definition of:
- What are the "20% of benchmarks"?
- How do we measure "80% of thesis impact"?
- What invariants must they satisfy?

**Example Aspiration** (lines 60-85):
```markdown
**Primary Benchmarks to Run:**

1. SelectVsConstructComparison
   - Effort: 1 benchmark execution
   - Value: Unique thesis contribution
   - Time: ~5 minutes

2. RealWorldDBpediaScale (500 movies)
   - Effort: 1 benchmark execution
   - Value: Credibility + practicality
   - Time: ~10 minutes
```

**Issue**: No formal invariants defined for these benchmarks!
- What is "SelectVsConstructComparison"? (no spec)
- How is "Value: Unique thesis contribution" measured? (no metric)
- Is benchmark deterministic? (no claim)

**How to Operationalize** (add formal invariants):
```markdown
# Operationalized Benchmark Invariants

## BENCHMARK_INVARIANT_1: SelectVsConstructComparison
Proof: Query results are bit-identical across 5 runs
Requirement: digest(run1) == digest(run2) == ... == digest(run5)
Implementation:
  - Run SelectVsConstructComparison 5 times
  - Compute SHA256 of result set for each run
  - Fail if any digest differs

## BENCHMARK_INVARIANT_2: RealWorldDBpediaScale
Proof: Throughput is stable within 5% variance
Requirement: (max_throughput - min_throughput) / avg_throughput < 0.05
Implementation:
  - Run RealWorldDBpediaScale 10 times
  - Measure throughput (queries/second)
  - Compute variance
  - Fail if variance > 5%

## BENCHMARK_INVARIANT_3: Critical 80% Pass
Proof: All critical benchmarks pass
Requirement: All benchmarks tagged "critical_80_percent" must pass
Implementation:
  - Mark critical benchmarks with tag
  - Run all tagged benchmarks
  - Fail if any critical benchmark fails
```

**Impact**: Current design is narrative-based, not invariant-based.
**Needed Change**: Define formal invariants with hard bounds.

---

## FINDING 6: AGENTS DESCRIBE EXECUTION, NOT STRUCTURE

### Finding 6.1: bb80-invariant-validator Uses Verbs of Execution

**File**: `/home/user/qlever/.claude/agents/bb80-invariant-validator.md`
**Lines**: 1-20
**Status**: FAIL - Describes validation rules, not self-referential structure

**Claimed Role** (lines 2-3):
```
name: bb80-invariant-validator
description: Validate that implementations maintain the minimal invariant set across all changes
```

**Problem**: The agent DESCRIBES how to validate OTHER systems, but CANNOT VALIDATE ITSELF.

**Current Structure** (lines 11-19):
```markdown
1. **Invariant Extraction**: Identify the minimal set of invariants that govern the system.
2. **Monoidal Composition Check**: Verify that implementation builds from invariants outward.
3. **Single-Pass Feasibility**: Confirm that implementation can execute in one pass.
4. **Deterministic Reconstruction**: Validate that any state in the system can be fully reconstructed.
```

**Gap**: No self-referential invariant!
- Who validates that the invariant-validator itself maintains invariants?
- How do we prove the validator is correct?
- Does the validator satisfy its own rules?

**How to Fix** (add self-reference):
```markdown
## SELF-REFERENTIAL INVARIANT

The invariant-validator ITSELF must satisfy:

1. **Validator Invariant**: The validator is itself an invariant proof
   - Input: Specification document
   - Output: Binary fact (spec is closed or not)
   - Proof: If output is "closed", universe is constructible

2. **No Circular Validation**: Validator cannot claim what cannot be proven
   - Cannot validate specifications it cannot formalize
   - Cannot prove determinism without determinism tests

3. **Evidence Required**: All validator claims must have evidence
   - If claiming "phase X is deterministic", must show determinism test
   - If claiming "artifact X is sealed", must show immutability proof
```

**Impact**: Agent claims to validate but provides no self-validation.
**Needed Change**: Add self-referential invariant proof.

---

### Finding 6.2: bb80-parallel-task-coordinator Uses Choreography Verbs

**File**: `/home/user/qlever/.claude/agents/bb80-parallel-task-coordinator.md`
**Lines**: 1-20
**Status**: FAIL - Describes orchestration, not invariant structure

**Claimed Role** (lines 2-3):
```
name: bb80-parallel-task-coordinator
description: Coordinate 10 concurrent agents operating independently under shared invariant
```

**Problematic Language** (lines 12-19):
```markdown
1. **Independent Agent Dispatch**: Spawn 10 agents immediately...
2. **Shared Invariant Enforcement**: Ensure all agents operate under the same invariant...
3. **Synchronization Point Detection**: Agents run independently until invariants stabilize...
4. **Concurrency Coverage**: Ensure 80% of the work surface is covered by concurrent execution...
```

**Issues**:
- "Spawn 10 agents" - HOW to spawn (execution detail)
- "Agents run independently" - Describes behavior, not invariant
- "Synchronization point detection" - Describes timing, not structure
- "Concurrency coverage" - Describes parallelism percentage, not invariant

**Gap**: No formal invariant about what "shared invariant stabilization" means

**How to Fix** (state formal invariant):
```markdown
## PARALLEL EXECUTION INVARIANT

Define "all agents operate under shared invariant" formally:

INVARIANT_STABILIZATION:
  ∀ agent ∈ {Agent_1, ..., Agent_N}:
    agent.result ∈ {INVARIANT_HOLDS, INVARIANT_VIOLATED}

  Stabilization occurs when:
    ∃ fact ∈ {INVARIANT_HOLDS, INVARIANT_VIOLATED}:
    ∀ agent ∈ {Agent_1, ..., Agent_N}:
      agent.result == fact

  (All agents must report the same fact)

PROOF:
  If agents report different results, either:
  1. Shared invariant is not truly shared
  2. Agents have incomplete information
  3. Specification is ambiguous
```

**Impact**: Agent provides choreography, not invariant specification.
**Needed Change**: State formal invariants, not execution procedures.

---

## FINDING 7: SEQUENCING ASSUMPTIONS ARE HIDDEN

### Finding 7.1: Makefile Phase Dependencies

**File**: `/home/user/qlever/Makefile`
**Lines**: 19, 52, 69, 96, 112, 128, 142
**Status**: FAIL - Implicit sequential dependencies

**Entry Point** (line 19):
```makefile
universe: verify-invariants phase-a phase-b phase-c phase-d phase-e phase-f artifact-seal
```

**Implicit Assumptions**:
1. phase-a must complete before phase-b (line 52: `phase-b: phase-a`)
2. phase-b must complete before phase-c (line 69: `phase-c: phase-b setup-dev-env`)
3. Dependencies are TRANSITIVE (phase-c implicitly depends on phase-a and phase-b)
4. Any phase failure stops entire pipeline

**Hidden Assumption**: "Phase X produces state that phase X+1 requires"

**Problem**: If this assumption is violated:
- Make succeeds with partial artifacts
- Downstream phases fail mysteriously
- Difficult to debug which phase is really broken

**How to Make Explicit**:
```makefile
# Document what each phase REQUIRES
phase-a-requires:
    @echo "phase-a requires:"
    @echo "  - Build directory exists"
    @echo "  - git repository is initialized"

# Document what each phase PRODUCES
phase-a-produces:
    @echo "phase-a produces:"
    @echo "  - $(COMPILER_ID_FILE) with compiler identity"
    @echo "  - $(ARTIFACTS_DIR)/flags.env with normalized flags"

# Verify requirements before executing
phase-b-verify-requires:
    @test -f $(COMPILER_ID_FILE) || (echo "FATAL: phase-a did not produce $(COMPILER_ID_FILE)"; exit 1)
    @test -f $(ARTIFACTS_DIR)/flags.env || (echo "FATAL: phase-a did not produce $(ARTIFACTS_DIR)/flags.env"; exit 1)
```

**Impact**: Current design hides dependencies; makes debugging hard.
**Needed Change**: Explicitly declare requirements and artifacts for each phase.

---

## FINDING 8: BRANCHING LOGIC IN CRITICAL PATHS

### Finding 8.1: Compiler Detection Uses Runtime Branch

**File**: `/home/user/qlever/Makefile`
**Lines**: 34
**Status**: FAIL - Environment-dependent branch

**Problematic Code**:
```bash
if command -v clang++ >/dev/null 2>&1; then CXX=$(command -v clang++); else CXX=$(command -v g++); fi;
```

**Why It's Problematic**:
1. Outcome depends on which compiler is installed
2. Two machines with different compilers produce different outputs
3. Not deterministic without explicit compiler specification

**Different Outcomes**:
```
Machine 1 (has clang):    CXX=/usr/bin/clang++
Machine 2 (only g++):     CXX=/usr/bin/g++
Result: Different COMPILER_ID_FILE → Different universe digests
```

**How to Fix** (require explicit specification):
```makefile
# Option 1: Require explicit CXX variable
CXX ?= $(error CXX variable must be specified)

phase-a: ensure-build-dir
    @$(CXX) --version >/dev/null || (echo "FATAL: $(CXX) not found"; exit 1)
    @$(CXX) -v 2>&1 | head -1 > $(COMPILER_ID_FILE)

# Usage: make CXX=/usr/bin/clang++ universe
```

**Or Option 2**: Detect and fail on ambiguity:
```makefile
phase-a: ensure-build-dir
    @CXX_CLANG=$$(command -v clang++ 2>/dev/null || echo ""); \
     CXX_GCC=$$(command -v g++ 2>/dev/null || echo ""); \
     if [ -n "$$CXX_CLANG" ] && [ -n "$$CXX_GCC" ]; then \
         echo "FATAL: Both clang++ and g++ available. Specify CXX explicitly."; \
         exit 1; \
     fi; \
     # ... proceed with unique compiler ...
```

**Impact**: Current design produces non-deterministic outputs.
**Needed Change**: Require explicit compiler specification.

---

## SUMMARY TABLE WITH FILE LOCATIONS

| Finding | File | Lines | Status | Issue | Fix |
|---------|------|-------|--------|-------|-----|
| Pure validator #1 | scripts/validate-receipts.sh | 1-68 | PASS | None | Keep as is |
| Pure validator #2 | scripts/verify-seal.sh | 1-299 | PASS | None | Keep as is |
| Pure validator #3 | test/BitUtilsTest.cpp | 1-82 | PASS | Could expand coverage | Add property-based tests |
| Needs atomicity | scripts/validate-invariants.sh | 73-86 | PARTIAL | Individual checks report separately | Collect all, report once |
| Executor not validator | scripts/seal-artifacts.sh | 64-146 | FAIL | Mutates state | Split into proof/exec/validate |
| Non-deterministic | Makefile | 34 | FAIL | Runtime compiler detection | Require explicit CXX |
| Unverified claim | Makefile | 112-120 | FAIL | Claims "deterministic" without bounds | Add determinism test |
| Incomplete sealing | Makefile | 128-137 | PARTIAL | May miss artifacts | Verify expected artifacts present |
| Imperative code | src/ServerMain.cpp | 29-100 | FAIL | No explicit invariant proofs | Extract ServerInvariant struct |
| Aspirational strategy | benchmark/80-20_THESIS_STRATEGY.md | 1-200 | FAIL | No operationalized invariants | Define formal invariant bounds |
| Agent validates externally | .claude/agents/bb80-invariant-validator.md | 1-20 | FAIL | No self-reference | Add self-referential invariant |
| Agent describes choreography | .claude/agents/bb80-parallel-task-coordinator.md | 1-20 | FAIL | Uses execution verbs | State formal invariants |
| Hidden dependencies | Makefile | 19-142 | FAIL | Implicit phase requirements | Make dependencies explicit |
| Environment-dependent branch | Makefile | 34 | FAIL | Conditional compiler choice | Require explicit compiler |

---

## CONCRETE EXAMPLES OF REFRAMING

### Example 1: From Execution-Oriented to Invariant-Oriented

**BEFORE** (Execution-oriented):
```bash
# Run validation checks
check_makefile_readable
check_entry_point
check_all_phases
check_fail_closed_semantics
# ... report individually ...
```

**AFTER** (Invariant-oriented):
```bash
# Prove invariant set is closed
prove_invariant_set_closed() {
    # Collect all checks
    local result=0
    test -f Makefile || result=1
    grep -q "^universe:" Makefile || result=1
    grep -q "^phase-[a-f]:" Makefile || result=1
    # ... all checks ...
    return $result
}

# Atomic report
if prove_invariant_set_closed; then
    echo "INVARIANT_SET_CLOSED"
    exit 0
else
    echo "INVARIANT_SET_OPEN"
    exit 1
fi
```

### Example 2: From Executor to Proof + Execution + Validation

**BEFORE** (Executor only):
```bash
seal_artifacts() {
    # Validation + execution + reporting mixed together
    if [[ ! -d "${artifacts_dir}" ]]; then
        error_exit "Directory not found"
        return 1
    fi
    # ... write manifest ...
    chmod 444 manifest
}
```

**AFTER** (Separated concerns):
```bash
# PART 1: Proof (can we do it?)
can_seal_artifacts() {
    [[ -d "${artifacts_dir}" ]] || return 1
    [[ -r "${artifacts_dir}" ]] || return 1
    return 0
}

# PART 2: Execution (do it)
seal_artifacts_now() {
    # ... perform mutations ...
    chmod 444 manifest
    return 0
}

# PART 3: Validation (did it work?)
verify_seal_created() {
    [[ -f "${manifest_path}" ]] || return 1
    local perms=$(stat -c '%a' "${manifest_path}")
    [[ "${perms}" == "444" ]] || return 1
    return 0
}
```

---

