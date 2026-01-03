# EPIC 14.0 — Mura Delta Severity Classification

**Phase**: Delta Discovery
**Status**: CLASSIFICATION ONLY — NO FIXES PERMITTED
**Date**: 2026-01-03

---

## Objective

Classify each identified delta between formalisms as:
- **🟢 SEMANTIC**: Legitimate structural difference (no unification required)
- **🟡 ACCIDENTAL**: Unintentional difference (safe to unify)
- **🔴 INTEGRITY RISK**: Difference causes hidden variance or inconsistency (must fix before unification)

---

## Axis 1: Ingress

### 🔴 INTEGRITY RISK: Error Hierarchy Divergence

**Delta**: SHACL throws `std::runtime_error`; Datalog throws `ParseException`; N3 inherits TurtleParser exceptions

**Impact**: Different exception types means catch-all handlers cannot uniformly treat parse failures. Unclear which layer is responsible for error recovery.

**Evidence**:
- ShaclShapeParser.cpp: uses throw std::runtime_error
- DatalogParser.h: throws ParseException
- RdfParser.h (N3): inherits exception handling from TurtleParser

**Severity**: HIGH — Breaks uniform error handling contract

**Fix Scope** (EPIC 14.1): Converge to single exception type with tagged payload (e.g., ParseException with formalism enum)

---

### 🟡 ACCIDENTAL: Parser Implementation Diversity

**Delta**: SHACL = hand-written recursive descent; Datalog = hand-written recursive descent; N3 = template-based extension

**Impact**: Three different parser implementations for similar problems; maintenance burden; inconsistent error messages

**Evidence**:
- ShaclShapeParser: manual string parsing + regex
- DatalogParser: recursive descent with tokenizer
- N3Parser: inherits from TurtleParser (no custom parsing)

**Severity**: MEDIUM — Code duplication but not a functional risk

**Fix Scope** (EPIC 14.1): Consider parser template or combinator library (optional; low priority)

---

### 🟢 SEMANTIC: Validation Strictness Variation

**Delta**: SHACL ingress is permissive (accepts incomplete shapes); N3 ingress is strict (Turtle validation); Datalog ingress is strict (syntax validation)

**Impact**: Legitimate design choice; different formalisms have different strictness requirements

**Evidence**:
- SHACL allows shapes with no constraints (valid per W3C spec)
- N3 reuses Turtle parser (strict by design)
- Datalog requires valid token stream (syntax is restrictive)

**Severity**: LOW — Not a bug; reflects formalism differences

---

## Axis 2: AST / Internal Representation

### 🔴 INTEGRITY RISK: Mutability Model Divergence

**Delta**: SHACL uses mutable vectors/maps for constraints; N3/Datalog use immutable TripleComponent/DatalogRule

**Impact**: SHACL allows post-creation modification of constraints (possible source of non-determinism). N3/Datalog guarantee immutability (deterministic).

**Evidence**:
- ShaclShape.h: NodeShape::constraints is `std::vector<ShaclConstraint>` (mutable)
- DatalogRule.h: all fields are `const` (immutable by design)
- TripleComponent: immutable getters (inherited immutability)

**Severity**: HIGH — Mutability can mask non-determinism in SHACL validation

**Fix Scope** (EPIC 14.1): Convert SHACL constraints to immutable model or mark as non-modifiable after construction

---

### 🟡 ACCIDENTAL: Constraint Representation Fragmentation

**Delta**: SHACL uses std::variant<int, double, string, ...>; N3/Datalog use explicit field-based structs (SparqlTriple, DatalogRule)

**Impact**: SHACL variant requires runtime type checking; N3/Datalog have compile-time safety. Code duplication in constraint handling.

**Evidence**:
- ShaclConstraint: std::get<int>() or std::get<string>() required for value access
- DatalogRule: direct field access (type-safe)

**Severity**: MEDIUM — Reduces readability and safety in SHACL

