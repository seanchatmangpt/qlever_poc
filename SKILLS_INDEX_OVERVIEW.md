# QLever Skills Index: Complete Overview

This document serves as the master index for all QLever skills and agents operating under the Big Bang 80/20 + EPIC 9 operational framework.

---

## Document Collection

This skill-by-phase index consists of 3 comprehensive documents:

### 1. **SKILL_BY_PHASE_INDEX.md** (PRIMARY INDEX)
Maps all 4 skills and 6 agents to each phase of the atomic cognitive cycle. Shows:
- Skill name, description, invocation timing
- Associated agents and their roles
- Success/failure outputs
- Validation rules and gates
- Summary table mapping phases to skills/agents
- Hard rules (non-negotiable requirements)
- Closure conditions

**Use this document to:** Understand which skills/agents execute in each phase and what gates they control.

### 2. **SKILL_AGENT_EXTRACTION.md** (DETAILED REFERENCE)
Complete extraction of all skills and agents with:
- Skill metadata and operational definitions
- When to invoke (timing and scope)
- Agent operational constraints (4 mandatory constraints per agent)
- Success/failure outputs (specific outputs for each)
- Validation rules & gates (detailed, machine-verifiable)
- Output format specifications
- Cross-reference: Skills to Agents

**Use this document to:** Get detailed specifications for implementing or validating each skill/agent.

### 3. **SKILL_PHASE_QUICK_REFERENCE.md** (QUICK LOOKUP)
Visual quick reference with:
- ASCII diagram of atomic cognitive cycle
- Phase-by-phase operation details
- Skill invocation checklist
- Critical abort conditions
- Agent authorship tracking
- Quick decision tree
- Summary table

**Use this document to:** Quickly understand phase flows, abort conditions, and decision points during task execution.

---

## The 4 Skills

All skills operate under Big Bang 80/20 framework. Each has ABORT-ON-AMBIGUITY gate.

### SKILL 1: bb80-specification-closure

**Purpose**: Verify domain is fully formalized before implementation

**When Invoked**: Phase 1 (Fan-Out gate) - beginning of every non-trivial task

**Associated Agent**: bb80-specification-validator

**Key Constraint**: Zero design freedom permitted. ABORT if specification incomplete.

**Output**: Binary verdict (CLOSED or INCOMPLETE)

**Key Validation Rule**: ABORT-ON-AMBIGUITY

**Success Gate**: Domain fully formalized, zero ambiguity, permission to proceed to Phase 2

**Failure Gate**: Specification incomplete, iterate on spec only (not code)

---

### SKILL 2: bb80-parallel-agents

**Purpose**: Spawn 10 concurrent agents operating independently under shared invariant

**When Invoked**: Phase 2a (Planning) - after specification closure, concurrently with Phase 2b

**Associated Agent**: bb80-parallel-task-coordinator (which spawns 10 agents)

**Key Constraint**: Serialization forbidden unless domain-level justification. Must achieve 80% concurrent coverage.

**Output**: 10 independent agents launched in parallel

**Key Validation Rule**: ABORT-ON-AMBIGUITY, serialization without justification aborts

**Success Gate**: 10 agents working in parallel, shared invariant enforced, 80% coverage

**Failure Gate**: Fewer than 10 agents, premature sync, or invariant violation

---

### SKILL 3: bb80-invariant-construction

**Purpose**: Build monoidal structures from minimal invariant set in single pass

**When Invoked**: Phase 2b (Implementation) - during/after Phase 2a agent dispatch

**Associated Agent**: bb80-invariant-validator

**Key Constraint**: Backtracking forbidden. Rework forbidden. No mutable external state.

**Output**: Verified monoidal construction with reconstructible state

**Key Validation Rule**: ABORT-ON-AMBIGUITY, backtracking forbidden, rework forbidden

**Success Gate**: Minimal invariants identified, monoidal composition verified, single-pass confirmed

**Failure Gate**: Backtracking needed, invariants incomplete, or monoidal structure violated

