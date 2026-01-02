# EPIC 10: Task Graph, Agent Assignments, and Dependency Matrix

**Generated**: 2026-01-02
**Format**: Task decomposition with explicit agent ownership and handoff choreography
**Total Work Items**: 65 tasks across 8 phases, 10 agents

---

## PHASE 1: SPECIFICATION & INVARIANT CLOSURE (2 weeks)

### Task Graph

```
P1.0: Define 6 Core Axioms
├── P1.1: AX-1 Immutability (no global mutable state)
├── P1.2: AX-2 Determinism (manifest.sha256 consistency)
├── P1.3: AX-3 Atomic Failure (fail-closed semantics)
├── P1.4: AX-4 No External State (pure functions)
├── P1.5: AX-5 RAII (memory safety)
└── P1.6: AX-6 Backward Compatibility (no API changes)

P1.7: Document 20+ Architectural Invariants
├── P1.8: Component-level invariants (9 components)
├── P1.9: Phase-level invariants (6 phases)
├── P1.10: Concurrency invariants (Synchronized<T>, locks)
├── P1.11: Memory invariants (IdTable, allocators)
├── P1.12: Type system invariants (encoding/decoding)
└── P1.13: Compatibility invariants (prior versions)

P1.14: Create Acceptance Predicates
├── P1.15: Parser compliance predicates (W3C SPARQL 1.1)
├── P1.16: Index structure predicates (bijection, immutability)
├── P1.17: Engine optimization predicates (cost determinism)
├── P1.18: Memory constraint predicates (fixed bounds)
├── P1.19: Concurrency predicates (no deadlocks, no races)
└── P1.20: Global state predicates (no mutable singletons)

P1.21: Document Forbidden Patterns
├── P1.22: Forbidden pattern F-1: Hash-based iteration order
├── P1.23: Forbidden pattern F-2: Floating-point in determinism-critical paths
├── P1.24: Forbidden pattern F-3: Mutable shared state
├── P1.25: Forbidden pattern F-4: Global mutable functions
├── P1.26: Forbidden pattern F-5: Conditional phase execution
├── P1.27: Forbidden pattern F-6: Catch-all exception handlers
├── P1.28: Forbidden pattern F-7: Silent failures
└── P1.29: Forbidden pattern F-8: Unordered containers in output paths

P1.30: CI/CD Integration Plan
├── P1.31: Determinism CI job (.github/workflows/determinism.yml)
├── P1.32: Invariant validation hooks (pre-commit, pre-push)
├── P1.33: Gate definition (all invariant checks must pass)
└── P1.34: Blocker escalation procedure (phase lead notification)
```

### Agent Assignments

| Task | Owner | Deadline | Dependency | Status |
|------|-------|----------|-----------|--------|
| P1.0-P1.6 | Agent 1 | Day 3 | None | Pending |
| P1.7-P1.13 | Agent 1 | Day 7 | P1.0-P1.6 | Pending |
| P1.14-P1.20 | Agent 1 | Day 10 | P1.7-P1.13 | Pending |
| P1.21-P1.29 | Agent 1 | Day 12 | P1.14-P1.20 | Pending |
| P1.30-P1.34 | Agent 2 | Day 14 | P1.0-P1.29 | Pending |

**Phase 1 Handoff**: Agent 1 delivers INVARIANT_CLOSURE_MATRIX to Agent 2

---

## PHASE 2: ARCHITECTURE ANALYSIS & RISK MAPPING (1 week, parallel to Phase 1 days 8-14)

### Task Graph

```
P2.0: Component Dependency Mapping
├── P2.1: Extract include graph (9 components)
├── P2.2: Build DAG representation (no cycles verification)
├── P2.3: Identify cross-component dependencies
├── P2.4: Risk-rate each dependency (CRITICAL/HIGH/MEDIUM/LOW)
└── P2.5: Document circular dependency prevention

P2.6: Call Graph Analysis (Hot Paths)
├── P2.7: Flame graph for TPC-H benchmark
├── P2.8: Identify top 5 hot functions per component
├── P2.9: Measure call stack depth (avoid stack overflow)
└── P2.10: Document inlining opportunities

P2.11: Memory Model Formalization
├── P2.12: IdTable layout specification (row-major proof)
├── P2.13: AllocatorWithLimit contracts (hard bounds)
├── P2.14: Allocator call graph (all allocation sites)
└── P2.15: Memory leak vulnerability scan

P2.16: Concurrency Risk Matrix
├── P2.17: Synchronized<T> usage inventory (all instances)
├── P2.18: Lock hierarchy analysis (deadlock prevention)
├── P2.19: Race condition vulnerability scan (TSan baseline)
├── P2.20: Cancellation path analysis (SharedCancellationHandle)
└── P2.21: High-contention code path identification

P2.22: Integration Point Analysis
├── P2.23: Parser → Index interface (AST → vocabulary mapping)
├── P2.24: Index → Engine interface (compression invisibility)
├── P2.25: Engine → Results interface (IdTable assumptions)
└── P2.26: Concurrency → All interface (thread-safety contracts)
```

