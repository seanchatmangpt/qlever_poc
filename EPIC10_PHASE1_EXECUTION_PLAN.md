# EPIC 10: PHASE 1 EXECUTION PLAN
## Specification & Invariant Closure (Weeks 1-2)

**Phase**: 1 of 8
**Duration**: 2 weeks (10 working days)
**Type**: Sequential (gates all other phases)
**Owner**: Agent 1 (Specification & Global State Lead)
**Supporting**: Agents 2-10 (review and sign-off)

**Mission**: Eliminate all design ambiguity. Freeze all architectural decisions. Document all invariants. Achieve zero degrees of freedom.

---

## WEEK 1: INVARIANT FORMALIZATION

**Objective**: Formalize 6 core axioms, identify 20+ architectural invariants, audit current codebase for violations.

### Day 1 (Monday): Axiom Formalization Workshop
**Time**: Full day (8 hours)
**Owner**: Agent 1
**Participants**: All 10 agents

**Activities**:
1. **Review BB80/20 + EPIC 9 principles** (1 hour)
   - Monoidal composition: What it means for EPIC 10
   - Determinism: Sources of non-determinism in QLever
   - Atomic failure: Fail-closed semantics in build system

2. **Formalize 6 Core Axioms** (4 hours)
   - **AX-1 (Immutability)**: Define "no global mutable state" precisely
     - Exception: Synchronized<T> wrapper (fine-grained locking)
     - Audit: Identify all global variables in codebase
     - Enforcement: Static analysis rule + CI gate

   - **AX-2 (Determinism)**: Define "manifest.sha256 identical" precisely
     - Sources: Hash randomization, floating-point, time-dependent code
     - Audit: Identify all std::unordered_map, double/float in critical paths
     - Enforcement: 100 builds test + CI gate

   - **AX-3 (Atomic Failure)**: Define "all phases complete or none" precisely
     - Mechanism: Build system stops at first error (set -e)
     - Rollback: Automatic on phase failure
     - Enforcement: Phase gate scripts

   - **AX-4 (No External State)**: Define "pure functions" precisely
     - Query execution: Reproducible from SPARQL input + RDF dataset
     - No side-effects: No file modifications, no network calls during execution
     - Enforcement: Code review + runtime assertions

   - **AX-5 (RAII)**: Define "resource acquisition = initialization" precisely
     - All allocations: Use RAII wrapper (unique_ptr, Synchronized<T>, IdTable)
     - No manual cleanup: Destructors handle all resource release
     - Enforcement: Valgrind clean + AddressSanitizer clean

   - **AX-6 (Backward Compatibility)**: Define "no API changes" precisely
     - Public APIs: Function signatures, class definitions, enum values (frozen)
     - Additive only: New features do not change existing APIs
     - Enforcement: Compatibility matrix (9 versions × 6 criteria)

3. **Document Enforcement Mechanisms** (2 hours)
   - For each axiom: CI/CD gate, static analysis rule, runtime assertion
   - For each axiom: Test suite validation (what tests prove compliance?)
   - For each axiom: Violation detection strategy (how do we catch violations early?)

4. **Sign-off**: All 10 agents approve axiom definitions (1 hour)

**Deliverable (Day 1)**:
- Draft section 1 of INVARIANT_CLOSURE_MATRIX.md: "6 Core Axioms" (1 KB)

---

### Day 2 (Tuesday): Component-Level Invariant Identification
**Time**: Full day (8 hours)
**Owner**: Agents 3-10 (domain experts)
**Review**: Agent 1

**Activities** (Parallel, by component):
1. **Parser Invariants** (Agent 3, 2 hours)
   - Immutable AST: No post-parse mutation (all AST nodes const after construction)
   - Deterministic parse order: No hash-based ordering in result structures
   - No external state pollution: Parser is pure function (no globals modified)
   - W3C SPARQL 1.1 compliance: Grammar matches specification exactly
   - Error recovery: Fail-safe, not fail-fast (parse continues on error, reports all errors)

2. **Index Invariants** (Agent 6, 2 hours)
   - Immutable index: Post-construction (no modifications after build)
   - Deterministic compression/decompression: No floating-point, no approximation
   - Vocabulary mapping: Bijective (1:1 with RDF universe, no collisions)
   - Consistency: Checksums validate integrity (proof trees for validation)

