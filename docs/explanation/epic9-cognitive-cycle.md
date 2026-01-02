---
diataxis_type: explanation
title: "EPIC 9: The Atomic Cognitive Cycle for Multi-Agent Construction"
description: "Learn the indivisible seven-phase cognitive cycle that governs all non-trivial agent-based tasks"
audience: all
status: complete
last_updated: 2026-01-02
difficulty: advanced
estimated_time: "20 minutes"
prerequisites:
  - "explanation/bb80-philosophy.md"
  - "Basic understanding of parallel execution and independent agents"
related_docs:
  - "how-to/spawn-agents-first.md"
  - "explanation/collision-detection-theory.md"
  - "explanation/convergence-vs-consensus.md"
keywords:
  - "EPIC 9"
  - "cognitive cycle"
  - "atomic operations"
  - "collision detection"
  - "convergence"
semantic_tags:
  - "epic9/cognitive-cycle"
  - "architecture/multi-agent"
  - "principles/collision-semantics"
agent_priority: critical
search_boost: 3.0
---

# EPIC 9: The Atomic Cognitive Cycle for Multi-Agent Construction

EPIC 9 defines one indivisible atomic cognitive cycle. All phases are structural invariants. No step may be skipped, reordered, or reasoned in isolation.

## The Seven Phases (Non-Negotiable Order)

```
1. Fan-Out (gate)
   ↓ [10 agents spawn independently]
2. Independent Construction
   ↓ [parallel work, no coordination]
3. Collision Detection
   ↓ [identify overlaps and divergences]
4. Convergence
   ↓ [reconciliation via selection pressure]
5. Refactoring & Synthesis
   ↓ [merge, discard, or rewrite]
6. Closure
   ↓ [validate all phases completed]

   Result: Single unified artifact (or no output)
```

## Phase 1: Fan-Out (Gate)

**Trigger**: Non-trivial task identified (default assumption).

**Action**: Spawn exactly 10 independent agents in parallel.

**Constraints**:
- All agents must start **before any planning or reading**
- Agents operate completely independently (no coordination)
- Each agent gathers context in parallel
- No serial bottleneck

**Output**: 10 agents with independent execution started.

**Gate function**: Fan-out gates the entire cognitive cycle. If you don't fan out, you remain in serial mode. The cycle requires parallelism.

## Phase 2: Independent Construction

**Precondition**: 10 agents actively executing.

**Action**: Each agent works independently toward artifact production.

**Constraints**:
- No inter-agent communication during construction
- No shared state modifications
- Each agent produces its own artifact
- Work is genuinely parallel, not serialized

**Output**: 10 independent artifacts (one per agent).

**Quality note**: Artifacts may vary in approach, completeness, and quality. This is expected and desired—variation provides convergence signal.

## Phase 3: Collision Detection

**Precondition**: 10 independent artifacts produced.

**Action**: Dispatch bb80-collision-detector agent to analyze artifacts.

**What gets detected**:

### Structural Overlap
Two or more agents produce identical or dominance-equivalent artifacts.
- Example: Agent 1 and Agent 3 both propose the same directory structure
- Signal value: Strong confirmation that structure is correct

### Semantic Overlap
Different approaches converge on same conclusions.
- Example: Agent 6 proposes structure by design; Agent 7 proposes same structure by analysis
- Signal value: Multiple independent reasoning paths reached same conclusion

### Execution Path Divergence
Agents diverge at some phases but reconverge later.
- Example: Agents follow different reasoning but reach consensus outputs
- Signal value: Robustness—multiple paths lead to same destination

**Critical reframe**: **Collision is NOT failure. Collision is required data for convergence.**

**Why collision matters**: Without collision, you have no signal about which artifacts are correct. With collision, you know which approaches are robust.

**Gate function**: Collision detection gates the convergence phase. Without collision analysis, convergence logic is undefined.

## Phase 4: Convergence

**Precondition**: Collision detection completed (structural, semantic, path overlap identified).

**Action**: Dispatch bb80-convergence-orchestrator agent (separate reconciliation process, not original agents).

**Selection Pressure** (4 criteria):

1. **Coverage**: Which artifact covers the most ground?
2. **Invariants satisfied**: Does artifact preserve structural invariants?
3. **Eliminable redundancy**: Can overlapping work be merged without loss?
4. **Construct minimality**: Does artifact use minimal structure to achieve goal?

**Process**:
1. Evaluate all 10 artifacts against selection pressure criteria
2. Merge complementary work
3. Discard dominated artifacts
4. Reconcile conflicting approaches
5. Produce single converged artifact