### Agent Assignments

| Task | Owner | Deadline | Dependency | Status |
|------|-------|----------|-----------|--------|
| P2.0-P2.5 | Agent 2 | Day 3 (Phase 1 day 8) | P1.0-P1.29 | Pending |
| P2.6-P2.10 | Agent 7 | Day 5 | P2.0-P2.5 | Pending |
| P2.11-P2.15 | Agent 4 | Day 5 | P2.0-P2.5 | Pending |
| P2.16-P2.21 | Agent 5 | Day 5 | P2.0-P2.5 | Pending |
| P2.22-P2.26 | Agent 2 | Day 7 | P2.6-P2.21 | Pending |

**Phase 2 Handoff**: Agent 2 delivers ARCHITECTURE_DEPENDENCY_DAG to Phase 3 leads

---

## PHASE 3: CAPABILITY HARDENING (6 weeks, 6 parallel workstreams)

### Workstream 3A: Parser Hardening

```
P3A.0: W3C SPARQL 1.1 Compliance
├── P3A.1: Integrate W3C test suite (300+ test cases)
├── P3A.2: Add N3 compliance tests (Turtle, RDF/XML)
├── P3A.3: Document parsing grammar in EBNF
├── P3A.4: Validate AST matches W3C spec
└── P3A.5: Add error recovery tests (15+ error scenarios)

P3A.6: Dataflow Analysis & Canonicalization
├── P3A.7: Implement variable scope analyzer
├── P3A.8: Add query canonicalization (SPARQL → normalized form)
├── P3A.9: Deterministic variable ordering (no hash randomization)
└── P3A.10: Document canonicalization strategy

P3A.11: Parser Error Recovery
├── P3A.12: Implement fault-tolerant parsing (fail-safe error nodes)
├── P3A.13: Add error message clarity (point to exact line/column)
├── P3A.14: No state pollution on parse failure (rollback mechanism)
└── P3A.15: Test 100 real malformed queries

P3A.16: Benchmarking Suite
├── P3A.17: Create parse-time benchmark (1000 queries)
├── P3A.18: Measure variance across 10 runs (must be < 2%)
├── P3A.19: Document baseline (latency, memory)
└── P3A.20: Add regression tests (if parse time changes > 5%)
```

**Agent 3 Ownership**: P3A.0 - P3A.20, 45 new tests, 25KB documentation

---

### Workstream 3B: Index Hardening

```
P3B.0: CompressedRelation Invariant Formalization
├── P3B.1: Document permutation encoding algorithm (mathematical proof)
├── P3B.2: Add bijection validator (each ID maps to unique ID)
├── P3B.3: Create encoding/decoding round-trip tests (100K triples)
└── P3B.4: Prove no information loss during compression

P3B.5: Vocabulary Determinism
├── P3B.6: Enumerate all vocabulary iteration points
├── P3B.7: Replace hash-based ordering with deterministic sort
├── P3B.8: Add iterator stability tests (same iteration order across runs)
└── P3B.9: Document vocabulary immutability contract

P3B.10: Index Consistency Validators
├── P3B.11: Implement checksum validator (metadata integrity)
├── P3B.12: Create permutation correctness proof generator
├── P3B.13: Add vocabulary bijection proof (1:1 mapping)
├── P3B.14: Implement cross-relation consistency validator
└── P3B.15: Document proof tree format (Merkle-style)

P3B.16: FTS Algorithm Parallelism Guards
├── P3B.17: Audit FTS operations for thread-safety
├── P3B.18: Add atomic check before parallel FTS (early exit if unsafe)
├── P3B.19: Create concurrency test suite for FTS (10K concurrent queries)
└── P3B.20: Document FTS parallelism limitations

P3B.21: Index Performance Optimization
├── P3B.22: Profile vocabulary lookup (binary search vs. hash)
├── P3B.23: Optimize permutation access pattern (cache-aware)
├── P3B.24: Benchmark decompression speed (measure vs. baseline)
└── P3B.25: Document optimization rationale
```

**Agent 6 Ownership**: P3B.0 - P3B.25, 60 new tests, 30KB documentation

