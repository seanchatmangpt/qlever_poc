# EPIC 14.0 — Formalism Capability Delta Matrix

**Phase**: Delta Discovery (Measurement Only)
**Status**: MEASUREMENT COMPLETE — NO REFACTORING PERMITTED
**Date**: 2026-01-03

---

## Objective

Ground-truth comparison of SHACL, ShEx, N3, and Datalog implementations across 7 structural axes. This document records **facts only** — no opinions, no fixes, no improvements. Classification of deltas is deferred to `MURA_DELTA_SEVERITY.md`.

---

## Axis 1: Ingress (File Formats, Parsers, Error Handling, Validation Strictness)

| Axis        | SHACL | ShEx | N3 | Datalog | Notes |
| ----------- | ----- | ---- | -- | ------- | ----- |
| **File Format** | Turtle | JSON-LD (stub) | Turtle | Text (custom syntax) | SHACL/N3 reuse Turtle; Datalog is unique |
| **Parser Type** | Hand-written recursive descent (ShaclShapeParser.cpp) | Stub only (no implementation) | Template-based extension of TurtleParser | Custom recursive descent (DatalogParser.cpp) | SHACL/Datalog use custom; N3 reuses |
| **Lexing/Tokenization** | Implicit in parser | None | Inherited from Turtle (Tokenizer<_T>) | Explicit tokenizer (DatalogTokenizer.h) | N3 zero overhead; Datalog has dedicated tokenizer |
| **Error Handling Style** | String-based messages; no formal exception taxonomy | N/A (stub) | Inherited Turtle error handling | ParseException + formal error types | Inconsistent styles: string vs. exception |
| **Error Propagation** | Throws std::runtime_error on parse failure | N/A | Throws from TurtleParser (inherited) | Throws ParseException | Two exception hierarchies in use |
| **Validation Strictness at Ingress** | Permissive (accepts incomplete shapes) | N/A | Strict Turtle validation (inherited) | Strict syntax validation | Range from permissive to strict |
| **JSON-LD Ingress Layer** | JsonLdIngressNormalizer (shared) | JsonLdIngressNormalizer (shared) | JsonLdIngressNormalizer (shared) | JsonLdIngressNormalizer (shared) | All four share normalization layer with guards |
| **Ingress Guard Enforcement** | IngressGuardConfig enforced (max size, depth, timeout) | IngressGuardConfig enforced | IngressGuardConfig enforced | IngressGuardConfig enforced | Uniform guard structure across all four |
| **Determinism at Ingress** | Normalized via IngressDigest + SHA256 hash | Normalized via IngressDigest + SHA256 hash | Normalized via IngressDigest + SHA256 hash | Normalized via IngressDigest + SHA256 hash | All produce deterministic digests |

---

## Axis 2: AST / Internal Representation

| Axis | SHACL | ShEx | N3 | Datalog | Notes |
| ---- | ----- | ---- | -- | ------- | ----- |
| **Primary Data Structure** | Variant-based ShaclConstraint + NodeShape + PropertyShape | None (stub) | TripleComponent (inherited from Turtle) | Explicit DatalogRule class | SHACL uses tagged union; Datalog explicit |
| **Mutability Model** | Mutable: vectors and maps for constraints | N/A | Immutable TripleComponent (const getters) | Immutable DatalogRule (const references) | SHACL mutable; others immutable |
| **Constraint Representation** | std::variant<int, double, string, NodeKind, vector, ...> | N/A | Triple patterns (S, P, O components) | SparqlTriple + SparqlFilter | SHACL uses variant; others explicit structs |
| **Rule/Pattern Storage** | Per-PropertyShape constraint vector | N/A | Vector of SparqlTriple per pattern | Vector of SparqlTriple in body; head predicate name | Parallel use of vector<SparqlTriple> |
| **Variable Representation** | String identifiers for nodeIds/shapeIds | N/A | Variable class (inherited from SPARQL) | Variable class (SPARQL-based) | N3/Datalog share Variable class |
| **Hashability** | NodeShape/PropertyShape not hashable | N/A | TripleComponent has implicit hashability | DatalogRule has explicit hash support (serialization) | SHACL lacks explicit hashing |
| **Canonical Ordering** | No explicit ordering guarantees | N/A | Turtle canonical form (inherited) | No explicit ordering (rules unordered in RuleDatabase) | Datalog has ordering opacity |
| **Memory Ownership** | Value semantics (copy/move) for constraints | N/A | Shared ownership via TripleComponent | Value semantics (move constructors) | Mix of ownership models |

