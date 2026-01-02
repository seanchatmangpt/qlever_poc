# EPIC 10: Adversarial Roadmap - Parallel Task Decomposition

**Generated**: 2026-01-02
**Branch**: claude/epic-10-adversarial-roadmap-ZiSkI
**Methodology**: 10-agent parallel analysis + collision detection + convergence
**Status**: Specification Closure - 8 phases + 10 agent assignments + critical path analysis

---

## EXECUTIVE SUMMARY

EPIC 10 decomposes QLever's architecture into **8 distinct phases** with **10 independent agent ownership clusters**. This roadmap enables:
- **Parallel feature development** (80% concurrency target)
- **Backward compatibility** (no API changes)
- **Deterministic invariant preservation** (monoidal composition)
- **Risk isolation** (critical path identification)

### Key Metrics
- **Total Components**: 9 (engine, index, parser, util, rdfTypes, global, libqlever, backports, build system)
- **Parallelizable Work**: 7/9 components (78% independent)
- **Sequential Dependencies**: 2/9 components (parser → engine, util → all)
- **Critical Path Length**: 12 weeks (parser hardening → engine optimization → integration tests)
- **Parallel Paths**: 6 independent workstreams

---

## PHASE DECOMPOSITION (8 Phases)

### Phase 1: SPECIFICATION & INVARIANT CLOSURE
**Duration**: 2 weeks
**Sequential**: Yes (gates all other phases)
**Owners**: Agents 1, 2 (Specification + Validation)

**Deliverables**:
- Formalize 6 core invariants (monoidal property, determinism, no mutable external state, backward compatibility, immutability, atomicity)
- Create INVARIANT_CLOSURE_MATRIX documenting all 20+ architectural invariants
- Define acceptance predicates for each phase
- Document forbidden patterns

**Success Criteria**:
- All design choices eliminated (zero degrees of freedom)
- Every component has written invariant specification
- CI/CD gates defined for invariant violations

**Handoff**: Written specification to Phase 2

---

### Phase 2: ARCHITECTURE ANALYSIS & RISK MAPPING
**Duration**: 1 week
**Sequential**: No (parallel to Phase 1 final days)
**Owners**: Agents 3, 4, 5 (Core components, types, concurrency)

**Deliverables**:
- Component dependency graph (DAG verification)
- Call graph for hot paths (engine, index, join operations)
- Memory model formalization (IdTable, AllocatorWithLimit constraints)
- Concurrency risk matrix (Synchronized<T>, lock patterns, race conditions)