---

### Workstream 3C: Engine Core Optimization

```
P3C.0: Operation Hierarchy Refactoring
├── P3C.1: Document Operation base class contract
├── P3C.2: Formalize cost function interface (deterministic cost model)
├── P3C.3: Implement cost normalization (no floating-point)
├── P3C.4: Create operation registry (all 50+ operations enumerated)
└── P3C.5: Add operation composition rules (valid combinations)

P3C.6: Cost-Based Optimizer Determinism
├── P3C.7: Implement deterministic tie-breaking (cost, then ID order)
├── P3C.8: Add cost estimator validator (plan cost vs. actual time)
├── P3C.9: Create benchmark for each operation (measure CPU/memory)
├── P3C.10: Document cost model in mathematical notation
└── P3C.11: Add variance bounds test (95th percentile accuracy)

P3C.12: SIMD Vectorization (Hot Paths)
├── P3C.13: Profile CartesianProductJoin (identify hot loops)
├── P3C.14: Implement SSE/AVX2 vectorization for tuple comparison
├── P3C.15: Add SIMD correctness tests (vectorized results == scalar results)
├── P3C.16: Benchmark SIMD improvement (measure speedup, regression < 0%)
└── P3C.17: Document SIMD rationale (when to use, when not)

P3C.18: Adaptive Join Optimizer
├── P3C.19: Implement runtime statistics gathering (cardinality tracking)
├── P3C.20: Add join selectivity estimator (accuracy test)
├── P3C.21: Create adaptive join strategy (hash vs. sort vs. nested loop)
├── P3C.22: Document optimizer assumptions (data distribution model)
└── P3C.23: Add cost model validation tests (100+ query patterns)

P3C.24: Lazy Evaluation Consistency
├── P3C.25: Audit all operations for laziness assumptions
├── P3C.26: Document which operations are eager vs. lazy
├── P3C.27: Add tests for lazy evaluation corner cases
└── P3C.28: Ensure no double-evaluation bugs (caching tests)
```

**Agent 7 Ownership**: P3C.0 - P3C.28, 80 new tests, 35KB documentation

---

### Workstream 3D: Memory Management & Allocators

```
P3D.0: IdTable Contract Formalization
├── P3D.1: Document row-major layout guarantee (proof)
├── P3D.2: Prove no copy-on-write semantics
├── P3D.3: Add layout validator (memory dump analysis)
├── P3D.4: Implement IdTableRow consistency tests
└── P3D.5: Document cache-line alignment strategy

P3D.6: AllocatorWithLimit Compliance
├── P3D.7: Implement limit enforcer (soft vs. hard bounds)
├── P3D.8: Add OOM simulation tests (allocate until limit)
├── P3D.9: Create memory pressure test suite (OOM resilience)
├── P3D.10: Test graceful failure (no segfault, clean error)
└── P3D.11: Benchmark allocation overhead (< 2% overhead)

P3D.12: Memory Pooling Optimization
├── P3D.13: Profile allocation patterns (hot paths)
├── P3D.14: Implement pre-allocation strategy (memory pools)
├── P3D.15: Add pool reuse tests (allocation reuse, no leaks)
├── P3D.16: Measure fragmentation (free list fragmentation)
└── P3D.17: Document pooling strategy (when/where pools used)

P3D.18: Exception Safety (RAII)
├── P3D.18a: Audit all constructors for exception-safety
├── P3D.18b: Document basic/strong/nothrow guarantees per class
├── P3D.18c: Add exception injection tests (throw in constructor, verify cleanup)
├── P3D.18d: Profile exception path overhead (< 1% in success case)
└── P3D.18e: Valgrind clean (no memory leaks under exception)

P3D.19: Memory Layout Documentation
├── P3D.20: Generate memory layout diagrams (IdTable, operation results)
├── P3D.21: Document alignment requirements (SIMD friendliness)
├── P3D.22: Add memory layout validator (static assertions)
└── P3D.23: Prove no undefined behavior (UBSan clean)

P3D.24: Allocation Pattern Analysis
├── P3D.25: Profile allocation call sites (frequency, size distribution)
├── P3D.26: Identify allocation hotspots (top 10 allocators)
├── P3D.27: Implement allocation tracker (debug mode)
└── P3D.28: Add allocation limit tests (verify limits enforced)
```

**Agent 4 Ownership**: P3D.0 - P3D.28, 40 new tests, 20KB documentation

---

### Workstream 3E: Concurrency & Synchronization