3. **Engine Invariants** (Agent 7, 3 hours)
   - No mutable query execution state: QueryExecutionTree immutable during execution
   - Deterministic join ordering: Cost model is pure function (no randomization)
   - Cancellation: Via SharedCancellationHandle only (no global flags)
   - Operation hierarchy: Virtual dispatch safe (no slicing, no dangling pointers)
   - Cost model determinism: Same query, same plan, every time

4. **Memory (IdTable) Invariants** (Agent 4, 2 hours)
   - Row-major layout: Fixed at compile time (no runtime changes)
   - Fixed column count: Determined by query, immutable after allocation
   - Allocator contracts: AllocatorWithLimit enforces hard bounds
   - No COW semantics: Copy is explicit, not implicit
   - Exception safety: Strong guarantee on all operations (no partial state)

5. **Concurrency Invariants** (Agent 5, 2 hours)
   - Synchronized<T> wraps all shared state: No raw mutexes or atomics
   - No deadlocks: Lock hierarchy total order enforced
   - No data races: ThreadSanitizer clean (0 reports)
   - Cancellation propagates atomically: All tasks notified via shared flag

6. **Type System Invariants** (Agent 4, 1 hour)
   - Fixed-point encoding: No floating-point in RDF values (determinism)
   - Encoding/decoding deterministic: Bijection (valid RDF ↔ valid IdTable values)
   - No approximation: Exact representation required

7. **Global State Invariants** (Agent 1, 1 hour)
   - Zero mutable singletons: All global state is const or Synchronized<T>
   - Configuration immutable after startup: RuntimeImmutable after construction
   - Deterministic initialization order: Static initialization fiasco avoided

8. **Build System Invariants** (Agent 2, 1 hour)
   - No circular dependencies: DAG verified (topological sort exists)
   - All components compile independently: No hidden cross-dependencies
   - Compiler flags deterministic: No random optimization levels

9. **Backward Compat Invariants** (Agent 1, 1 hour)
   - Version constants frozen: 15 version constants (no new formats without handler)
   - Format handlers for N-2 versions: All format handlers present
   - Serialization layer: Handles all versions (forward/backward compatible)
   - No breaking API changes: All public interfaces stable

**Deliverable (Day 2)**:
- Draft section 2 of INVARIANT_CLOSURE_MATRIX.md: "Component-Level Invariants (9 components)" (6 KB)

---

### Day 3 (Wednesday): Phase-Level Invariant Identification
**Time**: Full day (8 hours)
**Owner**: Agent 1 (phase orchestration)
**Review**: Agents 2-10

**Activities**:
1. **Phase 1 Invariants** (1 hour)
   - Specification closure gates all work: Phase 2 cannot start until P1 complete
   - Design choices frozen: Zero degrees of freedom after Week 2
   - All invariants documented: 6 axioms + 20+ architectural invariants

2. **Phase 2 Invariants** (1 hour)
   - Dependency DAG verified: No circular dependencies (topological sort exists)
   - Risk matrix complete: All collision zones rated, mitigation strategies defined
   - Memory model formalized: IdTable layout, allocator contracts documented

3. **Phase 3 Invariants** (2 hours)
   - All 6 workstreams pass ThreadSanitizer: 0 data races
   - All 6 workstreams pass valgrind: 0 memory leaks
   - Code coverage ≥95% on all changes
   - 280+ tests passing: 0 flakes in 10 runs

4. **Phase 4 Invariants** (1 hour)
   - TPC-H 22/22 queries pass: Regression < 5%
   - Compatibility matrix pass: 9 versions × 6 criteria = 54 tests
   - Determinism proof: 100 builds, manifest.sha256 identical

5. **Phase 5 Invariants** (1 hour)
   - Performance baseline documented: Flame graphs, metrics published
   - Hot paths profiled: CPU cycles, memory allocation, lock contention
   - Regression < 2%: Latency (P50/P95/P99), throughput (queries/sec), memory (peak RSS)

6. **Phase 6 Invariants** (1 hour)
   - Architecture.md complete: Component responsibilities, interfaces, invariants
   - 6 ADRs approved: Rationale for key design choices
   - Runbook complete: Startup, shutdown, troubleshooting, escalation

7. **Phase 7 Invariants** (30 minutes)
   - All 6 axioms validated: AX-1 through AX-6 proven
   - Compliance report signed: All phase invariants verified
   - CI integration complete: All gates passing