---

### SKILL 4: bb80-deterministic-receipts

**Purpose**: Validate final artifact via deterministic receipts (benchmarks, hashes, logs), not narratives

**When Invoked**: Phase 6 (Closure gate) - final validation before task completion

**Associated Agent**: bb80-receipt-validator

**Key Constraint**: Narrative arguments forbidden. All guards must pass. Reiteration after proof forbidden.

**Output**: Binary verdict (CERTIFIED or REJECTED) + deterministic receipts

**Key Validation Rule**: ABORT-ON-AMBIGUITY, narrative arguments forbidden, reiteration forbidden

**Success Gate**: All deterministic guards pass, all receipts valid, certification complete

**Failure Gate**: Any guard fails or receipts absent

---

## The 6 Agents

All agents operate under BB80/20 + EPIC 9 framework. Each has ABORT-ON-AMBIGUITY gate.

### AGENT 1: bb80-specification-validator

**Phase**: 1 (Fan-Out)

**Dispatched By**: bb80-specification-closure skill

**Role**: Verify specification closure before implementation

**Constraints**:
1. Specification Closure Verification (RDF, SPARQL, SHACL, C++20, CMake all formalized)
2. Completeness Analysis (identify gaps, ambiguities, open questions)
3. Iteration Point Detection (mark design choice locations)
4. Closure Report (binary CLOSED or INCOMPLETE verdict)

**Output Format**: Binary verdict + identified gaps (if INCOMPLETE)

**Key Gate**: Blocks progression to Phase 2 until CLOSED

---

### AGENT 2: bb80-parallel-task-coordinator

**Phase**: 2a (Planning & Agent Dispatch)

**Dispatched By**: bb80-parallel-agents skill

**Role**: Spawn and manage 10 concurrent agents operating independently

**Constraints**:
1. Independent Agent Dispatch (spawn 10 immediately, all in parallel)
2. Shared Invariant Enforcement (all agents under same constraint, no communication except state reporting)
3. Synchronization Point Detection (integrate only after invariant stability)
4. Concurrency Coverage (ensure 80% concurrent execution)

**Output Format**: 10 agent instances spawned, shared invariant defined, coverage metric reported

**Key Gate**: Ensures true parallelism (no premature synchronization)

---

### AGENT 3: bb80-invariant-validator

**Phase**: 2b (Implementation)

**Dispatched By**: bb80-invariant-construction skill

**Role**: Validate monoidal composition and single-pass feasibility

**Constraints**:
1. Invariant Extraction (identify minimal 20% dominating 80%)
2. Monoidal Composition Check (verify building from invariants outward only)
3. Single-Pass Feasibility (confirm one-pass execution from invariants)
4. Deterministic Reconstruction (validate state fully reconstructible)

**Output Format**: Identified invariant set, dominance quantification, monoidal structure map, feasibility proof

**Key Gate**: Blocks progression to Phase 3 until construction validated

---

### AGENT 4: bb80-collision-detector

**Phase**: 3 (Collision Detection)

**Dispatched By**: Direct dispatch (no associated skill) - EPIC 9 requirement

**Role**: Analyze 10 agent artifacts for structural/semantic/path collisions

**Constraints**:
1. Structural Overlap Detection (identify equivalent structures, quantify redundancy %)
2. Semantic Overlap Detection (identify convergent conclusions via different paths)
3. Execution Path Divergence Analysis (track divergences and reconvergences)
4. Collision Report (machine-parseable map: which artifacts collide, level, magnitude, hints)

**Output Format**: Machine-parseable collision map (not narrative)

**Key Gate**: Detects convergence readiness, identifies reconciliation opportunities

**Critical**: Collision is required signal (not failure!)

---

### AGENT 5: bb80-convergence-orchestrator

**Phase**: 4 (Convergence) & 5 (Refactoring)

**Dispatched By**: Direct dispatch (no associated skill) - EPIC 9 requirement