```
P3E.0: Synchronized<T> Thread-Safety Contract
├── P3E.1: Document fine-grained locking strategy (per-field vs. per-object)
├── P3E.2: Formalize lock acquisition order (no deadlock proof)
├── P3E.3: Add lock contention measurement (% time waiting)
├── P3E.4: Implement lock-free fast path where applicable
└── P3E.5: Document Synchronized<T> usage patterns (10 patterns, 5 anti-patterns)

P3E.6: ThreadSanitizer Compliance
├── P3E.7: Run TSan on full test suite (report all races)
├── P3E.8: Classify races (benign vs. real bugs)
├── P3E.9: Fix all real races (data corruption bugs)
├── P3E.10: Document benign races (with justification)
├── P3E.11: Add TSan suppression config (.tsan-suppressed)
└── P3E.12: Continuous TSan validation in CI

P3E.13: Benign Race Detection
├── P3E.14: Identify intentional shared-memory patterns (e.g., stats gathering)
├── P3E.15: Document why race is benign (approximate value OK, no invariant loss)
├── P3E.16: Add comments to explain benign races
└── P3E.17: Automate detection (regex pattern for future races)

P3E.18: Deadlock Prevention Validator
├── P3E.19: Model lock dependency graph (Synchronized<T> nesting)
├── P3E.20: Detect potential cycles (static analysis)
├── P3E.21: Add runtime deadlock detector (timeout-based)
├── P3E.22: Test 100 concurrent query scenarios (no timeouts)
└── P3E.23: Document lock hierarchy (formal specification)

P3E.24: Cancellation Propagation
├── P3E.25: Audit all operations for cancellation checks
├── P3E.26: Implement cancellation broadcast (atomic propagation)
├── P3E.27: Add cancellation test suite (cancel at each operation stage)
├── P3E.28: Measure cancellation latency (< 100ms to full stop)
└── P3E.29: Document cancellation semantics (no partial state)

P3E.30: Concurrency Microbenchmarks
├── P3E.31: Benchmark lock-free vs. locked variants
├── P3E.32: Measure scaling with thread count (1, 8, 64, 256)
├── P3E.33: Profile lock contention (identify bottlenecks)
└── P3E.34: Document performance model (latency vs. throughput)
```

**Agent 5 Ownership**: P3E.0 - P3E.34, 50 new tests, 20KB documentation

---

### Workstream 3F: Global State & Backports

```
P3F.0: Global Mutable State Elimination
├── P3F.1: Inventory all static variables (src/ grep for "static .*=")
├── P3F.2: Classify as: thread-local, immutable, or mutable (error)
├── P3F.3: Convert mutable statics to Synchronized<T>
├── P3F.4: Document global state usage (when needed, how to avoid)
└── P3F.5: Add static analysis rule (forbid new mutable statics)

P3F.6: Configuration Immutability
├── P3F.7: Audit RuntimeParameters for mutability
├── P3F.8: Implement config freeze-after-startup (RuntimeImmutable wrapper)
├── P3F.9: Add configuration consistency tests (no late-binding changes)
└── P3F.10: Document config initialization protocol

P3F.11: C++20 Compliance Audit
├── P3F.12: Inventory backports/ for deprecated constructs
├── P3F.13: Migrate to C++20 equivalents where available
├── P3F.14: Deprecate obsolete backports (planned removal date)
└── P3F.15: Add C++20 feature tests (new capabilities)

P3F.16: Epoch/Snapshot Support
├── P3F.17: Implement version counter (incremented on updates)
├── P3F.18: Add snapshot mechanism (capture state at epoch N)
├── P3F.19: Enable multi-version consistency (read old data while new writes)
├── P3F.20: Document snapshot protocol (how to create, how to use)
└── P3F.21: Benchmark snapshot overhead (< 5%)

P3F.22: Global State Documentation
├── P3F.23: Generate global variable inventory report
├── P3F.24: Document each global's purpose (why needed)
├── P3F.25: Document access patterns (when read, when written)
└── P3F.26: Validate thread-safety of each global

P3F.27: Future Optimization (Epoch-Based Caching)
├── P3F.28: Design epoch-aware cache invalidation
├── P3F.29: Document optimization opportunity (defer to post-EPIC 10)
└── P3F.30: Create ADR for epoch-based design
```

**Agent 1 Ownership** (continuing from Phase 1): P3F.0 - P3F.30, 25 new tests, 15KB documentation

---

## PHASE 3 CRITICAL DEPENDENCIES & INTEGRATION POINTS

