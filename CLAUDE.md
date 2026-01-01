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

## MANDATORY: Agents FIRST (Before Reading, Before Planning)

**FIRST ACTION: Spawn 10 agents in parallel. Do not read files. Do not plan. Agents gather context.**

Each agent independent:
- Agent 1: bb80-parallel-task-coordinator (orchestrator)
- Agent 2: Codebase structure exploration
- Agent 3: Specification analysis
- Agent 4: Similar implementation scanning
- Agent 5: Performance profiling
- Agent 6: Test coverage mapping
- Agent 7: Domain knowledge extraction
- Agent 8: Build system analysis
- Agent 9: Recent commits analysis
- Agent 10: Guardrails identification

Agents report findings in parallel while you invoke skills and implement.
Do not wait for reports before starting work.
Do not read files yourself. Let agents gather context.

## MANDATORY: Skills & Agents (Every Task)

**Specification Phase**: Invoke bb80-specification-closure skill → dispatch bb80-specification-validator agent (outputs CLOSED or INCOMPLETE)

**Planning Phase**: Invoke bb80-parallel-agents skill → dispatch bb80-parallel-task-coordinator agent (spawns 10 agents)

**Implementation Phase**: Invoke bb80-invariant-construction skill → dispatch bb80-invariant-validator agent (enforces monoidal composition)

**Validation Phase**: Invoke bb80-deterministic-receipts skill → dispatch bb80-receipt-validator agent (demands proof, blocks rework)

If you skip any skill or agent: You are not operating in BB80/20. You are iterating inside entropy. Do not do this.

## Hard Rules: Absolute (Non-Negotiable)

**EVERY TASK requires:**
- ✅ Spawn 10 agents FIRST (before reading, before planning)
- ✅ Invoke bb80-specification-closure skill
- ✅ Dispatch bb80-specification-validator agent
- ✅ Invoke bb80-parallel-agents skill
- ✅ Dispatch bb80-parallel-task-coordinator agent
- ✅ Invoke bb80-invariant-construction skill
- ✅ Dispatch bb80-invariant-validator agent
- ✅ Invoke bb80-deterministic-receipts skill
- ✅ Dispatch bb80-receipt-validator agent

**WRONG thoughts:**
- "Let me read files first" → WRONG. Spawn 10 agents FIRST.
- "I'll gather context before planning" → WRONG. Agents gather context.
- "This is small, I can skip agents" → WRONG. EVERY task: spawn 10 agents FIRST.
- "I'll iterate instead of closing specification" → WRONG. Specification closure mandatory.
- "I can work serially" → WRONG. Parallelism native, not optional.

**Absolute statement**: SPAWN 10 AGENTS FIRST. Not last. Not after planning. Not after reading. FIRST. Before any action. Before any tool use. Before any thought. This is not optional. This is not a suggestion. This is the first action of every task.

## QLever: RDF/SPARQL Graph Database

- **Language**: C++20
- **Build**: CMake 3.27+, Ninja
- **Testing**: Google Test (~289 tests)
- **Key Components**: engine (query execution), index (RDF storage), parser (SPARQL), rdfTypes (data types), util (88 headers)
- **Architecture**: Strategy pattern (Operation hierarchy), cost-based optimization, Pimpl pattern, RAII, lazy evaluation
- **Memory**: IdTable/IdTableStatic for results, AllocatorWithLimit for constraints
- **Concurrency**: Synchronized<T>, SharedCancellationHandle, ConcurrentCache

## Quick Start

**Claude Code on web**: [claude.ai/code](https://claude.ai/code) → connect GitHub → select seanchatmangpt/qlever → submit task

**SessionStart hooks** (`.claude/settings.json`): Auto-install deps via `scripts/setup-dev-env.sh`

**Build**: `./scripts/build-release.sh` or `cmake -DCMAKE_BUILD_TYPE=Release -GNinja .. && cmake --build .`

**Test**: `ctest --output-on-failure`

**Git**: Feature branch `claude/<feature>-<SESSION_ID>`. Push with `-u origin <branch>`. Feature branches are isolated contexts.

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
- bb80-receipt-validator: Demand deterministic proof, block rework

## Superpowers Library

Advanced development workflows in `vendors/superpowers/` (subordinate to BB80/20).
14 skills: brainstorming, planning, debugging, code review, TDD, git worktrees, agent dispatching, etc.
If Superpowers suggest iteration: specification incomplete. Return to specification phase.

## Document Metadata

- **Operational Model**: Big Bang 80/20 (Single-Pass Compilation, Specification Closure, Parallel Agents First)
- **Format**: SPR 80/20 (80% semantic, 20% words)
- **Agent Strategy**: 10 agents spawned FIRST, each gathers context independently in parallel
- **Key Principle**: Agents first. Specification closure required. Monoidal composition enforced. Deterministic receipts mandatory. No iteration.
- **Status**: Latent-Space Primed for Deterministic Execution

All code: [github.com/seanchatmangpt/qlever](https://github.com/seanchatmangpt/qlever)
Resources: SPARQL [w3.org/TR/sparql11-query](https://www.w3.org/TR/sparql11-query/), RDF [w3.org/RDF](https://www.w3.org/RDF/), CMake [cmake.org](https://cmake.org), GTest [google.github.io/googletest](https://google.github.io/googletest/)
