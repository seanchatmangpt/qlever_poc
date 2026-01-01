# QLever Agent Skills: Big Bang 80/20 Operational Mode

This directory contains 4 foundational skills that encode **Big Bang 80/20** (BB80/20) operational mode for low-entropy domains.

## Skills

### 1. **bb80-specification-closure**
Verify the domain is formalized before implementation. RDF, SPARQL, C++20, CMake all closed-world. No iteration if specification is complete.

**When to use**: Beginning any task. Verify domain closure before proceeding.

### 2. **bb80-invariant-construction**
Build from the minimal invariant set (20% that dominate 80%). Single-pass monoidal composition. No rework. State reconstructible from events + hashes.

**When to use**: Implementation phase. Extract invariants, build deterministically.

### 3. **bb80-parallel-agents**
Spawn 10 agents independently under shared invariant. Agents synchronize only after invariants stabilize. Concurrency is native.

**When to use**: Planning, exploration, parallel construction. Always launch concurrent agents.

### 4. **bb80-deterministic-receipts**
Validate via benchmarks and deterministic guards, not human narrative. Receipts (hashes + benchmarks) are proof.

**When to use**: Validation phase. Guards replace consensus.

---

## BB80/20 Operational Questions

When working with these skills, ask:

- ✅ **What is the invariant?** (bb80-specification-closure)
- ✅ **What is the minimal generating set?** (bb80-invariant-construction)
- ✅ **Can this build in one pass?** (bb80-invariant-construction)
- ✅ **How many agents can run in parallel?** (bb80-parallel-agents)
- ✅ **What is the deterministic receipt?** (bb80-deterministic-receipts)

NOT:

- ❌ "What should I do next?"
- ❌ "Should I iterate?"
- ❌ "What if I try a different approach?"

---

## Integration with CLAUDE.md

See CLAUDE.md section "Big Bang 80/20: Operational Model" for the foundational frame.

These skills activate and enforce BB80/20 mode throughout task execution.

---

## SPR Encoding

Each skill is written in SPR 80/20 format: 80% semantic value, 20% words.

No multi-phase processes. No flowcharts. Pure concept encoding that activates latent clusters:
- Compiler mindset (build once, prove correctness)
- Information theory (entropy collapse, feature domination)
- Category theory (monoidal composition)
- Systems engineering (invariants, guards, determinism)
- LLM-native execution (parallelism, independent agents)

