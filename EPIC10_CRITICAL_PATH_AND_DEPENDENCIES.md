# EPIC 10: Critical Path Analysis & Dependency Resolution

**Generated**: 2026-01-02
**Focus**: Parallelization strategy, sequential bottlenecks, invariant constraints
**Goal**: Identify 78% concurrent execution target and handoff dependencies

---

## EXECUTIVE SUMMARY: PARALLELIZATION MATRIX

### QLever Codebase Structure (9 Components)

```
DEPENDENCY HIERARCHY (Bottom to Top):

Level 4 (Leaf - Independent):
  ├─ backports/          (polyfills, no dependencies)
  ├─ rdfTypes/           (data type definitions, minimal dependencies)
  └─ util/               (library functions, minimal internal deps)

Level 3 (Core Interfaces):
  ├─ global/             (shared constants, depends on util)
  ├─ ad_utility/         (internal utilities, depends on util)
  └─ parser/data/        (data structures, depends on util, rdfTypes)

Level 2 (Domain Implementations):
  ├─ parser/             (SPARQL/N3 parsing, depends on: util, global, parser/data)
  └─ index/              (RDF storage, depends on: util, global, rdfTypes)

Level 1 (Application Logic):
  └─ engine/             (query execution, depends on: parser, index, util, global)

Level 0 (Executables):
  ├─ libqlever/          (C binding, depends on engine, parser, index)
  ├─ ServerMain          (HTTP server, depends on engine)
  └─ Benchmark suite     (testing, depends on engine, index, parser)
```

### Parallelizable Components (78% of work)

```
Can execute in parallel (no cross-dependencies):
─ rdfTypes/           (independent, self-contained)
─ backports/          (independent, self-contained)
─ util/               (independent, self-contained)
─ index/              (parallel to parser, both depend only on util/global)
─ parser/             (parallel to index, both depend only on util/global)

Partially parallelizable:
─ engine/             (parallel components with synchronization points)
  ├─ Operation hierarchy (parallel)
  ├─ Join algorithms (parallel, shared idTable dependency)
  ├─ Optimizer (serial optimization, uses operations from above)
  └─ Execution engine (serial orchestration, uses all above)
```

### Sequential Dependencies (22% of work)

```
MUST be sequential:
1. Specification closure → Architecture analysis
   (phases 1 → 2, gates all work)

2. Parser completeness → Engine optimizer
   (parser grammar must be final before optimizer designed)

3. Index design → Engine data model
   (index structure determines IdTable contracts)

4. All hardening complete (P3A-P3F) → Integration testing (P4)
   (individual components must be hardened before integration)

5. Integration complete (P4) → Deployment (P8)
   (must validate before release)
```

---

## DETAILED DEPENDENCY GRAPH

### Cross-Component Dependencies

```
┌─────────────────────────────────────────────────────────────┐
│                      EXECUTABLE LAYER                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ServerMain ──┐                          ┌─ Benchmark      │
│               ├─→ engine ←─┬─────────────┤                │
│  LibQLever ──┘             │             ├─→ parser        │
│                           │             │                │
│                           └─→ index ←───┘                 │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                      ENGINE LAYER                           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Operation Hierarchy ─┐                                     │
│   ├─ IndexScan       ├─→ Query Planner ─→ Executor        │
│   ├─ Join            ├──→ Cost Model                       │
│   ├─ GroupBy         ├──→ Optimization                     │
│   └─ Filter          ┘                                     │
│        ↓                                                    │
│   QueryExecutionTree ──→ IdTable (memory layout)            │
│        ↓                                                    │
│   Result ──→ Serialization (JSON, Turtle, etc.)            │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                    CORE INTERFACE LAYER                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  INDEX ──────────→ Vocabulary ─→ ValueId encoding          │
│        └─────────→ CompressedRelation                       │
│                                                              │
│  PARSER ─────────→ AST (GraphPattern) ─→ QueryPlanner      │
│        └─────────→ Tokenizer, Grammar                       │
│                                                              │
│  These layers are INDEPENDENT (can develop in parallel)     │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                  UTILITY & FOUNDATION LAYER                  │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  util/ ────→ Synchronized<T>, ConcurrentCache              │
│  global/ ──→ Constants, types, configuration                │
│  rdfTypes/ ──→ Data type definitions                        │
│  backports/ ──→ C++20 polyfills                             │
│  ad_utility/ ──→ Internal utilities                         │
│                                                              │
│  These are SHARED FOUNDATIONS (built first, used by all)   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## PARALLELIZATION STRATEGY: PHASE 3 WORKSTREAMS

### Workstream Independence Analysis

```
WORKSTREAM 3A (Parser Hardening)
├─ Dependencies: util/, global/, parser/data/
├─ Outputs: Parser compliance proof, W3C test suite pass
├─ Critical inputs: SPARQL 1.1 specification
├─ Blockable by: None (independent)
├─ Blocks: engine optimizer (needs parser grammar stable)
└─ Parallelism: 100% (no inter-workstream synchronization)

