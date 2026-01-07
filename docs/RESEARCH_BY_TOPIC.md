# QLever Research Materials by Topic

**Quick lookup guide**: Find research by what you're looking for, not by file structure.

---

## Operational Framework

### Big Bang 80/20
- `CLAUDE.md` - Complete framework definition
- `docs/explanation/bb80-philosophy.md` - Philosophical foundations
- `docs/MONOIDAL_CONSTRUCTION_LAW.md` - Mathematical laws
- `LAW_MINIMALITY_ANALYSIS.md` - Minimality principle

### EPIC 9: Atomic Cognitive Cycle
- `docs/explanation/epic9-cognitive-cycle.md` - 6-phase cycle definition
- `docs/explanation/collision-detection-theory.md` - Collision mechanics
- `docs/explanation/convergence-vs-consensus.md` - Convergence model
- `.claude/PHASE_COMMUNICATION_SPEC.md` - Phase data structures

### Specification Closure
- `docs/EPIC8_SPECIFICATION_CLOSURE.md` - (950 lines) Core specification formalism
- `EPIC10_SPECIFICATION_CLOSURE.md` - Application at scale
- `EPIC7_SPECIFICATION_CLOSURE_AUDIT.md` - Audit and gaps
- `.claude/agents/bb80-specification-validator.md` - Validation rules

### Monoidal Composition
- `docs/explanation/monoidal-composition.md` - Theory and practice
- `docs/MONOIDAL_CONSTRUCTION_LAW.md` - Mathematical proof
- `EPIC10_AXIOMS_FORMALIZED.md` - Formal axioms

### Iteration Prohibition
- `docs/explanation/bb80-philosophy.md` (Section: "Iteration as Defect Signal")
- `EPIC7_SPECIFICATION_CLOSURE_AUDIT.md` - Why iteration occurs

---

## Multi-Agent Systems & Coordination

### 10-Agent Parallelism
- `EPIC10_TASK_GRAPH_AND_ASSIGNMENTS.md` - 10-agent task distribution
- `EPIC10_CRITICAL_PATH_AND_DEPENDENCIES.md` - Dependency analysis
- `.claude/agents/bb80-parallel-task-coordinator.md` - Coordination rules
- `EPIC14_MEASUREMENT_PHASE_RECEIPT.md` - Real-world 10-agent execution

### Collision Detection
- `docs/explanation/collision-detection-theory.md` - 3 collision types
- `COLLISION_DETECTION_REPORT.md` - Detection algorithms
- `EPIC10-COLLISION-DETECTION-REPORT.md` - EPIC 10 collisions
- `EPIC14_COLLISION_DETECTION_REPORT.md` - 23 collisions across 10 agents
- `EPIC11_1_COLLISION_DETECTION_ANALYSIS.md` - Rust FFI collisions
- `.claude/agents/bb80-collision-detector.md` - Guard specifications

### Convergence & Selection Pressure
- `docs/explanation/convergence-vs-consensus.md` - Selection pressure model
- `CONVERGENCE_PLAN.md` - Convergence planning
- `EPIC10-CONVERGENCE-ARTIFACT.md` - Convergence artifact specification
- `EPIC14_CONVERGENCE_SYNTHESIS.md` - Selection pressure applied
- `.claude/agents/bb80-convergence-orchestrator.md` - Convergence algorithm

### Authorship Erasure
- `docs/explanation/convergence-vs-consensus.md` (Section: "Authorship Erasure")
- `.claude/agents/bb80-convergence-orchestrator.md` (Line 99)

---

## Deterministic Validation

### Receipts & Validation
- `RECEIPT_VALIDATION_SPECIFICATION.md` - (930 lines) Receipt format spec
- `DETERMINISTIC_RECEIPT_VALIDATION.md` - Validation checklist
- `.claude/agents/bb80-receipt-validator.md` - Receipt validator rules
- `EPIC_10_3_FINAL_CONVERGENCE_RECEIPT.md` - Real receipt example

### Guards
- `.claude/agents/bb80-receipt-validator.md` - 7 guard specifications
- `src/engine/readPlane/GuardConfiguration.h` - Guard implementation
- `docs/fail-closed-enforcement.md` - Fail-closed semantics