8. **Phase 8 Invariants** (30 minutes)
   - Release notes drafted: Features, changes, migrations, deprecations
   - Deployment plan approved: Canary plan, rollback procedure, monitoring dashboards
   - SLA metrics defined: 99.9% availability, P99 < 500ms

**Deliverable (Day 3)**:
- Draft section 3 of INVARIANT_CLOSURE_MATRIX.md: "Phase-Level Invariants (8 phases)" (3 KB)

---

### Day 4 (Thursday): Cross-Cutting Invariant Identification
**Time**: Full day (8 hours)
**Owner**: Agents 4, 5, 7 (memory, concurrency, performance)
**Review**: Agent 1

**Activities**:
1. **Concurrency Invariants** (Agent 5, 3 hours)
   - Synchronized<T> usage patterns: When required, when optional
   - Lock hierarchy: Total order documented (prevent deadlocks)
   - Lock granularity: Fine-grained (per-object) vs. coarse-grained (global)
   - Cancellation patterns: SharedCancellationHandle propagation
   - ThreadSanitizer compliance: TSan clean on every merge

2. **Memory Invariants** (Agent 4, 3 hours)
   - IdTable layout: Row-major, column count fixed (frozen in Phase 1)
   - AllocatorWithLimit: Hard bounds enforced (no unbounded allocation)
   - Memory pools: Block sizing decisions (initialization vs. hot-path)
   - Exception safety: Strong guarantee (no partial state on failure)
   - Valgrind compliance: Clean on every merge

3. **Performance Invariants** (Agent 7, 2 hours)
   - Hot paths profiled: Identified before optimization (Phase 5)
   - Cost model parameterized: 7% per join column (tunable without code change)
   - SIMD restrictions: Integer operations only (no floating-point non-determinism)
   - Regression limits: < 5% for TPC-H, < 2% for latency
   - Benchmarks: Throughput, latency (P50/P95/P99), memory (peak RSS)

**Deliverable (Day 4)**:
- Draft section 4 of INVARIANT_CLOSURE_MATRIX.md: "Cross-Cutting Invariants (concurrency, memory, performance)" (3 KB)

---

### Day 5 (Friday): Forbidden Patterns & Anti-Patterns Documentation
**Time**: Full day (8 hours)
**Owner**: Agent 1
**Review**: All 10 agents

**Activities**:
1. **Identify Forbidden Patterns** (4 hours)
   - Hash randomization in output paths (std::unordered_map iteration → std::map)
   - Floating-point in determinism-critical paths (cost model, RDF values)
   - Global mutable state outside Synchronized<T> (singletons, static variables)
   - Circular dependencies in build system (CMakeLists.txt validation)
   - API changes breaking backward compatibility (public headers frozen)
   - Manual cleanup (no delete, no free → RAII only)
   - Raw mutexes or atomics (use Synchronized<T> instead)
   - Time-dependent code in query execution (no std::chrono in hot paths)

2. **Document Detection Mechanisms** (2 hours)
   - Static analysis rules: Clang-tidy checks, custom scripts
   - CI gates: Automated enforcement on every merge
   - Code review checklists: Human validation during PR review
   - Runtime assertions: Debug-mode checks (release-mode disabled)

3. **Document Remediation Strategies** (2 hours)
   - For each forbidden pattern: How to fix if detected
   - For each anti-pattern: Preferred alternative (e.g., std::unordered_map → std::map)
   - For each violation: Escalation path (who decides if exception warranted?)

**Deliverable (Day 5)**:
- Draft section 5 of INVARIANT_CLOSURE_MATRIX.md: "Forbidden Patterns & Anti-Patterns" (3 KB)
- Week 1 complete: INVARIANT_CLOSURE_MATRIX.md draft ready for review (16 KB)

---

## WEEK 2: DESIGN DECISION FREEZE

**Objective**: Freeze all architectural decisions. Eliminate all degrees of freedom. Achieve specification closure.

### Day 6 (Monday): Design Choice Audit
**Time**: Full day (8 hours)
**Owner**: Agent 2 (Architecture Lead)
**Review**: All 10 agents