---

## Axis 3: Evaluation Kernel

| Axis | SHACL | ShEx | N3 | Datalog | Notes |
| ---- | ----- | ---- | -- | ------- | ----- |
| **Traversal Strategy** | Constraint-by-constraint evaluation with depth-first recursion (RecursiveShapeValidator) | None (stub) | None (no evaluation) | Fixpoint iteration with semi-naive evaluation | SHACL DFS; Datalog fixpoint |
| **Fixpoint Logic** | Recursive shape validation (implicit fixpoint for recursive constraints) | N/A | N/A | Explicit fixpoint computation (FixpointComputation.cpp) with iteration limit | SHACL implicit; Datalog explicit |
| **Join Strategy** | Implicit in constraint evaluation (no explicit join) | N/A | No joins | IdTable join via QueryExecutionTree (filtered rules) | Datalog has structured join via IdTable |
| **Reuse of IdTable / ValueId** | Yes: values passed as strings; no ValueId conversion | N/A | No (Turtle-level parsing) | Yes: heavily (IdTable for all rule results) | SHACL strings; Datalog uses ValueId |
| **Filter Application** | Pattern matching (regex, in-list, min/max) in constraint evaluator | N/A | N/A | Explicit SparqlFilter vector evaluated in body | Pattern-based vs. explicit filters |
| **SIMD Usage** | No SIMD in constraint evaluation | N/A | No SIMD | No SIMD in rule evaluation (IdTable uses SIMD elsewhere) | No formalism-specific SIMD |
| **Parallelization** | Multi-threaded validation (ShaclValidator with thread pool config) | N/A | No parallelization | Fixpoint iteration is sequential | SHACL parallelizable; Datalog sequential |
| **Caching at Kernel Level** | Multi-level: LRU cache + Bloom filter for negative lookups (ShaclValidationCache) | N/A | No kernel-level cache | Result caching via query execution context | SHACL has specialized cache; Datalog generic |
| **Recursion Handling** | Explicit: RecursiveShapeValidator for sh:node/sh:shape | N/A | N/A | Explicit: recursive rule detection + stratification checks | SHACL named recursion; Datalog predicate recursion |

---

## Axis 4: Lifecycle / Epoch Integration

| Axis | SHACL | ShEx | N3 | Datalog | Notes |
| ---- | ----- | ---- | -- | ------- | ----- |
| **Load Phase** | SEAL (shape loading via ShaclShapeRegistry.registerShape) | N/A (stub) | INGEST (compliance verification) | SEAL (rule loading via RuleDatabase.addRule) | Both shapes/rules loaded at SEAL; N3 at INGEST |
| **Epoch Binding** | Per-epoch ShaclValidator operation (cache bound to epoch) | Per-epoch via JsonLdIngressNormalizer token | Per-epoch via N3ComplianceVerifier | Per-epoch via DatalogEpochIsolationTest | All four epoch-aware |
| **Invalidation Rules** | Shape registry invalidation on shape re-registration | N/A | Compliance state invalidated on N3 content change | Rule database invalidation on rule modification | Registry/database models differ |
| **Caching Behavior** | Three-tier: validation result cache + constraint cache + Bloom filter | N/A | Compliance report cached by file digest | Result caching via execution context | Varying cache depths |
| **Eager vs. Lazy Loading** | Lazy constraint evaluation (on first query) | N/A | Eager feature analysis (ComplianceVerifier analyzes at load) | Lazy rule expansion (on query planning) | SHACL/Datalog lazy; N3 eager |
| **Reuse Across Epochs** | Shapes are epoch-specific (ShaclShapeRegistry per epoch) | N/A | N3 compliance state epoch-specific | Rules are epoch-specific (RuleDatabase per epoch) | All isolation models are epoch-local |
| **Snapshot/Checkpoint** | No explicit snapshot mechanism | N/A | Compliance report is snapshot | No explicit snapshot (rules stored in RuleDatabase) | N3 has report snapshots; others implicit |