### Benchmarks & Gates
- `benchmark/VARIANCE_GATE_README.md` - Latency variance measurement
- `benchmark/FFI_GATEKEEPER_README.md` - FFI overhead validation
- `BENCHMARK_AUDIT_REPORT.md` - Benchmark review
- `BENCHMARK_SURVEY.md` - Benchmark overview

### Determinism Verification
- `test/chaos/FORMAL_PROOF.md` - Chaos testing proof
- `CHAOS_INVARIANCE_RECEIPT.md` - Chaos test results
- `MEMORY_ISOLATION_RECEIPT.md` - Memory isolation proof
- `FFI_PERFORMANCE_RECEIPT.md` - FFI determinism validation

---

## Architecture & Design

### System Architecture
- `docs/explanation/architecture.md` - High-level overview
- `docs/SIMD_INGRESS_ARCHITECTURE.md` - SIMD architecture
- `EPIC10_DESIGN_DECISIONS_FROZEN.md` - 15 frozen design decisions
- `docs/EPIC_11.1_ARCHITECTURE_DECISION_RECORD.md` - ADR format

### Formalism Unification
- `src/engine/formalism/unified/DESIGN.md` - Master design
- `src/engine/formalism/unified/UNIFIED_INGRESS_DESIGN.md` - Ingress layer
- `src/engine/formalism/unified/UNIFIED_RESULT_DESIGN.md` - Result representation
- `src/engine/formalism/unified/EVALUATION_KERNEL_DESIGN.md` - Evaluation kernel
- `EPIC14_COLLISION_DETECTION_REPORT.md` - 4 formalisms (SHACL, Datalog, N3, ShEx)

### Epoch & Manifest
- `src/global/EPOCH_MANIFEST_SUMMARY.md` - Manifest structure
- `src/global/EPOCH_CACHE_INVALIDATION_DESIGN.md` - Cache invalidation
- `src/global/EPOCH_OBSERVABILITY_SUMMARY.md` - Observability
- `src/global/EpochManifest_Usage_Guide.md` - Usage patterns

### SIMD & Performance
- `src/engine/formalism/unified/SIMD_OPTIMIZATION_DESIGN.md` - SIMD design
- `EPIC10_SIMD_VECTORIZATION.md` - SIMD implementation
- `docs/explanation/optimization.md` - Optimization principles

---

## Implementation Specifics

### Hot-Path Optimization
- `docs/HOT_PATH_BOUNDARIES.md` - Hot-path definitions
- `EPIC10_CRITICAL_PATH_AND_DEPENDENCIES.md` - Critical path
- `docs/explanation/performance.md` - Performance principles

### Memory Management
- `docs/EPIC10.1_CANONICAL_RESULT_SERIALIZATION.md` - Result serialization
- `EPIC10_MEMORY_CONTRACT.md` - Memory contract definition
- `src/util/README_VMATH.md` - Vector math library

### FFI & Rust Integration
- `docs/explanation/ffi-performance.md` - FFI performance theory
- `rust/docs/explanations/architecture.md` - Rust FFI architecture
- `EPIC11_1_RUST_WORKSPACE_UNIFICATION.md` - Workspace consolidation
- `rust/docs/explanations/ffi-performance.md` - FFI optimization

### Cache Architecture
- `src/engine/formalism/unified/CACHE_DESIGN.md` - Cache design
- `AGENT4_CACHE_VERIFICATION.md` - Cache verification
- `src/global/EPOCH_CACHE_INVALIDATION_DESIGN.md` - Cache invalidation

---

## Testing & Verification

### Formal Verification
- `test/chaos/FORMAL_PROOF.md` - Mathematical proof of chaos invariance
- `FPV_WITNESS_STATUS_REPORT.md` - Formal verification results
- `test/engine/shacl/SHACL_W3C_COMPLIANCE.md` - SHACL compliance

### Test Infrastructure
- `test/golden_corpus/README.md` - Golden corpus overview
- `test/VERIFICATION_SUMMARY.md` - Verification results
- `test/PARALLELIZATION_COMPLETE.md` - Parallelization status

