# EPIC 14 — Convergence Synthesis: Unified Formalism Pipeline

**Phase**: Convergence (EPIC 14.1 Specification)
**Status**: SYNTHESIS COMPLETE
**Date**: 2026-01-03

---

## Executive Summary

This document synthesizes the EPIC 14.0 measurement phase into a **unified formalism pipeline architecture**. Selection pressure has been applied across 7 structural axes based on coverage, invariant preservation, minimality, and industrial quality. The result is a monoidal, single-pass construction specification for EPIC 14.1 implementation.

**Convergence Model**: Selection pressure (not voting)
**Atomic Cycle Status**: All phases completed (fan-out → independent construction → collision detection → convergence → refactoring → closure)
**Critical Blockers**: 5 integrity risks identified and remediation specified

---

## 1. Selection Pressure Analysis by Axis

### 1.1 Axis 1: Ingress (Parser + Validation + Error Handling)

**WINNER**: SHACL

**Selection Rationale**:
- **Coverage**: Hand-written parser with explicit control flow; W3C Turtle standard format
- **Invariants Preserved**: Integrated JsonLdIngressNormalizer with deterministic digest binding (SHA256)
- **Minimality**: Clean separation: Turtle path (hand-written) vs. JSON-LD path (simdjson via normalizer)
- **Industrial Quality**: Guard enforcement (max size, depth, timeout); fail-closed error semantics

**Runners-up Analysis**:
- **Datalog**: Custom parser is clean but proprietary syntax (not standardized)
- **N3**: Delegates to TurtleParser (minimal code but lacks ingress control)
- **ShEx**: Stub only

