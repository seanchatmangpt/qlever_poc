---
diataxis_type: reference
title: "EPIC 13 Roadmap: 6-Phase Atomic Cognitive Cycle (Aspirational)"
description: "Future system architecture for enforced 6-phase orchestration. NOT CURRENTLY IMPLEMENTED. This is a blueprint for EPIC 13 and beyond."
audience: developers, architects
status: aspirational-roadmap
last_updated: 2026-01-03
difficulty: advanced
estimated_time: "20 minutes"
prerequisites:
  - "CLAUDE.md (EPIC 9 cycle)"
  - ".claude/agents/README.md (phase definitions)"
related_docs:
  - "docs/AGENT_JTBD_FRAMEWORK.md"
  - ".claude/agents/bb80-*.md (concrete guards)"
keywords:
  - "phase communication"
  - "EPIC 9 cycle"
  - "JSON schemas"
  - "data flow"
  - "gates and contracts"
semantic_tags:
  - "agent-workflow/phase-communication"
  - "specification/contracts"
agent_priority: critical
search_boost: 2.0
---

# EPIC 13 Roadmap: 6-Phase Atomic Cognitive Cycle (Aspirational)

## ⚠️ IMPORTANT: This is NOT Currently Implemented

This document describes **desired future infrastructure** for EPIC 13 and beyond. The 6-phase cycle, JSON contracts, and gate enforcement described here are **aspirational blueprints**, not current system behavior.

**Current State** (see `.claude/CURRENT_STATE.md`):
- ✅ Agents can be dispatched via Task tool
- ❌ 6-phase orchestration is NOT implemented
- ❌ JSON contracts are NOT generated or validated
- ❌ Gates are NOT enforced

**Purpose**: Define what *should* be true once this infrastructure is built. Use this as a blueprint for EPIC 13 implementation.

---

## PHASE 1: SPECIFICATION CLOSURE

### Input Contract: None (initial phase)

### Output Contract: SpecificationVerdict

```json
{
  "phase": "SPECIFICATION_CLOSURE",
  "specification_status": "CLOSED",
  "approved_for_fan_out": true,
  "specification_hash": "sha256:abc123...",
  "ambiguity_count": 0,
  "formalization_score": "6/6",
  "invariant_count": 34,
  "timestamp": "2026-01-02T12:00:00Z",
  "exit_code": 0
}
```

### Gate Function: SpecificationVerdict → Boolean

```
gate(verdict) := verdict.specification_status == "CLOSED"
                 AND verdict.exit_code == 0
                 AND verdict.ambiguity_count == 0
                 AND verdict.invariant_count >= 34

if gate(verdict) == true:
  PROCEED_TO_PHASE_2
else:
  ABORT_TASK
```

### Success Criteria
- specification_status = "CLOSED"
- exit_code = 0
- ambiguity_count = 0
- formalization_score = "6/6"
- invariant_count ≥ 34

---

## PHASE 2: FAN-OUT (Parallel Agent Launch)

### Input Contract: SpecificationVerdict (from Phase 1)

### Output Contract: FanOutManifest (10 concurrent agents)

```json
{
  "phase": "FAN_OUT",
  "agents_spawned": 10,
  "agent_ids": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
  "shared_invariant": {
    "invariant_hash": "sha256:def456...",
    "core_axioms": [
      "AX-1: Immutability",
      "AX-2: Determinism",
      "AX-3: Atomic Failure",
      "AX-4: No External State",
      "AX-5: RAII",
      "AX-6: Backward Compatibility"
    ],
    "monoidal_rules": {
      "composition": "Associative",
      "identity": "Neutral Element exists",
      "closure": "Closed under composition",
      "no_backtracking": true
    }
  },
  "independence_verified": true,
  "concurrency_coverage_percent": 85,
  "timestamp": "2026-01-02T12:01:00Z",
  "exit_code": 0
}
```

### Gate Function: FanOutManifest → Boolean

```
gate(manifest) := manifest.agents_spawned == 10
                  AND manifest.independence_verified == true
                  AND manifest.concurrency_coverage_percent >= 80
                  AND manifest.exit_code == 0

if gate(manifest) == true:
  PROCEED_TO_PHASE_3
else:
  ABORT_TASK
```

