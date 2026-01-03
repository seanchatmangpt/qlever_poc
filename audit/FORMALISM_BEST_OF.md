# EPIC 14.0 — Best-in-Class Implementation Selection (Per Axis)

**Phase**: Delta Discovery
**Status**: SELECTION ONLY — NO REFACTORING PERMITTED
**Date**: 2026-01-03

---

## Objective

For each structural axis, identify which formalism currently has the **cleanest**, **most deterministic**, **most industrial** implementation. This is **selection**, not voting.

**Selection Criteria**:
1. **Cleanest**: Code is idiomatic, minimal, easy to reason about
2. **Most Deterministic**: Explicit guarantees; no surprises
3. **Most Industrial**: Production-ready; proven by tests; no aspirational code
4. **No Compromises**: Not "adequate for now" — actual quality

---

## Axis 1: Ingress (Parser + Error Handling + Validation)

### WINNER: **SHACL**

**Rationale**:
- Explicit hand-written parser (ShaclShapeParser.cpp) with clear control flow
- Deterministic Turtle format ingestion (standardized by W3C)
- Integrated validation at parse time (shapes checked during loading)
- Shared JsonLdIngressNormalizer layer with deterministic digest binding
- Guard enforcement with explicit fail-closed semantics
- Error messages are human-readable strings (cold path only)

**Runners-up**:
- **Datalog**: Custom parser is clean but lacks standardized format (proprietary syntax)
- **N3**: Delegates to Turtle parser (minimal code); lacks control; inherits all Turtle limitations
- **ShEx**: Stub only; no selection possible

---

## Axis 2: AST / Internal Representation

### WINNER: **Datalog**

**Rationale**:
- Explicit DatalogRule class with clear head, body, filters structure
- Immutable-by-design (const references, move semantics)
- Complete serialization support (AD_SERIALIZE_FRIEND_FUNCTION)
- Equality operators defined (QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL)
- Variable class reused from SPARQL (consistent type system)
- No mutable shared state; garbage-collection-friendly

**Runners-up**:
- **SHACL**: Variant-based constraints powerful but complex; mutable; no explicit hashing
- **N3**: Inherits immutability from TripleComponent (clean but not designed for N3)
- **ShEx**: No AST; selection not applicable

---

## Axis 3: Evaluation Kernel

### WINNER: **Datalog**

