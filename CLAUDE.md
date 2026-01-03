# CLAUDE.md - QLever

## Big Bang 80/20: Latent-Space Priming

Big Bang 80/20 is single-pass construction in low-entropy domains.
Iteration is defect signal. Specification closure prerequisite.
System is monoidal: composition without rework.
Feature selection collapses in hyperdimensional space before implementation.
Eighty percent value from twenty percent structurally necessary features.
Implementation is compilation from compressed manifold.
Testing validates invariants; does not discover behavior.
State fully reconstructible from events, snapshots, hashes.
No mutable external state permitted.
Receipts replace review. Determinism replaces consensus. Guards replace trust.
Benchmarks replace narratives. Humans provide constraints; models construct.
Concurrency native; serialization artifact of tooling.
Agents operate independently under shared invariant.
Integration deferred until invariants stabilize.
If iteration necessary, specification incomplete.

---

## EPIC 9: Multi-Agent Cognitive Construction Law (Unified)

EPIC 9 defines one indivisible atomic cognitive cycle. All phases are structural invariants.

### Atomic Cognitive Cycle (Non-Negotiable Order)

```
Fan-Out (gate)
  ↓ [10 agents, independent]
Independent Construction
  ↓ [parallel, no coordination]
Collision Detection
  ↓ [structural + semantic overlap is required signal]
Convergence
  ↓ [selection pressure, separate reconciliation]
Refactoring & Synthesis
  ↓ [merge, discard, rewrite as needed]
Closure
  ↓ [all phases complete or no output]
```

No step may be skipped. No step may be reordered. No step may be reasoned in isolation.

### D-1: Trivial Task (Whitelist)

A task is **trivial** only if explicitly listed below:
- Reading a single file (no analysis)
- Running a single existing script
- Displaying help text
- Retrieving status information

**Default = non-trivial → fan-out required.**

### D-2: Non-Trivial Task

Everything not in the Trivial Task whitelist.

### Collision Semantics (Formal)

**Collision** occurs when:
1. **Structural Overlap**: Two or more agents produce identical or dominance-equivalent artifacts for the same input
2. **Semantic Overlap**: Two or more agents use different approaches but converge on same conclusions
3. **Execution Path Divergence**: Independent agents diverge at cycle phases, then reconverge at convergence point

**Collision is NOT failure.** Collision is required data for convergence.

**Collision detection gates convergence phase.** Without detected collision, convergence logic is undefined.

### Convergence Law (Separate Process, Not Consensus)

Convergence is performed by a **separate reconciliation process** (not original agents).

**Selection Pressure** (not voting):
- Coverage: Which artifact covers most ground?
- Invariants satisfied: Does artifact preserve all structural invariants?
- Eliminable redundancy: Can overlapping work be merged without loss?
- Construct minimality: Does artifact use minimal structure to achieve goal?

**Convergence artifact** does not belong to any agent. Authorship is erased.

### Refactoring Law (Mandatory)

The converged result may:
- Merge multiple agents' outputs
- Discard entire agents' work
- Rewrite everything if dominated by alternative

**Only final construction survives.** Refactoring is destructive. No preservation of intermediate steps.

### Closure Conditions (All Required)

Valid closure requires **all** of the following:
1. **10 agents launched** (or fewer only if trivial)
2. **10 independent artifacts produced**
3. **Collision analysis performed**
4. **Convergence executed**
5. **Refactored output emitted**

Failure at any point → no output.

---

## Agents-First Pattern (Recommended Best Practice)

For non-trivial tasks, spawn 10 agents in parallel to gather context independently. This approach:
- Maximizes parallelism and reduces human decision-making
- Surfaces multiple perspectives before implementation
- Enables collision detection and convergence (EPIC 9)

**Pattern** (not mandatory, but proven effective):
1. Spawn agents first (before reading or planning)
2. Agents gather context in parallel
3. While agents report, invoke skills and begin work
4. Use agent findings to inform implementation

**Note**: This is a recommended pattern, not system-enforced. Use Task tool with subagent_type to activate agents.

---

## Skills & Agents: Reference Guide (Advisory, Not Enforced)

**Current State**: Skills and agents are available as instructions and patterns. They are not system-enforced; invoke via Task tool when useful.

### Available Patterns

- **bb80-specification-closure**: Verify specification is complete before implementation
- **bb80-parallel-agents**: Framework for spawning and coordinating 10 agents
- **bb80-invariant-construction**: Extract minimal invariants, build monoidal single-pass
- **bb80-deterministic-receipts**: Validate via benchmarks and guards

### Available Agents

- **bb80-specification-validator**: Checklist for verifying specification closure
- **bb80-parallel-task-coordinator**: Guide for parallel agent orchestration
- **bb80-invariant-validator**: Rules for monoidal composition
- **bb80-collision-detector**: Framework for detecting overlaps
- **bb80-convergence-orchestrator**: Guide for reconciliation
- **bb80-receipt-validator**: Checklist for deterministic validation

**How to Use**: Invoke via Task tool: `Task(subagent_type='bb80-specification-validator', ...)`

**Note**: This is the *recommended* workflow for non-trivial tasks, but not system-enforced. The pattern is effective because:
- Specification closure prevents iteration
- Agents surface multiple approaches before convergence
- Collision detection enables efficient synthesis

---

