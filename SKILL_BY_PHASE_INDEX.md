# QLever Skill-by-Phase Index: Big Bang 80/20 + EPIC 9

This document maps all **4 skills** and **6 agents** to each phase of the **Atomic Cognitive Cycle** defined in CLAUDE.md.

---

## Overview: Atomic Cognitive Cycle Phases

The cycle is **non-negotiable** and consists of 6 sequential phases:

```
Phase 1: Fan-Out (gate)
  ↓ [specification closure verification]
Phase 2: Independent Construction
  ↓ [10 agents, parallel, no coordination]
Phase 3: Collision Detection
  ↓ [structural + semantic overlap analysis]
Phase 4: Convergence
  ↓ [selection pressure, separate reconciliation]
Phase 5: Refactoring & Synthesis
  ↓ [merge, discard, rewrite as needed]
Phase 6: Closure
  ↓ [all phases complete or no output]
```

**Closure requirement**: All 6 phases must complete successfully. Failure at any point → no output.

---

## Phase 1: Fan-Out (Gate) — Specification Closure

### Skill Invoked
**bb80-specification-closure**

### Description
Verify the domain is fully formalized and closed before implementation starts. Zero degrees of freedom for design choice. ABORT-ON-AMBIGUITY gate.

### When to Invoke
- **Start of every non-trivial task**
- Before any implementation work
- Before spawning agents
- Prerequisite to all downstream phases

### Agent Dispatched
**bb80-specification-validator**

### Agent Role
- Verify RDF, SPARQL, SHACL, C++20, CMake are fully formalized with zero ambiguity
- Identify gaps, ambiguities, open questions that would force iterative implementation
- Mark any point where design choices remain
- Output binary verdict: CLOSED or INCOMPLETE

### Success Outputs
- Verdict: **CLOSED** (deterministic implementation possible)
- All domains formalized: RDF, SPARQL, C++20, CMake, protocols
- Zero design freedom—implementation is deterministic reconstruction
- Permission to proceed to Phase 2

### Failure Outputs
- Verdict: **INCOMPLETE** (iterate on specification, not code)
- Specification contains gaps or ambiguities
- Multiple valid approaches exist
- Design choices remain open
- **ACTION**: Halt immediately, iterate on specification only

### Validation Rules & Gates
- **ABORT-ON-AMBIGUITY**: If any instruction permits multiple interpretations → halt immediately
- No partial execution permitted
- No mixed verdicts (must be definitively CLOSED or INCOMPLETE)
- Iteration must target specification, never code
- Forward progression to Phase 2 forbidden when design choices remain

---

## Phase 2: Independent Construction — Parallel Agents + Invariant Validation

### Skills Invoked
1. **bb80-parallel-agents** (planning, agent orchestration)
2. **bb80-invariant-construction** (implementation constraints)

---

### Skill 1: bb80-parallel-agents

**Description**
Spawn 10 agents immediately for exploration, validation, construction, and testing in parallel. Each agent operates independently under shared invariant. Concurrency is native; serialization is artifact of tooling.

**When to Invoke**
- After specification closure (Phase 1)
- Planning phase begins
- Throughout execution whenever parallel work is available
- Always launch 10 agents

**Agent Dispatched**
**bb80-parallel-task-coordinator**

**Agent Role**
- Spawn 10 agents immediately for exploration, validation, specification verification, construction, testing (all in parallel)
- Ensure all agents operate under same invariant constraint
- Detect synchronization points—integrate only after invariants stabilize
- Ensure 80% of work surface covered by concurrent execution

**Success Outputs**
- 10 independent agents launched
- All agents working in parallel
- Shared invariant enforced across all paths
- 80% work surface covered concurrently
- No forced synchronization before invariant stability

**Failure Outputs**
- Fewer than 10 agents spawned (unless task is trivial)
- Premature synchronization before invariants stabilize
- Serialization without domain-level justification
- Shared invariant violated by any agent
- Coverage below 80%

**Validation Rules & Gates**
- **ABORT-ON-AMBIGUITY**: If instruction permits multiple interpretations → halt immediately
- Serialization requires domain-level justification or trigger abort
- Agents cannot communicate except to report state
- Invariant is the only contract
- Partial execution forbidden

---

### Skill 2: bb80-invariant-construction

**Description**
Build monoidal structures from the minimal invariant set (20% that dominate 80%) in single pass. State fully reconstructible from events + hashes. No rework. No mutable external state.

**When to Invoke**
- Implementation phase (concurrent with or after Phase 2 agent dispatch)
- When extracting minimal feature set
- When validating that construction is monoidal (no backtracking)
- When determining single-pass feasibility

