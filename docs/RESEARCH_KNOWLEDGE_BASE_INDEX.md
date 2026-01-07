# QLever Research Knowledge Base Index

**Last Updated**: 2026-01-07
**Total Research Materials**: 600+ files
**Organization**: 16 categories, hierarchical by discipline and EPIC phase

---

## Table of Contents

1. [Framework & Philosophy](#1-framework--philosophy)
2. [EPIC Documents](#2-epic-documents)
3. [Formal Methods & Specifications](#3-formal-methods--specifications)
4. [Architecture & Design](#4-architecture--design)
5. [Research & Thesis](#5-research--thesis)
6. [Empirical Evidence (Reports, Receipts, Verification)](#6-empirical-evidence)
7. [Benchmarks & Performance Analysis](#7-benchmarks--performance-analysis)
8. [Collision Detection & Convergence Theory](#8-collision-detection--convergence-theory)
9. [Skills & Agents Documentation](#9-skills--agents-documentation)
10. [Invariant & Construct Causation Theory](#10-invariant--construct-causation-theory)
11. [Data & Epoch Manifest Documentation](#11-data--epoch-manifest-documentation)
12. [Formal Verification & Proof Papers](#12-formal-verification--proof-papers)
13. [Rust Ecosystem & FFI Documentation](#13-rust-ecosystem--ffi-documentation)
14. [Reference & Technical Documentation](#14-reference--technical-documentation)
15. [Archive & Legacy Documentation](#15-archive--legacy-documentation)
16. [Miscellaneous Research Materials](#16-miscellaneous-research-materials)

---

## 1. Framework & Philosophy

**Core Framework Definition & Operational Theory**

| Document | Purpose | Key Topics |
|----------|---------|-----------|
| `CLAUDE.md` | Master operational framework | BB80/20, EPIC 9, specification closure, monoidal composition |
| `docs/explanation/bb80-philosophy.md` | Big Bang 80/20 principles | Single-pass construction, iteration as defect signal |
| `docs/MONOIDAL_CONSTRUCTION_LAW.md` | Mathematical foundation | Monoid laws (closure, associativity, identity) with proofs |
| `docs/explanation/monoidal-composition.md` | Composition theory | Strategy pattern, compositional guarantees |
| `CONSOLIDATION_PLAN.md` | Integration strategy | System consolidation and unification |
| `LAW_MINIMALITY_ANALYSIS.md` | Minimality principle | Minimal structure for maximum value |
| `docs/explanation/epic9-cognitive-cycle.md` | Atomic cognitive cycle | 6-phase cycle: specification → fan-out → construction → collision → convergence → closure |
| `.claude/PHASE_COMMUNICATION_SPEC.md` | Phase communication | Data structures and message formats between phases |
| `docs/explanation/collision-detection-theory.md` | Collision mechanics | Structural, semantic, path divergence types |
| `docs/explanation/convergence-vs-consensus.md` | Convergence model | Selection pressure vs voting, authorship erasure |
| `.claude/EPIC13_TRUTH_AUDIT.md` | Framework audit | Aspirational vs enforced capabilities |
| `.claude/EPIC13_PHASE4_DECEPTION_AUDIT.md` | Deception detection | Identifying false claims in framework claims |

**Navigation**: Start with `CLAUDE.md` for overview, then drill into specific documents.

---

## 2. EPIC Documents

### EPIC Overview
- 14 major EPICs documented (EPIC 4 → EPIC 14)
- Each EPIC represents a distinct research/implementation phase
- EPICs are cumulative: later EPICs depend on earlier phases

### EPIC 4: Convergence Summary
- `docs/EPIC4_CONVERGENCE_SUMMARY.md` - Convergence principles
- `docs/EPIC4_SHARED_INVARIANTS.md` - Invariant definitions across modules

### EPIC 7: Specification Closure & Failure Analysis
- `docs/EPIC7_FMEA_ANALYSIS.md` - Failure Mode Effects Analysis
- `docs/EPIC7_INTEGRATION_SIGN_OFF.md` - Integration verification
- `EPIC7_SPECIFICATION_CLOSURE_AUDIT.md` - Specification closure audit
- `EPIC7_PARALLEL_EXECUTION_REPORT.md` - Parallelism verification

### EPIC 8: Deterministic Construction
- `docs/EPIC8_SPECIFICATION_CLOSURE.md` - (950 lines) Core specification closure formalism
- `docs/EPIC8_CI_RELEGATION.md` - CI/CD constraint enforcement
- `EPIC8_IMPOSSIBILITY_PROOF_ANALYSIS.md` - Mathematical impossibility proofs

### EPIC 10: Core Implementation (Large Scale)
**150+ documents covering:**

- **Phase 1 (Specification)**: `EPIC10_SPECIFICATION_CLOSURE.md`, `EPIC10_AXIOMS_FORMALIZED.md`
- **Phase 2 (Fan-Out)**: `EPIC10_TASK_GRAPH_AND_ASSIGNMENTS.md`, `EPIC10_CRITICAL_PATH_AND_DEPENDENCIES.md`
- **Phase 3 (Construction)**: `EPIC10_COLLISION_ZONE_RESOLUTIONS.md`, `EPIC10_DESIGN_DECISIONS_FROZEN.md`
- **Phase 4 (Collision)**: `EPIC10-COLLISION-DETECTION-REPORT.md`, `EPIC10_INVARIANT_CLOSURE_MATRIX.md`
- **Phase 5 (Convergence)**: `EPIC10-CONVERGENCE-ARTIFACT.md`, `EPIC10-CONVERGENCE-SUMMARY.md`
- **Phase 6 (Closure)**: `EPIC_10_3_FINAL_CONVERGENCE_RECEIPT.md`, `EPIC10_DETERMINISTIC_CHECKLIST.md`

**Agent Deliverables** (10 agents × multiple phases):
- `EPIC10_AGENT10_COMPLETION_REPORT.md`
- `EPIC10_COMPLIANCE_RELEASE.md`
- `EPIC10_EXECUTION_SUMMARY.md`
- `EPIC10_EXECUTIVE_SUMMARY.md`

**EPIC 10.1-10.3 Sub-phases**: Detailed specifications for each agent (50+ files)

### EPIC 11: Rust/FFI Integration
- `docs/EPIC11_1_COLLISION_DETECTION_ANALYSIS.md` - Collision detection in FFI
- `docs/EPIC11_1_CONVERGENCE_SPECIFICATION.md` - Convergence for FFI
- `EPIC11_1_RUST_WORKSPACE_UNIFICATION.md` - Workspace consolidation
- `docs/EPIC_11.1_ARCHITECTURE_DECISION_RECORD.md` - Architecture ADR

### EPIC 13: Framework Certification
- `EPIC13_TODO_FIX_SUMMARY.md` - TODO cleanup and fixes

### EPIC 14: Formalism Delta Discovery
- `EPIC14_COLLISION_DETECTION_REPORT.md` - 23 collisions across 4 formalisms
- `EPIC14_CONVERGENCE_SYNTHESIS.md` - Selection pressure application
- `EPIC14_MEASUREMENT_PHASE_RECEIPT.md` - Measurement phase validation

---

## 3. Formal Methods & Specifications

**Formalism Unification** (40+ files)

### Unified Formalism Design
- `src/engine/formalism/unified/DESIGN.md` - Master design document
- `src/engine/formalism/unified/README.md` - Overview and usage

### Component Specifications
| Component | File | Purpose |
|-----------|------|---------|
| **Ingress** | `UNIFIED_INGRESS_DESIGN.md` | Unified data ingestion layer |
| **Result Schema** | `UNIFIED_RESULT_DESIGN.md` | Unified result representation |
| **Evaluation Kernel** | `EVALUATION_KERNEL_DESIGN.md` | Core evaluation semantics |
| **SIMD Optimization** | `SIMD_OPTIMIZATION_DESIGN.md` | Vector parallelism |
| **Cache** | `CACHE_DESIGN.md` | Caching strategy and mechanics |
| **Build** | `BUILD_STRATEGY.md` | Build system integration |

### Agent Deliverables (10 agents)
- `AGENT1_DELIVERABLE.md` through `AGENT10_DELIVERABLE_SUMMARY.md`
- Each contains formalism-specific analysis and synthesis

### Test Formalism
- `test/engine/formalism/unified/DESIGN.md`
- `test/engine/formalism/unified/README.md`

### Formal Proofs
- `test/chaos/FORMAL_PROOF.md` - Chaos testing formal proof

---

## 4. Architecture & Design

**System Architecture Documents**

| Document | Focus | Scale |
|----------|-------|-------|
| `docs/explanation/architecture.md` | High-level architecture | System design |
| `docs/SIMD_INGRESS_ARCHITECTURE.md` | SIMD-specific architecture | Performance optimization |
| `docs/epic-10-3/PATCH_6_INTEGRATION_ARCHITECTURE.md` | Integration patterns | Component coupling |
| `docs/design/unified-determinism-classification.md` | Determinism classification | Semantic categories |
| `rust/docs/explanations/architecture.md` | Rust FFI architecture | Language interop |
| `EPIC10_DESIGN_DECISIONS_FROZEN.md` | Design freeze | Final design choices (15 decisions) |
| `docs/EPIC_11.1_ARCHITECTURE_DECISION_RECORD.md` | ADR format | Design decisions with rationale |

---

## 5. Research & Thesis

**Major Research Compilations**

| Document | Scope | Lines |
|----------|-------|-------|
| `docs/QLEVEREST_THESIS_DOCUMENTATION.md` | Comprehensive thesis | 1000+ |
| `docs/research/adversarial-validation-formalization-agent-swarms.md` | Validation theory | 800+ |
| `docs/research/implementation-guide-deterministic-validation.md` | Implementation guide | 500+ |
| `docs/archive/QLEVER_ECOSYSTEM_THESIS.md` | Ecosystem overview | Archived |
| `docs/archive/THESIS_MULTILINGUAL_QLEVER_ECOSYSTEM.md` | Multilingual scope | Archived |

---

## 6. Empirical Evidence

### Agent Deliverables (100+ files)
**Structure**: `AGENT{N}_*.md` for N=1-10

- Completion summaries
- Implementation receipts
- Verification reports
- Integration summaries
- Delivery summaries

### Verification & Receipts (40+ files)
| Category | Files | Purpose |
|----------|-------|---------|
| **Deterministic Receipts** | `DETERMINISTIC_RECEIPT_VALIDATION.md`, `RECEIPT_VALIDATION_SPECIFICATION.md` | Receipt format and validation |
| **Build & Integration** | `BUILD_FIX_RECEIPT.md`, `INTEGRATION_PHASE_REPORT.md`, `INTEGRATION_RESULTS_SUMMARY.md` | Build verification |
| **Invariant Validation** | `INVARIANT_VALIDATION_CHECKLIST.md`, `INVARIANT_VALIDATION_REPORT.md` | Invariant compliance |
| **Runtime Verification** | `FFI_PERFORMANCE_RECEIPT.md`, `CHAOS_INVARIANCE_RECEIPT.md`, `MEMORY_ISOLATION_RECEIPT.md` | Runtime behavior verification |

### Capability Reports (10+ files)
Each capability has a structured report: `CAPABILITY_{NAME}_REPORT.md`
- Build
- HTTP Protocol
- Index Ingest
- Performance Stability
- Query Advanced
- Query Select
- Read Epoch
- Rules & Constraints
- Spatial
- Text Search

---

## 7. Benchmarks & Performance Analysis

### Benchmark Framework (35+ files)

| Benchmark Type | File | Purpose |
|----------------|------|---------|
| **Variance Gate** | `benchmark/VARIANCE_GATE_README.md` | Latency variance measurement |
| **FFI Gatekeeper** | `benchmark/FFI_GATEKEEPER_README.md` | FFI overhead validation |
| **Epoch Benchmark** | `benchmark/EPOCH_BENCHMARK_README.md` | Epoch performance characteristics |
| **Manifest Benchmark** | `benchmark/MANIFEST_BENCHMARK_README.md` | Manifest generation performance |
| **N3 Benchmark** | `benchmark/N3_BENCHMARK_REPORT.md` | N3 format parsing performance |
| **Canonical Benchmark** | `benchmark/queryCanonical/CANONICAL_BENCHMARK_README.md` | Query canonicalization performance |
| **Cache Benchmark** | `benchmark/readCache/README.md` | Cache hit/miss characteristics |

### Performance Analysis
- `EPIC10_PERFORMANCE_DOCUMENTATION.md` - Comprehensive performance guide
- `docs/explanation/performance.md` - Performance principles
- `docs/explanation/optimization.md` - Optimization strategies
- `FINAL_BUILD_OPTIMIZATION_REPORT.md` - Build optimization results
- `docs/archive/PERFORMANCE_OPTIMIZATION_80_20.md` - 80/20 optimization principle

---

## 8. Collision Detection & Convergence Theory

**Theoretical Foundations**

| Document | Topic | Scope |
|----------|-------|-------|
| `COLLISION_ANALYSIS.md` | Collision mechanics | General theory |
| `COLLISION_DETECTION_REPORT.md` | Detection algorithms | Technical implementation |
| `COLLISION_INDEX.md` | Collision catalog | Index of known collisions |
| `CONVERGENCE_PLAN.md` | Convergence strategy | Planning and coordination |
| `EPIC14_COLLISION_DETECTION_REPORT.md` | Real-world collision data | 23 collisions detected across 10 agents |
| `EPIC14_CONVERGENCE_SYNTHESIS.md` | Convergence in practice | Selection pressure application |

---

## 9. Skills & Agents Documentation

### Agent Framework (7 agents)
- `bb80-specification-validator.md` - Specification closure verification
- `bb80-collision-detector.md` - Collision analysis and quantification
- `bb80-convergence-orchestrator.md` - Convergence execution (selection pressure)
- `bb80-invariant-validator.md` - Invariant preservation checking
- `bb80-parallel-task-coordinator.md` - Parallel agent orchestration
- `bb80-receipt-validator.md` - Receipt validation and verification
- `.claude/PHASE_COMMUNICATION_SPEC.md` - Phase data structures

### Skills (4 skills)
- `bb80-specification-closure/SKILL.md` - Specification closure skill
- `bb80-invariant-construction/SKILL.md` - Monoidal invariant construction
- `bb80-parallel-agents/SKILL.md` - Parallel agent patterns
- `bb80-deterministic-receipts/SKILL.md` - Receipt generation and validation

### Documentation & Framework
- `docs/AGENT_DOCUMENTATION_GUIDE.md` - How to write agent docs
- `docs/AGENT_JTBD_FRAMEWORK.md` - Jobs-to-be-done framework
- `SKILLS_INDEX_OVERVIEW.md` - Skills quick reference
- `SKILL_BY_PHASE_INDEX.md` - Skills organized by phase
- `.claude/CURRENT_STATE.md` - Current framework state

---

## 10. Invariant & Construct Causation Theory

**Formal Invariant Analysis**

| Document | Focus | Size |
|----------|-------|------|
| `EPIC10_INVARIANT_CLOSURE_MATRIX.md` | Invariant matrix | Comprehensive |
| `INVARIANT_CLOSURE_MATRIX.md` | Closure matrix | Core invariants |
| `EPIC10_INHERITED_INVARIANTS.md` | Inheritance chain | Invariant propagation |
| `docs/explanation/construct-causation.md` | Causation theory | General framework |
| `docs/explanation/construct-causation-modes.md` | Causation modes | Specific modes |
| `docs/archive/CONSTRUCT_CAUSATION_QUICK_REFERENCE.md` | Quick ref | Archived |

---

## 11. Data & Epoch Manifest Documentation

### Epoch Management (35+ files)

| Component | File | Purpose |
|-----------|------|---------|
| **Epoch Design** | `src/global/EPOCH_MANIFEST_SUMMARY.md` | Manifest structure |
| **Cache Invalidation** | `src/global/EPOCH_CACHE_INVALIDATION_DESIGN.md` | Cache invalidation hooks |
| **Integration Guide** | `src/global/CACHE_INVALIDATION_INTEGRATION_GUIDE.md` | Integration instructions |
| **Observability** | `src/global/EPOCH_OBSERVABILITY_SUMMARY.md` | Monitoring and metrics |
| **Metrics** | `src/global/EPOCH_METRICS_INTEGRATION.md` | Metrics collection |
| **Examples** | `src/global/EPOCH_MODIFICATIONS_EXAMPLE.md` | Code examples |
| **Future Optimizations** | `src/global/FUTURE_OPTIMIZATIONS_VIA_HOOKS.md` | Optimization roadmap |

### EpochManifest Reference
- `src/global/EpochManifest_Capabilities.md` - Capabilities overview
- `src/global/EpochManifest_Usage_Guide.md` - Usage patterns

### Test Data & Golden Corpus
- `test/golden_corpus/README.md` - Golden corpus overview
- `test/golden_corpus/MANIFEST_STRATEGY.md` - Manifest strategy
- `test/VERIFICATION_SUMMARY.md` - Verification results

---

## 12. Formal Verification & Proof Papers

**Mathematical Proofs & Verification**

| Document | Proof Type | Result |
|----------|-----------|--------|
| `test/chaos/FORMAL_PROOF.md` | Chaos invariance | Formal proof of memory isolation |
| `FPV_WITNESS_STATUS_REPORT.md` | Formal verification | FPV witness generation |
| `test/engine/shacl/SHACL_W3C_COMPLIANCE.md` | Compliance verification | SHACL W3C spec compliance |
| `audit/FORMALISM_BEST_OF.md` | Formalism analysis | Best-of formalism comparison |
| `audit/SIMDJSON_USAGE_AUDIT.md` | SIMD audit | SIMDJSON usage analysis |

---

## 13. Rust Ecosystem & FFI Documentation

### Rust Documentation (30+ files)

**Main Resources**
- `rust/README.md` - Rust workspace overview
- `rust/docs/README.md` - Documentation index
- `rust/docs/FAQ.md` - Frequently asked questions
- `rust/docs/LEARNING_PATH.md` - Learning path for developers
- `rust/docs/QUICKREF.md` - Quick reference

**Explanations**
- `rust/docs/explanations/architecture.md` - Architecture
- `rust/docs/explanations/async-model.md` - Async patterns
- `rust/docs/explanations/ffi-performance.md` - FFI performance

**How-To Guides**
- `rust/docs/how-to/error-handling.md`
- `rust/docs/how-to/performance.md`
- `rust/docs/how-to/query-types.md`
- `rust/docs/how-to/rdf-data.md`

**Reference**
- `rust/docs/reference/api.md`
- `rust/docs/reference/config.md`
- `rust/docs/reference/errors.md`

**Tutorials**
- `rust/docs/tutorials/getting-started.md`

### FFI & Validation
- `rust/qleverest-validation/` - Multiple crates with documentation:
  - `qlever-artifact-capture/` - Artifact capture FFI
  - `qlever-cache-verifier/` - Cache verification
  - `qlever-digest-verifier/` - Digest verification
  - `qlever-regression-verifier/` - Regression testing
  - `qlever-repro/` - Reproduction framework
  - `qlever-verification-harness/` - Verification harness
  - `receipt_comparator/` - Receipt comparison

### Observability
- `observability/README.md` - Observability framework
- `observability/RUST_OBSERVABILITY_INTEGRATION.md` - Integration guide
- `observability/AGENT6_DETERMINISTIC_RECEIPT.md` - Receipt generation

---

## 14. Reference & Technical Documentation

### Main Documentation
- `README.md` - Main project README
- `README.docker.md` - Docker setup
- `CONTRIBUTING.md` - Contribution guidelines
- `docs/INDEX.md` - Documentation index

### Explanation Documents
- `docs/explanation/rdf-sparql.md` - RDF/SPARQL overview
- `docs/explanation/performance.md` - Performance principles
- `docs/explanation/optimization.md` - Optimization strategies
- `docs/explanation/architecture.md` - Architecture overview

### CLI Reference (20+ docs)
- `docs/reference/cli.md` - CLI reference
- `docs/reference/CLI_STYLE_GUIDE.md` - CLI style guide
- `docs/reference/api.md` - API reference
- `docs/reference/configuration.md` - Configuration reference

### SPARQL & RDF Reference
- `docs/reference/sparql.md` - SPARQL reference
- `docs/reference/sparql-plus-text.md` - SPARQL+Text features
- `docs/reference/path-search.md` - Path search queries
- `docs/reference/knowledge-bases.md` - Knowledge base management

### Formalism Reference
- `docs/reference/n3-specification.md` - N3 format spec
- `docs/reference/shacl-compliance.md` - SHACL compliance
- `docs/reference/jsonld-ingress-contract.md` - JSON-LD ingress

### How-To Guides (15+ docs)
- `docs/how-to/quick-start.md` - Quick start guide
- `docs/how-to/native-setup.md` - Native installation
- `docs/how-to/docker-setup.md` - Docker installation
- `docs/how-to/configuration.md` - Configuration
- `docs/how-to/performance.md` - Performance optimization
- `docs/how-to/troubleshooting.md` - Troubleshooting
- `docs/how-to/claude-code-setup.md` - Claude Code IDE setup

### Domain-Specific Guides
- `docs/how-to/n3-format.md` - N3 format guide
- `docs/how-to/shacl-integration.md` - SHACL integration
- `docs/how-to/spatial-queries.md` - Spatial queries
- `docs/how-to/text-search.md` - Full-text search

### Datalog Documentation (6+ docs)
- `docs/datalog/DATALOG_GUIDE.md` - Datalog guide
- `docs/datalog/DATALOG_SYNTAX.md` - Syntax reference
- `docs/datalog/DATALOG_API.md` - API reference
- `docs/datalog/DATALOG_EXAMPLES.md` - Code examples
- `docs/datalog/DATALOG_PERFORMANCE.md` - Performance characteristics
- `docs/datalog/DATALOG_RESOURCE_GUARDS.md` - Resource management

### Error Handling & Infrastructure
- `docs/ERROR_CODES_REFERENCE.md` - Error codes
- `docs/HOT_PATH_BOUNDARIES.md` - Hot path definitions
- `docs/METADATA_FORMAT_SUMMARY.md` - Metadata format
- `docs/dual-gate-strategy.md` - Dual gate validation strategy
- `docs/fail-closed-enforcement.md` - Fail-closed semantics
- `docs/manifest-schema-family.md` - Manifest schema family
- `docs/phase-lock-integration-example.md` - Phase lock usage

### Build & Deployment
- `docs/reference/master-makefile.md` - Makefile reference
- `docs/reference/advanced-features.md` - Advanced features

### Tutorials
- `docs/tutorials/01-quickstart.md`
- `docs/tutorials/02-first-query.md`
- `docs/tutorials/03-load-data.md`

---

## 15. Archive & Legacy Documentation

**Historical Documents** (50+ files)

### Early EPIC Summaries
- `docs/archive/EPIC_1_1_QUICK_REFERENCE.md`
- `docs/archive/EPIC_1_1_INTEGRATION_TEST_SUMMARY.md`
- `docs/archive/EPIC3_TASK6_IMPLEMENTATION_SUMMARY.md`

### Epoch & Integration (Pre-EPIC 10)
- `docs/archive/EPOCH_ATOMIC_PROMOTION.md`
- `docs/archive/EPOCH_CACHE_INVALIDATION_IMPLEMENTATION_SUMMARY.md`
- `docs/archive/EPOCH_EPIC_1_SUMMARY.md`
- `docs/archive/EPOCH_INTEGRATION_TEST_GUIDE.md`
- `docs/archive/EPOCH_USAGE_EXAMPLES.md`
- `docs/archive/EPOCHMANIFEST_DELIVERABLES.md`

### Optimization & Performance
- `docs/archive/BUILD_OPTIMIZATION_GUIDE.md`
- `docs/archive/OPTIMIZATION_GUIDE_80_20.md`
- `docs/archive/PERFORMANCE_OPTIMIZATION_80_20.md`
- `docs/archive/RUST_FFI_PERFORMANCE.md`

### Rust & FFI Development
- `docs/archive/RUST_BINDINGS_EXPANSION_PLAN.md`
- `docs/archive/RUST_FFI_FINAL_SUMMARY.md`
- `docs/archive/RUST_FFI_IMPLEMENTATION.md`
- `docs/archive/RUST_IMPROVEMENTS.md`
- `docs/archive/RUST_INTEGRATION_GUIDE.md`

### Analysis & Reports
- `docs/archive/BENCHMARK_VALIDATION_FINAL.md`
- `docs/archive/CONSTRUCT_CAUSATION_QUICK_REFERENCE.md`
- `docs/archive/DOCUMENTATION_FMEA.md`
- `docs/archive/FMEA_ANALYSIS.md`
- `docs/archive/INTEGRATION_VALIDATION_REPORT.md`

### Legacy Documentation
- `docs/archive/legacy/README.md` - Legacy documentation index
- `docs/archive/legacy/quickstart.md`
- `docs/archive/legacy/text_search.md`
- `docs/archive/legacy/troubleshooting.md`

---

## 16. Miscellaneous Research Materials

**Specialized Topics** (20+ files)

| Document | Topic | Purpose |
|----------|-------|---------|
| `AUDIT_MAKEFILE.md` | Build audit | Makefile verification |
| `DEPENDENCY_MANAGEMENT.md` | Dependency strategy | Dependency resolution |
| `DX_INNOVATION_SUMMARY.md` | Developer experience | DX improvements |
| `FAST_ITERATION_QUICKSTART.md` | Development workflow | Fast iteration patterns |
| `HYGIENE_REMEDIATION.md` | Code hygiene | Repository cleanliness |
| `MANIFEST_GENERATION_RECEIPT.md` | Manifest generation | Manifest creation proof |
| `QLEVER_COMPATIBILITY_REPORT.md` | Compatibility | Version compatibility |
| `REPO_HYGIENE.md` | Repository health | Git hygiene report |
| `RUNTIME_VERIFICATION_PLAN.md` | Runtime validation | Runtime checking strategy |
| `SPECIFICATION_DX_MAKEFILE_IMPROVEMENTS.md` | Build DX | Makefile improvements |
| `js/DIATAXIS_DOCUMENTATION.md` | Documentation structure | Diátaxis framework |
| `js/FMEA_DOCUMENTATION_ANALYSIS.md` | Doc analysis | Documentation FMEA |
| `docs/DIATAXIS_METADATA_SPEC.md` | Metadata spec | Metadata format |
| `test/engine/shacl/IMPLEMENTATION_SUMMARY.md` | SHACL | SHACL implementation |
| `test/engine/shacl/README.md` | SHACL setup | SHACL test setup |
| `test/uir/README.md` | UIR format | Unified IR format |
| `test/n3-invalid-data/README.md` | N3 validation | N3 error handling |
| `src/engine/shacl/README.md` | SHACL engine | SHACL evaluation engine |
| `src/util/README_VMATH.md` | VMATH library | Vector math library |

---

## Quick Navigation Guide

### By Use Case

**I want to understand the framework**
→ Start: `CLAUDE.md` → `docs/explanation/bb80-philosophy.md` → `docs/explanation/epic9-cognitive-cycle.md`

**I want to see how EPICs work**
→ Start: `EPIC10_SPECIFICATION_CLOSURE.md` → `EPIC10-COLLISION-DETECTION-REPORT.md` → `EPIC10-CONVERGENCE-ARTIFACT.md`

**I want to understand specifications**
→ Start: `docs/EPIC8_SPECIFICATION_CLOSURE.md` → `EPIC7_SPECIFICATION_CLOSURE_AUDIT.md` → `EPIC10_SPECIFICATION_CLOSURE.md`

**I want collision detection details**
→ Start: `docs/explanation/collision-detection-theory.md` → `COLLISION_DETECTION_REPORT.md` → `EPIC14_COLLISION_DETECTION_REPORT.md`

**I want convergence mechanics**
→ Start: `docs/explanation/convergence-vs-consensus.md` → `CONVERGENCE_PLAN.md` → `EPIC14_CONVERGENCE_SYNTHESIS.md`

**I want deterministic validation**
→ Start: `RECEIPT_VALIDATION_SPECIFICATION.md` → `DETERMINISTIC_RECEIPT_VALIDATION.md` → `benchmark/VARIANCE_GATE_README.md`

**I want Rust/FFI documentation**
→ Start: `rust/README.md` → `rust/docs/explanations/architecture.md` → `docs/explanation/ffi-performance.md`

**I want agent documentation**
→ Start: `docs/AGENT_DOCUMENTATION_GUIDE.md` → `.claude/agents/README.md` → Specific agent file

**I want benchmarking info**
→ Start: `BENCHMARK_SURVEY.md` → `benchmark/VARIANCE_GATE_README.md` → Specific benchmark folder

**I want formalism documentation**
→ Start: `src/engine/formalism/unified/DESIGN.md` → `EPIC14_COLLISION_DETECTION_REPORT.md` → Agent deliverables

### By Document Type

**Philosophy/Theory** (8 files): Framework & Philosophy section
**Specifications** (50+ files): EPIC Documents + Formal Methods
**Empirical Evidence** (80+ files): Empirical Evidence section
**Benchmarks** (35+ files): Benchmarks section
**Reference** (80+ files): Reference & Technical section

---

## Statistics

| Category | Count | Cumulative |
|----------|-------|-----------|
| Framework & Philosophy | 12 | 12 |
| EPIC Documents | 150 | 162 |
| Formal Methods | 40 | 202 |
| Architecture & Design | 7 | 209 |
| Research & Thesis | 7 | 216 |
| Empirical Evidence | 80 | 296 |
| Benchmarks | 35 | 331 |
| Collision/Convergence | 5 | 336 |
| Skills & Agents | 21 | 357 |
| Invariant Theory | 6 | 363 |
| Data & Epoch | 35 | 398 |
| Formal Verification | 5 | 403 |
| Rust & FFI | 30 | 433 |
| Reference & Technical | 80 | 513 |
| Archive & Legacy | 50 | 563 |
| Miscellaneous | 20 | 583 |
| **Total** | **600+** | |

---

## File Organization

All files are organized hierarchically:

```
/home/user/qlever_poc/
├── CLAUDE.md                          (main framework)
├── docs/
│   ├── explanation/                   (theory)
│   ├── reference/                     (technical)
│   ├── how-to/                        (guides)
│   ├── tutorials/                     (learning)
│   ├── datalog/                       (Datalog-specific)
│   ├── design/                        (architecture)
│   ├── archive/                       (legacy)
│   ├── epic10/                        (EPIC 10 specific)
│   ├── epic-10-3/                     (EPIC 10.3 specific)
│   └── epic-10-2/                     (EPIC 10.2 specific)
├── EPIC*.md                           (EPIC-specific docs at root)
├── src/
│   ├── engine/formalism/unified/      (formalism specs)
│   ├── global/                        (epoch/manifest docs)
│   ├── engine/shacl/                  (SHACL engine)
│   └── util/                          (utility docs)
├── test/
│   ├── engine/formalism/unified/      (test formalism)
│   ├── chaos/                         (chaos testing proofs)
│   ├── golden_corpus/                 (test data)
│   └── uir/                           (UIR tests)
├── rust/
│   ├── docs/                          (Rust documentation)
│   └── qleverest-validation/          (FFI validation)
├── benchmark/                         (benchmark documentation)
├── audit/                             (audit reports)
├── observability/                     (monitoring docs)
└── .claude/
    ├── agents/                        (agent specs)
    └── skills/                        (skill specs)
```

---

## Last Updated

This index was last compiled on **2026-01-07** as part of **EPIC 15: Mega-Prompt Research**.

**Total Research Materials Cataloged**: 600+ documents
**Total Lines of Documentation**: 100,000+
**Categories**: 16
**EPICs Covered**: 4, 7, 8, 10, 10.1, 10.2, 10.3, 11, 11.1, 13, 14