**Fix Scope** (EPIC 14.1): Consider moving SHACL to explicit constraint subtypes (Visitor pattern or tagged struct)

---

### 🟢 SEMANTIC: Hashability Gap in SHACL

**Delta**: SHACL NodeShape/PropertyShape lack explicit hashing; N3/Datalog have serialization/hash support

**Impact**: SHACL shapes cannot be used in hash-based containers; Datalog rules can be serialized

**Evidence**:
- ShaclShape.h: no operator== or operator<=> defined
- DatalogRule.h: QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL + AD_SERIALIZE_FRIEND_FUNCTION

**Severity**: LOW — SHACL shapes are keyed by shapeId strings (external hashing); not a functional gap

---

## Axis 3: Evaluation Kernel

### 🟢 SEMANTIC: Traversal Strategy Divergence

**Delta**: SHACL uses constraint-by-constraint DFS; Datalog uses fixpoint iteration; N3 has no evaluation kernel

**Impact**: Legitimate design choice reflecting formalism semantics (SHACL = property validation; Datalog = recursive rule expansion)

**Evidence**:
- ShaclConstraintEvaluator: evaluates each constraint independently
- FixpointComputation: iterative rule application until convergence
- N3: parsing only; no evaluation

**Severity**: LOW — Not unifiable; reflects fundamental formalism differences

---

### 🔴 INTEGRITY RISK: Caching Depth Disparity

**Delta**: SHACL has 3-tier cache (validation result + constraint + Bloom filter); Datalog has 1-tier generic caching

**Impact**: SHACL caching is specialized and fast; Datalog caching is generic and slower. Inconsistent performance characteristics across formalisms.

**Evidence**:
- ShaclValidationCache: ShaclValidationCache, ShaclValidationCache::ValidationResultCache, BloomFilter
- Datalog: uses QueryExecutionContext result cache (no formalism-specific optimization)

**Severity**: MEDIUM — Performance inconsistency; not a correctness issue but indicates underoptimization in Datalog

**Fix Scope** (EPIC 14.1): Add formalism-specific cache optimization for Datalog rules (optional; may defer)

---

### 🟡 ACCIDENTAL: Parallelization Asymmetry

**Delta**: SHACL supports multi-threaded validation; Datalog is single-threaded fixpoint

**Impact**: Datalog rules not parallelized; potential performance loss on multi-core systems

**Evidence**:
- ShaclValidator: _enableParallelValidation + _parallelThreads flags
- FixpointComputation: no thread pool usage

**Severity**: MEDIUM — Performance concern; not a correctness issue

**Fix Scope** (EPIC 14.1): Consider pipelining fixpoint iterations (optional; depends on rule complexity)

---

## Axis 4: Lifecycle / Epoch Integration

### 🟡 ACCIDENTAL: Load Phase Asymmetry

**Delta**: SHACL/Datalog loaded at SEAL phase; N3 compliance check at INGEST phase

**Impact**: N3 is validated earlier in pipeline; shapes/rules are validated later. Inconsistent validation ordering.

**Evidence**:
- SHACL: ShaclValidator operation created at SEAL (after index built)
- Datalog: RuleDatabase populated at SEAL
- N3: N3ComplianceVerifier runs at INGEST (before index built)

**Severity**: MEDIUM — Not a correctness issue; reflects different validation strategies

**Fix Scope** (EPIC 14.1): Standardize to SEAL-phase validation for all formalisms (or defer all to SERVE)

---

### 🟢 SEMANTIC: Caching Model Divergence

**Delta**: SHACL has specialized LRU+Bloom cache; Datalog uses generic QueryExecutionContext cache; N3 uses file digest

**Impact**: Legitimate trade-off between specialization (SHACL fast) and generality (Datalog reuses core cache)

**Evidence**:
- ShaclValidationCache: custom LRU + Bloom filter implementation
- Datalog: delegated to QueryExecutionContext.resultCache
- N3: compliance digest binding

**Severity**: LOW — Design choice, not a defect

---