### Chaos Testing
- `CHAOS_INVARIANCE_RECEIPT.md` - Chaos test results
- `test/chaos/FORMAL_PROOF.md` - Proof of invariance under chaos

---

## Performance Analysis

### Benchmarking
- `BENCHMARK_SURVEY.md` - Benchmark overview
- `benchmark/VARIANCE_GATE_README.md` - Latency variance measurement
- `benchmark/FFI_GATEKEEPER_README.md` - FFI overhead measurement
- `benchmark/EPOCH_BENCHMARK_README.md` - Epoch performance
- `benchmark/N3_BENCHMARK_REPORT.md` - N3 parsing performance

### Performance Documentation
- `EPIC10_PERFORMANCE_DOCUMENTATION.md` - Comprehensive guide
- `docs/explanation/performance.md` - Performance principles
- `docs/explanation/optimization.md` - Optimization strategies
- `FINAL_BUILD_OPTIMIZATION_REPORT.md` - Build optimization

### Metrics & Monitoring
- `src/global/EPOCH_METRICS_INTEGRATION.md` - Metrics collection
- `src/global/EPOCH_OBSERVABILITY_SUMMARY.md` - Observability
- `observability/RUST_OBSERVABILITY_INTEGRATION.md` - Rust observability

---

## Agent Framework

### Agent Specifications
- `.claude/agents/bb80-specification-validator.md` - Specification validator
- `.claude/agents/bb80-collision-detector.md` - Collision detector
- `.claude/agents/bb80-convergence-orchestrator.md` - Convergence orchestrator
- `.claude/agents/bb80-invariant-validator.md` - Invariant validator
- `.claude/agents/bb80-parallel-task-coordinator.md` - Task coordinator
- `.claude/agents/bb80-receipt-validator.md` - Receipt validator

### Agent Deliverables
- `AGENT1_DELIVERABLE.md` through `AGENT10_COMPLETION_REPORT.md`
- `src/engine/formalism/unified/AGENT{N}_DELIVERABLE.md` (N=1-10)
- `docs/epic-10-3/AGENT_{N}_*.md` files

### Documentation Framework
- `docs/AGENT_DOCUMENTATION_GUIDE.md` - How to write agent docs
- `docs/AGENT_JTBD_FRAMEWORK.md` - Jobs-to-be-done framework
- `SKILLS_INDEX_OVERVIEW.md` - Skills quick reference

---

## Invariants & Constraints

### Core Axioms
- `EPIC10_AXIOMS_FORMALIZED.md` - 6 core axioms
- `docs/EPIC8_SPECIFICATION_CLOSURE.md` - Axiom definitions

### Invariant Closure
- `EPIC10_INVARIANT_CLOSURE_MATRIX.md` - Invariant matrix
- `INVARIANT_CLOSURE_MATRIX.md` - Core invariants
- `EPIC10_INHERITED_INVARIANTS.md` - Invariant propagation
- `INVARIANT_VALIDATION_REPORT.md` - Validation results

### Construct Causation
- `docs/explanation/construct-causation.md` - Causation theory
- `docs/explanation/construct-causation-modes.md` - Causation modes

---

## SPARQL & RDF

### SPARQL Reference
- `docs/reference/sparql.md` - SPARQL query reference
- `docs/reference/sparql-plus-text.md` - SPARQL+Text extension
- `docs/reference/path-search.md` - Graph path queries
- `docs/reference/advanced-features.md` - Advanced features

### RDF & Formats
- `docs/explanation/rdf-sparql.md` - RDF/SPARQL overview
- `docs/reference/n3-specification.md` - N3 format specification
- `docs/reference/jsonld-ingress-contract.md` - JSON-LD ingress
- `benchmark/N3_BENCHMARK_REPORT.md` - N3 performance

### SHACL
- `docs/reference/shacl-compliance.md` - SHACL compliance
- `test/engine/shacl/SHACL_W3C_COMPLIANCE.md` - W3C compliance
- `docs/how-to/shacl-integration.md` - Integration guide