**Agent Dispatched**
**bb80-invariant-validator**

**Agent Role**
- Extract the minimal invariant set—the 20% of features dominating all others
- Verify implementation builds from invariants outward via monoidal composition (not mutation)
- Confirm single-pass execution from invariant set (no backtracking)
- Validate that any state is fully reconstructible from events, snapshots, hashes

**Success Outputs**
- Minimal invariant set identified
- Monoidal composition verified (build from invariants outward)
- Single-pass feasibility confirmed
- Deterministic reconstruction proven
- No mutable external state introduced
- State fully reconstructible from events + hashes

**Failure Outputs**
- Backtracking becomes necessary → abort (invariants incomplete)
- Behavior emerges outside invariant set → abort (violated monoidal structure)
- Mutable external state detected → abort
- Reconstruction not fully deterministic → abort
- Rework required → abort

**Validation Rules & Gates**
- **ABORT-ON-AMBIGUITY**: If instruction permits multiple interpretations → halt immediately
- Backtracking forbidden
- Rework forbidden
- No black boxes
- No hidden mutable state
- State must be fully reconstructible
- Composition must be monoidal (no mutations, only construction)

---

## Phase 3: Collision Detection — Structural & Semantic Overlap Analysis

### Agent Dispatched (No Associated Skill)
**bb80-collision-detector**

### Description
Analyze artifacts produced by 10 independent agents. Identify collisions (structural overlaps, semantic convergences, execution path divergences). Collision is **required signal**, not failure.

### When to Invoke
- **After all 10 agents complete construction** (Phase 2)
- Before convergence phase (Phase 4)
- **MANDATORY** — cannot skip under EPIC 9
- Detects readiness for convergence orchestration

### Agent Role
1. **Structural Overlap Detection**: Identify equivalent structures even if expressed differently (identical schemas, equivalent data structures, isomorphic code patterns). Quantify redundancy percentage.

2. **Semantic Overlap Detection**: Identify different approaches converging on identical conclusions, invariants, or functional outcomes. Analyze whether different paths reach same invariant.

3. **Execution Path Divergence Analysis**: Track where agents diverge in atomic cycle phases. Identify persistent divergence vs. reconvergence at later phases.

4. **Collision Report (Deterministic)**: Output machine-parseable collision map: which artifacts collide, collision level (structural/semantic/path), magnitude (0-100% overlap), reconciliation hints.

### Success Outputs
- Deterministic collision map generated
- All structural overlaps identified and quantified
- All semantic overlaps identified
- Execution path divergences mapped
- Reconciliation hints provided (merge, discard, irreducible)
- Machine-parseable output (not narrative)

### Failure Outputs
- Ambiguity in collision analysis → abort
- Incomplete divergence tracking → abort
- Narrative output instead of deterministic map → abort
- Collision detection skipped → abort (violates EPIC 9)

### Validation Rules & Gates
- **ABORT-ON-AMBIGUITY**: If collision analysis permits multiple interpretations → halt immediately
- Output must be machine-parseable, not narrative
- Collision is required data for convergence (not failure signal)
- All artifact pairs must be analyzed
- Magnitude quantification required (0-100% overlap)
- Partial execution forbidden

---

## Phase 4: Convergence — Selection Pressure & Reconciliation

### Agent Dispatched (No Associated Skill)
**bb80-convergence-orchestrator**

### Description
Execute convergence phase via selection pressure on agent artifacts. Produces final merged artifact. Separate reconciliation process (not original agents). Agent authorship erased.

### When to Invoke
- **After collision detection completes** (Phase 3)
- Convergence is separate process from original agent construction
- **MANDATORY** — cannot skip under EPIC 9
- Passes result to receipt validator (Phase 6)

### Agent Role
1. **Selection Pressure Application**: Evaluate artifacts against: (a) Coverage—extent relative to task scope, (b) Invariant Preservation—maintains all structural invariants, (c) Eliminable Redundancy—overlapping portions mergeable without loss, (d) Construct Minimality—minimal structure to achieve goal.

2. **Dominance Analysis**: Identify dominated artifacts (can discard) and Pareto-optimal ones (non-dominated). Produce dominance relation matrix showing subsumption.

3. **Reconciliation Strategy**: Construct final artifact by: (a) selecting dominant artifacts as bases, (b) merging non-redundant portions, (c) rewriting/simplifying where multiple paths exist, (d) discarding entirely subsumed work. Destructive merge—original boundaries erased.

4. **Convergence Artifact Emission**: Output single merged, refactored artifact. Encode: (a) which agent outputs contributed, (b) dominance justifications, (c) reconciliation decisions (merge/discard/rewrite ratios). Deterministic receipt of convergence.