**Rationale**:
- Explicit fixpoint computation (FixpointComputation.cpp) with iteration limits
- Semi-naive evaluation is an industry standard algorithm (proven, optimized)
- Uses IdTable for all intermediate results (consistent with QLever's core architecture)
- No special casing; uniform treatment of predicates
- Bounded by iteration limit (fail-closed)
- Integrates with QueryExecutionContext for resource management

**Runners-up**:
- **SHACL**: Multi-threaded parallelization is strong, but constraint evaluation is constraint-specific (not generalizable)
- **N3**: No evaluation kernel at all (only parsing)
- **ShEx**: No kernel; selection not applicable

---

## Axis 4: Lifecycle / Epoch Integration

### WINNER: **SHACL** (Tie with Datalog)

**Rationale**:
- Per-epoch ShaclValidator operation (clear SEAL-phase binding)
- Explicit lifecycle: shape registry invalidation on re-registration
- Multi-level caching strategy (LRU + Bloom filter) with epoch binding
- Cache eviction is predictable (LRU time order, Bloom filter bit-level)
- Integration with QueryExecutionTree (Operation trait) is clean

**Alternative: Datalog** (equal quality):
- Per-epoch RuleDatabase with clear SEAL-phase binding
- Rule invalidation on modification (predictable)
- Generic result caching via QueryExecutionContext
- No lifecycle surprises; minimal code

**Runners-up**:
- **N3**: Eager feature analysis at load (non-standard for a parser)
- **ShEx**: Stub only

---

## Axis 5: Observability

### WINNER: **SHACL**

**Rationale**:
- ShaclViolation objects with structured schema (focusNode, constraintComponent, severity, message)
- ViolationFormatter produces human-readable reports and RDF-serializable violations
- W3C standard violation shape (SPARQL-queryable RDF graph)
- Integration with Operation interface (results flow through QueryExecutionTree)
- Detailed contextual information (not just boolean)

**Runners-up**:
- **Datalog**: Tuple-based results (standard SPARQL format); lacks semantic context
- **N3**: Markdown text reports (human-readable but not machine-parseable as RDF)
- **ShEx**: No observability; selection not applicable

---

## Axis 6: Determinism Controls

### WINNER: **Datalog** (Tie with N3)

**Rationale**:
- DeterminismClassifier explicitly walks query tree and classifies all non-deterministic features
- Features: NOW(), RAND(), UUID(), STRUUID(), BNODE(), SERVICE clauses
- Classification is deterministic (same query → same classification every time)
- Fail-closed: non-deterministic queries are rejected for caching

**Alternative: N3** (equal quality):
- N3ComplianceVerifier explicitly detects unsupported non-deterministic features (formulae, implications, quantifiers, variables)
- Feature detection is comprehensive
- Report is deterministic and reproducible

**Runners-up**:
- **SHACL**: Implicit determinism (cache binding + epoch); no explicit non-determinism detection
- **ShEx**: Stub only

---

## Axis 7: Testing Discipline

### WINNER: **SHACL**

**Rationale**:
- 13 test files with structured test suites
- W3C SHACL Test Suite compliance (gold standard)
- Tests cover core constraints, logical shapes, property paths, SPARQL constraints
- Skipped tests are explicitly marked (GTEST_SKIP) with rationale
- Negative cases implicit in violation expectations

**Runners-up**:
- **Datalog**: 8+ comprehensive test files; strong coverage but no formal external standard
- **N3**: 2 integration tests + explicit negative corpus (invalid test cases); weak positive coverage
- **ShEx**: No tests; selection not applicable

---

## Summary: Best-in-Class Candidates

| Axis | Best-in-Class | Runner-up | Notes |
| ---- | ------------- | --------- | ----- |
| **1. Ingress** | **SHACL** | Datalog | SHACL's hand-written parser + deterministic Turtle is cleanest |
| **2. AST** | **Datalog** | SHACL | Datalog's explicit structure + immutability + serialization wins |
| **3. Evaluation** | **Datalog** | SHACL | Datalog's fixpoint is industrial standard; SHACL's parallelization strong |
| **4. Lifecycle** | **SHACL/Datalog** | N3 | Both have clean per-epoch binding; SHACL's caching adds depth |
| **5. Observability** | **SHACL** | Datalog | SHACL's rich violation schema beats Datalog's tuples |
| **6. Determinism** | **Datalog/N3** | SHACL | Both explicit; Datalog query-level, N3 feature-level |
| **7. Testing** | **SHACL** | Datalog | SHACL has W3C suite; Datalog has strong integration tests |

---

## Architectural Template Selection (Preliminary)

### If we were to create a **canonical μ-skeleton** for formalism integration:

**Best components to reuse** (not yet refactoring; just noting):

| Component | Source | Reason |
| --------- | ------ | ------ |
| **Ingress Pipeline** | SHACL | Hand-written parser + deterministic validation model |
| **AST Representation** | Datalog | Explicit structure + serialization + equality |
| **Evaluation Kernel** | Datalog | Fixpoint iteration + IdTable integration + resource bounds |
| **Lifecycle Management** | SHACL (with Datalog's caching model) | Per-epoch binding + invalidation strategy |
| **Observability** | SHACL (with Datalog's tuple format for compatibility) | Rich violation schema + SPARQL integration |
| **Determinism Controls** | Datalog (with N3's feature detection) | Explicit classification + non-determinism rejection |
| **Testing Framework** | SHACL (with N3's negative corpus structure) | W3C-like golden tests + negative cases |

**This is NOT a recommendation.** This is information for **EPIC 14.1 Convergence**.

---

## Caveats and Limitations

1. **ShEx is stub-only** — Cannot select from non-existent implementation; included for completeness
2. **N3 is 80/20** — Only basic Turtle-level features supported; advanced N3 (formulae, rules, variables) not evaluated
3. **Quality vs. Completeness** — SHACL is more complete (13 tests) but Datalog's design is cleaner; we selected on quality, not coverage
4. **Determinism Gap** — No formalism has explicit **rule-level** determinism tests; only query-level tests exist
5. **No Cross-Formalism Testing** — Each formalism tested in isolation; no determinism validation across formalisms

---

## Open Questions for EPIC 14.1

1. Should the canonical template prioritize Datalog's design (cleaner AST + evaluation) or SHACL's completeness (more tests)?
2. How to handle N3's missing evaluation kernel? Inherit from Datalog or create new?
3. Should ShEx remain stub-only or be implemented following the canonical template?
4. Where should cross-formalism determinism tests live?

---

**Status**: SELECTION COMPLETE — Awaiting EPIC 14.1 Convergence Instruction