---

## Axis 5: Observability

| Axis | SHACL | ShEx | N3 | Datalog | Notes |
| ---- | ----- | ---- | -- | ------- | ----- |
| **Output Form (Primary)** | Structured validation report (ShaclViolation objects) | N/A | Compliance report (markdown text + structured issues) | SPARQL result set (IdTable) | SHACL/Datalog binary; N3 textual |
| **Result Type** | Boolean conform + detailed violations | N/A | Boolean isCompatible + issue list | Query results (tuples) | SHACL/N3 validating; Datalog producing |
| **RDF Graph Shape** | Validation report as RDF graph (ShaclReportShape) | N/A | None (compliance text only) | Results convertible to RDF via SPARQL | SHACL native RDF; Datalog indirect |
| **SPARQL Exposure** | ShaclValidator is an Operation (integrated in QueryExecutionTree) | N/A | No direct SPARQL exposure | Datalog results exposed via QueryPlanner | Both expose to SPARQL; N3 does not |
| **Violation Structure** | FocusNode + ConstraintComponent + Message + Severity | N/A | Feature + LineNumber + LineContent + IsSupported | Result columns determined by query | Schemas differ radically |
| **Human Readability** | ViolationFormatter produces formatted messages | N/A | Markdown report with line-by-line detail | SPARQL result format (TSV/JSON) | SHACL best; Datalog tabular |
| **Machine Parsability** | Structured violation objects (deserializable) | N/A | Structured ComplianceIssue vector (JSON-serializable) | Result format is standard SPARQL | All parseable; different formats |

---

## Axis 6: Determinism Controls

| Axis | SHACL | ShEx | N3 | Datalog | Notes |
| ---- | ----- | ---- | -- | ------- | ----- |
| **Explicit Determinism Checks** | Integrated via QueryExecutionContext epoch tracking | RuleLanguageDialect enum in JsonLdIngressNormalizer | Feature detection in N3ComplianceVerifier (hasFormulae, hasVariable, etc.) | DeterminismClassifier analyzes query features (NOW, RAND, SERVICE) | SHACL/Datalog via query; N3 via feature analysis |
| **Non-Determinism Rejection** | Implicit: validation results are deterministic given input | Stub only | N3ComplianceVerifier flags unsupported features (non-deterministic) | DeterminismClassifier flags non-deterministic operations | N3/Datalog explicit; SHACL implicit |
| **Determinism Guarantees** | Per-epoch validation cache with SHA256 digest binding | Guard identity hash in IngressGuardConfig | Compliance digest from file SHA256 | Query fingerprint classification | All four use digest-based determinism |
| **Determinism Tests** | No explicit determinism unit tests | N/A | No explicit determinism tests | No explicit determinism tests for rules (only query-level) | Gap: no rule-level determinism testing |
| **Non-Determinism Logging** | No logging in hot path (fail-closed) | N/A | None (cold-path only) | None (determinism flags set but no enforcement) | Varying enforcement models |

---

## Axis 7: Testing Discipline

| Axis | SHACL | ShEx | N3 | Datalog | Notes |
| ---- | ----- | ---- | -- | ------- | ----- |
| **Unit Tests** | 13 test files: `ShaclComplianceTest.cpp`, `ShaclConstraintEvaluatorTest.cpp`, etc. | None (stub) | 2 integration tests + invalid data corpus | 8+ files: `DatalogQueryPlannerTest.cpp`, `DatalogRuleTest.cpp`, etc. | SHACL/Datalog have extensive tests; N3 minimal |
| **Golden Tests** | W3C SHACL Test Suite (ShaclComplianceTest + W3CShaclTestSuiteTest) | N/A | No formal golden corpus | No formal golden corpus (27 hybrid SPARQL/Datalog test queries) | Only SHACL has W3C suite |
| **Determinism Tests** | No explicit determinism validation tests | N/A | No determinism tests | No rule determinism tests (only query-level in other modules) | Gap: no cross-formalism determinism tests |
| **Disabled / Skipped Tests** | W3C_Class_Constraint marked GTEST_SKIP (not implemented) | N/A | None | None observed | SHACL acknowledges gaps; others silent |
| **Test Coverage Scope** | Core constraints + logical shapes + property paths + SPARQL constraints | N/A | Parser acceptance + compliance validation | Rule parsing + planning + fixpoint + epoch isolation | SHACL broadest; Datalog focused |
| **Negative Test Cases** | Implicit (violations expected in tests) | N/A | Explicit negative corpus: `invalid-escape.n3`, `malformed-iri.n3`, etc. | Implicit | N3 has best negative corpus |
| **Integration Tests** | ShaclPlanningStrategyTest (integrates with QueryPlanner) | N/A | N3IntegrationTest | DatalogIntegrationTest, QueryPlannerDatalogIntegrationTest | All have some integration coverage |

