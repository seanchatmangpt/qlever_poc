# EPIC 7 Specification Closure Audit

**Validator**: BB80/20 Specification Closure System
**Date**: 2026-01-01
**Repository**: /home/user/qlever
**Branch**: claude/epic-7-simd-upgrade-lw3jg

---

## SPECIFICATION CLOSURE VERDICT: INCOMPLETE

**Specification status**: NOT CLOSED (Multiple design freedoms remain)
**Can agents begin deterministic single-pass implementation?** NO
**Iteration required before implementation?** YES

---

## Critical Gaps Identified

### CATEGORY 1: Missing Formal Specification Document

#### Finding 1.1: No EPIC 7 specification document exists
- **Location**: `/home/user/qlever/docs/` (searched for EPIC 7 specification)
- **Impact**: HIGH
- **Evidence**:
  - EPIC 1: Documented in `EPOCH_EPICS_FINAL_SUMMARY.md` (300+ lines)
  - EPIC 3: Documented in `EPIC3_TASK6_IMPLEMENTATION_SUMMARY.md`
  - EPIC 4: Documented in `EPIC4_CONVERGENCE_SUMMARY.md` + `EPIC4_SHARED_INVARIANTS.md`
  - EPIC 7: NO formal specification document
- **Implication**: Specification closure cannot be verified without written formalization

#### Finding 1.2: Simdjson submodule not initialized
- **Location**: `/home/user/qlever/vendors/simdjson/`
- **Current state**: Empty directory (only `.` and `..`)
- **Submodule URL**: `https://github.com/seanchatmangpt/simdjson`
- **Impact**: HIGH
- **Blocker**: Cannot analyze existing simdjson code or integration patterns
- **Required action**: `git submodule update --init --recursive`

#### Finding 1.3: User-provided summary is aspirational, not formalized
- **Evidence**: User states "Design principles are non-negotiable" but provides no:
  - Interface specifications (C++ header files)
  - Error code enum definitions
  - Formal definitions of "hot-path" boundaries
  - Deterministic digest algorithm specifications
  - CI enforcement rule specifications

---

### CATEGORY 2: QLever Ingress Wrapper Interface Not Formalized

#### Finding 2.1: "QLever-owned ingress interfaces" mentioned but not defined
- **Requirement** (from user): "Wrap simdjson behind QLever-owned ingress interfaces"
- **What's actually specified**: Nothing. No interface contracts.
- **Missing specifications**:
  - Namespace/module name for ingress
  - Header file location and structure
  - Function signatures (C++20 compatible)
  - Error handling contract
  - Return types (success/failure, partial/complete)
  - Memory ownership (who allocates/deallocates?)
  - Thread-safety guarantees
- **Design freedoms remaining**:
  - Is ingress synchronous or async?
  - What is the error propagation strategy?
  - Does ingress buffer data or stream-process?
  - Are there multiple ingress paths (SIMD vs fallback)?

#### Finding 2.2: "Error codes only" diagnostic strategy not formalized
- **Requirement** (from user): "Replace diagnostics with error codes, deterministic digests"
- **What's actually specified**: Nothing.
  - No enum definition of error codes
  - No error code allocation scheme
  - No error code semantics (is 1 = parse error? memory error? semantic error?)
  - No error code registry or documentation format
- **Design freedoms remaining**:
  - Error code format (integer? string? struct with fields?)
  - Error code ranges (allocation to different components)
  - Distinguishing recoverable vs. fatal errors
  - How to embed error codes in digests

---

### CATEGORY 3: Hot-Path Boundaries Not Formally Defined

#### Finding 3.1: "Hot-path" is ambiguous in specification
- **Requirement** (from user):
  - "Remove all hot-path logging, printing, and telemetry"
  - "Enforce zero side effects in execution paths"