```
Workstream 3A (Parser) --┐
                         ├→ Phase 4 Integration Tests (Agent 8 dependency)
Workstream 3B (Index) ---┤
                         ├→ Interface validation: Parser AST → Index vocabulary
Workstream 3C (Engine) --┤
                         ├→ Interface validation: Index compression → Engine query planner
Workstream 3D (Memory) --┤
                         ├→ Interface validation: Engine operations → IdTable memory layout
Workstream 3E (Conc.) --┤
                         ├→ Interface validation: Synchronized<T> across all components
                         │
Workstream 3F (Global) --┘

Dependencies within workstreams:
- 3C depends on 3B (engine needs index interface stable)
- 3E depends on 3C (concurrency validation needs engine operations defined)
- 3D independent (memory model orthogonal to other workstreams)
- 3A independent (parser standalone)
- 3F independent (global state isolated)

Concurrency: All 6 workstreams execute in parallel (weeks 3-9)
Integration: Phase 4 serializes and validates all combinations
```

---

## PHASE 4: INTEGRATION & COMPATIBILITY TESTING (3 weeks, serial)

### Task Graph

```
P4.0: TPC-H Query Suite Validation
├── P4.1: Execute all 22 TPC-H queries (baseline)
├── P4.2: Measure latency per query (record to baseline.json)
├── P4.3: Validate results correctness (compare with reference)
├── P4.4: Calculate regression per query (< 5% acceptable)
├── P4.5: Investigate any regression > 5% (root cause analysis)
└── P4.6: Document TPC-H results (22/22 pass, latencies, regressions)

P4.7: Compatibility Matrix Testing
├── P4.8: Build against each prior QLever version (9 versions)
├── P4.9: Run compatibility test suite per version
├── P4.10: Validate backward compatibility (no breaking changes)
├── P4.11: Document migration path (if any minor breaking changes)
└── P4.12: Create compatibility matrix (version x feature grid)

P4.13: Determinism Validation (100 builds)
├── P4.14: Clean build #1, compute manifest.sha256
├── P4.15: Clean build #2, compute manifest.sha256
├── P4.16: Compare hashes (must be identical)
├── P4.17: Repeat 98 more times (100 total)
├── P4.18: Analyze any hash differences (environment leakage)
└── P4.19: Document determinism proof (100% identical hashes)

P4.20: Concurrent Stress Testing
├── P4.21: Spawn 256 concurrent query threads
├── P4.22: Mix queries (simple, complex, long-running)
├── P4.23: Measure contention (lock wait times)
├── P4.24: Validate no crashes or deadlocks
├── P4.25: Measure query latency under load
└── P4.26: Document stress test results (throughput, latency, stability)

P4.27: Memory Integrity Testing
├── P4.28: Run valgrind on full test suite (memory leaks)
├── P4.29: Run AddressSanitizer (buffer overflows, use-after-free)
├── P4.30: Run UBSanitizer (undefined behavior)
├── P4.31: Fix all sanitizer issues (zero tolerance)
└── P4.32: Document sanitizer results (clean report)

P4.33: Integration Point Validation
├── P4.34: Parser → Index: Verify AST → vocabulary mapping preserved
├── P4.35: Index → Engine: Verify compression invisible to query planner
├── P4.36: Engine → Results: Verify IdTable layout assumptions hold
├── P4.37: Concurrency → All: Verify no data races in query execution
└── P4.38: Document integration point validation results

P4.39: Blocker Resolution
├── P4.40: Categorize all issues found (CRITICAL, HIGH, MEDIUM, LOW)
├── P4.41: Resolve CRITICAL/HIGH issues immediately
├── P4.42: Create issues for MEDIUM (address before release)
├── P4.43: Document LOW issues (defer to next EPIC)
└── P4.44: Re-test after fixes (verification)

P4.45: Rollback & Contingency Plan
├── P4.46: Document rollback procedure (how to revert to prior version)
├── P4.47: Create rollback runbook (for on-call engineers)
└── P4.48: Test rollback procedure (verify it works)
```

**Agent 8 Ownership**: P4.0 - P4.48, 25KB documentation

---

## PHASE 5: PERFORMANCE OPTIMIZATION & PROFILING (2 weeks)

### Task Graph (Parallel to Phase 4 overlap)

