# QLever Skill-by-Phase Quick Reference

This is a visual quick reference for the 6-phase atomic cognitive cycle and associated skills/agents.

---

## The Atomic Cognitive Cycle: Visual Map

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                    ATOMIC COGNITIVE CYCLE (EPIC 9)                          │
│                         6 Mandatory Phases                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

PHASE 1: FAN-OUT (GATE)
─────────────────────────────────────────────────────────────────────────────

Specification Closure Verification

  Skill: bb80-specification-closure
  Agent: bb80-specification-validator

  OPERATION:
    ✓ Verify RDF fully formalized
    ✓ Verify SPARQL fully formalized
    ✓ Verify C++20 fully formalized
    ✓ Verify CMake fully formalized
    ✓ Zero design freedom confirmed
    ✓ No ambiguities present

  OUTPUT: CLOSED or INCOMPLETE (binary verdict)

  GATE PASS → Proceed to Phase 2
  GATE FAIL → Iterate on specification only (no code)

  ABORT CONDITIONS:
    • Specification incomplete
    • Multiple valid approaches exist
    • Design choices remain open
    • Ambiguous instructions
    • No definitive closure verdict


PHASE 2a: INDEPENDENT CONSTRUCTION (Planning & Agent Dispatch)
─────────────────────────────────────────────────────────────────────────────

Parallel Agent Orchestration

  Skill: bb80-parallel-agents
  Agent: bb80-parallel-task-coordinator

  OPERATION:
    ✓ Spawn 10 agents immediately
    ✓ All agents work in parallel
    ✓ Under shared invariant
    ✓ Independent execution (no coordination)
    ✓ 80% work surface concurrent
    ✓ Synchronize only after invariant stabilization

  OUTPUT: 10 independent agents launched

  GATE PASS → Agents running in parallel
  GATE FAIL → Abort (fewer than 10 agents or premature sync)

  ABORT CONDITIONS:
    • Fewer than 10 agents spawned (non-trivial task)
    • Premature synchronization
    • Serialization without domain justification
    • Shared invariant violated
    • Coverage below 80%


PHASE 2b: INDEPENDENT CONSTRUCTION (Implementation)
─────────────────────────────────────────────────────────────────────────────

Invariant-Driven Monoidal Construction

  Skill: bb80-invariant-construction
  Agent: bb80-invariant-validator

  OPERATION (during Phase 2a):
    ✓ Extract minimal invariant set (20% → 80%)
    ✓ Verify monoidal composition (no mutation)
    ✓ Confirm single-pass feasibility (no backtracking)
    ✓ Validate deterministic reconstruction
    ✓ No mutable external state
    ✓ State reconstructible from events + hashes

  OUTPUT: Validated monoidal construction

  GATE PASS → Invariants hold, construction valid
  GATE FAIL → Abort (backtracking required or monoidal violation)

  ABORT CONDITIONS:
    • Backtracking becomes necessary
    • Invariants incomplete
    • Behavior outside invariant set
    • Mutable external state detected
    • Non-deterministic reconstruction
    • Rework required


PHASE 3: COLLISION DETECTION
─────────────────────────────────────────────────────────────────────────────

Artifact Overlap Analysis (10 agents → artifacts)

  Agent: bb80-collision-detector (no associated skill)
  Invocation: Direct dispatch (mandatory, EPIC 9)

  OPERATION (after 10 agents complete):
    ✓ Analyze all 10 agent artifacts
    ✓ Detect structural overlaps (identical schemas, equiv. structures)
    ✓ Detect semantic overlaps (same conclusions, different paths)
    ✓ Track execution path divergences
    ✓ Quantify overlap (0-100% per collision)
    ✓ Generate deterministic collision map

  OUTPUT: Machine-parseable collision map
           (which artifacts collide, level, magnitude, hints)

  GATE PASS → Collision map complete, ready for convergence
  GATE FAIL → Abort (ambiguous or incomplete collision analysis)

  COLLISION IS REQUIRED SIGNAL (not failure!)

  ABORT CONDITIONS:
    • Ambiguous collision analysis
    • Incomplete divergence tracking
    • Narrative output (not machine-parseable)
    • Collision detection skipped
    • Any artifact pair not analyzed


PHASE 4: CONVERGENCE
─────────────────────────────────────────────────────────────────────────────