- **What's actually specified**: No formal definition of what constitutes a "hot-path"
- **Design freedoms remaining**:
  - Which execution paths are "hot"?
    - All query execution code?
    - Only the inner loops of join/filter operations?
    - Only SIMD vectorized paths?
    - All ingress processing?
  - What constitutes a "side effect"?
    - Logging (clearly forbidden)
    - Metrics collection? (not mentioned)
    - Performance counters? (not mentioned)
    - Cache population? (not mentioned, but likely necessary)
  - Is there a hot/cold boundary, or is ALL code restricted?
  - How to distinguish logging in cold paths (allowed?) vs hot paths (forbidden)?

#### Finding 3.2: No static analyzer rules for enforcement
- **Requirement**: "CI enforce silence invariants"
- **What's actually specified**: No concrete static check rules or AST patterns to detect violations
- **Design freedoms**:
  - Detect all log statements? Or specific functions?
  - Regex-based? Clang-based? Custom tool-based?
  - What about indirect calls (function pointers)?

---

### CATEGORY 4: Deterministic Digests Not Formally Specified

#### Finding 4.1: "Deterministic digests unchanged across runs" is vague
- **Requirement** (from user):
  - "Deterministic digests, structural validation"
  - "Benchmarks: Deterministic digests unchanged across runs"
- **What's actually specified**: Nothing.
  - No digest algorithm defined
  - No specification of what data is included/excluded
  - No hash function specified (SHA-256? BLAKE3?)
  - No serialization format specified
- **Design freedoms**:
  - What should digest cover?
    - Parse tree structure only?
    - Including resolved entity IDs?
    - Including semantic validation results?
    - Including SIMD-specific optimizations applied?
  - How to ensure determinism?
    - Alphabetical field ordering (like EPIC 4)?
    - Big-endian vs little-endian?
    - Floating point handling?
  - How are multiple runs compared?
    - Exact byte match? Or semantic equivalence?
  - Are digests hierarchical (parse digest + validation digest)?

#### Finding 4.2: No "structural validation" specification
- **User states**: "structural validation, no runtime heuristics"
- **Missing specification** of:
  - What "structural" means (SHACL shapes? Custom grammar?)
  - What validation must complete before ingestion?
  - What happens if structural validation fails?
  - Is structural validation SIMD-accelerated?

---

### CATEGORY 5: SIMD Technique Extraction Not Formalized

#### Finding 5.1: Reusable SIMD techniques mentioned but not enumerated
- **Requirement** (from user): "Extract reusable SIMD techniques (parsing, validation, scanning, error classification)"
- **What's actually specified**: No list of specific SIMD techniques to extract
  - No criteria for "reusability"
  - No specification of which simdjson techniques are applicable to RDF parsing
- **Design freedoms**:
  - Which parsing algorithms apply to RDF?
    - JSON parsing uses different SIMD patterns than N-Triples
    - Turtle format (prefixes, blank nodes) may not fit JSON patterns
  - What does "scanning" mean in RDF context?
    - Character-by-character scanning? (vectorizable)
    - Token recognition? (may require state machine)
  - What does "error classification" mean?
    - Categorizing malformed syntax?
    - Categorizing semantic violations?
  - Which simdjson internal functions are public API vs internal?

---

### CATEGORY 6: CI Enforcement Not Formally Specified

#### Finding 6.1: "CI enforce silence invariants" has no specification
- **Requirement** (from user):
  - "CI enforce silence invariants"
  - "static checks, build failures if logging detected"
- **What's actually specified**: No concrete CI rule file
  - No error message format
  - No list of functions/patterns to detect
- **Design freedoms**:
  - False positive tolerance?
    - Is `LOG_DEBUG` in unreachable code a violation?
    - Is `cout` in a fallback error path a violation?
  - Which CI system (GitHub Actions, Jenkins, custom)?
  - How to fail the build?
    - Compilation error? (requires compiler plugin)
    - External lint tool? (clang-tidy, custom analyzer)
  - What's the remediation process?
    - Auto-fix? Manual review? Exceptions list?

---

### CATEGORY 7: Acceptance Criteria Are Not Binary

