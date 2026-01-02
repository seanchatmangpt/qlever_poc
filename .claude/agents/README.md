# QLever Agents: Big Bang 80/20 + EPIC 9 Operational Mode

This directory contains **6 foundational agents** that enforce **Big Bang 80/20** (BB80/20) operational constraints and **EPIC 9** (Unified Atomic Cognitive Cycle) throughout task execution.

## Agents

### BB80/20 Core Agents (4)

### 1. **bb80-specification-validator**
Verify specification closure before implementation begins. Domain must be fully formalized (RDF, SPARQL, C++20, CMake) with zero design freedom. If multiple valid approaches exist, specification is incomplete—iterate on spec, not code.

**When to use**: Start of any task. Block iteration inside incomplete specifications.

### 2. **bb80-invariant-validator**
Validate that implementations maintain the minimal invariant set (20% dominating 80%). Verify monoidal composition—building from invariants outward, not mutation. State fully reconstructible from events + hashes.

**When to use**: During implementation. Ensure single-pass feasibility. Reject backtracking.

### 3. **bb80-parallel-task-coordinator**
Coordinate 10 concurrent agents operating independently under shared invariant. Maximize parallelism—80% of work surface covered concurrently. Agents synchronize only after invariants stabilize.

**When to use**: Planning phase and throughout execution. Always launch 10 agents in parallel.

### 4. **bb80-receipt-validator**
Validate work via deterministic receipts (benchmarks, event logs, state hashes)—not narratives. Guards are automated checkpoints. If work passes all guards, it is correct. Benchmarks replace narratives. Determinism replaces consensus.

**When to use**: After implementation and integration. Proof-based certification blocks iteration.

---

### EPIC 9 Agents (2) — Collision & Convergence

### 5. **bb80-collision-detector** (NEW - EPIC 9)
Identify structural, semantic, and execution path collisions across artifacts produced by 10 independent agents. Collision is required signal (not failure). Outputs deterministic collision map: which artifacts collide, at what level, overlap magnitude (0-100%), and reconciliation hints.

**When to use**: After all 10 agents complete construction. Before convergence phase. Detects readiness for convergence orchestration.

### 6. **bb80-convergence-orchestrator** (NEW - EPIC 9)
Execute convergence phase via selection pressure on agent artifacts. Evaluates coverage, invariant preservation, redundancy elimination, construct minimality. Produces final merged artifact by selecting dominant artifacts, merging non-redundant portions, rewriting where needed, discarding subsumed work. Final artifact authorship erased.

**When to use**: After collision detection completes. Synthesizes final artifact from 10 independent outputs. Passes result to receipt validator.

---

## Agent Invocation Pattern

**Specification Phase**:
```
→ bb80-specification-validator (Verify closure)
→ If incomplete, iterate on specification only
```

**Planning Phase**:
```
→ bb80-parallel-task-coordinator (Spawn 10 agents)
→ All agents work independently under shared invariant
```

**Implementation Phase**:
```
→ bb80-invariant-validator (Check monoidal composition)
→ Verify single-pass feasibility
→ Block backtracking
```

**Collision Detection Phase** (EPIC 9):
```
→ bb80-collision-detector (Analyze 10 agent outputs)
→ Identify structural, semantic, execution path overlaps
→ Output deterministic collision map
```

**Convergence Phase** (EPIC 9):
```
→ bb80-convergence-orchestrator (Execute selection pressure)
→ Merge, discard, rewrite to synthesize final artifact
→ Erase agent authorship boundaries
```

**Validation Phase**:
```
→ bb80-receipt-validator (Demand deterministic proof)
→ Benchmarks + event logs + state hashes
→ Proof-based certification blocks rework
```

---

## BB80/20 + EPIC 9 Operational Questions

Agents enforce these questions throughout execution:

**BB80/20 Core**:
- ✅ **Is specification closed?** (bb80-specification-validator)
- ✅ **What is the minimal invariant?** (bb80-invariant-validator)
- ✅ **Can this execute in one pass from invariants?** (bb80-invariant-validator)
- ✅ **How many agents in parallel?** (bb80-parallel-task-coordinator)

**EPIC 9 Atomic Cycle**:
- ✅ **What collisions exist across artifacts?** (bb80-collision-detector)
- ✅ **Which artifacts dominate?** (bb80-convergence-orchestrator)
- ✅ **Can overlapping work be merged?** (bb80-convergence-orchestrator)
- ✅ **What is the minimal final construction?** (bb80-convergence-orchestrator)

**Validation**:
- ✅ **What is the deterministic receipt?** (bb80-receipt-validator)
- ✅ **Does work pass all guards?** (bb80-receipt-validator)

NOT (These are forbidden):

- ❌ "What should I do next?"
- ❌ "Should I iterate?"
- ❌ "Does this look good?"
- ❌ "What if I try a different approach?"
- ❌ "Collision means something failed"
- ❌ "Can I skip collision detection?"

---

## SPR Encoding

Each agent is written in SPR 80/20 format (4-section structure, ~4 sentences per section).

No flowcharts. No multi-phase processes. Pure operational constraint encoding.

Agents activate latent clusters:
- Compiler mindset (build once, prove correctness)
- Information theory (entropy collapse, invariant domination)
- Category theory (monoidal composition)
- Systems engineering (guards, determinism, event sourcing)
- LLM-native execution (independent parallelism)

---

## Integration with Skills

See `.claude/skills/` for complementary skill-level operational guidance:
- **bb80-specification-closure** (skill) - What specification closure means
- **bb80-invariant-construction** (skill) - How to build monoidal structures
- **bb80-parallel-agents** (skill) - Parallelism mechanics
- **bb80-deterministic-receipts** (skill) - Receipt validation techniques

Agents (here) enforce constraints at task execution level.
Skills provide conceptual/operational guidance.

---

## Integration with CLAUDE.md

See CLAUDE.md:
- "Big Bang 80/20: Latent-Space Priming" - Foundational frame
- "EPIC 9: Multi-Agent Cognitive Construction Law (Unified)" - Atomic cognitive cycle
- "Collision Semantics" - Formal definition of collision detection
- "Convergence Law" - Separate reconciliation process
- "Refactoring Law" - Merge, discard, rewrite mandate
- "Closure Conditions" - All 6 phases required or no output

These 6 agents implement the BB80/20 + EPIC 9 frame operationally throughout all non-trivial tasks.