### Datalog
- `docs/datalog/DATALOG_GUIDE.md` - Datalog guide
- `docs/datalog/DATALOG_SYNTAX.md` - Syntax reference
- `docs/datalog/DATALOG_API.md` - API reference
- `docs/datalog/DATALOG_EXAMPLES.md` - Examples
- `docs/datalog/DATALOG_PERFORMANCE.md` - Performance

---

## Development Guides

### Quick Start & Setup
- `docs/how-to/quick-start.md` - Quick start
- `docs/how-to/native-setup.md` - Native setup
- `docs/how-to/docker-setup.md` - Docker setup
- `docs/how-to/claude-code-setup.md` - Claude Code IDE setup

### Configuration & Deployment
- `docs/how-to/configuration.md` - Configuration guide
- `docs/reference/configuration.md` - Configuration reference
- `README.docker.md` - Docker documentation
- `docs/reference/master-makefile.md` - Makefile reference

### Troubleshooting
- `docs/how-to/troubleshooting.md` - Troubleshooting guide
- `docs/how-to/n3-troubleshooting.md` - N3 format issues
- `docs/ERROR_CODES_REFERENCE.md` - Error codes

### Development Workflow
- `FAST_ITERATION_QUICKSTART.md` - Fast iteration guide
- `docs/how-to/fast-iteration-guide.md` - Iteration workflow
- `docs/how-to/performance.md` - Performance optimization

### Rust Development
- `rust/docs/LEARNING_PATH.md` - Rust learning path
- `rust/docs/QUICKREF.md` - Quick reference
- `rust/docs/how-to/error-handling.md` - Error handling
- `rust/docs/how-to/performance.md` - Performance tips

---

## Knowledge Domains

### Text Search
- `docs/how-to/text-search.md` - Full-text search guide
- `CAPABILITY_TEXT_SEARCH_REPORT.md` - Text search capabilities

### Spatial Queries
- `docs/how-to/spatial-queries.md` - Spatial query guide
- `CAPABILITY_SPATIAL_REPORT.md` - Spatial capabilities

### Knowledge Bases
- `docs/reference/knowledge-bases.md` - KB management
- `docs/how-to/configuration.md` - KB configuration

### Data Ingestion
- `docs/how-to/n3-format.md` - N3 ingestion
- `CAPABILITY_INDEX_INGEST_REPORT.md` - Ingest capabilities
- `test/golden_corpus/MANIFEST_STRATEGY.md` - Data strategy

---

## HTTP & Server

### HTTP API
- `docs/reference/cli.md` - CLI reference
- `CAPABILITY_HTTP_PROTOCOL_REPORT.md` - HTTP protocol capabilities

### Query Types
- `rust/docs/how-to/query-types.md` - Query type reference
- `docs/reference/api.md` - API reference

### Performance Stability
- `CAPABILITY_PERFORMANCE_STABILITY_REPORT.md` - Stability metrics

---

## Error Handling & Reliability

### Error Codes
- `docs/ERROR_CODES_REFERENCE.md` - Complete error reference
- `docs/reference/errors.md` - Rust error types

### Fail-Closed Semantics
- `docs/fail-closed-enforcement.md` - Fail-closed design
- `.claude/agents/bb80-receipt-validator.md` - Guard failures

### Reliability & FMEA
- `docs/EPIC7_FMEA_ANALYSIS.md` - Failure mode analysis
- `FMEA_ANALYSIS.md` - FMEA methodology

---

## Meta & Infrastructure

### Documentation Structure
- `docs/INDEX.md` - Main documentation index
- `docs/DIATAXIS_METADATA_SPEC.md` - Metadata specification
- `js/DIATAXIS_DOCUMENTATION.md` - Documentation structure
- `docs/reference/CLI_STYLE_GUIDE.md` - Style guide

### Repository Management
- `CONTRIBUTING.md` - Contribution guidelines
- `REPO_HYGIENE.md` - Repository hygiene report
- `DEPENDENCY_MANAGEMENT.md` - Dependency strategy
- `DX_INNOVATION_SUMMARY.md` - DX improvements