#### Finding 7.1: Acceptance criteria are vague, not machine-testable
- **User states**: "Benchmarks improve or remain stable"
- **This is NOT a binary acceptance criterion**. Design freedoms:
  - "Improve" by how much? 1%? 10%?
  - "Remain stable" within what tolerance? 5%? 10%?
  - Across how many benchmarks? All? 80%?
  - Which benchmarks? Synthetic? Production workloads?
  - Measured on what hardware? (Baseline system specs missing)

#### Finding 7.2: No baseline metrics defined
- **User states**: "Benchmarks improve or remain stable"
- **Missing specification** of:
  - What the baseline performance is (pre-EPIC 7)
  - Which benchmarks to use (no benchmark suite specified)
  - What "performance" means (latency? throughput? memory?)
  - Variance tolerance across runs

#### Finding 7.3: "Zero logging" acceptance criterion needs formalization
- **User states**: "Hot paths contain zero logging or printing"
- **Missing specification** of:
  - Precise definition of coverage (see Finding 3.1)
  - Automated verification method
  - Allowed exceptions (if any)
  - Audit report format (for verification)

---

### CATEGORY 8: Scope Ambiguities

#### Finding 8.1: Scope boundary with existing ingress unclear
- **User states** (IN): "Vendoring, wrapping, audit, purge, CI"
- **Unclear**:
  - Should EPIC 7 replace existing RDF ingress (RdfParser)?
  - Or wrap it alongside SIMD path?
  - What happens to non-SIMD-compatible formats (Turtle prefixes)?
  - Is fallback to existing parser in scope or out?

#### Finding 8.2: Interaction with EPIC 4 read-plane not specified
- **EPIC 4** (recent): Read-plane hardening with workload capture/replay
- **EPIC 7** (proposed): SIMD ingress & hot-path purification
- **Questions**:
  - Does read-plane observe SIMD parsing decisions?
  - Are digests from EPIC 4 and EPIC 7 compatible?
  - Who owns cache invalidation (EPIC 1.1 vs EPIC 7)?

---

### CATEGORY 9: Missing Integration Contracts

#### Finding 9.1: No specification of SIMD path vs fallback
- **User implies**: "SIMD as first-class"
- **Unclear**:
  - When does SIMD path activate? (always? for certain formats?)
  - What triggers fallback to non-SIMD code?
  - Is fallback allowed in production?
  - How are parsing failures reported?

---

### CATEGORY 10: No Specification For Conformance Tests

#### Finding 10.1: "Conformance tests" mentioned but not specified
- **User states**: "conformance tests" as acceptance criterion
- **Missing specification** of:
  - Test cases (which RDF samples?)
  - Test data location
  - Success criteria for each test
  - How to validate conformance

---

## Iteration Points Requiring Specification Closure

### Required Actions (in priority order):

**[1] CRITICAL: Create formal EPIC 7 specification document**
- Parallel to: `/home/user/qlever/docs/EPIC4_CONVERGENCE_SUMMARY.md`
- Must include:
  - a) Ingress wrapper interface (C++ header sketch)
  - b) Error code enum definition (all codes with semantics)
  - c) Hot-path boundary definition (formal list of functions)
  - d) Deterministic digest algorithm (hash function + data coverage)
  - e) CI enforcement rules (concrete analyzer rules)
  - f) SIMD techniques inventory (per-technique specification)
  - g) Acceptance criteria (binary, measurable)
  - h) Baseline metrics (pre-EPIC 7 performance)

**[2] CRITICAL: Initialize and analyze simdjson submodule**
- Current: Empty directory
- Action: `git submodule update --init --recursive`
- Analysis:
  - Identify public API
  - List SIMD primitives applicable to RDF
  - Document fallback paths
  - Catalog memory allocation patterns

**[3] HIGH: Formalize hot-path boundaries**
- Current: Ambiguous
- Required:
  - List all function signatures that are "hot"
  - Specify call graph boundaries (transitive closures)
  - Define measurement criteria (execution count? latency contribution?)
  - Document cold-path exceptions (if any)