**Activities**:
1. **Audit All Outstanding Design Choices** (6 hours)
   - **IdTable Layout**: Row-major vs. column-major? → **FREEZE: Row-major**
   - **Memory Allocation Strategy**: Pooling vs. per-query? → **FREEZE: AllocatorWithLimit with pooling**
   - **Cost Model Constants**: 7% per join column tunable? → **FREEZE: Parameterized (tunable in Phase 5 only)**
   - **Concurrency Primitive**: Synchronized<T> vs. raw mutexes? → **FREEZE: Synchronized<T> required**
   - **Version Handling**: Add VERSION_2_SIMD now or later? → **FREEZE: Add now (Prerequisite 1)**
   - **Runtime Parameters**: Hot-path or initialization? → **FREEZE: Move to initialization (Prerequisite 3)**
   - **Join Algorithm Selection**: Function pointer table vs. switch? → **FREEZE: Function pointer table (branchless)**
   - **SIMD Restrictions**: Integer-only or allow float? → **FREEZE: Integer-only (determinism)**
   - **Backward Compatibility**: Support N-2 versions or N-1? → **FREEZE: N-2 (9 versions tested)**
   - **Parser Grammar**: Freeze now or after P3A? → **FREEZE: Now (W3C SPARQL 1.1 spec)**

2. **Document Each Decision** (2 hours)
   - For each decision: Rationale (why this choice?)
   - For each decision: Alternatives considered (what was rejected?)
   - For each decision: Consequences (what does this enable/prevent?)
   - For each decision: Enforcement (how do we prevent changes?)

**Deliverable (Day 6)**:
- Draft section 6 of INVARIANT_CLOSURE_MATRIX.md: "Design Choices Frozen" (4 KB)

---

### Day 7 (Tuesday): Collision Zone Mitigation Finalization
**Time**: Full day (8 hours)
**Owner**: Agent 9 (Collision Detection Lead)
**Review**: Agents 1, 2, 7

**Activities**:
1. **Finalize IdTable Collision Mitigation** (2 hours)
   - Document: IdTable layout frozen (row-major, column-based)
   - Document: If SIMD needs different layout, use versioned adapter layer
   - Document: IdTableSOA (old) → IdTableAOS (new) adapter interface
   - Prerequisite 4 verification: Checklist complete

2. **Finalize Cost Model Collision Mitigation** (2 hours)
   - Document: Cost model constants parameterized (7% → JOIN_COLUMN_COST_FACTOR)
   - Document: Performance phase may only modify constants, not logic
   - Prerequisite 2 verification: Checklist complete

3. **Finalize Filter Collision Mitigation** (2 hours)
   - Document: Runtime parameter decisions moved to initialization
   - Document: Branchless phase eliminates hot-path conditionals only
   - Prerequisite 3 verification: Checklist complete

4. **Finalize Version Constant Collision Mitigation** (2 hours)
   - Document: Pre-add VERSION_2_SIMD handler (empty, accepts old format)
   - Document: Branchless phase does not eliminate version checks (critical for backward compat)
   - Prerequisite 1 verification: Checklist complete

**Deliverable (Day 7)**:
- Section 7 of INVARIANT_CLOSURE_MATRIX.md: "Collision Zone Mitigation Strategies" (4 KB)

---

### Day 8 (Wednesday): Prerequisite Verification
**Time**: Full day (8 hours)
**Owner**: Agent 1 (Phase Lead)
**Review**: All 10 agents

**Activities**:
1. **Verify Prerequisite 1: Serialization Format Envelope** (1 hour)
   - [ ] Version enum includes VERSION_2_SIMD (in code)
   - [ ] Format handler skeleton exists (accepts VERSION_1 format)
   - [ ] Documented: "After this point, format changes require explicit version handler"
   - **Status**: PASS/FAIL (if FAIL, blocker escalated)

2. **Verify Prerequisite 2: Cost Model Parameterization** (1 hour)
   - [ ] DynamicCostFactors.h contains named constant (JOIN_COLUMN_COST_FACTOR = 0.07)
   - [ ] All hardcoded "7%" references replaced with named constant
   - [ ] Documented: "Performance phase may only modify constants, not logic"
   - **Status**: PASS/FAIL

3. **Verify Prerequisite 3: Runtime Parameter Audit** (1 hour)
   - [ ] All getRuntimeParameter calls identified (Filter.cpp: enablePrefilterOnIndexScans_)
   - [ ] Each call classified as hot-path or initialization
   - [ ] Documented: "Branchless phase may only eliminate hot-path decisions after moving to init"
   - **Status**: PASS/FAIL