---

## Summary: Implementation Maturity

| Formalism | Status | Ingress | AST | Evaluation | Lifecycle | Observability | Determinism | Testing |
| --------- | ------ | ------- | --- | ---------- | --------- | ------------- | ----------- | ------- |
| **SHACL** | **Complete** | ✓ Custom | ✓ Variant-based | ✓ Full DFS | ✓ Per-epoch | ✓ Rich reports | ✓ Implicit | ✓ 13 tests + W3C |
| **ShEx** | **Stub Only** | ✗ Enum stub | ✗ None | ✗ None | ✓ Skeleton | ✗ None | ✓ Enum | ✗ None |
| **N3** | **Functional (80/20)** | ✓ Inherited | ✓ Immutable | ✗ None | ✓ Per-epoch | ✗ Text-only | ✓ Feature flags | ✓ 2 tests + corpus |
| **Datalog** | **Complete** | ✓ Custom | ✓ Explicit | ✓ Fixpoint | ✓ Per-epoch | ✓ Tuple-based | ✓ Query-level | ✓ 8+ tests |

---

## Deltas by Axis (Unclassified)

**Axis 1 (Ingress)**:
- Parser diversity: hand-written (SHACL, Datalog) vs. template-based (N3) vs. stub (ShEx)
- Error hierarchy: std::runtime_error vs. ParseException
- Strictness variation: permissive (SHACL) vs. strict (N3)
- Shared ingress normalization layer (all four use JsonLdIngressNormalizer + guards)

**Axis 2 (AST)**:
- Mutability: SHACL mutable; N3/Datalog immutable
- Representation: variant (SHACL) vs. explicit structs (N3, Datalog)
- Hashability gap in SHACL

**Axis 3 (Evaluation)**:
- Traversal: DFS (SHACL) vs. fixpoint (Datalog) vs. none (N3)
- Caching depth: 3-tier (SHACL) vs. 1-tier (Datalog)
- Parallelization: SHACL supports; Datalog does not

**Axis 4 (Lifecycle)**:
- Load phase: SEAL (SHACL, Datalog) vs. INGEST (N3)
- Caching model: specialized (SHACL) vs. generic (Datalog)

**Axis 5 (Observability)**:
- Output types: structured violations (SHACL) vs. markdown (N3) vs. tuples (Datalog)
- SPARQL integration: native (SHACL, Datalog) vs. none (N3)

**Axis 6 (Determinism)**:
- Enforcement model: implicit (SHACL) vs. explicit flags (N3, Datalog)
- Testing gap: no cross-formalism determinism tests

**Axis 7 (Testing)**:
- Coverage: 13 tests (SHACL) vs. 8+ (Datalog) vs. 2 (N3) vs. 0 (ShEx)
- W3C compliance: only SHACL has formal W3C test suite
- Negative corpus: only N3 has explicit invalid test cases

---

## Data Quality Notes

- **SHACL**: Complete implementation; all data collected from 20+ source files
- **ShEx**: Stub-only; data collected from single enum + ingress normalizer
- **N3**: Functional 80/20; data from TurtleParser inheritance + compliance verifier
- **Datalog**: Complete implementation; all data collected from 15+ source files
- **Shared Ingress Layer**: All four use JsonLdIngressNormalizer with unified guard configuration

---

**Status**: MEASUREMENT PHASE COMPLETE — Awaiting EPIC 14.1 Instruction
**Next Phase**: MURA Delta Severity Classification (no refactoring)