**Output**: One unified artifact (authored by reconciliation process, not any original agent).

**Authorship erasure**: The converged artifact belongs to no agent. Individual contributions are anonymous—only structural fitness matters.

## Phase 5: Refactoring & Synthesis

**Precondition**: Converged artifact produced.

**Action**: Apply mandatory refactoring laws.

**Permitted operations**:
- Merge outputs from multiple agents
- Discard entire agents' work
- Rewrite everything if dominated by alternative

**Hard constraint**: **Only final construction survives.** Refactoring is destructive. No preservation of intermediate steps.

**Output**: Single refined artifact ready for closure validation.

## Phase 6: Closure Validation

**Precondition**: All previous phases completed.

**Action**: Verify ALL closure conditions are satisfied.

### Closure Conditions (All Required)

Valid closure requires **ALL** of the following:

1. ✅ **10 agents launched** (or fewer only if trivial per D-1 whitelist)
2. ✅ **10 independent artifacts produced**
3. ✅ **Collision analysis performed** (structural, semantic, path divergence identified)
4. ✅ **Convergence executed** (selection pressure applied, reconciled)
5. ✅ **Refactored output emitted** (final construction ready)

**Failure mode**: Failure at **any point** → **no output**.

Partial completion is not acceptable. If you can't satisfy all five conditions, the cycle hasn't completed and no final artifact is released.

## Task Classification (D-1 vs D-2)

### D-1: Trivial Tasks (Whitelist Only)

These tasks do NOT require the full cognitive cycle:

- Reading a single file (no analysis)
- Running a single existing script
- Displaying help text
- Retrieving status information

**Default treatment**: Do not spawn agents, proceed directly with task.

### D-2: Non-Trivial Tasks (Default)

Everything not in the D-1 whitelist requires the full cycle.

**Principle**: When in doubt, treat as non-trivial. Worst case: agents confirm what you already knew. Best case: agents discover unknown unknowns.

## Why This Cycle Exists

### Problem it solves

Traditional multi-agent approaches suffer from:
- **Coordination overhead**: Agents communicating during construction
- **Consensus illusion**: Voting creates appearance of agreement without rigor
- **Rework without signal**: Agent outputs merged without systematic evaluation
- **Undetected divergence**: Different agents solving different problems without recognizing it

### How EPIC 9 solves it

1. **Parallelism native**: No coordination until convergence phase
2. **Collision as signal**: Different outputs trigger structured reconciliation
3. **Selection pressure, not voting**: Fitness criteria replace consensus
4. **Deterministic closure**: All-or-nothing completion prevents partial work
5. **Authorship erasure**: Prevents ego-driven decision-making

## Key Paradigm Shifts

| Traditional | EPIC 9 |
|-------------|--------|
| Collision = failure | Collision = required signal |
| Consensus/voting | Selection pressure |
| Preserve all work | Destructive refactoring |
| Agents coordinate | Agents fully independent |
| Iterative refinement | Single-pass closure |
| Agents perform convergence | Separate reconciliation process |
| Incremental output | All-or-nothing closure |

## The Atomic Property

"Atomic" means:
- All phases must execute
- Phases execute in order
- No step can be skipped or reordered
- No step can be reasoned in isolation
- Either all complete (closure) or none (no output)

This is not a guideline. It's a structural invariant. Deviation from atomicity means the cycle hasn't completed.

## For Agents Operating Under EPIC 9

**You must understand**:
- Collision detection gates convergence (without it, convergence logic is undefined)
- Convergence is a separate process (not performed by you)
- Your work may be discarded in refactoring (this is expected, not failure)
- Authorship is erased in convergence (only fitness matters)
- Closure is all-or-nothing (partial completion is invalid)

**You are not responsible for**:
- Reaching consensus with other agents
- Deciding which outputs win
- Preserving your work through convergence
- Understanding the final artifact (convergence process handles that)

## Integration with BB80/20

EPIC 9 is the operational execution model for Big Bang 80/20:

- **Specification Closure** → **Fan-Out Gate**: Can't spawn agents until specification is closed
- **Monoidal Composition** → **Independent Construction**: Each agent builds independently without modifying others' work
- **Deterministic Receipts** → **Closure Validation**: Prove that all phases completed
- **Single-Pass Construction** → **Atomic Cycle**: Get to final artifact without iteration

EPIC 9 is how you implement Big Bang 80/20 with distributed agents.