4. **Verify Prerequisite 4: IdTable Layout Stabilization** (1 hour)
   - [ ] IdTable layout documented (current: row-major, column-based)
   - [ ] Layout frozen in current form (no changes without versioned adapter)
   - [ ] If SIMD needs different layout: adapter layer skeleton exists
   - **Status**: PASS/FAIL

5. **Verify Prerequisite 5: Regression Test Baseline** (2 hours)
   - [ ] All TPC-H queries executed on unmodified code
   - [ ] Query execution times recorded (baseline for regression detection)
   - [ ] Join selection recorded for each query (expected algorithm)
   - [ ] Documented: "Branchless phase must not degrade these times."
   - **Status**: PASS/FAIL

6. **Verify Prerequisite 6: Compiler Capability Detection** (1 hour)
   - [ ] CMakeLists.txt detects CPU capabilities (SSE4.2, AVX2, AVX-512)
   - [ ] SIMD code only compiled if CPU supports it
   - [ ] Documented: "SIMD instructions only compiled if CPU supports them"
   - **Status**: PASS/FAIL

7. **Verify Prerequisite 7: Organizational Alignment on Priorities** (1 hour)
   - [ ] Query pattern priorities documented (OLAP vs. OLTP)
   - [ ] Performance improvement targets defined (e.g., "SPARQL-STAR queries 2x faster")
   - [ ] Trade-off policy documented (e.g., "Favor OLAP over OLTP if conflict")
   - **Status**: PASS/FAIL (requires organizational decision-maker sign-off)

**Deliverable (Day 8)**:
- Prerequisite verification report: All 7 prerequisites PASS/FAIL status
- **GATE**: If ANY prerequisite FAILS, Phase 1 is BLOCKED (escalate immediately)

---

### Day 9 (Thursday): Final Review & Consolidation
**Time**: Full day (8 hours)
**Owner**: Agent 1
**Review**: All 10 agents (sign-off required)

**Activities**:
1. **Consolidate INVARIANT_CLOSURE_MATRIX.md** (3 hours)
   - Merge all sections (1-7) into single document
   - Format consistency check (markdown, headings, formatting)
   - Hyperlink cross-references (e.g., "See AX-2 for determinism details")
   - Table of contents generation

2. **Final Review Workshop** (4 hours, all 10 agents)
   - Agent 1 presents full document (30 minutes)
   - Each agent reviews their section (60 minutes)
   - Dispute resolution: Design conflicts escalated to formal decision (90 minutes)
   - Final edits: Incorporate feedback (60 minutes)

3. **Sign-off Round** (1 hour)
   - Each agent signs off: "I approve this specification as frozen"
   - Signature format: `Agent [X] ([Role]): APPROVED on [Date]`
   - Escalation: If any agent does not sign off, blocker escalated (Phase 1 extends)

**Deliverable (Day 9)**:
- INVARIANT_CLOSURE_MATRIX.md final version (20 KB)
- All 10 agents signed off (approval recorded in document)

---

### Day 10 (Friday): Commit, Tag, Phase 2 Gate Preparation
**Time**: Full day (8 hours)
**Owner**: Agent 1
**Review**: Agent 2 (Phase 2 lead)

**Activities**:
1. **Commit INVARIANT_CLOSURE_MATRIX.md** (1 hour)
   - Git commit message: "feat(EPIC 10): Phase 1 specification closure complete - all design choices frozen"
   - Commit to main branch (or feature branch if using branch strategy)
   - Tag commit: `EPIC10_SPEC_CLOSED`

2. **Phase 2 Gate Preparation** (3 hours)
   - Prepare handoff to Agent 2: INVARIANT_CLOSURE_MATRIX.md + prerequisite verification report
   - Phase 2 Gate Criteria documented (see below)
   - Phase 2 kickoff meeting scheduled (Monday of Week 3)

3. **Retrospective** (2 hours)
   - What went well in Phase 1? (monoidal composition achieved?)
   - What was difficult? (design conflicts? prerequisite blockers?)
   - What should Phase 2-8 learn from Phase 1? (lessons learned)

4. **Phase 1 Closure Report** (2 hours)
   - Summary: What was delivered?
   - Status: All acceptance criteria met?
   - Blockers: Any unresolved issues escalated?
   - Next steps: Phase 2 ready to start?

**Deliverable (Day 10)**:
- INVARIANT_CLOSURE_MATRIX.md committed and tagged
- Phase 1 Closure Report (5 KB)
- Phase 2 Gate Criteria documented (see below)