Selection Pressure & Reconciliation (10 artifacts → 1)

  Agent: bb80-convergence-orchestrator (no associated skill)
  Invocation: Direct dispatch (mandatory, EPIC 9)

  OPERATION (after collision detection):
    ✓ Apply selection pressure (4 criteria):
       1. Coverage (extent relative to task)
       2. Invariant Preservation (structural integrity)
       3. Eliminable Redundancy (can merge without loss?)
       4. Construct Minimality (minimal structure?)
    ✓ Identify dominated vs. Pareto-optimal artifacts
    ✓ Build dominance relation matrix
    ✓ Execute reconciliation strategy:
       - Select dominant artifacts as bases
       - Merge non-redundant portions
       - Rewrite/simplify where needed
       - Discard entirely subsumed work
    ✓ Erase original agent authorship boundaries

  OUTPUT: Single merged, refactored artifact
           (deterministic convergence receipt)

  GATE PASS → Converged artifact ready, authorship erased
  GATE FAIL → Abort (failed selection pressure or reconciliation)

  REFACTORING IS DESTRUCTIVE
  → Only final construction survives
  → Original boundaries erased

  ABORT CONDITIONS:
    • Multiple artifacts emitted (not single)
    • Selection pressure criteria unsatisfied
    • Agent authorship preserved
    • Deterministic receipt missing
    • Narrative instead of deterministic analysis


PHASE 5: REFACTORING & SYNTHESIS
─────────────────────────────────────────────────────────────────────────────

Merge, Discard, Rewrite (typically part of Phase 4)

  No separate skill/agent; part of bb80-convergence-orchestrator

  OPERATION (during/after Phase 4):
    ✓ Merge compatible portions
    ✓ Discard dominated/redundant work
    ✓ Rewrite for clarity/efficiency where needed
    ✓ Ensure all dominance justifications preserved
    ✓ Verify final artifact passes receipt validators

  OUTPUT: Final refactored artifact
           (ready for Phase 6 validation)

  GATE PASS → Refactored artifact complete
  GATE FAIL → Abort (refactoring incomplete or invalid)

  MANDATORY INVARIANTS:
    → Only final construction survives
    → No intermediate steps preserved
    → Authorship boundaries completely erased
    → All refactoring decisions documented


PHASE 6: CLOSURE (GATE)
─────────────────────────────────────────────────────────────────────────────

Deterministic Receipt Validation & Certification

  Skill: bb80-deterministic-receipts
  Agent: bb80-receipt-validator

  OPERATION (final validation):
    ✓ Generate deterministic receipts:
       - Benchmark results (measured, specific numbers)
       - State hashes (cryptographic verification)
       - Event logs (complete execution trace)
       - Guard evaluations (automated checks)
    ✓ Validate against all guards:
       - Type-checked invariants
       - Benchmark thresholds
       - Correctness proofs
    ✓ Reject narrative arguments (must have proof)
    ✓ Achieve proof-based certification
       (if all guards pass → correct by definition)
    ✓ Block reiteration after proof

  OUTPUT: CERTIFIED or REJECTED (binary verdict)

  GATE PASS → Task complete, output valid, no reiteration
  GATE FAIL → Abort immediately

  ALL GUARDS MUST PASS:
    → No exceptions for guard failures
    → No qualifications like "mostly correct"
    → Determinism replaces consensus
    → Proof replaces narrative

  ABORT CONDITIONS:
    • Any single guard fails
    • Benchmark thresholds not met
    • Receipts absent
    • Narrative arguments provided
    • State reconstruction invalid
    • Determinism violated
    • Attempted reiteration after proof


CLOSURE CONDITION
─────────────────────────────────────────────────────────────────────────────

Valid closure requires ALL of the following:

  ✓ Phase 1: Specification closure verified (CLOSED)
  ✓ Phase 2a: 10 agents spawned independently
  ✓ Phase 2b: Invariants extracted, monoidal composition verified
  ✓ Phase 3: Collision analysis performed (map generated)
  ✓ Phase 4: Convergence executed (single artifact produced)
  ✓ Phase 5: Refactoring completed (final artifact emitted)
  ✓ Phase 6: Deterministic receipts validated (all guards passed)

  Failure at ANY point → NO OUTPUT
```

---

## Skill Invocation Checklist

For every non-trivial task:

```
PHASE 1 (FAN-OUT)
  ☐ Invoke: bb80-specification-closure skill
  ☐ Dispatch: bb80-specification-validator agent
  ☐ Require: CLOSED verdict
  ☐ If INCOMPLETE: iterate specification only, return to Phase 1