**Components Analyzed**:
1. **engine/** (226 files): Query execution, operations, joins, optimization
2. **index/** (52 files): RDF storage, vocabulary, compression, FTS
3. **parser/** (48 files): SPARQL/N3 parsing, Datalog, AST construction
4. **rdfTypes/** (15 files): Data type definitions, encoding/decoding
5. **util/** (142 headers): Allocators, concurrency, memory management

**Success Criteria**:
- Dependency DAG has no cycles
- All unsafe patterns documented and risk-rated (CRITICAL, HIGH, MEDIUM, LOW)
- Memory usage bounds defined for each component

**Handoff**: Architecture blueprint to Phase 3

---

### Phase 3: CAPABILITY HARDENING (Parallel: 6 workstreams)
**Duration**: 6 weeks
**Sequential**: No (fully parallel, dependency on Phase 2 only)
**Owners**: Agents 6, 7, 8, 9, 10, and supporting sub-teams

**Parallel Workstreams** (Independent, no coordination):

#### Workstream 3A: Parser Hardening
**Owner**: Agent 6
**Components**: src/parser/ + src/parser/sparqlParser/ + src/parser/data/
**Work Items**:
- Add SPARQL 1.1 compliance tests (W3C test suite integration)
- Implement dataflow analysis for query canonicalization
- Create parser error recovery (fail-safe, not fail-fast)
- Add benchmark suite for parsing performance (variance < 2%)

**Invariants to Preserve**:
- Immutable AST (no post-parse mutation)
- Deterministic parse order (no hash-based ordering in result structures)
- No external state pollution (parser is pure function)

**Tests**: 45+ new tests, 100% code coverage on error paths

---

#### Workstream 3B: Index Hardening
**Owner**: Agent 7
**Components**: src/index/ + src/index/vocabulary/
**Work Items**:
- Formalize CompressedRelation invariants (permutation encoding correctness)
- Add deterministic ordering to vocabulary iteration
- Implement index consistency validators (checksums, proof trees)
- Optimize FTS algorithms with parallelism guards

**Invariants to Preserve**:
- Immutable index post-construction
- Deterministic compression/decompression (no floating-point, no approximation)
- Vocabulary mapping is bijective (1:1 with RDF universe)

**Tests**: 60+ index-specific tests, permutation correctness proofs

---

#### Workstream 3C: Engine Core Optimization
**Owner**: Agent 8
**Components**: src/engine/ (operation hierarchy, joins, optimization)
**Work Items**:
- Refactor Operation hierarchy for cost-based optimization determinism
- Implement adaptive join optimizer with cost bounds
- Add SIMD vectorization to hottest join operations (CartesianProductJoin, IndexScan)
- Create cost model validator (plan cost vs. actual time)

**Invariants to Preserve**:
- No mutable query execution state (QueryExecutionTree is immutable)
- Deterministic join ordering (cost model is pure function)
- Cancellation via SharedCancellationHandle only (no global flags)

**Tests**: 80+ engine tests, 15+ benchmark suites, cost model validation

---

#### Workstream 3D: Memory Management & Allocators
**Owner**: Agent 9
**Components**: src/engine/idTable/ + src/util/ (allocators, memory)
**Work Items**:
- Formalize IdTable memory contract (row-major layout, no COW semantics)
- Implement AllocatorWithLimit compliance validator
- Add memory pressure test suite (OOM resilience)
- Optimize memory pooling for query execution pipeline

**Invariants to Preserve**:
- Fixed memory bounds (no unbounded allocation)
- Deterministic memory layout (no pointer-dependent state)
- Exception-safe deallocation (RAII, destructors always safe)

**Tests**: 40+ allocator tests, stress tests for OOM scenarios

---

#### Workstream 3E: Concurrency & Synchronization
**Owner**: Agent 10
**Components**: src/util/Synchronized.h + ConcurrentCache + SharedCancellationHandle
**Work Items**:
- Formalize Synchronized<T> thread-safety contract (fine-grained locking)
- Add ThreadSanitizer compliance (TSan --report-signal-unsafe=0)
- Implement benign race detector (for intentional shared-memory patterns)
- Create deadlock prevention validator (timeout analysis)

**Invariants to Preserve**:
- No deadlocks under any execution order
- No data races on protected state
- Cancellation propagates atomically to all tasks

**Tests**: 50+ concurrency tests, TSan cleanup, stress tests with 128 threads

---

#### Workstream 3F: Global State & Backports
**Owner**: Agent 1 (continuing from Phase 1)
**Components**: src/global/, src/backports/, src/ad_utility/
**Work Items**:
- Eliminate global mutable state (move to runtime contexts)
- Audit backports for C++20 compliance (no deprecated constructs)
- Formalize configuration management (RuntimeParameters immutability)
- Add epoch/snapshot support for multi-version consistency

**Invariants to Preserve**:
- No mutable singletons (GlobalState → thread-local or context-injected)
- Deterministic initialization order
- Configuration frozen after startup (RuntimeImmutable after construction)

**Tests**: 25+ global state tests

---

**Phase 3 Success Criteria**:
- All 6 workstreams complete with zero integration issues
- 280+ new tests passing, 0 flakes in 10 runs each
- Memory overhead < 5% vs. baseline
- Query latency regression < 2%
- Code coverage on changes ≥ 95%

**Handoff**: Hardened components to Phase 4

---

### Phase 4: INTEGRATION & COMPATIBILITY TESTING
**Duration**: 3 weeks
**Sequential**: Yes (gates Phase 5, depends on Phase 3)
**Owners**: Agents 2, 3 (Integration validators)

**Integration Points** (Critical):
1. **Parser → Index**: AST canonicalization doesn't break vocabulary encoding
2. **Index → Engine**: Compression format invisible to query planner
3. **Engine → Results**: IdTable layout assumptions hold under all join combinations
4. **Concurrency → All**: Synchronized<T> + ConcurrentCache safe under 256 concurrent queries

**Test Suites**:
- **TPC-H Benchmark Suite**: All 22 queries must complete in < 5% regression
- **Compatibility Matrix**: 10 prior QLever versions × current features
- **Determinism Tests**: Same query deterministic hash across 100 runs
- **Stress Tests**: 1M+ triple dataset, 512 concurrent connections

**Blocker Resolution**:
- Any regression > 5%: analyze root cause, create issue, propose fix
- Any flaky test: understand non-determinism source, eliminate
- Any backward compatibility break: document migration path or revert

**Success Criteria**:
- TPC-H: 22/22 queries pass within performance SLA
- No regressions in existing query patterns
- 0 flakes in 10 full test runs
- Determinism validated: manifest.sha256 identical across environments

**Handoff**: Validated integrated system to Phase 5

---

### Phase 5: PERFORMANCE OPTIMIZATION & PROFILING
**Duration**: 2 weeks
**Sequential**: No (parallel with Phase 4 overlap acceptable)
**Owners**: Agents 4, 5 (Memory + concurrency performance)

**Hot Path Optimization** (Based on agent analysis):
1. **CartesianProductJoin**: SIMD + memory prefetch
2. **IndexScan**: Cache-aware permutation access pattern
3. **GroupByImpl**: Hash table optimization, allocation pooling
4. **VocabularyLookup**: Perfect hashing or binary search

**Profiling & Benchmarking**:
- Perf flame graphs (CPU cycles distribution)
- Memory allocation profiler (jemalloc stats)
- Lock contention analysis (Synchronized<T> hold times)
- Cache miss rates (L1, L2, L3, TLB)

**Benchmarks**:
- **Throughput**: queries/sec on TPC-H (must improve > 0%)
- **Latency**: P50, P95, P99 response time (regression < 2%)
- **Memory**: Peak RSS under concurrent load (no > 10% increase)
- **Scalability**: Linear or better scaling with thread count

**Success Criteria**:
- ≥ 1 hot path optimized with measurable improvement
- Benchmark regressions explained and acceptable
- Profiling data published (flame graphs, tables)

**Handoff**: Performance baseline to Phase 6

---

### Phase 6: DOCUMENTATION & DESIGN RECORDS
**Duration**: 1 week
**Sequential**: No (parallel to Phase 5)
**Owners**: Agents 1, 2 (Specification refinement)

**Artifacts**:
- **ARCHITECTURE.md**: Component responsibilities, interfaces, invariants
- **CONCURRENCY_MODEL.md**: Thread-safety guarantees, lock hierarchy, deadlock prevention
- **MEMORY_MODEL.md**: IdTable layout, allocator contracts, memory bounds
- **QUERY_EXECUTION.md**: Operation hierarchy, cost model, optimization strategy
- **DETERMINISM.md**: Sources of non-determinism, mitigation strategies
- **DEVELOPMENT_GUIDE.md**: How to add new operations, index types, query patterns

**Design Decision Records** (ADR format):
- ADR-1: Why Synchronized<T> vs. atomic_ref
- ADR-2: Why IdTable row-major vs. column-major
- ADR-3: Why eager evaluation in some operations vs. lazy everywhere
- ADR-4: Why manual memory management vs. Rust/GC

**Success Criteria**:
- Architecture document complete and reviewed
- Every public API has documented invariants
- Every design decision recorded with rationale

**Handoff**: Documentation package to Phase 7

---

### Phase 7: COMPLIANCE & INVARIANT VALIDATION
**Duration**: 1 week
**Sequential**: No (parallel to Phase 6)
**Owners**: Agents 6, 7, 8 (Validation teams)

**Invariant Validation**:
- **AX-1 (Immutability)**: Static analysis (no mutable statics, only Synchronized<T>)
- **AX-2 (Determinism)**: Determinism validator (hashes of 100 builds must match)
- **AX-3 (Atomicity)**: Atomic failure validator (no partial state on failure)
- **AX-4 (No External State)**: Static analysis (no global non-thread-local state)
- **AX-5 (RAII)**: Memory safety validator (valgrind/AddressSanitizer clean)
- **AX-6 (Backward Compatibility)**: Compatibility matrix validation (all prior versions still work)

**Automated Enforcement**:
- CI/CD gates: all invariant checks must pass before merge
- Pre-commit hooks: local validation of high-risk changes
- Runtime assertions: Synchronized<T> debug checks, allocation limits

**Success Criteria**:
- All 6 core axioms validated ✓
- 0 invariant violations reported
- CI integration complete and passing

**Handoff**: Validated codebase to Phase 8

---

### Phase 8: CLOSURE & DEPLOYMENT PREPARATION
**Duration**: 1 week
**Sequential**: Yes (final integration gate)
**Owners**: Agents 1, 2, 3 (Final review + release)

**Closure Checklist**:
- ✓ All 8 phases completed or marked as intentional skip
- ✓ 0 open blockers (all issues resolved or documented)
- ✓ Release notes drafted (features, changes, migrations)
- ✓ Semantic versioning updated (major/minor/patch)
- ✓ Deprecation notices sent (if any)
- ✓ Performance regression analysis published
- ✓ Breaking changes documented with migration guide
- ✓ Rollback procedure documented

**Deployment Preparation**:
- Canary deployment plan (1% traffic)
- Rollback triggers and procedures
- Monitoring dashboards (QPS, latency, errors)
- SLA metrics (99.9% availability, P99 < 500ms)

**Success Criteria**:
- All closure conditions met
- Ready for production deployment
- Runbook for on-call engineers published

---

## 10 AGENT ASSIGNMENTS

### Agent 1: SPECIFICATION & GLOBAL STATE LEAD
**Responsibility**: Specification closure, global state elimination, final release orchestration
**Phases**: 1 (lead), 2 (support), 6 (doc), 7 (validation), 8 (lead)
**Components**: CLAUDE.md, src/global/, configuration system
**Handoff Points**: Phase 1→2 (invariant spec), Phase 3→4 (component hardening), Phase 8 (release)

**Invariants to Enforce**:
- All design choices formalized before implementation
- No global mutable state outside Synchronized<T>
- Configuration immutable after startup

---

### Agent 2: ARCHITECTURE & INTEGRATION VALIDATOR
**Responsibility**: Dependency DAG verification, integration testing, cross-phase validation
**Phases**: 2 (lead), 3 (support across workstreams), 4 (lead), 6 (support), 8 (support)
**Components**: Entire codebase dependency analysis, CMakeLists.txt optimization
**Handoff Points**: Phase 2→3 (architecture blueprint), Phase 4→5 (integration results), Phase 8 (final validation)

**Invariants to Enforce**:
- No circular dependencies in build system
- All components compile independently
- No API changes that break backward compatibility

---

### Agent 3: PARSER HARDENING & DATAFLOW LEAD
**Responsibility**: SPARQL/N3 parser compliance, AST immutability, canonicalization
**Phases**: 3A (lead), 4 (support), 7 (validation)
**Components**: src/parser/, src/parser/sparqlParser/, src/parser/data/
**Handoff Points**: Phase 3→4 (parser compliance), Phase 7→8 (validation proof)

**Invariants to Enforce**:
- W3C SPARQL 1.1 compliance
- AST immutable after construction
- Deterministic parse order (no hash randomization)
- Error recovery without state pollution

---

### Agent 4: MEMORY MANAGEMENT & TYPE SYSTEM LEAD
**Responsibility**: IdTable contracts, allocator limits, type encoding determinism
**Phases**: 3D (lead), 5 (performance profiling), 7 (validation)
**Components**: src/engine/idTable/, src/util/, src/rdfTypes/
**Handoff Points**: Phase 3→4 (memory contracts), Phase 5→6 (memory model documentation)

**Invariants to Enforce**:
- Fixed memory bounds on all allocations
- IdTable layout deterministic (row-major, no COW)
- Type encoding/decoding bijective

---

### Agent 5: CONCURRENCY & SYNCHRONIZATION LEAD
**Responsibility**: Thread-safety contracts, cancellation propagation, deadlock prevention
**Phases**: 3E (lead), 5 (performance profiling), 7 (validation)
**Components**: src/util/Synchronized.h, ConcurrentCache, SharedCancellationHandle
**Handoff Points**: Phase 3→4 (concurrency validation), Phase 5→6 (concurrency model doc)

**Invariants to Enforce**:
- No data races on shared state
- No deadlocks under any execution order
- Cancellation propagates atomically

---

### Agent 6: INDEX HARDENING & VALIDATION LEAD
**Responsibility**: Index structure formalization, vocabulary bijection, consistency validation
**Phases**: 3B (lead), 4 (support), 7 (lead)
**Components**: src/index/, src/index/vocabulary/, CompressedRelation
**Handoff Points**: Phase 3→4 (index compliance), Phase 7→8 (validation proof)

**Invariants to Enforce**:
- Index immutable post-construction
- Vocabulary mapping is bijective (1:1)
- Deterministic permutation encoding

---

### Agent 7: QUERY ENGINE OPTIMIZATION LEAD
**Responsibility**: Operation hierarchy refactoring, join optimization, cost model determinism
**Phases**: 3C (lead), 4 (support), 5 (hot path optimization), 7 (validation)
**Components**: src/engine/, src/engine/sparqlExpressions/
**Handoff Points**: Phase 3→4 (engine refactoring), Phase 5→6 (optimization results)

**Invariants to Enforce**:
- QueryExecutionTree immutable during execution
- Cost model is pure function (deterministic)
- No global state pollution in operation execution

---

### Agent 8: ENGINE INTEGRATION & STRESS TESTING LEAD
**Responsibility**: Integration test orchestration, TPC-H compliance, stress testing
**Phases**: 3 (support across workstreams), 4 (lead), 5 (support), 7 (validation)
**Components**: test/ suite, benchmark/
**Handoff Points**: Phase 4→5 (integration validation), Phase 7→8 (compliance proof)

**Invariants to Enforce**:
- All components integrate without API breakage
- TPC-H 22/22 queries pass within SLA
- 0 flaky tests across 10 full runs

---

### Agent 9: MEMORY PROFILING & PERFORMANCE OPTIMIZATION LEAD
**Responsibility**: Flame graph analysis, allocation pooling, cache optimization
**Phases**: 3D (support), 5 (lead), 6 (support)
**Components**: src/engine/idTable/, memory-intensive operations
**Handoff Points**: Phase 5→6 (performance baseline), Phase 6→7 (optimization documentation)

**Invariants to Enforce**:
- Memory regression < 10% under concurrent load
- Allocation patterns deterministic (no randomization)
- Cache utilization optimized (L1/L2/L3 hit rates)

---

### Agent 10: DETERMINISM VALIDATOR & CLOSURE LEAD
**Responsibility**: Determinism validation, manifest hash consistency, closure verification
**Phases**: 3E (support), 7 (lead), 8 (support)
**Components**: Entire codebase (determinism perspective), CI/CD enforcement
**Handoff Points**: Phase 7→8 (determinism proof), Phase 8 (release readiness)

**Invariants to Enforce**:
- manifest.sha256 identical across 100 builds
- No floating-point, no approximation in determinism-critical paths
- No hash-based ordering in output structures

---

## EXECUTION ENVELOPE (Constraints)

### MANDATORY CONSTRAINTS
1. **No API Changes** (backward compatibility)
   - All public headers maintain ABI compatibility
   - New features are additive only
   - Deprecation notices for planned removals (next major version)

2. **Deterministic Output** (monoidal property)
   - manifest.sha256 must be identical across builds
   - No floating-point in index encoding/decoding
   - No hash randomization in iteration

3. **Memory Bounds** (fixed allocation)
   - AllocatorWithLimit enforces hard bounds
   - OOM scenarios fail gracefully (not segfault)
   - Memory overhead < 10% vs. baseline

4. **Atomic Failure** (fail-closed semantics)
   - All phases fail atomically (no partial state)
   - Build stops at first error (set -e semantics)
   - Rollback automatic on failure

5. **Thread Safety** (no data races)
   - Synchronized<T> required for shared state
   - No raw mutexes or atomics
   - ThreadSanitizer must pass clean

6. **Concurrency Native** (not retrofitted)
   - All new operations assume concurrent execution
   - Cancellation via SharedCancellationHandle
   - No global state pollution

---

## CRITICAL PATH ANALYSIS

### Timeline (Weeks)
```
Week 1-2:   Phase 1 (Specification)
Week 3-9:   Phase 2 (Analysis) + Phase 3 (Hardening, 6 parallel workstreams)
            ├─ Workstream 3A: Parser (6 weeks)
            ├─ Workstream 3B: Index (6 weeks)
            ├─ Workstream 3C: Engine (6 weeks)
            ├─ Workstream 3D: Memory (6 weeks)
            ├─ Workstream 3E: Concurrency (6 weeks)
            └─ Workstream 3F: Global State (6 weeks)
Week 10-12: Phase 4 (Integration Testing, serial)
Week 12-13: Phase 5 (Performance, parallel to Phase 4 end)
Week 13:    Phase 6 (Documentation, parallel to Phase 5)
Week 13:    Phase 7 (Validation, parallel to Phase 6)
Week 14:    Phase 8 (Closure)
```

### Critical Path
1. Phase 1: Specification (2 weeks) - GATES ALL OTHERS
2. Phase 2: Analysis (1 week) - gates Phase 3 start but doesn't block parallel work
3. Phase 3: Hardening (6 weeks, PARALLEL) - critical work
4. Phase 4: Integration (3 weeks, SERIAL) - longest serial path
5. Phase 8: Closure (1 week)

**Total Critical Path**: 13 weeks (Phases 1 → 2 → 3 → 4 → 8)

### Parallel Paths (Can hide behind critical path)
- Phase 5: Performance (2 weeks, can start week 12)
- Phase 6: Documentation (1 week, can start week 13)
- Phase 7: Validation (1 week, can start week 13)

### Parallelism Opportunity
- **Weeks 3-9**: 6 independent workstreams (78% parallelism) ✓
- **Weeks 10-13**: 4 parallel phases (Phases 4, 5, 6, 7 with overlap) ✓

---

## HANDOFF PROTOCOL

Each phase transition requires:
1. **Artifact Delivery**: All deliverables committed to main branch
2. **Acceptance Criteria**: 100% of phase success criteria met
3. **Invariant Validation**: All phase-level invariants proven or waived
4. **Documentation**: Design decisions recorded in ADR format
5. **Test Coverage**: ≥95% code coverage on changes, 0 flakes in 10 runs

**Blocker Resolution**: Any unmet criteria → phase lead addresses in real-time, no deferral

---

## RISK MATRIX

### CRITICAL RISKS (Probability: Medium, Impact: High)

| Risk | Mitigation | Owner |
|------|-----------|-------|
| Circular dependency in engine/index/parser | Dependency DAG validator in CI; refactor if found | Agent 2 |
| Determinism loss in concurrency | TSan + determinism validator on every merge | Agent 10 |
| TPC-H regression > 5% | Hotpath profiling; roll back unsafe optimization | Agent 7 |
| Memory blowup under concurrent load | Allocator limiter enforced at test time | Agent 4 |

### HIGH RISKS (Probability: Low, Impact: High)

| Risk | Mitigation | Owner |
|------|-----------|-------|
| API breakage discovered in Phase 4 | Compatibility matrix covers 10 prior versions | Agent 2 |
| Flaky test in benchmark suite | Benchmark run 10x; compute variance; exclude if > 2% | Agent 8 |
| Non-determinism in vocabulary encoding | Bijection proof + consistency validator | Agent 6 |
| Deadlock in concurrent query execution | Deadlock prevention validator; timeout analysis | Agent 5 |

### MEDIUM RISKS (Probability: High, Impact: Medium)

| Risk | Mitigation | Owner |
|------|-----------|-------|
| Specification incomplete (iteration needed) | Phase 1 gates all others; design choices frozen | Agent 1 |
| Workstream overlap (3A/3B/3C diverge) | Integration tests catch mismatches early (Phase 4) | Agent 8 |
| Performance regression in low-impact operations | TPC-H focus; other regressions < 2% acceptable | Agent 7 |

---

## DELIVERABLES SUMMARY

| Phase | Owner | Artifact | Format | Size |
|-------|-------|----------|--------|------|
| 1 | Agent 1 | INVARIANT_CLOSURE_MATRIX | Markdown | 20KB |
| 2 | Agent 2 | ARCHITECTURE_DEPENDENCY_DAG | Graphviz + Markdown | 15KB |
| 3A | Agent 3 | PARSER_COMPLIANCE_SUITE | 45 tests + docs | 25KB |
| 3B | Agent 6 | INDEX_CONSISTENCY_VALIDATOR | 60 tests + docs | 30KB |
| 3C | Agent 7 | ENGINE_OPTIMIZATION_REPORT | Flame graphs + ADRs | 35KB |
| 3D | Agent 4 | MEMORY_MODEL_SPEC | Markdown + proofs | 20KB |
| 3E | Agent 5 | CONCURRENCY_VALIDATOR | TSan report + docs | 20KB |
| 3F | Agent 1 | GLOBAL_STATE_ELIMINATION | Code + tests | 15KB |
| 4 | Agent 8 | INTEGRATION_TEST_RESULTS | TPC-H report + matrix | 25KB |
| 5 | Agent 9 | PERFORMANCE_BASELINE | Flame graphs + tables | 30KB |
| 6 | Agents 1-2 | ARCHITECTURE.md + CONCURRENCY_MODEL.md | Design docs | 40KB |
| 7 | Agents 6-10 | INVARIANT_VALIDATION_PROOF | Static analysis report | 25KB |
| 8 | Agents 1-2 | RELEASE_NOTES + RUNBOOK | Markdown | 15KB |

**Total Deliverables**: 18 artifacts, ~290KB documentation, 280+ tests

---

## SUCCESS CRITERIA (CLOSURE GATE)

All conditions required for EPIC 10 closure:

- ✓ All 8 phases executed (no skips without formal waiver)
- ✓ All 10 agents delivered phase artifacts
- ✓ Collision detection: Zero unresolved conflicts between workstreams
- ✓ Convergence: All artifacts merged without loss
- ✓ Integration testing: TPC-H 22/22 queries pass, P99 < 500ms
- ✓ Invariant validation: 100% of 6 core axioms proven
- ✓ Test coverage: ≥95% on all changes, 0 flakes in 10 runs
- ✓ Backward compatibility: All prior versions still work (9 versions tested)
- ✓ Documentation: ARCHITECTURE.md, all ADRs, runbook complete
- ✓ Performance: Latency regression < 2%, memory regression < 10%
- ✓ Determinism: manifest.sha256 identical across 100 builds
- ✓ Deployment ready: Canary plan, rollback procedure, monitoring dashboards

**EPIC 10 is COMPLETE when all 12 closure gates are satisfied.**

---

## REFERENCES

- **CLAUDE.md**: BB80/20 + EPIC 9 operational model
- **EPIC 8 Specification**: Atomic construction law (foundation)
- **EPIC 9 Specification**: Cognitive cycle (parallel agents protocol)
- **QLever Architecture**: 9 core components, 289 tests, 1M+ LOC
- **CMake Build System**: 21 CMakeLists.txt, hierarchical dependencies

---

**Document Owner**: Agent 10 (Determinism Validator)
**Last Updated**: 2026-01-02
**Branch**: claude/epic-10-adversarial-roadmap-ZiSkI
**Status**: SPECIFICATION CLOSED - Ready for Phase 1 Execution
