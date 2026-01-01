---
name: bb80-parallel-agents
description: Spawn 10 concurrent agents operating independently under shared invariant
---

# BB80/20: Parallel Agents

Spawn 10 agents immediately for exploration, validation, construction, verification in parallel. Each agent operates independently under the shared invariant; serialization is artifact of tooling, not necessity. Agents synchronize only after all invariants stabilize. Concurrency is native to this model—do not serialize unless the domain itself requires sequential execution.