**[4] HIGH: Define deterministic digest algorithm**
- Current: Vague ("deterministic digests unchanged")
- Required:
  - Specify hash algorithm (SHA-256? BLAKE3?)
  - Specify data coverage (parse tree? validation? semantics?)
  - Specify serialization format (alphabetical fields like EPIC 4?)
  - Specify verification method (reproduce digest in CI?)

**[5] HIGH: Formalize error code system**
- Current: "error codes only" without enumeration
- Required:
  - Enum class with all error codes
  - Semantic specification per code
  - Recovery/fallback actions
  - Integration with existing QLever error handling

**[6] MEDIUM: Create CI enforcement rules specification**
- Current: "CI enforce silence invariants" without rules
- Required:
  - Specify analyzer tool (clang-tidy? Custom?)
  - Specify detection patterns (AST rules? Regex?)
  - Specify false-positive mitigation
  - Specify remediation workflow

**[7] MEDIUM: Define SIMD technique extraction criteria**
- Current: "Extract reusable SIMD techniques" without list
- Required:
  - Inventory of simdjson SIMD techniques
  - Per-technique: applicable RDF formats
  - Per-technique: fallback behavior
  - Per-technique: integration points

**[8] MEDIUM: Specify acceptance criteria (binary, measurable)**
- Current: "Benchmarks improve or remain stable" (vague)
- Required:
  - Baseline performance metrics (pre-EPIC 7)
  - Benchmark suite (specific workloads)
  - Threshold definitions (% improvement? variance tolerance?)
  - Automated verification (CI test script)

**[9] LOW: Clarify scope interactions**
- Current: Ambiguous with EPIC 1.1, EPIC 4
- Required:
  - Specify interaction with existing ingress pipeline
  - Specify interaction with epoch manifests (EPIC 1.1)
  - Specify interaction with read-plane (EPIC 4)
  - Document cache invalidation ownership

---

## Specification Closure Checklist

- [ ] Formal specification document created
- [ ] Simdjson submodule initialized and analyzed
- [ ] Hot-path boundaries formally defined (function list)
- [ ] Deterministic digest algorithm specified
- [ ] Error code system enumerated (enum + semantics)
- [ ] CI enforcement rules formalized (concrete rules)
- [ ] SIMD technique inventory created
- [ ] Acceptance criteria formalized (binary, measurable)
- [ ] Baseline metrics documented
- [ ] Scope interactions clarified
- [ ] All design freedoms removed (zero ambiguity)
- [ ] Specification reviewed and approved (closure sign-off)

**Current completion**: 0/12 (0%)

---

## Conclusion

### VERDICT: INCOMPLETE

The specification summary provided by the user is **ASPIRATIONAL AND WELL-INTENTIONED** but **LACKS THE FORMAL PRECISION** required for deterministic single-pass implementation.

Multiple design freedoms remain open:
- What code is "hot-path"? (ambiguous)
- How are error codes defined? (not enumerated)
- How are deterministic digests computed? (not specified)
- What are CI enforcement rules? (not concrete)
- Which SIMD techniques are in scope? (not inventoried)
- Are benchmarks measured against what baseline? (not specified)

**AGENTS CANNOT BEGIN IMPLEMENTATION** without risking:
- Inconsistent error handling
- Non-deterministic behavior (digest variability)
- Silent logging violations (no CI enforcement rules)
- Non-reusable SIMD abstractions (no technique specification)
- Rework when specification gaps discovered mid-implementation

### REQUIRED NEXT STEP

Return to specification phase. Create formal EPIC 7 specification document following the pattern of EPIC 4 convergence summary (complete, formalized, zero ambiguity). Only after specification closure can agents proceed with deterministic single-pass construction.

---

## Validator Signature

This closure analysis was performed by the BB80/20 Specification Validator.

- **Specification Status**: INCOMPLETE (Return to specification phase)
- **Iteration Required**: YES
- **Implementation Blocked**: YES (until specification closed)
- **Date**: 2026-01-01