---

## PHASE 2 GATE CRITERIA

**Phase 1 → Phase 2 Gate**:

Phase 2 **CANNOT START** until all of the following are **TRUE**:

### 1. Specification Closure Complete ✓
- [ ] INVARIANT_CLOSURE_MATRIX.md committed to repository
- [ ] Document tagged: EPIC10_SPEC_CLOSED
- [ ] Document size: ≥ 15 KB (sufficient detail)
- [ ] Document contains all 7 sections:
  - [ ] Section 1: 6 Core Axioms (AX-1 through AX-6)
  - [ ] Section 2: Component-Level Invariants (9 components)
  - [ ] Section 3: Phase-Level Invariants (8 phases)
  - [ ] Section 4: Cross-Cutting Invariants (concurrency, memory, performance)
  - [ ] Section 5: Forbidden Patterns & Anti-Patterns
  - [ ] Section 6: Design Choices Frozen
  - [ ] Section 7: Collision Zone Mitigation Strategies

### 2. All Design Choices Frozen ✓
- [ ] Zero degrees of freedom: All decisions documented in Section 6
- [ ] No outstanding design questions: All "TBD" removed from document
- [ ] All 10 agents signed off: Approval recorded in document

### 3. All 7 Prerequisites Verified ✓
- [ ] Prerequisite 1 (Serialization Format Envelope): PASS
- [ ] Prerequisite 2 (Cost Model Parameterization): PASS
- [ ] Prerequisite 3 (Runtime Parameter Audit): PASS
- [ ] Prerequisite 4 (IdTable Layout Stabilization): PASS
- [ ] Prerequisite 5 (Regression Test Baseline): PASS
- [ ] Prerequisite 6 (Compiler Capability Detection): PASS
- [ ] Prerequisite 7 (Organizational Alignment on Priorities): PASS

### 4. All Collision Zones Have Mitigation Strategies ✓
- [ ] IdTable & Memory Management (95% risk): Mitigation documented in Section 7
- [ ] Adaptive Optimization Algorithms (85% risk): Mitigation documented
- [ ] Filter & Expression Evaluation (75% risk): Mitigation documented
- [ ] Version & Backward Compatibility Constants (70% risk): Mitigation documented

### 5. Enforcement Mechanisms Defined ✓
- [ ] For each axiom: CI/CD gate defined
- [ ] For each invariant: Test validation defined
- [ ] For each forbidden pattern: Detection mechanism defined
- [ ] For each collision zone: Early-warning signal defined

### 6. No Iteration Required ✓
- [ ] Specification is complete: No missing sections
- [ ] Specification is unambiguous: No "TBD", "to be determined", "unclear"
- [ ] Specification is deterministic: All choices made, documented, frozen
- [ ] Specification is closed: No further iteration permitted

**Gate Enforcement**: If ANY criterion is FALSE, Phase 2 is **BLOCKED**. Phase 1 extends until all criteria are TRUE.

**Gate Owner**: Agent 1 (Phase 1 lead) validates criteria, Agent 2 (Phase 2 lead) confirms readiness.

---

## NO AMBIGUITY, NO ITERATION

**Ambiguity Elimination**:
- All design choices made: Section 6 documents every decision
- All invariants formalized: Sections 1-4 define constraints
- All forbidden patterns enumerated: Section 5 lists anti-patterns
- All collision zones mitigated: Section 7 provides strategies

**Iteration Prevention**:
- Design freeze: Week 2, Day 10 (no changes permitted after commit)
- Sign-off required: All 10 agents approve (disputes resolved before sign-off)
- Gate enforcement: Phase 2 cannot start until all criteria met
- Monoidal composition: Changes merge without rework (no re-architecture)

**Specification Closure Guarantee**:
- Zero degrees of freedom: All choices made
- Deterministic output: Same specification, every time
- No rework: Single-pass execution (BB80/20 principle)

---

## ANTI-PATTERNS TO AVOID IN PHASE 1

**Anti-Pattern 1: "We'll decide this later"**
- **Problem**: Deferred design decisions create ambiguity
- **Solution**: All decisions made in Week 1-2, documented in Section 6
- **Enforcement**: No "TBD" permitted in final document

**Anti-Pattern 2: "Let's see what happens in Phase 3"**
- **Problem**: Phase 3 workstreams need frozen specification to proceed
- **Solution**: All design choices frozen before Phase 2 starts
- **Enforcement**: Gate criteria block Phase 2 if decisions incomplete

