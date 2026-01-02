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

## MANDATORY: Agents FIRST (Before Reading, Before Planning)

**FIRST ACTION: Spawn 10 agents in parallel. Do not read files. Do not plan. Agents gather context.**

Agents report findings in parallel while you invoke skills and implement.
Do not wait for reports before starting work.
Do not read files yourself. Let agents gather context.

---

## MANDATORY: Skills & Agents (Every Non-Trivial Task)

**Specification Phase**: Invoke bb80-specification-closure skill → dispatch bb80-specification-validator agent (outputs CLOSED or INCOMPLETE)

**Planning Phase**: Invoke bb80-parallel-agents skill → dispatch bb80-parallel-task-coordinator agent (spawns 10 agents)

**Implementation Phase**: Invoke bb80-invariant-construction skill → dispatch bb80-invariant-validator agent (enforces monoidal composition)

**Collision Detection Phase**: Dispatch bb80-collision-detector agent (identifies structural, semantic, path divergences)

**Convergence Phase**: Dispatch bb80-convergence-orchestrator agent (executes selection pressure, reconciliation)

**Validation Phase**: Invoke bb80-deterministic-receipts skill → dispatch bb80-receipt-validator agent (demands proof, blocks rework)

If you skip any skill or agent: You are not operating in BB80/20 + EPIC 9. You are iterating inside entropy. Do not do this.

---

## Hard Rules: Absolute (Non-Negotiable)

**EVERY NON-TRIVIAL TASK requires:**
- ✅ Spawn 10 agents FIRST (before reading, before planning)
- ✅ Invoke bb80-specification-closure skill
- ✅ Dispatch bb80-specification-validator agent
- ✅ Invoke bb80-parallel-agents skill
- ✅ Dispatch bb80-parallel-task-coordinator agent
- ✅ Invoke bb80-invariant-construction skill
- ✅ Dispatch bb80-invariant-validator agent
- ✅ Dispatch bb80-collision-detector agent (NEW - EPIC 9)
- ✅ Dispatch bb80-convergence-orchestrator agent (NEW - EPIC 9)
- ✅ Invoke bb80-deterministic-receipts skill
- ✅ Dispatch bb80-receipt-validator agent

**WRONG thoughts:**
- "Let me read files first" → WRONG. Spawn 10 agents FIRST.
- "I'll gather context before planning" → WRONG. Agents gather context.
- "This is small, I can skip agents" → WRONG. EVERY non-trivial task: spawn 10 agents FIRST.
- "I'll iterate instead of closing specification" → WRONG. Specification closure mandatory.
- "I can work serially" → WRONG. Parallelism native, not optional.
- "Collision means something failed" → WRONG. Collision is required signal for convergence.

**Absolute statement**: SPAWN 10 AGENTS FIRST. Not last. Not after planning. Not after reading. FIRST. Before any action. Before any tool use. Before any thought. This is not optional. This is not a suggestion. This is the first action of every non-trivial task.

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

**Build**: `./scripts/build-release.sh` (or `cmake -DCMAKE_BUILD_TYPE=Release -GNinja .. && cmake --build .` for advanced)

**Test**: `./scripts/run-tests.sh` (or `ctest --output-on-failure` for manual control)

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

- **Operational Model**: Big Bang 80/20 (Single-Pass Compilation, Specification Closure, Parallel Agents First) + EPIC 9 (Atomic Cognitive Cycle, Collision Detection, Convergence)
- **Format**: SPR 80/20 (80% semantic, 20% words)
- **Agent Strategy**: 10 agents spawned FIRST, each gathers context independently in parallel
- **Collision Model**: Structural + semantic + path divergence detected, not aborted
- **Convergence Model**: Separate reconciliation process via selection pressure
- **Key Principle**: Agents first. Specification closure required. Monoidal composition enforced. Collision expected. Convergence mandatory. Deterministic receipts mandatory. No iteration.
- **Status**: EPIC 9 (Unified) Atomic Cognitive Cycle - Latent-Space Primed for Deterministic Execution

---

All code: [github.com/seanchatmangpt/qlever](https://github.com/seanchatmangpt/qlever)
Resources: SPARQL [w3.org/TR/sparql11-query](https://www.w3.org/TR/sparql11-query/), RDF [w3.org/RDF](https://www.w3.org/RDF/), CMake [cmake.org](https://cmake.org), GTest [google.github.io/googletest](https://google.github.io/googletest/)