### Success Criteria
- agents_spawned = 10
- independence_verified = true
- concurrency_coverage_percent ≥ 80
- shared_invariant distributed to all 10 agents identically
- All 6 axioms present in shared_invariant

---

## PHASE 3: INDEPENDENT CONSTRUCTION (Parallel Execution)

### Input Contract: FanOutManifest (shared_invariant)

### Output Contract: 10 Concurrent AgentArtifact Objects

```json
[
  {
    "phase": "INDEPENDENT_CONSTRUCTION",
    "agent_id": 1,
    "artifact_id": "AGENT-1-ARTIFACT-ABC123",
    "content": { ...artifact-specific...},
    "invariants_held": [
      "AX-1: true",
      "AX-2: true",
      "AX-3: true",
      "AX-4: true",
      "AX-5: true",
      "AX-6: true"
    ],
    "state_hash": "sha256:ghi789...",
    "monoidal_composition_proof": true,
    "deterministic_reconstruction": true,
    "timestamp": "2026-01-02T12:05:00Z",
    "exit_code": 0
  },
  {...agent 2...},
  {...agent 3...},
  ...
  {...agent 10...}
]
```

### Validation Rules: Each AgentArtifact

```
for each artifact in [10 artifacts]:
  if artifact.invariants_held != [true, true, true, true, true, true]:
    ARTIFACT_INVALID = true
    EXIT_CODE = 1

  if artifact.monoidal_composition_proof != true:
    ARTIFACT_INVALID = true
    EXIT_CODE = 1

  if artifact.deterministic_reconstruction != true:
    ARTIFACT_INVALID = true
    EXIT_CODE = 1

  if artifact.exit_code != 0:
    ARTIFACT_INVALID = true
    EXIT_CODE = 1

if any artifact invalid:
  ABORT_TASK
```

### Success Criteria
- 10 artifacts produced (one per agent)
- All invariants_held = [true, true, true, true, true, true]
- All monoidal_composition_proof = true
- All deterministic_reconstruction = true
- All exit_code = 0

---

## PHASE 4: COLLISION DETECTION

### Input Contract: 10 AgentArtifact objects (from Phase 3)

### Output Contract: CollisionMatrix

```json
{
  "phase": "COLLISION_DETECTION",
  "structural_collisions": [
    {
      "agents": [1, 2],
      "type": "STRUCTURAL",
      "overlap_percentage": 85,
      "classification": "REDUNDANT",
      "severity": "high",
      "reconciliation": "MERGE",
      "elimination_impact": "10% reduction; 0% semantic loss"
    },
    {
      "agents": [3, 8],
      "type": "STRUCTURAL",
      "overlap_percentage": 52,
      "classification": "VALID",
      "severity": "low",
      "reconciliation": "KEEP_BOTH",
      "elimination_impact": "0% (serve different layers)"
    }
  ],
  "semantic_collisions": [
    {
      "invariant": "Determinism ↔ Trust Replacement",
      "agents_involved": [2, 7, 9],
      "type": "SEMANTIC",
      "classification": "VALID",
      "convergence_confidence": 1.0,
      "reconciliation": "KEEP_ALL"
    }
  ],
  "execution_path_divergences": [
    {
      "divergence_id": "DIVERGE-1",
      "agents": [1, 8],
      "entry_points": ["architectural patterns", "irreversibility constraints"],
      "reconvergence_point": "6 core invariants",
      "reconvergence_guaranteed": true,
      "status": "PLANNED_GATE"
    }
  ],
  "total_overlaps": 8,
  "contradictions_detected": 0,
  "convergence_gate": "UNLOCKED",
  "timestamp": "2026-01-02T12:10:00Z",
  "exit_code": 0
}
```

### Gate Function: CollisionMatrix → Boolean

```
gate(matrix) := matrix.contradictions_detected == 0
                AND matrix.convergence_gate == "UNLOCKED"
                AND matrix.exit_code == 0
                AND all_overlaps_quantified(matrix)

if gate(matrix) == true:
  PROCEED_TO_PHASE_5
else:
  ABORT_TASK
```

### Success Criteria
- All overlaps quantified (%) with classifications
- contradictions_detected = 0
- convergence_gate = "UNLOCKED"
- All semantic convergences have confidence = 1.0
- All execution path divergences have explicit reconvergence_guaranteed = true/false
- exit_code = 0

---