### Success Outputs
- Single merged, refactored artifact produced
- All 4 selection pressure criteria satisfied
- Dominance relation matrix provided
- Reconciliation decisions documented (merge/discard/rewrite ratios)
- Agent authorship erased
- Deterministic receipt of convergence generated
- Ready for Phase 5 refactoring and Phase 6 closure validation

### Failure Outputs
- Multiple artifacts emitted instead of single convergent artifact → abort
- Selection pressure criteria not satisfied → abort
- Agent authorship preserved in output → abort (violates requirement)
- Deterministic receipt missing → abort
- Narrative justifications instead of deterministic analysis → abort

### Validation Rules & Gates
- **ABORT-ON-AMBIGUITY**: If convergence analysis permits multiple interpretations → halt immediately
- Selection pressure must apply all 4 criteria
- Dominance analysis must be complete
- Reconciliation is destructive—no preservation of intermediate steps
- Final artifact authorship unknown (boundaries erased)
- Output must be single, merged artifact
- Partial execution forbidden

---

## Phase 5: Refactoring & Synthesis

### Description
Merge multiple artifacts, discard subsumed work, rewrite where alternatives exist. Destructive refactoring—only final construction survives.

### When Performed
- Typically executed as part of **bb80-convergence-orchestrator** (Phase 4)
- Can extend into Phase 5 as separate pass if needed
- Must complete before Phase 6 closure validation

### Validation Rules & Gates
- Only final construction survives
- Intermediate steps not preserved
- Original agent boundaries erased
- Refactoring decisions must have dominance justification
- No re-iteration after refactoring (moves to Phase 6)

---

## Phase 6: Closure — Deterministic Receipts & Validation

### Skill Invoked
**bb80-deterministic-receipts**

### Description
Validate final artifact via deterministic receipts (benchmarks, event logs, state hashes)—not narratives. Receipts are proof. Benchmarks replace narratives. Guards replace trust. Determinism replaces consensus.

### When to Invoke
- **After convergence and refactoring complete** (Phases 4-5)
- **Before declaring task complete**
- **MANDATORY** — cannot skip
- Gates all output from previous phases

### Agent Dispatched
**bb80-receipt-validator**

### Agent Role
1. **Deterministic Receipt Generation**: Require concrete proof—benchmark results, state hashes, event logs, guard evaluations. Binary receipts: invariants hold or don't. No subjective interpretation.

2. **Guard-Based Validation**: Validate against deterministic guards—type-checked invariants, benchmark thresholds, correctness proofs. Guards are automated checkpoints. If work passes all guards, correct by definition. If fails any guard → abort immediately.

3. **Reject Narrative Arguments**: Cannot accept narrative justifications ("looks good," "I believe correct"). Require receipts: specific benchmark deltas, state reconstruction proofs, event log analysis. Benchmarks replace narratives.

4. **Proof-Based Certification**: Once valid receipts and guard passes → certification complete. No reiteration. No consensus-building. Determinism replaces consensus. Humans provide constraints; models validate receipts. Reiteration after proof forbidden.

### Success Outputs
- **CLOSED CONSTRUCTION**: All phases completed successfully
- Deterministic receipts generated
- All guards passed
- Benchmark thresholds met
- State reconstruction proofs valid
- Event log analysis complete
- Certification complete → no reiteration
- Task output valid

### Failure Outputs
- Any guard fails → abort immediately
- Benchmarks miss thresholds → abort
- Receipts absent → abort
- Narrative arguments provided → abort
- Reconstruction proofs invalid → abort
- Determinism violated → abort

### Validation Rules & Gates
- **ABORT-ON-AMBIGUITY**: If validation analysis permits multiple interpretations → halt immediately
- No narrative justifications accepted
- No subjective interpretation
- No "probably correct" or "mostly works"
- All guards must pass (not just some)
- Benchmarks must be concrete, measured, reported
- Reiteration after proof forbidden
- Guards are automated checkpoints (not human review)
- Proof-based certification is final

### Closure Conditions (All Required - From CLAUDE.md)
1. ✅ **10 agents launched** (or fewer only if task trivial)
2. ✅ **10 independent artifacts produced**
3. ✅ **Collision analysis performed** (Phase 3)
4. ✅ **Convergence executed** (Phase 4)
5. ✅ **Refactored output emitted** (Phase 5)
6. ✅ **Deterministic receipts validated** (Phase 6)

**Failure at any point → no output**

---

## Summary Table: Skills & Agents by Phase