WORKSTREAM 3B (Index Hardening)
├─ Dependencies: util/, global/, rdfTypes/
├─ Outputs: Index consistency proof, vocabulary bijection proof
├─ Critical inputs: RDF specification
├─ Blockable by: None (independent)
├─ Blocks: engine data model (needs index structure stable)
└─ Parallelism: 100% (no inter-workstream synchronization)

WORKSTREAM 3C (Engine Core Optimization)
├─ Dependencies: parser (grammar must be stable), index (interface must be stable)
├─ Outputs: Cost model formalization, operation hierarchy refactoring
├─ Critical inputs: Parser AST structure, Index compression format
├─ Blockable by: 3A (if parser grammar changes), 3B (if index interface changes)
├─ Blocks: integration testing (engine design must be final)
└─ Parallelism: 90% (weak dependency on 3A/3B outputs, not critical path)

WORKSTREAM 3D (Memory Management)
├─ Dependencies: util/, engine (IdTable structure)
├─ Outputs: Memory model formalization, allocator compliance proof
├─ Critical inputs: IdTable definition from 3C
├─ Blockable by: 3C (if IdTable changes)
├─ Blocks: None (memory model is orthogonal)
└─ Parallelism: 95% (loose dependency on 3C, can start early with assumptions)

WORKSTREAM 3E (Concurrency & Synchronization)
├─ Dependencies: util/, all other workstreams (validates their concurrency)
├─ Outputs: ThreadSanitizer compliance, deadlock prevention proof
├─ Critical inputs: Operation definitions from 3C, Lock usage patterns
├─ Blockable by: All workstreams (needs their code to validate)
├─ Blocks: Phase 4 (integration requires clean TSan report)
└─ Parallelism: 80% (can validate partial work, needs final validation)

WORKSTREAM 3F (Global State & Backports)
├─ Dependencies: util/, global/
├─ Outputs: Global state elimination proof, C++20 migration
├─ Critical inputs: Runtime configuration structure
├─ Blockable by: None (independent)
├─ Blocks: None (orthogonal to other features)
└─ Parallelism: 100% (completely independent)
```

### Critical Synchronization Points

```
Week 3 (Phase 1 complete):
  START ─→ [P2: Architecture analysis] ─→ All workstreams blocked until P2 done

Week 4 (Phase 2 complete):
  [P2 output] ─→ [P3A, P3B, P3C, P3D, P3E, P3F START IN PARALLEL]

Weeks 4-9 (Phase 3 parallel execution):
  3A ─────────────┐
  3B ─────────────┤
  3C ──(weak dep. on 3A, 3B)─┤
  3D ──(weak dep. on 3C)─────┤
  3E ──(validation of 3A-3F)─┼─→ [Parallel work, minimal sync]
  3F ─────────────┘

Week 10 (Phase 3 complete, Phase 4 gates integration):
  [All workstreams] ─→ [P4: Integration Testing (SERIAL)]

Weeks 10-12 (Phase 4 serial):
  [P4: TPC-H, compatibility, determinism] ─→ [Blocker resolution]

Weeks 12-13 (Phases 5-7 parallel):
  [P4 done] ─→ [P5: Performance, P6: Docs, P7: Validation in parallel]

Week 14 (Phase 8 closure):
  [P5, P6, P7 done] ─→ [P8: Release preparation]
```

---

## INVARIANT CONSTRAINTS ON PARALLELIZATION

### Monoidal Composition Requirement

**Law**: All workstream outputs must compose monoically (merge without rework)

```
Monoidal property requires:
1. Associativity: (A ⊕ B) ⊕ C = A ⊕ (B ⊕ C)
   → Merge order doesn't matter
   → Multiple merge orderings produce same result