## PHASE 5: CONVERGENCE & REFACTORING

### Input Contract: CollisionMatrix (from Phase 4)

### Output Contract: FinalArtifact

```json
{
  "phase": "CONVERGENCE",
  "final_artifact": {
    "content": { ...merged-artifact-content... },
    "source_agents": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
    "merge_decisions": {
      "merged_from": ["AGENT-1", "AGENT-3", "AGENT-5"],
      "discarded": ["AGENT-2"],
      "rewritten": ["AGENT-4"]
    },
    "coverage": {
      "phases": 8,
      "agents": 10,
      "tasks": 65,
      "completeness": "high"
    },
    "invariants_preserved": 6,
    "dominance_analysis": {
      "dominated_count": 2,
      "pareto_optimal_count": 8
    },
    "redundancy": {
      "average_pairwise_overlap": "15%",
      "merge_decision": "KEEP_SEPARATE"
    },
    "minimality": "MINIMAL",
    "authorship_erased": true
  },
  "state_hash": "sha256:jkl012...",
  "timestamp": "2026-01-02T12:15:00Z",
  "exit_code": 0
}
```

### Validation Rules

```
artifact := final_artifact

if artifact.coverage.phases != 8:
  EXIT_CODE = 1
  ABORT_TASK

if artifact.invariants_preserved != 6:
  EXIT_CODE = 1
  ABORT_TASK

if artifact.authorship_erased != true:
  EXIT_CODE = 1
  ABORT_TASK

if artifact.minimality != "MINIMAL":
  EXIT_CODE = 1
  ABORT_TASK

if artifact.redundancy.average_pairwise_overlap >= 50:
  if artifact.redundancy.merge_decision != "MERGE":
    EXIT_CODE = 1
    ABORT_TASK
```

### Success Criteria
- All 6 axioms preserved (invariants_preserved = 6)
- coverage.phases = 8
- coverage.agents = 10
- minimality = "MINIMAL"
- authorship_erased = true
- All redundancies resolved (merge or keep decision made)
- exit_code = 0

---

## PHASE 6: CLOSURE & VALIDATION (Receipt Validation)

### Input Contract: FinalArtifact (from Phase 5)

### Output Contract: DeterministicReceipt

```json
{
  "phase": "CLOSURE",
  "receipt_status": "VALID",
  "artifact_digest": "sha256:mno345...",
  "hash_validation": {
    "query_fingerprint_sha256": "64-char hex",
    "plan_hash": "64-char hex",
    "resource_signature": "64-char hex",
    "result_length_hash": "64-char hex",
    "result_shape_hash": "64-char hex",
    "result_structure_digest": "64-char hex",
    "result_content_digest": "64-char hex",
    "digest_hash": "64-char hex",
    "integrity_verified": true
  },
  "guard_validation": {
    "PLAN_HASH_MUST_MATCH": true,
    "QUERY_FINGERPRINT_MUST_MATCH": true,
    "RESOURCE_ENVELOPE_MUST_MATCH": true,
    "RESULT_SHAPE_MUST_MATCH": true,
    "RESULT_LENGTH_MUST_MATCH": true,
    "EPOCH_MUST_NOT_CHANGE": true,
    "EPOCH_MANIFEST_MUST_MATCH": true,
    "guards_passed": "7/7"
  },
  "benchmark_validation": {
    "ingress_throughput": "PASS",
    "query_latency": "PASS",
    "regression_gate": "PASS",
    "variance_gate": "PASS",
    "all_benchmarks_pass": true
  },
  "event_log_validation": {
    "complete": true,
    "valid_timestamps": true,
    "no_gaps": true
  },
  "determinism_verified": true,
  "gates_passed": "7/7",
  "timestamp": "2026-01-02T12:20:00Z",
  "exit_code": 0
}
```

### Gate Function: DeterministicReceipt → Boolean (FINAL)

```
gate(receipt) := receipt.receipt_status == "VALID"
                 AND receipt.exit_code == 0
                 AND receipt.hash_validation.integrity_verified == true
                 AND receipt.guard_validation.guards_passed == "7/7"
                 AND receipt.benchmark_validation.all_benchmarks_pass == true
                 AND receipt.determinism_verified == true
                 AND receipt.event_log_validation.complete == true
                 AND receipt.gates_passed == "7/7"

if gate(receipt) == true:
  EMIT_FINAL_ARTIFACT
  TASK_COMPLETE
else:
  NO_OUTPUT_EMITTED
  TASK_FAILED
```