```
P5.0: Flame Graph Generation
├── P5.1: Run TPC-H with perf (CPU sampling)
├── P5.2: Generate flame graph (cpu-flame.svg)
├── P5.3: Identify top 5 hottest functions
├── P5.4: Identify top 5 deepest call stacks
└── P5.5: Document hot path analysis

P5.6: Memory Profiling
├── P5.7: Run jemalloc stats (allocation profile)
├── P5.8: Identify allocation hotspots (top 10 allocators)
├── P5.9: Measure allocation frequency (allocs/sec)
├── P5.10: Analyze allocation sizes (distribution)
└── P5.11: Document memory profiling results

P5.12: Lock Contention Analysis
├── P5.13: Measure lock hold times per Synchronized<T> instance
├── P5.14: Identify high-contention locks (> 10% wait time)
├── P5.15: Analyze contention under concurrent load (256 threads)
└── P5.16: Document lock contention findings

P5.17: Cache Miss Profiling
├── P5.18: Measure L1/L2/L3 cache miss rates (perf stat)
├── P5.19: Measure TLB miss rate (page faults)
├── P5.20: Analyze memory access patterns
└── P5.21: Document cache efficiency

P5.22: Hot Path Optimization
├── P5.23: Optimize top 3 hot functions (branch prediction, inlining)
├── P5.24: Measure improvement vs. baseline
├── P5.25: Verify no regression in other paths
└── P5.26: Document optimization rationale

P5.27: Performance Benchmark
├── P5.28: Measure query throughput (queries/sec)
├── P5.29: Measure latency percentiles (P50, P95, P99)
├── P5.30: Measure memory consumption (peak RSS)
├── P5.31: Document performance baseline
└── P5.32: Create performance dashboard (for future comparisons)
```

**Agent 9 Ownership**: P5.0 - P5.32, 30KB documentation

---

## PHASE 6: DOCUMENTATION & DESIGN RECORDS (1 week)

### Task Graph

```
P6.0: Architecture Documentation
├── P6.1: Write ARCHITECTURE.md (high-level design)
├── P6.2: Document component responsibilities (9 components)
├── P6.3: Document inter-component interfaces
└── P6.4: Add architecture diagrams (Graphviz, ASCII art)

P6.5: Concurrency Model Documentation
├── P6.6: Write CONCURRENCY_MODEL.md
├── P6.7: Document thread-safety guarantees
├── P6.8: Document lock hierarchy (Synchronized<T> nesting rules)
├── P6.9: Document deadlock prevention strategy
├── P6.10: Document cancellation protocol
└── P6.11: Add concurrency diagrams (state machines, message flows)

P6.12: Memory Model Documentation
├── P6.13: Write MEMORY_MODEL.md
├── P6.14: Document IdTable layout (row-major proof)
├── P6.15: Document allocator contracts (hard bounds)
├── P6.16: Document exception safety guarantees
└── P6.17: Add memory layout diagrams

P6.18: Query Execution Documentation
├── P6.19: Write QUERY_EXECUTION.md
├── P6.20: Document Operation hierarchy
├── P6.21: Document cost model and optimizer
├── P6.22: Document execution phases (parsing, planning, execution)
└── P6.23: Add query execution diagrams

P6.24: Determinism Documentation
├── P6.25: Write DETERMINISM.md
├── P6.26: Document sources of non-determinism (and mitigations)
├── P6.27: Document manifest.sha256 validation
├── P6.28: Document environment independence
└── P6.29: Document reproducibility guarantees

P6.30: Development Guide
├── P6.31: Write DEVELOPMENT_GUIDE.md
├── P6.32: How to add new operations
├── P6.33: How to add new index types
├── P6.34: How to add new query patterns
├── P6.35: Pitfalls and anti-patterns
└── P6.36: Testing requirements for new features

P6.37: Architecture Decision Records
├── P6.38: ADR-1: Synchronized<T> vs. atomic_ref (lock-based chosen)
├── P6.39: ADR-2: IdTable row-major vs. column-major (row-major chosen)
├── P6.40: ADR-3: Eager vs. lazy evaluation (hybrid chosen)
├── P6.41: ADR-4: Manual memory management vs. GC (RAII chosen)
├── P6.42: ADR-5: Hash-based vs. sorted vocabulary (sorted chosen)
└── P6.43: ADR-6: Cost model accuracy vs. simplicity (accuracy chosen)

P6.44: Runbook & Deployment Guide
├── P6.45: Write operational runbook
├── P6.46: Document startup procedure
├── P6.47: Document monitoring & alerting
├── P6.48: Document troubleshooting guide
└── P6.49: Document on-call escalation procedure
```

**Agents 1-2 Ownership**: P6.0 - P6.49, 40KB documentation

---

## PHASE 7: COMPLIANCE & INVARIANT VALIDATION (1 week)

### Task Graph