**Merge/Discard/Rewrite Decision**:
- **MERGE**: SHACL's JsonLdIngressNormalizer layer (already shared across all four)
- **REWRITE**: Converge error handling to single exception hierarchy (fix integrity risk #1)
- **DISCARD**: None (all parsers serve semantic purposes)

**Integration Points**:
- `JsonLdIngressNormalizer::normalizeForDialect(input, dialect, epoch, guards)` → mandatory entry point
- `IngressGuardConfig` → mandatory bounds enforcement
- `IngressDigest::compute(normalized, epoch, guard_hash)` → mandatory determinism binding

---

### 1.2 Axis 2: AST / Internal Representation

**WINNER**: Datalog

**Selection Rationale**:
- **Coverage**: Explicit `DatalogRule` class with head, body, filters structure
- **Invariants Preserved**: Immutable by design (const references, move semantics)
- **Minimality**: Complete serialization support (`AD_SERIALIZE_FRIEND_FUNCTION`); equality operators defined
- **Industrial Quality**: Variable class reused from SPARQL (consistent type system); no mutable shared state

**Runners-up Analysis**:
- **SHACL**: Variant-based constraints powerful but complex; **mutable** (integrity risk #2); no explicit hashing
- **N3**: Inherits immutability from TripleComponent (clean but not N3-specific design)
- **ShEx**: No AST

**Merge/Discard/Rewrite Decision**:
- **REWRITE**: Convert SHACL constraints to immutable model (fix integrity risk #2)
- **MERGE**: Adopt Datalog's AST pattern: explicit structs + serialization + equality
- **DISCARD**: SHACL's std::variant model (replace with explicit subtypes)

**Integration Points**:
- Canonical AST pattern: `struct FormalismRule { const Head head; const vector<Body> body; const vector<Filter> filters; }`
- Serialization: `AD_SERIALIZE_FRIEND_FUNCTION` on all AST nodes
- Equality: `QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL` on all AST nodes
- Hashing: SHA256 digest of serialized form

---

### 1.3 Axis 3: Evaluation Kernel

**WINNER**: Datalog

**Selection Rationale**:
- **Coverage**: Explicit fixpoint computation (FixpointComputation.cpp) with iteration limits
- **Invariants Preserved**: Semi-naive evaluation (industry standard algorithm); bounded iteration (fail-closed)
- **Minimality**: Uses IdTable for all intermediate results (consistent with QLever core)
- **Industrial Quality**: Integrates with QueryExecutionContext for resource management; no special-casing

**Runners-up Analysis**:
- **SHACL**: Multi-threaded parallelization strong, but constraint evaluation is constraint-specific (not generalizable)
- **N3**: No evaluation kernel (parsing only)
- **ShEx**: No kernel

**Merge/Discard/Rewrite Decision**:
- **MERGE**: Datalog's fixpoint kernel as canonical evaluation model
- **REWRITE**: SHACL constraint evaluation to use fixpoint model (unify traversal strategies)
- **DISCARD**: SHACL's constraint-by-constraint DFS (replace with fixpoint)

**Integration Points**:
- `FixpointComputation::evaluate(rules, context, iteration_limit)` → canonical evaluation entry
- `IdTable` → mandatory intermediate result storage
- `QueryExecutionContext` → mandatory resource bound enforcement
- Iteration limit: fail-closed at configurable threshold (default 1000)

**Parallelization Note**: SHACL's multi-threading capability can be reintroduced as **parallel fixpoint iteration** (pipeline fixpoint rounds, not constraint-by-constraint)

---

### 1.4 Axis 4: Lifecycle / Epoch Integration

**WINNER**: SHACL (with Datalog's generic caching as fallback)

**Selection Rationale**:
- **Coverage**: Per-epoch ShaclValidator operation; clear SEAL-phase binding
- **Invariants Preserved**: Shape registry invalidation on re-registration (predictable lifecycle)
- **Minimality**: Multi-level caching (LRU + Bloom filter) with epoch binding; cache eviction is predictable
- **Industrial Quality**: Integration with QueryExecutionTree (Operation trait) is clean

**Datalog Alternative** (equal quality):
- Per-epoch RuleDatabase; SEAL-phase binding
- Generic result caching via QueryExecutionContext
- Minimal code; no lifecycle surprises

**Merge/Discard/Rewrite Decision**:
- **MERGE**: SHACL's per-epoch binding model + Datalog's generic cache (resolve integrity risk #5)
- **REWRITE**: Standardize load phase to SEAL for all formalisms (fix accidental delta #4)
- **DISCARD**: N3's eager INGEST-phase validation (move to SEAL)

**Integration Points**:
- `FormalismRegistry::registerArtifact(artifact, epoch)` → SEAL-phase entry
- `EpochCache<Key, Value>` → mandatory cache binding
- `EpochInvalidation::onEpochChange(old_epoch, new_epoch)` → mandatory invalidation hook

**Caching Strategy Convergence**:
- **L1 Cache**: Result cache (LRU, per-epoch, size-bounded)
- **L2 Cache**: Bloom filter (negative lookups, fail-fast)
- **L3 Cache**: QueryExecutionContext generic cache (fallback)

---

### 1.5 Axis 5: Observability

**WINNER**: SHACL

**Selection Rationale**:
- **Coverage**: ShaclViolation objects with structured schema (focusNode, constraintComponent, severity, message)
- **Invariants Preserved**: W3C standard violation shape (SPARQL-queryable RDF graph)
- **Minimality**: ViolationFormatter produces both human-readable reports and RDF-serializable violations
- **Industrial Quality**: Integration with Operation interface (results flow through QueryExecutionTree)

**Runners-up Analysis**:
- **Datalog**: Tuple-based results (standard SPARQL format but lacks semantic context)
- **N3**: Markdown text reports (human-readable but not machine-parseable as RDF)
- **ShEx**: No observability

**Merge/Discard/Rewrite Decision**:
- **MERGE**: SHACL's violation schema as canonical output model (fix integrity risk #3)
- **REWRITE**: Extend W3C SHACL report shape to cover all formalisms
- **DISCARD**: N3's markdown-only output (replace with structured violations)

**Integration Points**:
- `FormalismViolation` struct: `{ focusNode, constraintComponent, message, severity, formalism, context }`
- `ViolationReport::toRDF()` → W3C-compatible RDF graph
- `ViolationReport::toMarkdown()` → human-readable text
- `ViolationReport::toJSON()` → machine-parseable JSON
- All formalisms expose results via `Operation` interface (SPARQL-queryable)

**Unified Schema Extension**:
```turtle
:FormalismViolation a sh:ValidationResult ;
  sh:focusNode ?focus ;
  sh:resultSeverity ?severity ;
  sh:resultMessage ?message ;
  sh:sourceConstraintComponent ?component ;
  :formalism ?formalism ;  # NEW: SHACL | ShEx | N3 | Datalog
  :context ?context .      # NEW: formalism-specific context
```

---

### 1.6 Axis 6: Determinism Controls

**WINNER**: Datalog (with N3's feature detection)

**Selection Rationale**:
- **Coverage**: DeterminismClassifier explicitly walks query tree and classifies non-deterministic features (NOW, RAND, UUID, STRUUID, BNODE, SERVICE)
- **Invariants Preserved**: Classification is deterministic (same query → same classification every time)
- **Minimality**: Fail-closed (non-deterministic queries rejected for caching)
- **Industrial Quality**: Integrates with query planning; explicit feature enumeration

**N3 Alternative** (equal quality):
- N3ComplianceVerifier detects unsupported non-deterministic features (formulae, implications, quantifiers, variables)
- Feature detection is comprehensive
- Report is deterministic and reproducible

**Merge/Discard/Rewrite Decision**:
- **MERGE**: Datalog's DeterminismClassifier + N3's feature detection (fix accidental delta #5)
- **REWRITE**: Extend DeterminismClassifier to cover all formalism-specific non-deterministic features
- **DISCARD**: SHACL's implicit epoch-only determinism (replace with explicit classification)

**Integration Points**:
- `DeterminismClassifier::classify(artifact)` → mandatory classification before caching
- `DeterminismResult` enum: `{ DETERMINISTIC, NON_DETERMINISTIC, UNKNOWN }`
- Non-deterministic features registry:
  - SPARQL: NOW(), RAND(), UUID(), STRUUID(), BNODE(), SERVICE
  - N3: formulae, implications, quantifiers, variables
  - SHACL: recursive shapes (if cycle-dependent on external state)
  - Datalog: recursive rules (if unbounded)

**Critical Gap Fix (Integrity Risk #4)**:
- Add **rule-level determinism tests** for all formalisms
- Test corpus: same rule → same classification (100% reproducibility)
- Negative tests: known non-deterministic patterns must be flagged

---

### 1.7 Axis 7: Testing Discipline

**WINNER**: SHACL (with N3's negative corpus structure)

**Selection Rationale**:
- **Coverage**: 13 test files with structured test suites; W3C SHACL Test Suite compliance (gold standard)
- **Invariants Preserved**: Tests cover core constraints, logical shapes, property paths, SPARQL constraints
- **Minimality**: Skipped tests explicitly marked (GTEST_SKIP) with rationale
- **Industrial Quality**: External standard (W3C) provides ground truth

**Runners-up Analysis**:
- **Datalog**: 8+ comprehensive tests; strong coverage but no formal external standard
- **N3**: 2 integration tests + explicit negative corpus (invalid test cases); weak positive coverage
- **ShEx**: No tests

**Merge/Discard/Rewrite Decision**:
- **MERGE**: SHACL's W3C-style golden tests + N3's negative corpus (fix accidental delta #6)
- **REWRITE**: Add external reference standards for N3/Datalog (fix integrity risk #7)
- **DISCARD**: None (all tests serve purposes)

**Integration Points**:
- `FormalismComplianceTest` suite: W3C-style golden tests for each formalism
- `FormalismNegativeCorpus` suite: explicit invalid inputs (parse errors, semantic violations)
- `FormalismDeterminismTest` suite: same input → same output (100 iterations, SHA256 comparison)
- External standards:
  - SHACL: W3C SHACL spec
  - ShEx: W3C ShEx spec (when implemented)
  - N3: W3C N3 Community Group spec
  - Datalog: Deductive database semantics (academic references)

---

## 2. Final Unified Pipeline Architecture

### 2.1 Data Flow (Ingress → AST → Evaluation → Results)

```
┌─────────────────────────────────────────────────────────────────┐
│                       INGRESS PHASE (SEAL)                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Input (JSON-LD or native format)                              │
│    ↓                                                            │
│  JsonLdIngressNormalizer::normalizeForDialect(input, dialect,  │
│                                                epoch, guards)   │
│    ↓                                                            │
│  IngressGuardConfig enforcement (max size, depth, timeout)     │
│    ↓                                                            │
│  Format-specific parser:                                       │
│    - SHACL: ShaclShapeParser (Turtle) or simdjson (JSON-LD)    │
│    - ShEx: TBD (will use simdjson for JSON-LD)                 │
│    - N3: TurtleParser (Turtle) or simdjson (JSON-LD)           │
│    - Datalog: DatalogParser (text) or simdjson (JSON-LD)       │
│    ↓                                                            │
│  IngressDigest::compute(normalized, epoch, guard_hash)         │
│    ↓                                                            │
│  Exception handling: single exception hierarchy                │
│    (ParseException with formalism tag)                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                         AST PHASE                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Immutable AST construction (Datalog pattern):                 │
│    struct FormalismRule {                                      │
│      const Head head;                                          │
│      const vector<Body> body;                                  │
│      const vector<Filter> filters;                             │
│      AD_SERIALIZE_FRIEND_FUNCTION(FormalismRule);              │
│      QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL;              │
│    };                                                           │
│    ↓                                                            │
│  AST serialization (AD_SERIALIZE_FRIEND_FUNCTION)              │
│    ↓                                                            │
│  AST hashing (SHA256 of serialized form)                       │
│    ↓                                                            │
│  DeterminismClassifier::classify(ast)                          │
│    → DETERMINISTIC | NON_DETERMINISTIC | UNKNOWN               │
│    ↓                                                            │
│  FormalismRegistry::registerArtifact(ast, epoch)               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    EVALUATION PHASE (SERVE)                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Query planning (QueryExecutionTree integration)               │
│    ↓                                                            │
│  Fixpoint evaluation (Datalog pattern):                        │
│    FixpointComputation::evaluate(rules, context, iteration_limit)│
│    ↓                                                            │
│  Semi-naive iteration:                                         │
│    - Initialize: δ_0 = base facts                             │
│    - Iterate: δ_{i+1} = rules(δ_i) ∖ δ_i                      │
│    - Converge: when δ_{i+1} = ∅ or iteration limit reached    │
│    ↓                                                            │
│  IdTable intermediate results (QLever core type)               │
│    ↓                                                            │
│  EpochCache lookup (L1: LRU, L2: Bloom, L3: generic)           │
│    ↓                                                            │
│  Resource bounds enforcement (QueryExecutionContext)           │
│    - Max memory: AllocatorWithLimit                            │
│    - Max time: timeout guard                                   │
│    - Max iterations: fail-closed                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                      RESULTS PHASE                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  FormalismViolation construction (SHACL pattern):              │
│    struct FormalismViolation {                                 │
│      ValueId focusNode;                                        │
│      ConstraintComponent constraintComponent;                  │
│      string message;                                           │
│      Severity severity;                                        │
│      Formalism formalism;  // NEW                              │
│      json context;          // NEW (formalism-specific)        │
│    };                                                           │
│    ↓                                                            │
│  ViolationReport serialization:                                │
│    - toRDF() → W3C-compatible RDF graph                        │
│    - toMarkdown() → human-readable text                        │
│    - toJSON() → machine-parseable JSON                         │
│    ↓                                                            │
│  Operation interface integration (SPARQL-queryable)            │
│    ↓                                                            │
│  Result caching (epoch-bound, determinism-gated)               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

### 2.2 Component Interactions

```
┌────────────────────────────────────────────────────────────────────┐
│                    CANONICAL μ-SKELETON                            │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  1. JsonLdIngressNormalizer (MANDATORY)                           │
│     - Entry point for all formalisms                              │
│     - Guard enforcement                                           │
│     - Digest binding                                              │
│                                                                    │
│  2. FormalismParser (STRATEGY PATTERN)                            │
│     - Interface: parse(input) → AST                               │
│     - Implementations: ShaclParser, ShExParser, N3Parser,         │
│                        DatalogParser                              │
│     - Exception: ParseException(formalism, message, line, col)    │
│                                                                    │
│  3. FormalismAST (IMMUTABLE, SERIALIZABLE)                        │
│     - Pattern: const fields + AD_SERIALIZE + equality             │
│     - Hash: SHA256(serialize(ast))                                │
│     - Determinism: DeterminismClassifier::classify(ast)           │
│                                                                    │
│  4. FormalismRegistry (EPOCH-BOUND)                               │
│     - Phase: SEAL                                                 │
│     - Invalidation: onEpochChange                                 │
│     - Cache: EpochCache<ASTHash, CompiledArtifact>                │
│                                                                    │
│  5. FormalismEvaluator (FIXPOINT PATTERN)                         │
│     - Algorithm: Semi-naive fixpoint                              │
│     - Bounds: iteration_limit, memory_limit, time_limit           │
│     - Results: IdTable (QLever core type)                         │
│                                                                    │
│  6. FormalismViolation (W3C-EXTENDED SCHEMA)                      │
│     - Core: focusNode, constraintComponent, message, severity     │
│     - Extension: formalism, context                               │
│     - Serialization: RDF, JSON, Markdown                          │
│                                                                    │
│  7. FormalismOperation (SPARQL INTEGRATION)                       │
│     - Extends: Operation                                          │
│     - Integration: QueryExecutionTree                             │
│     - Caching: Determinism-gated                                  │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

---

### 2.3 Integration Points with Existing QLever Code

| Component | QLever Integration Point | Contract |
|-----------|--------------------------|----------|
| **JsonLdIngressNormalizer** | `src/engine/ingress/JsonLdIngressNormalizer.h` | Mandatory entry for all formalisms; already implemented |
| **IngressGuardConfig** | `src/engine/ingress/IngressGuardConfig.h` | Bounds enforcement; already implemented |
| **IngressDigest** | `src/engine/ingress/IngressDigest.h` | SHA256 determinism binding; already implemented |
| **IdTable** | `src/engine/IdTable.h` | Intermediate result storage; core QLever type |
| **QueryExecutionContext** | `src/engine/QueryExecutionContext.h` | Resource bounds (memory, time); core QLever type |
| **Operation** | `src/engine/Operation.h` | SPARQL integration; strategy pattern base |
| **QueryExecutionTree** | `src/engine/QueryExecutionTree.h` | Query planning integration; core QLever type |
| **AllocatorWithLimit** | `src/util/AllocatorWithLimit.h` | Memory bounds enforcement; core QLever utility |
| **Variable** | `src/parser/data/Variable.h` | Variable representation (SPARQL-based); shared across formalisms |
| **SparqlTriple** | `src/parser/data/SparqlTriple.h` | Triple pattern representation; shared across formalisms |

**Key Design Decision**: Reuse QLever core types (IdTable, QueryExecutionContext, Operation) rather than creating formalism-specific abstractions. This ensures monoidal composition (formalism code composes with QLever core without rework).

---

## 3. Selection Decisions Per Axis (Summary Table)

| Axis | Best-in-Class | Decision | Rationale |
|------|---------------|----------|-----------|
| **1. Ingress** | **SHACL** | MERGE: JsonLdIngressNormalizer (all)<br>REWRITE: Unified exception hierarchy | SHACL's hand-written parser + deterministic validation + guards |
| **2. AST** | **Datalog** | MERGE: Immutable AST pattern<br>REWRITE: Convert SHACL to immutable<br>DISCARD: SHACL's variant model | Datalog's explicit structure + serialization + equality |
| **3. Evaluation** | **Datalog** | MERGE: Fixpoint evaluation kernel<br>REWRITE: SHACL to fixpoint<br>DISCARD: SHACL's DFS | Datalog's semi-naive fixpoint is industry standard |
| **4. Lifecycle** | **SHACL** (Datalog tie) | MERGE: Per-epoch binding + caching<br>REWRITE: Standardize to SEAL phase<br>DISCARD: N3's INGEST-phase | SHACL's multi-level cache + Datalog's generic model |
| **5. Observability** | **SHACL** | MERGE: SHACL violation schema<br>REWRITE: Extend to all formalisms<br>DISCARD: N3's markdown-only | SHACL's W3C-standard violation shape + RDF serialization |
| **6. Determinism** | **Datalog** (N3 tie) | MERGE: DeterminismClassifier + feature detection<br>REWRITE: Add rule-level tests<br>DISCARD: SHACL's implicit model | Datalog's explicit query classification + N3's features |
| **7. Testing** | **SHACL** | MERGE: W3C-style golden tests + negative corpus<br>REWRITE: Add external standards (N3/Datalog)<br>DISCARD: None | SHACL's W3C compliance + N3's explicit negatives |

---

## 4. Merge/Discard/Rewrite Recommendations

### 4.1 MUST MERGE (Mandatory for EPIC 14.1)

These components are **best-in-class** and must be adopted across all formalisms:

1. **JsonLdIngressNormalizer** (SHACL) → All formalisms
   - Path: `src/engine/ingress/JsonLdIngressNormalizer.{h,cpp}`
   - Status: Already shared; formalize as mandatory
   - Integration: Add formalism-specific validation hooks

2. **Immutable AST Pattern** (Datalog) → All formalisms
   - Template: `const fields + AD_SERIALIZE + equality + hashing`
   - Status: Datalog complete; SHACL needs conversion; N3 inherited; ShEx TBD
   - Integration: Create `FormalismAST` base concept

3. **Fixpoint Evaluation Kernel** (Datalog) → All formalisms
   - Path: `src/engine/datalog/FixpointComputation.{h,cpp}`
   - Status: Datalog complete; SHACL needs rewrite; N3 needs implementation; ShEx TBD
   - Integration: Create `FormalismEvaluator` interface

4. **Per-Epoch Lifecycle** (SHACL + Datalog) → All formalisms
   - Pattern: SEAL-phase registration + epoch-bound caching + invalidation
   - Status: SHACL complete; Datalog complete; N3 needs move from INGEST; ShEx TBD
   - Integration: Create `FormalismRegistry` with epoch binding

5. **Violation Schema** (SHACL) → All formalisms
   - Pattern: W3C SHACL report shape + formalism extension
   - Status: SHACL complete; Datalog needs wrapper; N3 needs replacement; ShEx TBD
   - Integration: Create `FormalismViolation` struct with RDF/JSON/Markdown serialization

6. **DeterminismClassifier** (Datalog + N3) → All formalisms
   - Pattern: Explicit non-determinism feature detection
   - Status: Datalog complete (query-level); N3 complete (feature-level); SHACL needs addition; ShEx TBD
   - Integration: Extend `DeterminismClassifier` with formalism-specific features

7. **Testing Framework** (SHACL + N3) → All formalisms
   - Pattern: W3C-style golden tests + explicit negative corpus + determinism tests
   - Status: SHACL has W3C suite; N3 has negatives; Datalog needs external standard; ShEx TBD
   - Integration: Create `FormalismComplianceTest` template

---

### 4.2 MUST REWRITE (Fix Integrity Risks)

These components have **integrity risks** and must be rewritten in EPIC 14.1:

1. **Error Hierarchy Divergence** (Integrity Risk #1)
   - Current: SHACL uses `std::runtime_error`; Datalog uses `ParseException`; N3 inherits
   - Fix: Converge to single `ParseException` with formalism tag
   - Location: All parsers (`ShaclShapeParser`, `DatalogParser`, `N3Parser`)
   - Test: Error handling uniformity test (catch ParseException across all formalisms)

2. **SHACL Mutability Model** (Integrity Risk #2)
   - Current: SHACL uses mutable `std::vector<ShaclConstraint>`
   - Fix: Convert to immutable model (const fields + factory construction)
   - Location: `src/engine/shacl/ShaclShape.h`
   - Test: AST immutability test (attempt to modify post-construction should fail)

3. **Output Type Fragmentation** (Integrity Risk #3)
   - Current: SHACL = ShaclViolation; N3 = markdown; Datalog = tuples
   - Fix: Extend W3C SHACL report shape to cover all formalisms
   - Location: Create `src/engine/formalism/FormalismViolation.h`
   - Test: Violation schema conformance test (all formalisms produce W3C-compatible RDF)

4. **Determinism Testing Gap** (Integrity Risk #4)
   - Current: No rule-level determinism tests for any formalism
   - Fix: Add determinism unit tests (same input → same output, 100 iterations, SHA256 comparison)
   - Location: Create `test/FormalismDeterminismTest.cpp` for each formalism
   - Test: 100% reproducibility (same rule, 100 runs, identical SHA256 hash)

5. **Caching Depth Disparity** (Integrity Risk #5)
   - Current: SHACL has 3-tier cache; Datalog has 1-tier cache
   - Fix: Standardize to 3-tier model (L1: LRU, L2: Bloom, L3: generic)
   - Location: Create `src/engine/formalism/FormalismCache.h`
   - Test: Cache hit rate benchmarks (should be comparable across formalisms)

---

### 4.3 CAN DISCARD (Replaced by Best-in-Class)

These components are **dominated** by better alternatives and can be discarded:

1. **SHACL's std::variant Constraint Model**
   - Replaced by: Datalog's explicit struct model
   - Rationale: Variant requires runtime type checking; explicit structs are compile-time safe

2. **SHACL's Constraint-by-Constraint DFS Evaluation**
   - Replaced by: Datalog's fixpoint evaluation
   - Rationale: Fixpoint is more general; DFS is constraint-specific
   - Note: SHACL's multi-threading can be reintroduced as **parallel fixpoint iteration**

3. **N3's Markdown-Only Output**
   - Replaced by: SHACL's W3C violation schema
   - Rationale: Markdown is not machine-parseable as RDF; SHACL schema is W3C standard

4. **N3's INGEST-Phase Validation**
   - Replaced by: SHACL/Datalog's SEAL-phase validation
   - Rationale: Consistent lifecycle; all formalisms validated at same phase

5. **SHACL's Implicit Determinism Model**
   - Replaced by: Datalog's explicit DeterminismClassifier
   - Rationale: Implicit epoch binding is not sufficient; explicit classification needed

---

### 4.4 MUST PRESERVE (Semantic Differences)

These components reflect **legitimate formalism differences** and must be preserved:

1. **Parser Diversity** (Turtle vs. JSON-LD vs. text)
   - SHACL: Turtle (W3C standard)
   - N3: Turtle (W3C standard)
   - Datalog: Custom text syntax (proprietary)
   - ShEx: TBD (likely JSON-LD or Turtle)
   - Rationale: Different formalisms have different native formats

2. **Validation Strictness**
   - SHACL: Permissive (accepts incomplete shapes per W3C spec)
   - N3: Strict (Turtle validation)
   - Datalog: Strict (syntax validation)
   - Rationale: Different formalisms have different strictness requirements

3. **Determinism Scope**
   - SPARQL/Datalog: Query-level determinism
   - N3: Feature-level determinism
   - SHACL: Epoch-level determinism
   - Rationale: Different scopes appropriate for different formalisms

---

## 5. Integration Roadmap (EPIC 14.1 Implementation Plan)

### Phase 1: Foundation (Mandatory Prerequisites)

**Goal**: Establish shared infrastructure for all formalisms

**Tasks**:
1. Create `src/engine/formalism/` directory (new)
2. Implement `FormalismException.h` (unified exception hierarchy)
3. Implement `FormalismAST.h` (immutable AST concept)
4. Implement `FormalismRegistry.h` (per-epoch registration)
5. Implement `FormalismEvaluator.h` (fixpoint interface)
6. Implement `FormalismViolation.h` (W3C-extended schema)
7. Implement `FormalismCache.h` (3-tier caching)
8. Implement `FormalismDeterminism.h` (extended classifier)

**Deliverables**:
- 8 header files in `src/engine/formalism/`
- Unit tests for each component
- Integration with existing QLever types (IdTable, QueryExecutionContext, Operation)

**Acceptance Criteria**:
- All components compile with existing codebase (no rework)
- All components have 100% test coverage
- All components are epoch-aware and deterministic

---

### Phase 2: SHACL Convergence (Fix Integrity Risks)

**Goal**: Convert SHACL to canonical μ-skeleton

**Tasks**:
1. Convert `ShaclShape` to immutable model (fix integrity risk #2)
2. Replace std::variant with explicit constraint subtypes
3. Replace DFS evaluation with fixpoint evaluation
4. Add explicit DeterminismClassifier (fix integrity risk #4)
5. Add rule-level determinism tests
6. Migrate to unified exception hierarchy (fix integrity risk #1)
7. Standardize to 3-tier caching (fix integrity risk #5)

**Deliverables**:
- Rewritten `ShaclShape.h` (immutable)
- Rewritten `ShaclConstraintEvaluator.cpp` (fixpoint-based)
- New `ShaclDeterminismTest.cpp` (100 iterations, SHA256 comparison)
- Migrated to `FormalismException`

**Acceptance Criteria**:
- All existing SHACL tests pass (including W3C suite)
- No rework of existing code outside SHACL module
- SHACL evaluation is deterministic (100% reproducibility)

---

### Phase 3: Datalog Convergence (Add Missing Components)

**Goal**: Bring Datalog to canonical μ-skeleton

**Tasks**:
1. Wrap Datalog results in `FormalismViolation` (fix integrity risk #3)
2. Add rule-level determinism tests (fix integrity risk #4)
3. Upgrade to 3-tier caching (fix integrity risk #5)
4. Migrate to unified exception hierarchy (fix integrity risk #1)
5. Add W3C-style golden tests (reference: deductive database semantics)

**Deliverables**:
- New `DatalogViolationAdapter.cpp` (wraps tuples in FormalismViolation)
- New `DatalogDeterminismTest.cpp` (100 iterations, SHA256 comparison)
- Upgraded `DatalogCache.h` (L1: LRU, L2: Bloom, L3: generic)
- Migrated to `FormalismException`

**Acceptance Criteria**:
- Datalog results are W3C-compatible RDF (via FormalismViolation)
- Datalog evaluation is deterministic (100% reproducibility)
- No rework of existing code outside Datalog module

---

### Phase 4: N3 Convergence (Add Missing Components)

**Goal**: Bring N3 to canonical μ-skeleton

**Tasks**:
1. Move validation from INGEST phase to SEAL phase
2. Implement evaluation kernel (currently parsing-only)
3. Replace markdown output with `FormalismViolation` (fix integrity risk #3)
4. Add rule-level determinism tests (fix integrity risk #4)
5. Add W3C-style golden tests (reference: W3C N3 Community Group spec)
6. Migrate to unified exception hierarchy (fix integrity risk #1)

**Deliverables**:
- New `N3Evaluator.cpp` (fixpoint-based)
- Rewritten `N3ComplianceVerifier.cpp` (outputs FormalismViolation)
- New `N3DeterminismTest.cpp` (100 iterations, SHA256 comparison)
- Migrated to `FormalismException`

**Acceptance Criteria**:
- N3 has evaluation kernel (not just parsing)
- N3 results are W3C-compatible RDF (via FormalismViolation)
- N3 evaluation is deterministic (100% reproducibility)
- No rework of existing code outside N3 module

---

### Phase 5: ShEx Implementation (From Scratch)

**Goal**: Implement ShEx following canonical μ-skeleton

**Tasks**:
1. Implement ShEx parser (JSON-LD or Turtle)
2. Implement ShEx AST (immutable, serializable)
3. Implement ShEx evaluator (fixpoint-based)
4. Implement ShEx violation adapter (FormalismViolation)
5. Implement ShEx determinism classifier
6. Implement W3C ShEx Test Suite integration

**Deliverables**:
- New `src/engine/shex/` directory
- Complete implementation following canonical μ-skeleton
- W3C ShEx Test Suite compliance

**Acceptance Criteria**:
- ShEx implementation is monoidal (no rework of existing code)
- ShEx passes W3C ShEx Test Suite
- ShEx is deterministic (100% reproducibility)

---

### Phase 6: Cross-Formalism Validation (Final Integration)

**Goal**: Verify all formalisms are converged and deterministic

**Tasks**:
1. Create cross-formalism determinism test suite
2. Create cross-formalism violation schema conformance tests
3. Create cross-formalism caching benchmarks
4. Create cross-formalism SPARQL integration tests
5. Create formalism interoperability tests (SHACL + Datalog, etc.)

**Deliverables**:
- New `test/FormalismIntegrationTest.cpp`
- Determinism receipt (all formalisms 100% reproducible)
- Performance receipt (cache hit rates, query latencies)
- Interoperability receipt (formalism composition works)

**Acceptance Criteria**:
- All formalisms produce identical outputs for identical inputs (SHA256 comparison)
- All formalisms conform to W3C violation schema
- All formalisms have comparable performance (within 2x variance)
- Formalism composition is possible (SHACL + Datalog queries work)

---

## 6. Critical Path and Dependencies

```
┌─────────────────────────────────────────────────────────────────┐
│                     CRITICAL PATH (DAG)                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Phase 1: Foundation                                           │
│    ├─ FormalismException.h                                     │
│    ├─ FormalismAST.h                                           │
│    ├─ FormalismRegistry.h                                      │
│    ├─ FormalismEvaluator.h                                     │
│    ├─ FormalismViolation.h                                     │
│    ├─ FormalismCache.h                                         │
│    └─ FormalismDeterminism.h                                   │
│         ↓                                                       │
│  Phase 2: SHACL Convergence (depends on Phase 1)               │
│    ├─ Convert ShaclShape to immutable                          │
│    ├─ Replace DFS with fixpoint                                │
│    └─ Add determinism tests                                    │
│         ↓                                                       │
│  Phase 3: Datalog Convergence (depends on Phase 1)             │
│    ├─ Wrap results in FormalismViolation                       │
│    ├─ Upgrade caching                                          │
│    └─ Add determinism tests                                    │
│         ↓                                                       │
│  Phase 4: N3 Convergence (depends on Phase 1)                  │
│    ├─ Implement evaluation kernel                              │
│    ├─ Replace markdown with FormalismViolation                 │
│    └─ Add determinism tests                                    │
│         ↓                                                       │
│  Phase 5: ShEx Implementation (depends on Phase 1)             │
│    └─ Implement from scratch using μ-skeleton                  │
│         ↓                                                       │
│  Phase 6: Cross-Formalism Validation (depends on Phases 2-5)   │
│    └─ Integration tests + determinism receipt                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Parallelization Opportunities**:
- Phases 2, 3, 4, 5 can run in parallel (after Phase 1 completes)
- Phase 6 requires all previous phases to complete

**Critical Blocker**: Phase 1 must complete before any formalism-specific work begins

---

## 7. Deterministic Receipts (Validation Criteria)

### 7.1 Phase 1 Receipt (Foundation)

**Requirement**: All foundation components must be:
- Epoch-aware (no cross-epoch state leakage)
- Deterministic (same input → same output)
- Serializable (AD_SERIALIZE support)
- Testable (100% coverage)

**Validation**:
```cpp
// Test: Same epoch, same input → same digest
auto digest1 = FormalismRegistry::registerArtifact(ast, epoch);
auto digest2 = FormalismRegistry::registerArtifact(ast, epoch);
ASSERT_EQ(digest1, digest2);  // SHA256 equality

// Test: Different epochs → different digests
auto digest3 = FormalismRegistry::registerArtifact(ast, epoch + 1);
ASSERT_NE(digest1, digest3);  // Epoch isolation
```

---

### 7.2 Phase 2 Receipt (SHACL Convergence)

**Requirement**: SHACL must:
- Be immutable (no post-construction modification)
- Use fixpoint evaluation (no DFS)
- Be deterministic (100% reproducibility)
- Pass all W3C tests

**Validation**:
```cpp
// Test: Immutability (attempt to modify should fail at compile time)
ShaclShape shape = parser.parse(input);
// shape.constraints.push_back(...);  // Should not compile

// Test: Determinism (100 iterations, same output)
std::set<std::string> hashes;
for (int i = 0; i < 100; ++i) {
  auto result = evaluator.evaluate(shape, data);
  hashes.insert(sha256(result));
}
ASSERT_EQ(hashes.size(), 1);  // Only one unique hash
```

---

### 7.3 Phase 3 Receipt (Datalog Convergence)

**Requirement**: Datalog must:
- Output W3C-compatible violations
- Be deterministic (100% reproducibility)
- Have 3-tier caching
- Have golden tests

**Validation**:
```cpp
// Test: Violation schema conformance
auto result = evaluator.evaluate(rule, data);
ASSERT_TRUE(result.conformsToW3C());  // RDF graph validation

// Test: Determinism (100 iterations, same output)
std::set<std::string> hashes;
for (int i = 0; i < 100; ++i) {
  auto result = evaluator.evaluate(rule, data);
  hashes.insert(sha256(result));
}
ASSERT_EQ(hashes.size(), 1);  // Only one unique hash
```

---

### 7.4 Phase 4 Receipt (N3 Convergence)

**Requirement**: N3 must:
- Have evaluation kernel (not just parsing)
- Output W3C-compatible violations
- Be deterministic (100% reproducibility)
- Load at SEAL phase (not INGEST)

**Validation**:
```cpp
// Test: Evaluation kernel exists
auto result = evaluator.evaluate(n3_rule, data);
ASSERT_FALSE(result.empty());  // Not a stub

// Test: SEAL-phase loading
auto registry = FormalismRegistry::getInstance();
ASSERT_EQ(registry.getLoadPhase(Formalism::N3), Phase::SEAL);
```

---

### 7.5 Phase 5 Receipt (ShEx Implementation)

**Requirement**: ShEx must:
- Follow canonical μ-skeleton
- Pass W3C ShEx Test Suite
- Be deterministic (100% reproducibility)
- Be monoidal (no rework of existing code)

**Validation**:
```cpp
// Test: W3C conformance
auto suite = W3CShExTestSuite::load();
for (auto& test : suite.tests) {
  auto result = evaluator.evaluate(test.schema, test.data);
  ASSERT_EQ(result, test.expected);
}

// Test: Monoidality (no changes to existing files)
auto git_diff = system("git diff --stat");
ASSERT_EQ(git_diff.changed_files_outside_shex, 0);
```

---

### 7.6 Phase 6 Receipt (Cross-Formalism Validation)

**Requirement**: All formalisms must:
- Produce identical outputs for identical inputs (determinism)
- Conform to W3C violation schema (interoperability)
- Have comparable performance (within 2x variance)
- Compose without rework (monoidality)

**Validation**:
```cpp
// Test: Cross-formalism determinism
auto shacl_hash = sha256(shacl_evaluator.evaluate(input, data));
auto datalog_hash = sha256(datalog_evaluator.evaluate(input, data));
// (Note: Inputs are semantically equivalent but formalism-specific)

// Test: Violation schema conformance
ASSERT_TRUE(shacl_result.conformsToW3C());
ASSERT_TRUE(datalog_result.conformsToW3C());
ASSERT_TRUE(n3_result.conformsToW3C());
ASSERT_TRUE(shex_result.conformsToW3C());

// Test: Performance parity
ASSERT_LT(shacl_latency / datalog_latency, 2.0);  // Within 2x
ASSERT_LT(n3_latency / datalog_latency, 2.0);
ASSERT_LT(shex_latency / datalog_latency, 2.0);
```

---

## 8. Risk Mitigation and Failure Modes

### 8.1 Known Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **SHACL refactoring breaks W3C compliance** | HIGH | Run W3C test suite after every change; block merge on failure |
| **Fixpoint evaluation is slower than DFS** | MEDIUM | Benchmark before/after; optimize fixpoint if >2x slower; consider parallel fixpoint |
| **N3 evaluation kernel is underspecified** | HIGH | Defer to W3C N3 Community Group spec; implement subset (80/20) |
| **Cross-formalism determinism is impossible** | HIGH | Fail-closed: if determinism cannot be guaranteed, reject caching |
| **ShEx implementation is too complex** | MEDIUM | Follow μ-skeleton strictly; reuse existing components; start with 80/20 subset |

---

### 8.2 Failure Modes

| Failure Mode | Detection | Recovery |
|--------------|-----------|----------|
| **Foundation phase incomplete** | Unit tests fail | Block all formalism-specific work until fixed |
| **SHACL refactoring breaks tests** | CI failure | Revert to original; isolate breaking change; fix incrementally |
| **Determinism tests fail** | SHA256 comparison fails | Identify non-deterministic component; add explicit ordering; re-test |
| **Performance regression** | Benchmark shows >2x slowdown | Profile hot path; optimize fixpoint; consider caching; revert if unfixable |
| **Cross-formalism integration fails** | Violation schema mismatch | Extend W3C schema; add formalism-specific context; re-validate |

---

### 8.3 Rollback Plan

If any phase fails and cannot be fixed within iteration budget:

1. **Phase 1 Failure**: Abort EPIC 14.1; return to EPIC 14.0 (measurement only)
2. **Phase 2 Failure**: Isolate SHACL; continue with Datalog/N3/ShEx
3. **Phase 3 Failure**: Isolate Datalog; continue with SHACL/N3/ShEx
4. **Phase 4 Failure**: Isolate N3; continue with SHACL/Datalog/ShEx
5. **Phase 5 Failure**: Defer ShEx to EPIC 14.2; complete SHACL/Datalog/N3
6. **Phase 6 Failure**: Accept partial convergence; document incompatibilities

**Fail-Closed Principle**: If full convergence is impossible, accept partial convergence with explicit documentation of deltas.

---

## 9. Appendix: Agent Contribution Summary

### Agent 1 (AST) — Datalog's Immutable Model

**Contribution**:
- Explicit DatalogRule class (head, body, filters)
- Immutability (const references, move semantics)
- Serialization support (AD_SERIALIZE_FRIEND_FUNCTION)
- Equality operators (QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL)

**Selection Decision**: **MERGE** across all formalisms (best-in-class AST pattern)

---

### Agent 2 (Ingress) — SHACL's Parser + Normalization

**Contribution**:
- Hand-written ShaclShapeParser (Turtle)
- JsonLdIngressNormalizer (simdjson-based, shared)
- IngressGuardConfig (max size, depth, timeout)
- IngressDigest (SHA256 determinism binding)

**Selection Decision**: **MERGE** JsonLdIngressNormalizer (already shared); **REWRITE** error hierarchy

---

### Agent 3 (Evaluation) — Datalog's Fixpoint Kernel

**Contribution**:
- FixpointComputation (semi-naive evaluation)
- Iteration limits (fail-closed)
- IdTable integration (QLever core type)
- QueryExecutionContext integration (resource bounds)

**Selection Decision**: **MERGE** as canonical evaluation model; **REWRITE** SHACL to use fixpoint

---

### Agent 4 (Cache) — SHACL's Multi-Level Cache

**Contribution**:
- L1: LRU cache (validation results, epoch-bound)
- L2: Bloom filter (negative lookups, fail-fast)
- L3: Generic cache (QueryExecutionContext fallback)
- Cache eviction (predictable, time-ordered)

**Selection Decision**: **MERGE** as canonical caching model; **REWRITE** Datalog to 3-tier

---

### Agent 5 (Results) — SHACL's Violation Schema

**Contribution**:
- ShaclViolation struct (focusNode, constraintComponent, severity, message)
- ViolationFormatter (RDF, JSON, Markdown serialization)
- W3C SHACL report shape (SPARQL-queryable)
- Operation interface integration

**Selection Decision**: **MERGE** as canonical output model; **REWRITE** all formalisms to use

---

### Agent 6 (Determinism) — Datalog's Classifier + N3's Features

**Contribution**:
- DeterminismClassifier (query-level: NOW, RAND, UUID, SERVICE)
- N3ComplianceVerifier (feature-level: formulae, implications, quantifiers)
- Explicit non-determinism rejection (fail-closed caching)
- Deterministic classification (same query → same result)

**Selection Decision**: **MERGE** both models; **REWRITE** to extend across all formalisms

---

### Agent 7 (Operation) — SHACL's Strategy Pattern

**Contribution**:
- ShaclValidator extends Operation (strategy pattern)
- QueryExecutionTree integration (SPARQL-queryable)
- Thread-safe execution (multi-threaded validation)
- Clean lifecycle (SEAL-phase binding)

**Selection Decision**: **MERGE** Operation integration; **PRESERVE** SHACL's multi-threading (reintroduce as parallel fixpoint)

---

### Agent 8 (SIMD) — Datalog's IdTable Ops

**Contribution**:
- IdTable for intermediate results (SIMD-optimized in QLever core)
- Vectorized join operations (via QueryExecutionTree)
- No formalism-specific SIMD (delegates to core)

**Selection Decision**: **PRESERVE** delegation to QLever core; no formalism-specific SIMD needed

---

### Agent 9 (Testing) — SHACL's W3C Suite + N3's Negatives

**Contribution**:
- SHACL: 13 test files + W3C SHACL Test Suite (golden tests)
- N3: Explicit negative corpus (invalid test cases)
- Datalog: 8+ comprehensive tests (local, no external standard)

**Selection Decision**: **MERGE** W3C-style golden tests + explicit negative corpus; **REWRITE** to add external standards

---

### Agent 10 (CMake) — Build System Integration

**Contribution**:
- Architecture-neutral builds (CMake 3.27+, Ninja)
- Build gates (tests must pass before merge)
- Monoidal composition (formalisms compose without rework)

**Selection Decision**: **PRESERVE** existing build system; add formalism-specific test targets

---

## 10. Closure Conditions

This convergence synthesis is **COMPLETE** if and only if:

1. ✅ **10 agent deliverables analyzed** (AST, Ingress, Evaluation, Cache, Results, Determinism, Operation, SIMD, Testing, CMake)
2. ✅ **Collision detection performed** (overlaps identified: ingress normalization, exception hierarchy, caching models, violation schemas)
3. ✅ **Selection pressure applied** (coverage, invariants, minimality, industrial quality)
4. ✅ **Convergence executed** (merge/discard/rewrite decisions for each axis)
5. ✅ **Refactored output emitted** (unified pipeline architecture, integration roadmap, deterministic receipts)

**CLOSURE**: All phases of EPIC 9 atomic cognitive cycle completed. Output is ready for EPIC 14.1 implementation.

---

## 11. References

### Internal Documents
- `/home/user/qlever/audit/FORMALISM_DELTA_MATRIX.md` — Ground truth comparison
- `/home/user/qlever/audit/FORMALISM_BEST_OF.md` — Best-in-class selections
- `/home/user/qlever/audit/MURA_DELTA_SEVERITY.md` — Integrity risk classification
- `/home/user/qlever/audit/SIMDJSON_USAGE_AUDIT.md` — simdjson usage audit

### External Standards
- W3C SHACL: https://www.w3.org/TR/shacl/
- W3C ShEx: https://shex.io/
- W3C N3 Community Group: https://www.w3.org/community/n3-dev/
- Deductive Databases (Datalog): Academic references (Ullman, "Principles of Database and Knowledge-Base Systems")

### QLever Core Types
- `src/engine/IdTable.h` — Result storage
- `src/engine/QueryExecutionContext.h` — Resource bounds
- `src/engine/Operation.h` — Strategy pattern base
- `src/engine/QueryExecutionTree.h` — Query planning

---

**Document Status**: SYNTHESIS COMPLETE — READY FOR EPIC 14.1 IMPLEMENTATION

**Next Action**: Begin Phase 1 (Foundation) implementation

**Responsible Agent**: Implementation delegated to EPIC 14.1 executor (separate from convergence orchestrator)

---

**Document Hash**: `SHA256(EPIC14_CONVERGENCE_SYNTHESIS.md)` = [computed on write]

**Signature**: Convergence Orchestrator, EPIC 9 Framework

**Date**: 2026-01-03