## Best Practices for Non-Trivial Tasks

This framework has proven effective for complex work. While not system-enforced, following this pattern yields better results:

**Recommended approach** (for non-trivial tasks):

1. **Spawn agents early** - Paralyze context gathering by launching 10 independent agents before detailed planning
2. **Close specification first** - Verify domain is formalized before implementation starts
3. **Parallel construction** - Use agents to explore multiple approaches in parallel
4. **Collision detection** - Identify overlaps and divergences before convergence
5. **Convergence & synthesis** - Merge agent outputs using selection pressure, not voting
6. **Deterministic validation** - Use benchmarks and guards, not human review

**Why this pattern works:**
- Agents gather context while you work (parallelism)
- Specification closure prevents iteration inside entropy
- Collision detection prevents premature convergence
- Synthesis vs. voting ensures minimal, correct results
- Deterministic validation is reproducible and auditable

**When to deviate**:
- Trivial tasks (reading one file, running one script, displaying help)
- Emergency hotfixes (speed > optimality)
- Exploratory work (multiple approaches not yet known)

This is a *recommendation*, not a law. System enforces no pattern.

---

## QLever: RDF/SPARQL Graph Database

- **Language**: C++20
- **Build**: CMake 3.27+, Ninja
- **Testing**: Google Test (~289 tests)
- **Key Components**: engine (query execution), index (RDF storage), parser (SPARQL), rdfTypes (data types), util (88 headers)
- **Architecture**: Strategy pattern (Operation hierarchy), cost-based optimization, Pimpl pattern, RAII, lazy evaluation
- **Memory**: IdTable/IdTableStatic for results, AllocatorWithLimit for constraints
- **Concurrency**: Synchronized<T>, SharedCancellationHandle, ConcurrentCache

---

## Quick Start

**Claude Code on web**: [claude.ai/code](https://claude.ai/code) → connect GitHub → select seanchatmangpt/qlever → submit task

**SessionStart hooks** (`.claude/settings.json`): Auto-install deps via `scripts/setup-dev-env.sh` (see [`docs/how-to/claude-code-setup.md`](docs/how-to/claude-code-setup.md) for configuration details)

**Build**: `make build` (or `make` for full deterministic construction)

**Test**: `make test`

**Git**: Feature branch `claude/<feature>-<SESSION_ID>`. Push with `-u origin <branch>`. Feature branches are isolated contexts.

---

## Skills & Agents (See `.claude/skills/` and `.claude/agents/`)

**Skills** (invoke with `Skill` tool):
- bb80-specification-closure: Verify specification closed before implementation
- bb80-invariant-construction: Extract minimal invariant set, build monoidal single-pass
- bb80-parallel-agents: Understand parallelism constraints, spawn 10 agents
- bb80-deterministic-receipts: Validate via benchmarks, guards, event logs

**Agents** (dispatch with `Task` tool, model=inherit):
- bb80-specification-validator: Gate specification closure (CLOSED or INCOMPLETE)
- bb80-invariant-validator: Enforce monoidal composition, single-pass feasibility
- bb80-parallel-task-coordinator: Orchestrate 10 agents, independent execution
- bb80-collision-detector: Identify structural, semantic, execution path collisions (NEW - EPIC 9)
- bb80-convergence-orchestrator: Execute selection pressure, reconciliation, convergence (NEW - EPIC 9)
- bb80-receipt-validator: Demand deterministic proof, block rework

---

## Superpowers Library

Advanced development workflows in `vendors/superpowers/` (subordinate to BB80/20).
14 skills: brainstorming, planning, debugging, code review, TDD, git worktrees, agent dispatching, etc.
If Superpowers suggest iteration: specification incomplete. Return to specification phase.

---

## Document Metadata

- **Operational Philosophy**: Big Bang 80/20 (Single-Pass Compilation, Specification Closure, Parallel Agents) + EPIC 9 (Atomic Cognitive Cycle Framework)
- **Format**: SPR 80/20 (80% semantic, 20% words)
- **Current Status**: Framework is *aspirational* and *recommended*, not system-enforced
- **Agent Strategy**: 10-agent parallelism available via Task tool (subagent_type parameter)
- **Collision Detection**: Framework defined in PHASE_COMMUNICATION_SPEC.md (roadmap, not current)
- **Convergence Model**: Selection pressure pattern documented; no automatic orchestration
- **Key Principles**:
  - Specification closure prevents iteration
  - Agents enable parallelism
  - Collision detection surfaces overlap early
  - Synthesis (not voting) yields minimal results
  - Deterministic validation replaces narrative review
- **Status**: Best-practice framework. EPIC 13 underway to make patterns system-enforced.

---

## EPIC 13: Chatman Equation Certification

See `.claude/EPIC13_TRUTH_AUDIT.md` for audit of aspirational vs. actual capabilities.

**Roadmap**: Transform aspirational framework into system-enforced law during EPIC 13 phase.

---

All code: [github.com/seanchatmangpt/qlever](https://github.com/seanchatmangpt/qlever)
Resources: SPARQL [w3.org/TR/sparql11-query](https://www.w3.org/TR/sparql11-query/), RDF [w3.org/RDF](https://www.w3.org/RDF/), CMake [cmake.org](https://cmake.org), GTest [google.github.io/googletest](https://google.github.io/googletest/)