**Role**: Execute convergence via selection pressure, reconcile 10 artifacts into 1

**Constraints**:
1. Selection Pressure Application (evaluate all artifacts by 4 criteria: coverage, invariant preservation, eliminable redundancy, construct minimality)
2. Dominance Analysis (identify dominated vs. Pareto-optimal artifacts)
3. Reconciliation Strategy (merge, discard, rewrite to synthesize final artifact)
4. Convergence Artifact Emission (produce single merged artifact, encode reconciliation decisions)

**Output Format**: Single merged artifact, dominance matrix, reconciliation receipt

**Key Gate**: Produces final converged artifact (authorship erased)

**Critical**: Refactoring is destructive — only final construction survives

---

### AGENT 6: bb80-receipt-validator

**Phase**: 6 (Closure)

**Dispatched By**: bb80-deterministic-receipts skill

**Role**: Validate final artifact via deterministic receipts, block reiteration

**Constraints**:
1. Deterministic Receipt Generation (benchmarks, hashes, logs, guards - all concrete)
2. Guard-Based Validation (all guards must pass, no exceptions)
3. Reject Narrative Arguments (require proof, no subjective justifications)
4. Proof-Based Certification (once valid, certification final, no reiteration)

**Output Format**: Receipt validation report, guard matrix, certification verdict (CERTIFIED or REJECTED)

**Key Gate**: Blocks task completion until all guards pass, prevents reiteration

**Critical**: Benchmarks replace narratives. Guards replace trust. Determinism replaces consensus.

---

## Phase-by-Phase Execution

### Phase 1: Specification Closure (Fan-Out Gate)
- **Skill**: bb80-specification-closure
- **Agent**: bb80-specification-validator
- **Gates**: CLOSED verdict required
- **Abort**: Specification incomplete, multiple valid approaches, design choices remaining
- **Duration**: Once per task, completion mandatory

### Phase 2a: Parallel Agent Orchestration (Planning)
- **Skill**: bb80-parallel-agents
- **Agent**: bb80-parallel-task-coordinator
- **Gates**: 10 agents spawned, shared invariant defined, 80% coverage
- **Abort**: Fewer agents, premature sync, invariant violation, coverage <80%
- **Duration**: Agents run in parallel throughout Phase 2b

### Phase 2b: Invariant-Driven Construction (Implementation)
- **Skill**: bb80-invariant-construction
- **Agent**: bb80-invariant-validator
- **Gates**: Invariants identified, monoidal composition verified, single-pass confirmed
- **Abort**: Backtracking needed, invariants incomplete, monoidal structure violated
- **Duration**: During/after Phase 2a, concurrent with agent execution

### Phase 3: Collision Detection (Analysis)
- **Agent**: bb80-collision-detector (direct dispatch, no skill)
- **Gates**: Collision map generated, all artifacts analyzed
- **Abort**: Ambiguous analysis, incomplete tracking, narrative output
- **Duration**: After Phase 2 agents complete
- **Key**: Collision is required signal (not failure!)

### Phase 4: Convergence (Reconciliation)
- **Agent**: bb80-convergence-orchestrator (direct dispatch, no skill)
- **Gates**: Single merged artifact produced, dominance justified
- **Abort**: Multiple artifacts, criteria unsatisfied, authorship preserved
- **Duration**: After Phase 3 collision detection
- **Key**: Refactoring destructive—only final construction survives, authorship erased

### Phase 5: Refactoring & Synthesis (Optimization)
- **Part Of**: bb80-convergence-orchestrator (Phase 4)
- **Gates**: Final artifact ready, all refactoring decisions documented
- **Abort**: Intermediate steps preserved, boundaries retained
- **Duration**: During or immediately after Phase 4

### Phase 6: Deterministic Receipts (Closure Gate)
- **Skill**: bb80-deterministic-receipts
- **Agent**: bb80-receipt-validator
- **Gates**: All guards pass, all receipts valid
- **Abort**: Any guard fails, benchmarks not met, receipts absent, narratives provided
- **Duration**: Once per task, completion mandatory
- **Key**: If all gates pass → no reiteration permitted

