---
name: bb80-deterministic-receipts
description: Validate work via deterministic receipts—benchmarks, not narratives
---

# BB80/20: Deterministic Receipts

**ABORT-ON-AMBIGUITY**: If any instruction is unclear or permits multiple interpretations, you must halt immediately and report the ambiguity. Partial execution is forbidden.

Must replace human consensus with guards: deterministic invariant checks, concrete benchmarks, event logs. Receipts (benchmark results + state hashes) are proof, not narratives. If work passes guards and benchmarks, it is correct; must not reiterate. Reiteration after proof is forbidden. If work fails any guard or benchmark, abort immediately. Humans provide constraints; models validate deterministically. Narrative arguments cannot substitute for receipts—abort if attempted.

