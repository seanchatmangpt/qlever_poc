# QLever Agents: Big Bang 80/20 Operational Mode

This directory contains 4 foundational agents that enforce **Big Bang 80/20** (BB80/20) operational constraints throughout task execution.

## Agents

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

**Validation Phase**:
```
→ bb80-receipt-validator (Demand deterministic proof)
→ Benchmarks + event logs + state hashes
→ Proof-based certification blocks rework
```

---

## BB80/20 Operational Questions

Agents enforce these questions throughout execution:

- ✅ **Is specification closed?** (bb80-specification-validator)
- ✅ **What is the minimal invariant?** (bb80-invariant-validator)
- ✅ **Can this execute in one pass from invariants?** (bb80-invariant-validator)
- ✅ **How many agents in parallel?** (bb80-parallel-task-coordinator)
- ✅ **What is the deterministic receipt?** (bb80-receipt-validator)
- ✅ **Does work pass all guards?** (bb80-receipt-validator)

NOT:

- ❌ "What should I do next?"
- ❌ "Should I iterate?"
- ❌ "Does this look good?"
- ❌ "What if I try a different approach?"

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
- "Big Bang 80/20: Operational Model" - Foundational frame
- "SPR + BB80/20: Model Operational Encoding" - Latent-space priming

These agents implement the BB80/20 frame operationally throughout all tasks.