**Anti-Pattern 3: "We can iterate on the invariants"**
- **Problem**: Iteration violates BB80/20 single-pass principle
- **Solution**: Invariants finalized in Phase 1, no iteration permitted
- **Enforcement**: Specification closure is absolute (no reopening)

**Anti-Pattern 4: "This collision zone is low risk, we can handle it later"**
- **Problem**: All collision zones require mitigation strategies
- **Solution**: Section 7 documents strategies for all zones (even low risk)
- **Enforcement**: Gate criteria require all 4 zones mitigated

**Anti-Pattern 5: "Prerequisites are optional suggestions"**
- **Problem**: Prerequisites are blocking gates, not suggestions
- **Solution**: All 7 prerequisites verified before Phase 1 completes
- **Enforcement**: Prerequisite verification report required (Day 8)

---

## SUCCESS METRICS FOR PHASE 1

**Metric 1: Specification Size**
- **Target**: INVARIANT_CLOSURE_MATRIX.md ≥ 15 KB
- **Actual**: _____ KB (measured on Day 10)
- **Pass Criteria**: ≥ 15 KB (sufficient detail for implementation)

**Metric 2: Agent Sign-Off Rate**
- **Target**: 10/10 agents signed off
- **Actual**: _____ / 10 (measured on Day 9)
- **Pass Criteria**: 10/10 (unanimous approval required)

**Metric 3: Prerequisite Verification Rate**
- **Target**: 7/7 prerequisites PASS
- **Actual**: _____ / 7 (measured on Day 8)
- **Pass Criteria**: 7/7 (all prerequisites verified)

**Metric 4: Design Decisions Frozen**
- **Target**: Zero degrees of freedom (all "TBD" removed)
- **Actual**: _____ outstanding decisions (measured on Day 6)
- **Pass Criteria**: 0 (no ambiguity remains)

**Metric 5: Time to Completion**
- **Target**: 10 working days (2 weeks)
- **Actual**: _____ days (measured on Day 10)
- **Pass Criteria**: ≤ 12 days (2-day buffer acceptable)

**Metric 6: Specification Closure**
- **Target**: CLOSED ✓ (binary decision)
- **Actual**: _____ (CLOSED or INCOMPLETE, measured on Day 10)
- **Pass Criteria**: CLOSED (no iteration permitted)

---

## PHASE 1 COMPLETION CHECKLIST

**Day 10 Final Validation** (Agent 1 signs off):

- [ ] INVARIANT_CLOSURE_MATRIX.md committed to repository
- [ ] Document tagged: EPIC10_SPEC_CLOSED
- [ ] Document size: ≥ 15 KB
- [ ] All 7 sections complete (no missing sections)
- [ ] All 10 agents signed off (approval recorded)
- [ ] All 7 prerequisites verified (PASS)
- [ ] All design choices frozen (zero degrees of freedom)
- [ ] All collision zones mitigated (Section 7 complete)
- [ ] No "TBD" or ambiguous language remains
- [ ] Phase 2 Gate Criteria documented
- [ ] Phase 1 Closure Report delivered
- [ ] Phase 2 kickoff meeting scheduled (Week 3, Monday)

**If ALL items checked**: Phase 1 is **COMPLETE** ✓
**If ANY item unchecked**: Phase 1 is **INCOMPLETE** ✗ (extends until complete)

---

## NEXT STEPS

**Immediate (Day 10, Friday)**:
- Commit INVARIANT_CLOSURE_MATRIX.md
- Tag commit: EPIC10_SPEC_CLOSED
- Deliver Phase 1 Closure Report

**Week 3 (Phase 2 Start)**:
- Agent 2 leads architecture analysis
- Dependency DAG verification
- Risk matrix completion
- Memory model formalization

**Week 4-9 (Phase 3 Start)**:
- 6 parallel workstreams begin (P3A-P3F)
- All workstreams use frozen specification (INVARIANT_CLOSURE_MATRIX.md)
- No design changes permitted (specification is closed)

---

**Document Status**: AUTHORITATIVE (Phase 1 execution plan)
**Ambiguity**: ZERO (all activities defined, acceptance criteria clear)
**Iteration Required**: NO (single-pass execution, deterministic)
**Phase 1 Ready**: YES (upon prerequisite verification on Day 8)