---

## Critical Rules: All Mandatory

### Hard Rules (Non-Negotiable)

1. **Agents First**: Spawn 10 agents FIRST (before reading, before planning)
2. **All Phases**: All 6 phases execute sequentially—none skipped, none reordered
3. **No Iteration**: Specification closure required, no iterative implementation
4. **Deterministic**: Deterministic receipts mandatory, no narrative arguments
5. **Parallelism**: Concurrency native, serialization requires domain justification
6. **Single-Pass**: Monoidal composition enforced, no backtracking, no rework
7. **Collision Signals**: Collision is required data (not failure), convergence mandatory
8. **Proof-Based**: Benchmarks replace narratives, guards replace trust
9. **Closure Requirement**: All 6 phases complete or NO OUTPUT

### Abort Conditions (Across All Phases)

- ABORT-ON-AMBIGUITY: Any instruction permits multiple interpretations
- Specification incomplete or ambiguous (Phase 1)
- Design choices remaining (Phase 1)
- Fewer than 10 agents (non-trivial, Phase 2a)
- Premature synchronization or serialization without justification (Phase 2a)
- Backtracking or rework needed (Phase 2b)
- Invariants incomplete or monoidal structure violated (Phase 2b)
- Collision analysis incomplete or narrative (Phase 3)
- Multiple artifacts or criteria unsatisfied (Phase 4)
- Any guard fails or receipts absent (Phase 6)
- Narrative arguments provided (Phases 1-6)
- Reiteration attempted after proof (Phase 6)

---

## Skill-by-Phase Summary Table

| Phase | Skill | Agent | Mandate | Input | Output | Gate |
|-------|-------|-------|---------|-------|--------|------|
| 1 | bb80-specification-closure | bb80-specification-validator | Mandatory | Domain spec | CLOSED/INCOMPLETE | ✓ Gates Phase 2 |
| 2a | bb80-parallel-agents | bb80-parallel-task-coordinator | Mandatory | Spec CLOSED | 10 agents spawned | ✓ 80% coverage |
| 2b | bb80-invariant-construction | bb80-invariant-validator | Mandatory | Agent work | Invariant-validated | ✓ Single-pass ok |
| 3 | (none) | bb80-collision-detector | Mandatory (EPIC 9) | 10 artifacts | Collision map | ✓ Map generated |
| 4 | (none) | bb80-convergence-orchestrator | Mandatory (EPIC 9) | Collision map | 1 merged artifact | ✓ Converged |
| 5 | (part of 4) | (part of 4) | Mandatory | Phase 4 artifact | Refactored artifact | ✓ Finalized |
| 6 | bb80-deterministic-receipts | bb80-receipt-validator | Mandatory | Final artifact | CERTIFIED/REJECTED | ✓ All guards |

---

## Authorship & Ownership Tracking

```
PHASES 1-3: Agent Authorship Preserved
  ✓ Phase 1 verdict owned by bb80-specification-validator
  ✓ Phase 2 artifacts owned by their respective 10 agents
  ✓ Phase 3 collision map owned by bb80-collision-detector

PHASE 4-5: Authorship ERASED
  ✗ Phase 4-5 merged artifact has NO OWNER
  ✗ Original agent boundaries removed
  ✗ Only final construction survives
  ✓ What IS retained: dominance justifications, reconciliation decisions, merge/discard/rewrite ratios

PHASE 6: Certified as Ownerless
  ✓ Final artifact certified without authorship
  ✓ Receipt validator confirms proof, not ownership
  ✓ Determinism replaces consensus (authorship irrelevant)
```

---

## When to Use Each Document