PHASE 2a (PLANNING)
  ☐ Invoke: bb80-parallel-agents skill
  ☐ Dispatch: bb80-parallel-task-coordinator agent
  ☐ Spawn: exactly 10 agents
  ☐ Verify: shared invariant defined

PHASE 2b (IMPLEMENTATION)
  ☐ Invoke: bb80-invariant-construction skill
  ☐ Dispatch: bb80-invariant-validator agent
  ☐ Extract: minimal invariant set
  ☐ Verify: monoidal composition, single-pass feasibility

PHASE 3 (COLLISION)
  ☐ Dispatch: bb80-collision-detector agent (no skill)
  ☐ Generate: machine-parseable collision map
  ☐ Analyze: all 10 artifact pairs
  ☐ Quantify: overlap magnitude per collision

PHASE 4 (CONVERGENCE)
  ☐ Dispatch: bb80-convergence-orchestrator agent (no skill)
  ☐ Apply: 4-criteria selection pressure
  ☐ Build: dominance relation matrix
  ☐ Produce: single merged artifact (authorship erased)

PHASE 5 (REFACTORING)
  ☐ Merge: non-redundant portions
  ☐ Discard: dominated work
  ☐ Rewrite: clarity/efficiency where needed
  ☐ Finalize: artifact ready for Phase 6

PHASE 6 (CLOSURE)
  ☐ Invoke: bb80-deterministic-receipts skill
  ☐ Dispatch: bb80-receipt-validator agent
  ☐ Generate: benchmarks, hashes, event logs
  ☐ Validate: all guards pass
  ☐ Require: CERTIFIED verdict
  ☐ Block: any reiteration
```

---

## Critical Abort Conditions (Across All Phases)

```
SPECIFICATION (Phase 1)
  ✗ Specification incomplete or ambiguous
  ✗ Multiple valid approaches exist
  ✗ Design choices remaining
  ✗ Ambiguous instructions detected

PARALLELIZATION (Phase 2a)
  ✗ Fewer than 10 agents spawned (non-trivial)
  ✗ Premature synchronization attempted
  ✗ Shared invariant violated
  ✗ Coverage below 80%

INVARIANT VALIDATION (Phase 2b)
  ✗ Backtracking becomes necessary
  ✗ Invariants incomplete
  ✗ Monoidal structure violated
  ✗ Mutable external state introduced
  ✗ State reconstruction non-deterministic

COLLISION DETECTION (Phase 3)
  ✗ Collision analysis ambiguous
  ✗ Artifact pairs not fully analyzed
  ✗ Narrative output (not machine-parseable)
  ✗ Collision detection skipped

CONVERGENCE (Phase 4)
  ✗ Multiple artifacts emitted (not single)
  ✗ Selection pressure criteria unsatisfied
  ✗ Agent authorship preserved
  ✗ Dominance justifications missing

REFACTORING (Phase 5)
  ✗ Intermediate steps preserved (should be erased)
  ✗ Authorship boundaries retained
  ✗ Refactoring decisions undocumented

RECEIPT VALIDATION (Phase 6)
  ✗ Any guard fails (even one)
  ✗ Benchmarks not met
  ✗ Receipts absent
  ✗ Narrative arguments provided
  ✗ Reconstruction proofs invalid
  ✗ Reiteration attempted after proof
```

---

## Agent Authorship & Output Ownership

```
PHASES 1-2: Agent Work (Identifiable)
┌─────────────────────────────┐
│ Phase 1: bb80-specification-validator
│   → Output: CLOSED/INCOMPLETE verdict
│   → Authorship: owned by agent
│
│ Phase 2a: bb80-parallel-task-coordinator + 10 agents
│   → Output: 10 independent artifacts
│   → Authorship: each artifact owned by its agent
│
│ Phase 2b: bb80-invariant-validator
│   → Output: validation reports
│   → Authorship: owned by agent
└─────────────────────────────┘

PHASE 3: Collision Detector (Identifiable)
┌─────────────────────────────┐
│ Phase 3: bb80-collision-detector
│   → Input: 10 artifacts from Phase 2
│   → Output: collision map
│   → Authorship: owned by collision detector
└─────────────────────────────┘