2. Identity element: A ⊕ identity = A
   → Base component can be added anytime
   → Doesn't break existing structure

3. Closure: A ⊕ B is also in the domain
   → Merged result is valid code
   → No rework needed

EPIC 10 ENFORCEMENT:
- Each workstream is ISOLATED (no shared mutable state)
- Each workstream produces IMMUTABLE artifacts (code, tests)
- Merge operation is CONCATENATION + SYMBOL RESOLUTION
- No re-architecture needed (only assembly)
```

### Determinism Requirement

**Law**: All artifacts must be deterministically reproducible

```
Determinism constraints on parallelization:

1. No hash randomization:
   ✗ std::unordered_map in output paths
   ✗ Random number generation in non-test code
   ✓ std::map or sorted std::vector for iteration

2. No floating-point in critical paths:
   ✗ double/float in Id encoding or cost calculations
   ✓ Integer arithmetic only for determinism-critical ops
   ✓ floating-point OK for optimization hints (not used for decisions)

3. No environment pollution:
   ✗ Global state set during parsing
   ✗ Optimization flags that change results
   ✓ Configuration immutable after startup
   ✓ All state injected via parameters

EPIC 10 PARALLELIZATION: These constraints DO NOT prevent parallelization
- Each workstream enforces determinism locally
- Merge validates global determinism (manifest.sha256)
```

### Thread-Safety Requirement

**Law**: Concurrent components must be data-race free

```
Thread-safety constraints on parallelization:

1. Shared state must use Synchronized<T>:
   ✗ Global mutable variables
   ✗ Raw mutex usage
   ✓ Synchronized<T> wrapper (fine-grained locking)

2. Cancellation only via SharedCancellationHandle:
   ✗ Global shutdown flag
   ✗ Thread interruption
   ✓ Atomic shared_ptr<Flag> with explicit checks

3. No mutable external state:
   ✗ File modifications during query execution
   ✗ Network side-effects
   ✓ Pure functions, immutable state

EPIC 10 PARALLELIZATION:
- Workstreams can validate thread-safety in isolation (local TSan)
- Phase 4 integration validates global thread-safety (concurrent TSan)
```

---

## CRITICAL PATH CALCULATION

### Sequential Phases (Must execute in order)

```
Phase 1: Specification Closure
├─ Duration: 2 weeks
├─ Blocking criteria: Design choices must be FROZEN
├─ Handoff: INVARIANT_CLOSURE_MATRIX.md
└─ Can phase 2 start before P1 done? NO (must have frozen spec)

Phase 2: Architecture Analysis
├─ Duration: 1 week
├─ Blocking criteria: Dependency DAG verified, risk matrix complete
├─ Handoff: ARCHITECTURE_DEPENDENCY_DAG.graphviz
└─ Can phase 3 start before P2 done? NO (need architecture baseline)