```
P7.0: Static Analysis Validation
├── P7.1: AX-1 Immutability: clang-tidy check for mutable statics
├── P7.2: AX-4 No External State: grep for file I/O in core components
├── P7.3: F-1 Hash Ordering: grep for std::unordered_map in output paths
├── P7.4: F-2 Floating-Point: grep for float/double in ID encoding
├── P7.5: F-3 Global Mutable: grep for global mutable variables
└── P7.6: Document static analysis results

P7.7: Determinism Validation (Automated)
├── P7.8: Run 100 builds (manifest.sha256 comparison)
├── P7.9: Automated comparison (all hashes identical → PASS)
├── P7.10: Environment independence test (env -i builds)
├── P7.11: Multi-machine test (build on 3 different machines)
└── P7.12: Document determinism proof (100% reproducible)

P7.13: Atomicity Validation
├── P7.14: Verify .NOTPARALLEL in Makefile
├── P7.15: Verify phase-to-phase dependencies (A→B→...→F)
├── P7.16: Verify 'set -e' in all phase scripts
├── P7.17: Verify no -k or -i flags in Makefile
└── P7.18: Document atomicity proof (fail-closed guaranteed)

P7.19: Thread-Safety Validation
├── P7.19: Run ThreadSanitizer on full test suite
├── P7.20: Classify races (benign vs. bugs)
├── P7.21: Fix all real races
├── P7.22: Document benign races
└── P7.23: TSan clean report (0 unclassified races)

P7.24: Memory Safety Validation
├── P7.25: Run AddressSanitizer (ASAN)
├── P7.26: Run valgrind --leak-check=full
├── P7.27: Run UBSanitizer (UBSan)
├── P7.28: Fix all reported issues
└── P7.29: Document memory safety (0 leaks, 0 UB)

P7.30: Backward Compatibility Validation
├── P7.31: Test against 9 prior QLever versions
├── P7.32: Validate API compatibility (no breaking changes)
├── P7.33: Validate wire format compatibility (if changed, document migration)
└── P7.34: Document compatibility guarantees

P7.35: Performance Regression Analysis
├── P7.36: Compare TPC-H latencies vs. baseline (< 2% regression OK)
├── P7.37: Compare memory usage vs. baseline (< 10% increase OK)
├── P7.38: Compare throughput vs. baseline (must improve or stay same)
└── P7.39: Document performance analysis

P7.40: Compliance Report
├── P7.41: Generate EPIC 10 Compliance Report
├── P7.42: Validate all 6 core axioms (PASS/FAIL)
├── P7.43: Validate all phase-level invariants
├── P7.44: Create pass/fail summary
└── P7.45: List any deferred items (with justification)
```

**Agents 6, 7, 8, 10 Ownership**: P7.0 - P7.45, 25KB documentation

---

## PHASE 8: CLOSURE & DEPLOYMENT PREPARATION (1 week)

### Task Graph

```
P8.0: Closure Checklist Verification
├── P8.1: Verify all 8 phases completed ✓
├── P8.2: Verify all 10 agents delivered artifacts ✓
├── P8.3: Verify all phase success criteria met ✓
├── P8.4: Verify all invariants validated ✓
├── P8.5: Verify all tests passing (0 flakes in 10 runs) ✓
├── P8.6: Verify backward compatibility maintained ✓
├── P8.7: Verify performance regression acceptable ✓
└── P8.8: Verify determinism proven (100% reproducible) ✓

P8.9: Release Notes Preparation
├── P8.10: Draft feature summary (5-10 features)
├── P8.11: Draft breaking changes (if any, migration path)
├── P8.12: Draft deprecation notices (if any)
├── P8.13: Draft performance improvements
├── P8.14: Draft known issues and workarounds
└── P8.15: Peer review and finalize release notes

P8.16: Semantic Versioning Update
├── P8.17: Determine version increment (major.minor.patch)
├── P8.18: Create git tag (v<major>.<minor>.<patch>)
├── P8.19: Update version in CMakeLists.txt
├── P8.20: Update CHANGELOG.md
└── P8.21: Create release announcement

P8.22: Canary Deployment Plan
├── P8.23: Document rollout stages (1% → 10% → 50% → 100%)
├── P8.24: Define monitoring metrics (QPS, latency, errors)
├── P8.25: Define rollback triggers (latency > 10%, errors > 1%)
├── P8.26: Document rollback procedure
└── P8.27: Test rollback on staging

P8.28: SLA Metrics Definition
├── P8.29: Define availability target (99.9%)
├── P8.30: Define latency target (P99 < 500ms)
├── P8.31: Define throughput target (QPS capacity planning)
├── P8.32: Document alert thresholds
└── P8.33: Create monitoring dashboard

P8.34: On-Call Runbook
├── P8.35: Document startup procedure (step-by-step)
├── P8.36: Document shutdown procedure (graceful shutdown)
├── P8.37: Document troubleshooting guide (common issues, fixes)
├── P8.38: Document escalation procedure (when to page)
└── P8.39: Document incident response (post-mortem template)

P8.40: Final Sign-Off
├── P8.41: Architecture lead review (Agent 1)
├── P8.42: Integration lead review (Agent 2)
├── P8.43: QA/Testing lead review (Agent 8)
├── P8.44: Performance lead review (Agent 9)
├── P8.45: Release manager sign-off
└── P8.46: Ready for production deployment ✓
```