## Axis 5: Observability

### 🔴 INTEGRITY RISK: Output Type Fragmentation

**Delta**: SHACL = ShaclViolation objects; N3 = markdown text + structured issues; Datalog = SPARQL tuples

**Impact**: Each formalism produces different output types. Cannot write uniform validation consumer. No standard violation schema across all formalisms.

**Evidence**:
- ShaclViolation: focusNode, constraintComponent, message, severity (W3C standard)
- N3ComplianceIssue: feature, description, lineNumber (N3-specific)
- Datalog: IdTable result columns (SPARQL-specific)

**Severity**: HIGH — Breaks composability; each consumer must understand all three formats

**Fix Scope** (EPIC 14.1): Define unified violation schema (extend W3C SHACL report shape to cover all formalisms)

---

### 🟡 ACCIDENTAL: SPARQL Integration Asymmetry

**Delta**: SHACL/Datalog expose results via Operation (SPARQL-queryable); N3 does not

**Impact**: N3 validation results cannot be queried via SPARQL; asymmetric integration

**Evidence**:
- ShaclValidator: extends Operation; integrates with QueryExecutionTree
- DatalogPlanner: integrates with QueryPlanner; produces IdTable results
- N3ComplianceVerifier: outputs markdown text only

**Severity**: MEDIUM — Not a correctness issue; inconsistent composability

**Fix Scope** (EPIC 14.1): Wrap N3ComplianceVerifier in Operation or standardize all to non-SPARQL results

---

### 🟢 SEMANTIC: Result Granularity Differences

**Delta**: SHACL = per-violation detail; Datalog = per-tuple (minimal); N3 = per-feature

**Impact**: Different reporting granularities reflect formalism semantics

**Evidence**:
- SHACL: every constraint violation is reported with context
- Datalog: results are tuples (unadorned)
- N3: reports feature usage at file level

**Severity**: LOW — Legitimate formalism differences

---

## Axis 6: Determinism Controls

### 🔴 INTEGRITY RISK: Determinism Testing Gap

**Delta**: SHACL/Datalog have no explicit rule-level determinism tests; only query-level tests exist in other modules

**Impact**: Cannot guarantee that SHACL constraint evaluation is deterministic; cannot guarantee that Datalog rule execution is deterministic. Only query-level determinism is verified.

**Evidence**:
- ShaclComplianceTest: no determinism tests (only compliance)
- DatalogQueryPlannerTest: no determinism tests (only planning)
- DeterminismClassifier: tests query features only (not rule execution)

**Severity**: HIGH — Missing verification; potential hidden non-determinism

**Fix Scope** (EPIC 14.1): Add determinism unit tests for each formalism (rule-level, not query-level)

---

### 🟡 ACCIDENTAL: Non-Determinism Detection Inconsistency

**Delta**: Datalog has DeterminismClassifier (queries); N3 has ComplianceVerifier (features); SHACL has implicit epoch binding

**Impact**: Different mechanisms for detecting non-determinism; no cross-formalism consistency check

**Evidence**:
- DeterminismClassifier: classifies NOW(), RAND(), UUID(), SERVICE
- N3ComplianceVerifier: detects formulae, implications, quantifiers, variables
- SHACL: relies on epoch isolation (implicit)

**Severity**: MEDIUM — Reduces confidence in determinism across all formalisms

**Fix Scope** (EPIC 14.1): Converge to single determinism classification model (extend DeterminismClassifier)

---

### 🟢 SEMANTIC: Determinism Scope Differences

**Delta**: Query-level determinism (Datalog/SPARQL) vs. Feature-level determinism (N3) vs. Epoch-level determinism (SHACL)

**Impact**: Different determinism scopes are appropriate for different formalisms

**Evidence**:
- Query determinism: relevant when results are cached
- Feature determinism: relevant when parsing rules
- Epoch determinism: relevant when storing snapshots

**Severity**: LOW — Legitimate scope differences

---

## Axis 7: Testing Discipline