| Need | Document |
|------|----------|
| Understand phase-by-phase flow | SKILL_BY_PHASE_INDEX.md |
| Implement or validate a specific skill/agent | SKILL_AGENT_EXTRACTION.md |
| Quick lookup or decision-making | SKILL_PHASE_QUICK_REFERENCE.md |
| Master index and overview | SKILLS_INDEX_OVERVIEW.md (this document) |

---

## Key Principles: Big Bang 80/20 + EPIC 9

### Big Bang 80/20
- Single-pass construction in low-entropy domains
- Specification closure prerequisite
- Monoidal composition (no rework)
- 20% features dominate 80%
- Testing validates invariants; doesn't discover behavior
- Concurrency native; serialization artifact of tooling
- Determinism replaces consensus
- Benchmarks replace narratives

### EPIC 9: Atomic Cognitive Cycle
- 6 phases (non-negotiable, sequential)
- 10 agents (independent parallelism)
- Collision detection (required signal, not failure)
- Convergence via selection pressure (separate process)
- Refactoring destructive (only final survives)
- Closure via deterministic receipts (proof-based)

---

## File Locations

All skill and agent definitions are in the QLever repository:

**Skills Directory**: `/home/user/qlever/.claude/skills/`
- `README.md` - Overview of 4 skills
- `bb80-specification-closure/SKILL.md` - Skill definition
- `bb80-parallel-agents/SKILL.md` - Skill definition
- `bb80-invariant-construction/SKILL.md` - Skill definition
- `bb80-deterministic-receipts/SKILL.md` - Skill definition

**Agents Directory**: `/home/user/qlever/.claude/agents/`
- `README.md` - Overview of 6 agents
- `bb80-specification-validator.md` - Agent definition
- `bb80-parallel-task-coordinator.md` - Agent definition
- `bb80-invariant-validator.md` - Agent definition
- `bb80-collision-detector.md` - Agent definition
- `bb80-convergence-orchestrator.md` - Agent definition
- `bb80-receipt-validator.md` - Agent definition

**Index Documents (Generated)**:
- `/home/user/qlever/SKILL_BY_PHASE_INDEX.md` - Primary phase-by-phase index
- `/home/user/qlever/SKILL_AGENT_EXTRACTION.md` - Detailed skill/agent extraction
- `/home/user/qlever/SKILL_PHASE_QUICK_REFERENCE.md` - Quick visual reference
- `/home/user/qlever/SKILLS_INDEX_OVERVIEW.md` - This document

**Foundation**: `/home/user/qlever/CLAUDE.md` - BB80/20 + EPIC 9 operational frame

---

## Document Metadata

- **Framework**: Big Bang 80/20 (Single-Pass Compilation, Specification Closure, Parallel Agents First) + EPIC 9 (Atomic Cognitive Cycle, Collision Detection, Convergence)
- **Skill Count**: 4 foundational skills
- **Agent Count**: 6 agents (4 BB80/20 core + 2 EPIC 9 new)
- **Phases**: 6 mandatory, sequential, non-reorderable
- **Requirements**: All phases complete or no output
- **Status**: EPIC 9 (Unified) Atomic Cognitive Cycle fully mapped
- **Generated**: 2026-01-02
- **Version**: 1.0 (Complete Index)

---

## Quick Start: Using This Index

1. **First Time?** Read SKILL_PHASE_QUICK_REFERENCE.md (visual overview)
2. **Implementing a Phase?** Find your phase in SKILL_BY_PHASE_INDEX.md
3. **Validating a Skill/Agent?** Look up details in SKILL_AGENT_EXTRACTION.md
4. **Need Full Reference?** This document (SKILLS_INDEX_OVERVIEW.md)
5. **Source Definitions?** See file locations above (in `.claude/` directories)

---

**All skills and agents operate under identical framework:**
- ABORT-ON-AMBIGUITY gate (all must pass)
- Specification closure prerequisite
- Deterministic output required
- Proof-based validation mandatory
- No iteration after proof
- Authorship erasure (Phases 4-5)
- Parallel execution (where possible)
- Single-pass construction (where specified)