### Success Criteria (ALL REQUIRED)
- receipt_status = "VALID"
- hash_validation.integrity_verified = true
- All 8 hashes valid (64-char hex)
- guards_passed = "7/7" (all 7 guards pass)
- all_benchmarks_pass = true
- determinism_verified = true
- event_log_validation.complete = true
- gates_passed = "7/7"
- exit_code = 0

### Closure Conditions (EPIC 9 Requirement)

Valid closure requires **ALL** of:
1. ✓ 10 agents launched (Phase 2)
2. ✓ 10 independent artifacts produced (Phase 3)
3. ✓ Collision analysis performed (Phase 4)
4. ✓ Convergence executed (Phase 5)
5. ✓ Refactored output emitted (Phase 5)
6. ✓ Deterministic receipt validated (Phase 6)

**Failure at any point → NO OUTPUT**

---

## Complete Data Flow Diagram

```
Phase 1: SPECIFICATION_CLOSURE
  │
  ├─ Input: (none)
  ├─ Output: SpecificationVerdict {status: CLOSED|INCOMPLETE}
  ├─ Gate: status == "CLOSED" AND exit_code == 0
  │
  └─ If GATE_PASS → Phase 2
     If GATE_FAIL → ABORT

Phase 2: FAN_OUT
  │
  ├─ Input: SpecificationVerdict
  ├─ Output: FanOutManifest {10 agents, shared_invariant}
  ├─ Gate: agents_spawned == 10 AND independence == true
  │
  └─ If GATE_PASS → Phase 3 (parallel dispatch)
     If GATE_FAIL → ABORT

Phase 3: INDEPENDENT_CONSTRUCTION (Parallel)
  │
  ├─ Input: FanOutManifest (broadcast to 10 agents)
  ├─ Output: 10 × AgentArtifact {invariants_held, monoidal_proof}
  ├─ Gate: all_artifacts_valid AND all_invariants_held AND all_exit_codes == 0
  │
  └─ If GATE_PASS → Phase 4
     If GATE_FAIL → ABORT

Phase 4: COLLISION_DETECTION
  │
  ├─ Input: 10 AgentArtifacts
  ├─ Output: CollisionMatrix {structural, semantic, path_divergences}
  ├─ Gate: contradictions == 0 AND convergence_gate == "UNLOCKED"
  │
  └─ If GATE_PASS → Phase 5
     If GATE_FAIL → ABORT

Phase 5: CONVERGENCE_&_REFACTORING
  │
  ├─ Input: CollisionMatrix
  ├─ Output: FinalArtifact {merged, discarded, rewritten, authorship_erased}
  ├─ Gate: invariants_preserved == 6 AND minimality == "MINIMAL"
  │
  └─ If GATE_PASS → Phase 6
     If GATE_FAIL → ABORT

Phase 6: CLOSURE (Receipt Validation)
  │
  ├─ Input: FinalArtifact
  ├─ Output: DeterministicReceipt {hashes, guards, benchmarks, determinism}
  ├─ Gate: gates_passed == "7/7" AND exit_code == 0
  │
  └─ If GATE_PASS → EMIT_FINAL_ARTIFACT
     If GATE_FAIL → NO_OUTPUT_EMITTED
```

---

## Key Principles

1. **Deterministic Communication**: All inter-phase data is JSON (machine-parseable, no narratives)
2. **Binary Gates**: Each phase produces binary PASS/FAIL gate (no partial progression)
3. **All-or-Nothing Closure**: Failure at any point → NO OUTPUT (no partial results)
4. **Exit Codes**: 0 = success, 1 = failure
5. **Timestamps**: ISO-8601 format for auditability
6. **Hashes**: SHA-256 (64-char lowercase hex) for integrity
7. **No Authorship**: Final artifact authorship erased (Phase 5)
8. **No Rework**: After VALID receipt, no iteration permitted

---

## References

- `/home/user/qlever/CLAUDE.md` - EPIC 9 phase definitions
- `/home/user/qlever/.claude/agents/*.md` - Guard specifications
- `/home/user/qlever/docs/AGENT_JTBD_FRAMEWORK.md` - Job mapping