PHASES 4-5: Convergence & Refactoring (Authorship ERASED)
┌─────────────────────────────┐
│ Phase 4: bb80-convergence-orchestrator
│   → Input: 10 artifacts + collision map
│   → Process: selection pressure + reconciliation
│   → Output: SINGLE MERGED ARTIFACT
│
│ Phase 5: Refactoring (within Phase 4 or separate)
│   → Process: merge, discard, rewrite
│   → Output: final artifact
│
│   ⚠ CRITICAL: AUTHORSHIP ERASED
│     → Original agent boundaries removed
│     → Final artifact has NO OWNER
│     → Source attribution removed
│     → Only final construction survives
│
│   ✓ What IS retained:
│     → Dominance justifications
│     → Reconciliation decisions
│     → Merge/discard/rewrite ratios
│     → Which agents contributed (process trace)
└─────────────────────────────┘

PHASE 6: Receipt Validator (Certifier)
┌─────────────────────────────┐
│ Phase 6: bb80-receipt-validator
│   → Input: merged artifact from Phase 4-5
│   → Output: CERTIFIED verdict + receipts
│   → Role: validates, does not own
│   → Authorship: artifact remains ownerless
└─────────────────────────────┘

FINAL OUTPUT: Ownerless, Deterministically Certified Artifact
```

---

## Quick Decision Tree

```
START: New Task
  │
  ├─→ Trivial? (read 1 file, run 1 script, display help, get status)
  │   ├─ YES → Execute directly (no BB80/20 cycle needed)
  │   └─ NO → Continue
  │
  └─→ NON-TRIVIAL: Execute Full Atomic Cognitive Cycle
      │
      ├─→ PHASE 1: Specification Closure (bb80-specification-closure skill)
      │   ├─ CLOSED? → Continue to Phase 2
      │   └─ INCOMPLETE? → Iterate specification only, return to Phase 1
      │
      ├─→ PHASE 2a: Spawn 10 Agents (bb80-parallel-agents skill)
      │   └─ 10 agents launched in parallel
      │
      ├─→ PHASE 2b: Invariant Validation (bb80-invariant-construction skill)
      │   ├─ Invariants valid? → Continue to Phase 3
      │   └─ Invariants invalid? → ABORT
      │
      ├─→ PHASE 3: Collision Detection (bb80-collision-detector agent)
      │   ├─ Collision map generated? → Continue to Phase 4
      │   └─ Analysis incomplete? → ABORT
      │
      ├─→ PHASE 4: Convergence (bb80-convergence-orchestrator agent)
      │   ├─ Single artifact converged? → Continue to Phase 5
      │   └─ Convergence failed? → ABORT
      │
      ├─→ PHASE 5: Refactoring & Synthesis
      │   ├─ Final artifact ready? → Continue to Phase 6
      │   └─ Refactoring incomplete? → ABORT
      │
      └─→ PHASE 6: Receipt Validation (bb80-deterministic-receipts skill)
          ├─ All guards pass? → OUTPUT VALID ✓
          └─ Any guard fails? → ABORT ✗
```

---

## Summary Table: When Each Skill/Agent Is Active

| Phase | Component | Type | Action | Duration |
|-------|-----------|------|--------|----------|
| 1 | bb80-specification-closure | Skill | Invoke | Once, completion required |
| 1 | bb80-specification-validator | Agent | Dispatch | Once, verification |
| 2a | bb80-parallel-agents | Skill | Invoke | Once, orchestration |
| 2a | bb80-parallel-task-coordinator | Agent | Dispatch | Once, spawns 10 agents |
| 2a | 10 independent agents | Agents | Parallel execution | All simultaneously |
| 2b | bb80-invariant-construction | Skill | Invoke | Once, validation |
| 2b | bb80-invariant-validator | Agent | Dispatch | Once, invariant check |
| 3 | bb80-collision-detector | Agent | Dispatch | Once, collision analysis |
| 4 | bb80-convergence-orchestrator | Agent | Dispatch | Once, reconciliation |
| 5 | (part of Phase 4) | — | — | — |
| 6 | bb80-deterministic-receipts | Skill | Invoke | Once, final validation |
| 6 | bb80-receipt-validator | Agent | Dispatch | Once, certification |

---

## Document Metadata

- **Format**: Visual quick reference + decision tree
- **Audience**: Implementers, reviewers, task executors
- **Key Principle**: Agents first. All phases mandatory. No iteration. Determinism required.
- **Status**: EPIC 9 (Unified) Atomic Cognitive Cycle fully defined