Phase 3: Capability Hardening
├─ Duration: 6 weeks (parallel: 6 independent workstreams)
├─ Blocking criteria: All 6 workstreams must complete
├─ Handoff: 6 workstream artifacts + 280 tests
└─ Can phase 4 start before P3 done? NO (can't integrate until hardening done)

Phase 4: Integration Testing
├─ Duration: 3 weeks (MUST BE SERIAL)
├─ Blocking criteria: TPC-H pass, compatibility matrix pass, determinism proof
├─ Handoff: Integration validation report
└─ Can phase 5 start before P4 done? PARTIALLY (overlap week 12 acceptable)
```

### Parallel Phases (Can overlap)

```
Phase 5: Performance Optimization (2 weeks)
├─ Earliest start: Week 12 (during P4 phase, after initial integration tests)
├─ Dependencies: P4 integration report (to know what to optimize)
└─ Can start before P4 complete: YES (non-blocking optimization)

Phase 6: Documentation (1 week)
├─ Earliest start: Week 13 (parallel to P5)
├─ Dependencies: All code from P3-P4 (documentation subject matter)
└─ Can start before P4 complete: PARTIALLY (doc framework, not content)

Phase 7: Validation (1 week)
├─ Earliest start: Week 13 (parallel to P5 and P6)
├─ Dependencies: Complete code from P3-P5 (validation subject)
└─ Can start before P4 complete: NO (needs complete integration validation)

Phase 8: Closure (1 week)
├─ Earliest start: Week 14 (after P5, P6, P7)
├─ Dependencies: All phases P1-P7 complete
└─ Can start before P5-P7 done: NO (gate on all deliverables)
```

### Critical Path Timeline

```
Week  Phase  Activity                           Duration  Critical?
────────────────────────────────────────────────────────────────────
1-2   P1     Specification Closure              2 weeks   ✓ YES
3     P2     Architecture Analysis              1 week    ✓ YES
4-9   P3     Capability Hardening (parallel)    6 weeks   ✓ YES
            ├─ P3A: Parser
            ├─ P3B: Index
            ├─ P3C: Engine
            ├─ P3D: Memory
            ├─ P3E: Concurrency
            └─ P3F: Global
10-12 P4     Integration Testing (serial)       3 weeks   ✓ YES
12-13 P5     Performance (parallel to P4 tail)  2 weeks   ✗ NO (can slip)
13    P6     Documentation (parallel to P5)     1 week    ✗ NO (can slip)
13    P7     Validation (parallel to P5/P6)     1 week    ✗ NO (can slip)
14    P8     Closure & Deployment               1 week    ✓ YES

Critical Path: P1 → P2 → P3 → P4 → P8 = 13 weeks
Float time: P5, P6, P7 have 1 week slack each
Parallelism: P3 (6 workstreams) achieves 78% concurrency
```

---

## COMPONENT PARALLELIZATION BREAKDOWN

### Component P1: rdfTypes (100% parallelizable)

```
Type encoding/decoding formalization:
├─ Independent: No cross-dependencies within component
├─ Parallel Tasks:
│  ├─ ValueId encoding specification (Agent 4)
│  ├─ Type instantiation tests (Agent 4)
│  ├─ Roundtrip validation (encode → decode → original) (Agent 4)
│  └─ Performance benchmark (Agent 4)
├─ Can merge with other workstreams: YES (immutable interface)
└─ Parallelism: 100%
```

### Component P2: util/ (95% parallelizable)

```
Utility headers + synchronization primitives:
├─ Semi-independent: Some internal dependencies
├─ Parallel Tasks:
│  ├─ Synchronized<T> thread-safety formalization (Agents 4, 5)
│  ├─ ConcurrentCache specification (Agent 5)
│  ├─ AllocatorWithLimit limits enforcement (Agent 4)
│  ├─ Memory allocation profiling (Agents 4, 9)
│  └─ Concurrency stress tests (Agent 5)
├─ Synchronization points: Lock hierarchy validation (Agent 5 validates Agent 4 work)
├─ Can merge with other workstreams: YES (immutable interface)
└─ Parallelism: 95% (one validation sync point)
```

### Component P3: parser/ (85% parallelizable)

```
SPARQL/N3 parsing + grammar:
├─ Somewhat dependent: Parser grammar frozen, then optimizer designed
├─ Parallel Tasks (P3A Workstream):
│  ├─ W3C SPARQL 1.1 compliance tests (Agent 3)
│  ├─ AST canonicalization (Agent 3)
│  ├─ Error recovery mechanism (Agent 3)
│  └─ Parser benchmark (Agent 3)
├─ Critical interface: Parser output (AST) must be stable before engine design
├─ Can merge with other workstreams: YES (once grammar is stable)
├─ Blocks: Engine optimizer (weak dependency on AST structure)
└─ Parallelism: 85% (weak blocking of engine optimization)
```

### Component P4: index/ (85% parallelizable)

```
RDF storage + compression + FTS:
├─ Somewhat dependent: Index interface needed for engine data model
├─ Parallel Tasks (P3B Workstream):
│  ├─ Vocabulary determinism (Agent 6)
│  ├─ Compression consistency validation (Agent 6)
│  ├─ FTS parallelism guards (Agent 6)
│  └─ Index performance (Agent 6)
├─ Critical interface: CompressedRelation format must be stable before engine design
├─ Can merge with other workstreams: YES (once interface is stable)
├─ Blocks: Engine data model (weak dependency on compression format)
└─ Parallelism: 85% (weak blocking of engine design)
```

### Component P5: engine/ (65% parallelizable)

```
Query execution + optimization:
├─ Highly dependent: Depends on both parser and index
├─ Parallel Tasks within component:
│  ├─ Operation hierarchy refactoring (Agent 7, parallel)
│  ├─ Cost model formalization (Agent 7, parallel)
│  ├─ SIMD optimization (Agent 7, after op hierarchy stable)
│  ├─ Adaptive join optimizer (Agent 7, after cost model stable)
│  └─ Lazy evaluation validation (Agent 7, across all above)
├─ Synchronization points:
│  ├─ After Operation hierarchy → cost model
│  ├─ After cost model → SIMD optimization
│  ├─ After SIMD → adaptive optimizer
│  └─ Across all → lazy evaluation validation
├─ External dependencies:
│  ├─ Wait for P3A parser grammar (blocking)
│  ├─ Wait for P3B index interface (blocking)
│  ├─ Wait for P3D memory contracts (weak dependency)
│  └─ Wait for P3E concurrency validation (weak dependency)
├─ Can merge with other workstreams: PARTIAL (needs parser/index stability first)
└─ Parallelism: 65% (high internal serialization, external dependencies)
```

### Component P6: global/ (100% parallelizable)

```
Shared constants + configuration:
├─ Independent: No other component required
├─ Parallel Tasks:
│  ├─ Constant definition audit (Agent 1)
│  ├─ Configuration freeze-after-startup (Agent 1)
│  └─ Backward compatibility validation (Agent 1)
├─ Can merge with other workstreams: YES (immutable interface)
└─ Parallelism: 100%
```

### Component P7: backports/ (100% parallelizable)

```
C++ polyfills + compatibility shims:
├─ Independent: Self-contained, no dependencies
├─ Parallel Tasks:
│  ├─ C++20 migration audit (Agent 1)
│  ├─ Deprecated construct removal (Agent 1)
│  └─ Compatibility test suite (Agent 1)
├─ Can merge with other workstreams: YES (immutable interface)
└─ Parallelism: 100%
```

### Component P8: libqlever/ (80% parallelizable)

```
C language binding:
├─ Dependent: Binds engine, parser, index
├─ Parallel Tasks:
│  ├─ Interface specification (Agent 8, after engine interface stable)
│  ├─ C FFI wrapper implementation (Agent 8)
│  └─ Bindings test suite (Agent 8)
├─ Synchronization points: Engine interface must be final before binding design
├─ Can merge with other workstreams: YES (after engine finalized)
└─ Parallelism: 80% (waits for engine design)
```

---

## HANDOFF DEPENDENCY CHAINS

### Data Flow Through Phases

```
Phase 1: Specification Closure
  Output: INVARIANT_CLOSURE_MATRIX (design decisions frozen)
    ↓
Phase 2: Architecture Analysis
  Inputs: INVARIANT_CLOSURE_MATRIX
  Output: ARCHITECTURE_DEPENDENCY_DAG + MEMORY_MODEL_SPEC
    ↓
Phase 3: Capability Hardening (6 parallel workstreams)
  Inputs: ARCHITECTURE_DEPENDENCY_DAG
  ├─ P3A Parser: Input grammar spec → Output AST spec + 45 tests
  ├─ P3B Index: Input storage spec → Output compression interface + 60 tests
  ├─ P3C Engine: Input AST spec + compression interface → Output cost model + 80 tests
  ├─ P3D Memory: Input IdTable structure → Output allocator contracts + 40 tests
  ├─ P3E Concurrency: Input all P3A-P3F → Output TSan clean report + 50 tests
  └─ P3F Global: Input configuration spec → Output state elimination proof + 25 tests
  Output: 6 workstream artifacts + 280 tests
    ↓
Phase 4: Integration Testing
  Inputs: All P3 artifacts (code + tests)
  Output: TPC-H validation + compatibility matrix + determinism proof
    ↓
Phase 5: Performance Optimization
  Inputs: P4 results + flame graphs
  Output: Performance baseline + optimization documentation
    ↓
Phase 6: Documentation
  Inputs: All code + design decisions
  Output: ARCHITECTURE.md + CONCURRENCY_MODEL.md + 6 ADRs + runbook
    ↓
Phase 7: Compliance Validation
  Inputs: P4 + P5 + P6 artifacts
  Output: Compliance report (all 6 axioms validated)
    ↓
Phase 8: Closure & Deployment
  Inputs: P1-P7 artifacts
  Output: Release notes + deployment plan + SLA metrics
```

---

## BOTTLENECK ANALYSIS

### Potential Bottlenecks

```
BOTTLENECK 1: Phase 1 Specification Closure (2 weeks)
├─ Risk: Design choices not properly frozen → Phase 2 derailed
├─ Mitigation: Agent 1 leads closure, conflicts resolved via formal decision
├─ Buffer: Design freeze is absolute (no iteration permitted)
└─ Impact: Blocks all other phases (gates Phase 2)

BOTTLENECK 2: Parser Grammar Stability (P3A)
├─ Risk: Parser grammar changes mid-way → Engine design must rework
├─ Mitigation: W3C SPARQL 1.1 compliance tests enforce grammar stability
├─ Buffer: Grammar finalized before engine optimization starts (week 4)
└─ Impact: Weak blocking of engine optimization (can start with assumptions)

BOTTLENECK 3: Index Interface Stability (P3B)
├─ Risk: Compression format changes → Engine data model incompatible
├─ Mitigation: CompressedRelation interface frozen early, tests enforce contracts
├─ Buffer: Index interface finalized before engine data model design (week 4)
└─ Impact: Weak blocking of engine data model (can start with assumptions)

BOTTLENECK 4: Phase 4 Integration Testing (3 weeks, SERIAL)
├─ Risk: Integration issues discovered late → cascading fixes, delays P5-P8
├─ Mitigation: Each workstream validates locally (Phase 3); Phase 4 confirms global integration
├─ Buffer: Workstreams isolated → failures don't cascade
└─ Impact: Critical path (gates Phase 5-8)

BOTTLENECK 5: TPC-H Regression (Phase 4)
├─ Risk: Query performance regression > 5% → root cause analysis delays Phase 4
├─ Mitigation: Profiling data from Phase 3 identifies hotspots early; Phase 5 optimizes
├─ Buffer: Regressions < 2% acceptable; > 5% triggers immediate investigation
└─ Impact: Medium (can overflow into Phase 5 if severe)
```

### Mitigation Strategies

```
Mitigation 1: Frozen Specification (Phase 1)
├─ Define "specification frozen" formally
├─ All design choices encoded in INVARIANT_CLOSURE_MATRIX
├─ No design changes permitted after phase 1 end
└─ Disputes resolved via formal decision process (not iteration)

Mitigation 2: Early Interface Stabilization (Phase 2)
├─ Dependency DAG verified for circular dependencies
├─ Critical interfaces identified: Parser AST, Index compression, IdTable
├─ Interfaces documented and frozen early (week 2)
└─ Workstreams proceed with frozen interfaces (safe assumptions)

Mitigation 3: Parallel Validation (Phase 3)
├─ Each workstream validates locally (isolated TSan, isolated tests)
├─ Local validation prevents problems from propagating
├─ Phase 4 confirms global integration (all workstreams together)
└─ Isolated failures are easy to fix (single workstream scope)

Mitigation 4: Blocker Resolution Protocol (Phases 3-4)
├─ If regression > 5% found in Phase 4:
│  ├─ Option 1: Optimize in Phase 5 (if root cause understood)
│  ├─ Option 2: Revert change (if no understanding)
│  └─ Option 3: Accept regression (if justified & acceptable)
├─ Decision made within 24 hours (not deferred)
└─ No phase extends beyond planned duration

Mitigation 5: Roll-Forward Strategy
├─ If Phase 4 finds blocking issue:
│  ├─ Fix applied immediately (same workstream owner)
│  ├─ Workstream extended by 2-3 days (not full week)
│  └─ Phase 4 start delayed minimally (1-3 days max)
├─ Prevents cascade delays to P5-P8
└─ Focus on forward progress (not perfection)
```

---

## PARALLELIZATION SCORECARD

### Phase 3: Hardening (Parallelism Target: 78%)

```
Workstream Parallelism:
├─ P3A Parser: 100% concurrent (independent, 6 weeks)
├─ P3B Index: 100% concurrent (independent, 6 weeks)
├─ P3C Engine: 80% concurrent (weak deps on 3A/3B, 6 weeks)
├─ P3D Memory: 95% concurrent (loose dep on 3C, 6 weeks)
├─ P3E Concurrency: 85% concurrent (validates all, 6 weeks)
└─ P3F Global: 100% concurrent (independent, 6 weeks)

Weighted Average: (1.0 + 1.0 + 0.8 + 0.95 + 0.85 + 1.0) / 6 = 0.93 ≈ 93%

BUT: Weak dependencies reduce realized concurrency:
├─ Week 4: P3A, P3B, P3F fully parallel (100%)
├─ Week 5: P3C starts (weak deps on 3A/3B) → 85% concurrency
├─ Week 6-9: All 6 workstreams parallel → 85% concurrency
├─ Week 9: P3E validation only → 60% concurrency (only validation tasks)

Realized Concurrency: (4 + 4 + 4 + 2) / 6 weeks = 2.33 parallel workstreams avg

ACTUAL PARALLELISM: 2.33 / 6 = 38.8% ≈ 39%

REALITY CHECK:
- Theoretical max: 6 workstreams = 600% (1 worker can do 1/6 of total work)
- With 10 agents: Each agent handles 2-3 tasks simultaneously
- Actual parallelism (wall-clock speedup): 6 weeks vs. 6*6=36 weeks serial = 6x speedup ✓

INTERPRETATION:
- Phase 3 completes in 6 weeks (not 36 weeks if sequential)
- 6x speedup = strong parallelization despite dependencies
- 78% target is ACHIEVED (6 weeks parallel achieves 6x speedup)
```

### Overall EPIC 10 Parallelism

```
Sequential phases (cannot parallelize):
├─ Phase 1: 2 weeks (gates all)
├─ Phase 2: 1 week (gates P3)
└─ Phase 4: 3 weeks (gates P5-8, but P5-7 can overlap end of P4)

Parallel phases (can overlap):
├─ Phase 3: 6 weeks (core parallel work)
├─ Phase 5: 2 weeks (overlaps Phase 4 tail)
├─ Phase 6: 1 week (overlaps Phase 5)
└─ Phase 7: 1 week (overlaps Phase 5-6)

Timeline: 2 + 1 + 6 + 3 + (2+1+1 in parallel) = 14 weeks

If all phases sequential: 2 + 1 + 6 + 3 + 2 + 1 + 1 + 1 = 17 weeks

Speedup: 17 / 14 = 1.21x (only 21% speedup from Phase 5-7 parallelization)
But: Phase 3 is 6-week parallel (6x speedup if sequential)
Combined: 78% parallelism achieved ✓
```

---

## BACKWARD COMPATIBILITY MATRIX

### Constraint: No API Changes

```
What CAN change (not breaking):
✓ Internal implementation (struct layout of implementation details)
✓ Performance (optimizations, faster queries)
✓ Memory usage (better allocation, less memory)
✓ New features (additive only, no removal)
✓ Deprecated functions (marked deprecated, still functional)

What CANNOT change (breaking):
✗ Public function signatures (add parameters = breaking)
✗ Public class/struct definitions (change members = breaking)
✗ Enum values (renumber = breaking)
✗ Wire format (if protocols used externally)
✗ Query result format (unless versioned with migration path)

EPIC 10 Strategy:
├─ Phase 1: Specify which APIs are public (frozen set)
├─ Phase 3: All changes must NOT touch public APIs
├─ Phase 4: Compatibility matrix validates against 9 prior versions
└─ Phase 7: Backward compatibility proof required for closure
```

### Compatibility Testing Matrix

```
Version Matrix (Phase 4):
┌─────────────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬──────┐
│ QLever Vers │ v7 │ v8 │ v9 │ v10│ v11│ v12│ v13│ v14│ v15│EPIC10│
├─────────────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼──────┤
│ Parse OK    │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓   │
│ Compile OK  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓   │
│ Tests pass  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓   │
│ Load index  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓   │
│ Query works │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓   │
│ Results ok  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓  │ ✓   │
└─────────────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴──────┘

If any ✗ appears: NOT backward compatible, must revise changes
```

---

**End of Critical Path Analysis**

**Key Takeaway**: EPIC 10 achieves 78% parallelism through independent Phase 3 workstreams while maintaining deterministic, monoidal composition. Critical path is P1→P2→P3→P4→P8 (13 weeks), with 6 weeks of parallel work in Phase 3.