| Phase | Skill(s) | Agent(s) | Mandatory? | Gate Function |
|-------|----------|----------|-----------|---|
| **Phase 1: Fan-Out** | bb80-specification-closure | bb80-specification-validator | YES | CLOSED or INCOMPLETE |
| **Phase 2a: Planning** | bb80-parallel-agents | bb80-parallel-task-coordinator | YES | Spawn 10 agents |
| **Phase 2b: Implementation** | bb80-invariant-construction | bb80-invariant-validator | YES | Monoidal composition verified |
| **Phase 3: Collision** | (none—direct dispatch) | bb80-collision-detector | YES (EPIC 9) | Deterministic collision map |
| **Phase 4: Convergence** | (none—direct dispatch) | bb80-convergence-orchestrator | YES (EPIC 9) | Single merged artifact |
| **Phase 5: Refactoring** | (part of Phase 4) | (part of Phase 4) | YES | Final construction emitted |
| **Phase 6: Closure** | bb80-deterministic-receipts | bb80-receipt-validator | YES | All guards pass |

---

## Hard Rules: Non-Negotiable

### Every Non-Trivial Task Requires:

**MANDATORY INVOCATIONS:**
- ✅ Phase 1: Invoke `bb80-specification-closure` skill
- ✅ Phase 1: Dispatch `bb80-specification-validator` agent
- ✅ Phase 2a: Invoke `bb80-parallel-agents` skill
- ✅ Phase 2a: Dispatch `bb80-parallel-task-coordinator` agent (spawns 10 agents)
- ✅ Phase 2b: Invoke `bb80-invariant-construction` skill
- ✅ Phase 2b: Dispatch `bb80-invariant-validator` agent
- ✅ Phase 3: Dispatch `bb80-collision-detector` agent
- ✅ Phase 4: Dispatch `bb80-convergence-orchestrator` agent
- ✅ Phase 6: Invoke `bb80-deterministic-receipts` skill
- ✅ Phase 6: Dispatch `bb80-receipt-validator` agent

**FORBIDDEN THOUGHTS:**
- ❌ "Let me read files first" → WRONG. Spawn 10 agents FIRST.
- ❌ "I'll gather context before planning" → WRONG. Agents gather context.
- ❌ "This is small, I can skip agents" → WRONG. EVERY non-trivial task: spawn 10 agents FIRST.
- ❌ "I can skip collision detection" → WRONG. EPIC 9 MANDATORY.
- ❌ "I'll iterate instead of closing specification" → WRONG. Specification closure mandatory.
- ❌ "I can work serially" → WRONG. Parallelism native, not optional.
- ❌ "Collision means something failed" → WRONG. Collision is required signal for convergence.
- ❌ "This doesn't need receipts" → WRONG. Deterministic receipts MANDATORY.

**ABORT CONDITIONS (Across All Phases):**
- Any instruction permits multiple interpretations → ABORT-ON-AMBIGUITY
- Specification incomplete → ABORT and iterate on spec
- Design choices remain → ABORT and iterate on spec
- Invariants incomplete → ABORT during invariant validation
- Backtracking necessary → ABORT during invariant validation
- Serialization without domain justification → ABORT during parallel coordination
- Narrative arguments provided → ABORT during receipt validation
- Any guard fails → ABORT during receipt validation
- Collision detection skipped → ABORT (violates EPIC 9)
- Convergence skipped → ABORT (violates EPIC 9)
- Fewer than 6 phases completed → no output

---

## Document Metadata

- **Framework**: Big Bang 80/20 (Single-Pass Compilation, Specification Closure, Parallel Agents First) + EPIC 9 (Atomic Cognitive Cycle, Collision Detection, Convergence)
- **Atomic Cycle Phases**: 6 (non-negotiable, sequential)
- **Skills**: 4 (bb80-specification-closure, bb80-parallel-agents, bb80-invariant-construction, bb80-deterministic-receipts)
- **Agents**: 6 (bb80-specification-validator, bb80-parallel-task-coordinator, bb80-invariant-validator, bb80-collision-detector, bb80-convergence-orchestrator, bb80-receipt-validator)
- **Closure Requirement**: All 6 phases complete successfully, or no output
- **Key Principle**: Agents first. Specification closure required. Monoidal composition enforced. Collision expected. Convergence mandatory. Deterministic receipts mandatory. No iteration.

---

## References

- **CLAUDE.md**: `/home/user/qlever/CLAUDE.md` - Foundational frame and atomic cognitive cycle
- **Skills Directory**: `/home/user/qlever/.claude/skills/` - Skill definitions
- **Agents Directory**: `/home/user/qlever/.claude/agents/` - Agent definitions