### 🔴 INTEGRITY RISK: Coverage Gap in N3 and Datalog

**Delta**: SHACL has W3C test suite (golden tests); N3 has 2 integration tests; Datalog has 8+ tests but no external standard

**Impact**: N3 and Datalog lack external validation; SHACL has W3C reference. Cannot verify correctness against external spec for N3/Datalog.

**Evidence**:
- ShaclComplianceTest: references W3C SHACL spec (https://www.w3.org/TR/shacl/)
- N3IntegrationTest: local tests only
- DatalogQueryPlannerTest: local tests only

**Severity**: HIGH — Reduced confidence in N3/Datalog correctness

**Fix Scope** (EPIC 14.1): Consider external reference standards (N3 RFC; Datalog semantics papers)

---

### 🟡 ACCIDENTAL: Negative Test Coverage Imbalance

**Delta**: SHACL has implicit negative cases (violations); N3 has explicit negative corpus (invalid test cases); Datalog has implicit negative cases

**Impact**: N3 is better documented for error handling; SHACL/Datalog rely on implicit assumptions

**Evidence**:
- ShaclComplianceTest: violations expected implicitly
- N3: explicit invalid_*.n3 test files with parse errors
- DatalogQueryPlannerTest: no negative test cases documented

**Severity**: MEDIUM — Reduces clarity on error boundaries

**Fix Scope** (EPIC 14.1): Add explicit negative test corpus for all formalisms (follow N3 pattern)

---

### 🟢 SEMANTIC: Skipped Test Documentation

**Delta**: SHACL explicitly marks skipped tests (GTEST_SKIP); N3/Datalog have no skipped tests visible

**Impact**: SHACL acknowledges implementation gaps; N3/Datalog gaps are silent

**Evidence**:
- ShaclComplianceTest: W3C_Class_Constraint marked GTEST_SKIP
- N3/Datalog tests: no GTEST_SKIP observed

**Severity**: LOW — Transparency issue; not a correctness gap

---

## Summary: Critical vs. Accidental Deltas

| Category | Count | Examples |
| -------- | ----- | -------- |
| **🔴 INTEGRITY RISK** | 5 | Error hierarchy; SHACL mutability; Caching disparity; Output fragmentation; Determinism testing gap |
| **🟡 ACCIDENTAL** | 6 | Parser diversity; Constraint representation; Parallelization; Load phase; Non-determinism detection; Negative test coverage |
| **🟢 SEMANTIC** | 5 | Validation strictness; Traversal strategy; Caching model; Result granularity; Determinism scope |

---

## Recommended Unification Priority (EPIC 14.1)

### MUST FIX (Integrity Risks — blocks convergence):
1. **Error Hierarchy** → Single exception type with formalism tag
2. **SHACL Mutability** → Immutable constraint model or freeze semantics
3. **Output Fragmentation** → Unified violation schema (W3C-compatible)
4. **Determinism Testing Gap** → Add rule-level determinism tests for all formalisms
5. **Caching Disparity** → Add formalism-specific optimization to Datalog (or simplify SHACL)

### SHOULD FIX (Accidental Deltas — improves maintainability):
1. **Error Handling** → Converge to single exception hierarchy
2. **Parser Diversity** → Consider parser combinator library (or document differences)
3. **Constraint Representation** → Move SHACL to explicit subtypes
4. **Load Phase** → Standardize to SEAL-phase validation
5. **Non-Determinism Detection** → Extend DeterminismClassifier across all formalisms
6. **Negative Tests** → Add explicit negative test corpus for all formalisms

### CAN DEFER (Semantic Differences — preserve intentionality):
1. Traversal strategy (DFS vs. fixpoint vs. none)
2. Caching model (specialized vs. generic)
3. Result granularity (per-violation vs. per-tuple)
4. Determinism scope (query vs. feature vs. epoch)

---

**Status**: CLASSIFICATION COMPLETE — Awaiting EPIC 14.1 Unification Instruction