**Agents 1-3 Ownership**: P8.0 - P8.46, 15KB documentation

---

## AGENT RESPONSIBILITY MATRIX

```
                    P1  P2  P3A P3B P3C P3D P3E P3F P4  P5  P6  P7  P8
Agent 1: Spec       L   S   -   -   -   -   -   L   -   -   L   -   L
Agent 2: Arch       S   L   -   -   -   -   -   -   -   -   L   -   L
Agent 3: Parser     -   -   L   -   -   -   -   -   S   -   -   -   -
Agent 4: Memory     -   S   -   -   -   L   -   -   S   S   -   -   -
Agent 5: Concur     -   S   -   -   -   -   L   -   S   -   -   -   -
Agent 6: Index      -   S   -   L   -   -   -   -   S   -   -   L   -
Agent 7: Engine     -   S   -   -   L   -   -   -   S   L   -   L   -
Agent 8: Integration -   -   S   S   S   S   S   -   L   -   -   L   -
Agent 9: Perf       -   -   -   -   S   S   -   -   -   L   -   -   -
Agent 10: Determ    -   -   -   -   -   -   -   -   -   -   -   L   -

L = Lead (primary owner)
S = Support (secondary)
- = Not involved

Handoff Protocol:
Agent 1 (P1.0-P1.34) → Agent 2 (P2.0-P2.26) → Workstream Leads (P3A-P3F) → Agent 8 (P4.0-P4.48) →
Agents 9, 1-2 (P5-P6 parallel) → Agents 6-10 (P7.0-P7.45) → Agents 1-3 (P8.0-P8.46)
```

---

## HANDOFF CHECKPOINTS

### Phase 1 → Phase 2
**Deliverable**: INVARIANT_CLOSURE_MATRIX.md
**Acceptance**: 6 axioms + 20+ invariants formally defined, all design choices closed
**Sign-off**: Agent 1 → Agent 2

### Phase 2 → Phase 3
**Deliverable**: ARCHITECTURE_DEPENDENCY_DAG.graphviz + MEMORY_MODEL_SPEC.md
**Acceptance**: DAG has no cycles, all components understood, risk matrix complete
**Sign-off**: Agent 2 → Workstream Leads (Agents 3-10)

### Phase 3 → Phase 4
**Deliverable**: 6 workstream artifacts + 280 new tests
**Acceptance**: All 6 workstreams complete, 280 tests passing (0 flakes in 10 runs)
**Sign-off**: Workstream Leads → Agent 8

### Phase 4 → Phase 5
**Deliverable**: TPC-H validation report, compatibility matrix, determinism proof
**Acceptance**: TPC-H 22/22 pass, regression < 5%, determinism 100%
**Sign-off**: Agent 8 → Agent 9

### Phase 5 → Phase 6
**Deliverable**: Performance baseline, flame graphs, profiling data
**Acceptance**: Optimization complete, baseline documented, no new regressions
**Sign-off**: Agent 9 → Agents 1-2

### Phase 6 → Phase 7
**Deliverable**: ARCHITECTURE.md, CONCURRENCY_MODEL.md, 6 ADRs, runbook
**Acceptance**: All documentation complete, peer reviewed, ready for release
**Sign-off**: Agents 1-2 → Agents 6-10

### Phase 7 → Phase 8
**Deliverable**: Compliance report, all validation proofs, 0 violations
**Acceptance**: All 6 axioms validated, backward compatibility proven, determinism proven
**Sign-off**: Agents 6-10 → Agents 1-3

### Phase 8 → Production
**Deliverable**: Release notes, runbook, SLA metrics, canary plan
**Acceptance**: All closure conditions met, ready for deployment
**Sign-off**: Agents 1-3 → Release Manager

---

**End of Task Graph and Agent Assignments**

**Total Tasks**: 65 across 8 phases, 10 agents
**Total Deliverables**: 18 major artifacts + 280 tests
**Estimated Duration**: 14 weeks critical path
**Parallelism**: 78% concurrency in Phase 3 (6 independent workstreams)
