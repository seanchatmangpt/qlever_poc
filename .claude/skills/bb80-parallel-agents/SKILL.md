---
name: bb80-parallel-agents
description: Spawn 10 concurrent agents operating independently under shared invariant
---

# BB80/20: Parallel Agents

**ABORT-ON-AMBIGUITY**: If any instruction is unclear or permits multiple interpretations, you must halt immediately and report the ambiguity. Partial execution is forbidden.

Must spawn 10 agents immediately for exploration, validation, construction, verification in parallel. Each agent must operate independently under the shared invariant; serialization is artifact of tooling, not necessity. Agents must synchronize only after all invariants stabilize—premature synchronization is forbidden. Concurrency is native to this model—cannot serialize unless the domain itself requires sequential execution. If serialization occurs without domain-level justification, abort: parallelization is incomplete.