### Build & CI
- `AUDIT_MAKEFILE.md` - Makefile audit
- `docs/EPIC8_CI_RELEGATION.md` - CI constraint enforcement
- `BUILD_FIX_RECEIPT.md` - Build verification

---

## Research Synthesis

### BB80/20 Thesis
- `docs/QLEVEREST_THESIS_DOCUMENTATION.md` - Comprehensive thesis
- `docs/research/adversarial-validation-formalization-agent-swarms.md` - Validation formalization
- `docs/research/implementation-guide-deterministic-validation.md` - Implementation guide

### EPIC 15: Mega-Prompt Research
- `docs/research/adversarial-validation-formalization-agent-swarms.md` - Multi-agent research
- `docs/research/implementation-guide-deterministic-validation.md` - Implementation guide

---

## Archive & Legacy

### Historical Documentation
- `docs/archive/CLAUDE.md` - Original framework document
- `docs/archive/EPIC_1_1_INTEGRATION_TEST_SUMMARY.md` - Early EPIC results
- `docs/archive/EPOCH_ATOMIC_PROMOTION.md` - Early epoch work

### Legacy Tutorials
- `docs/archive/legacy/README.md` - Legacy docs index
- `docs/archive/legacy/quickstart.md` - Original quickstart
- `docs/archive/legacy/text_search.md` - Text search guide
- `docs/archive/legacy/troubleshooting.md` - Troubleshooting

### Performance Archives
- `docs/archive/PERFORMANCE_OPTIMIZATION_80_20.md` - Optimization guide
- `docs/archive/OPTIMIZATION_GUIDE_80_20.md` - 80/20 optimization
- `docs/archive/RUST_FFI_PERFORMANCE.md` - FFI performance

---

## Statistics & Metrics

**Research Materials by Topic**:
- Framework: 30+ documents
- Multi-Agent: 25+ documents
- Validation: 40+ documents
- Architecture: 20+ documents
- Performance: 35+ documents
- Testing: 15+ documents
- Development: 25+ documents
- Knowledge Domains: 10+ documents
- Infrastructure: 20+ documents
- Archive: 50+ documents

**Total Documents Across All Topics**: 600+
**Total Lines of Documentation**: 100,000+
**Estimated Reading Time**: 200+ hours at 500 words/hour

---

## How to Use This Index

1. **Find a specific topic** → Use the table of contents
2. **Look up a question** → Scan the relevant section
3. **Explore an area** → Start with the first document, follow references
4. **Get comprehensive overview** → Read the "See Also" cross-references

---

## Cross-Reference Map

Key documents often referenced together:

- **Specification Closure**: `EPIC8_SPECIFICATION_CLOSURE.md` ↔ `EPIC10_SPECIFICATION_CLOSURE.md` ↔ `EPIC7_SPECIFICATION_CLOSURE_AUDIT.md`
- **Collision & Convergence**: `collision-detection-theory.md` ↔ `convergence-vs-consensus.md` ↔ `EPIC14_COLLISION_DETECTION_REPORT.md`
- **Deterministic Validation**: `RECEIPT_VALIDATION_SPECIFICATION.md` ↔ `DETERMINISTIC_RECEIPT_VALIDATION.md` ↔ `benchmark/VARIANCE_GATE_README.md`
- **Agent Framework**: `.claude/agents/*.md` ↔ `docs/AGENT_DOCUMENTATION_GUIDE.md` ↔ `AGENT*_DELIVERABLE.md`
- **Epoch & Manifest**: `src/global/EPOCH_MANIFEST_SUMMARY.md` ↔ `src/global/EPOCH_CACHE_INVALIDATION_DESIGN.md` ↔ `test/golden_corpus/MANIFEST_STRATEGY.md`
- **Formalism**: `src/engine/formalism/unified/DESIGN.md` ↔ `EPIC14_COLLISION_DETECTION_REPORT.md` ↔ `AGENT*_DELIVERABLE.md`

---

Last Updated: **2026-01-07**
See Also: [`docs/RESEARCH_KNOWLEDGE_BASE_INDEX.md`](RESEARCH_KNOWLEDGE_BASE_INDEX.md) for hierarchical organization
