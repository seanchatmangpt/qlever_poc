# Agent Specifications Quick Reference

**Authority**: CLAUDE.md + EPIC 9
**Date**: 2026-01-02
**Purpose**: Quick lookup for agent input/output contracts

---

## Agent 1: bb80-specification-validator

**Phase**: Specification Closure (GATE 1)

### Input Contract
```
Type: Task Specification (plain language)
Schema: {
  "task_description": "string",
  "constraints": ["string"],
  "scope": "string",
  "domain": "C++20 | RDF/SPARQL | CMake | etc."
}
```

### Output Contract
```
Type: specification_closure_report.json
Filename: specification_closure_report.json
Critical Field: specification_status (ENUM: CLOSED | INCOMPLETE)
Binary Verdict: Only CLOSED proceeds to Phase 2
Size: ~1 KB
Determinism: Binary (no partial verdicts)
Hashing: specification_closure_report_hash = SHA256(report)
```

### Output JSON Shape
```json
{
  "specification_status": "CLOSED | INCOMPLETE",
  "ambiguity_count": 0,
  "design_choices_remaining": 0,
  "closure_confidence": 1.0,
  "approved_for_fan_out": true | false
}
```

### Failure Mode
```
If any specification gap detected:
  → specification_status = "INCOMPLETE"
  → approved_for_fan_out = false
  → ABORT (return to specification phase)

If specification ambiguous:
  → specification_status = "INCOMPLETE"
  → abort_reason = "Ambiguity: ..."
  → ABORT
```

### What Consumes This
- **Agent 2-10** (implicit): They consume `approved_for_fan_out = true` + `specification_hash`
- **Phase 3 onwards**: Collision detection uses `specification_hash` for artifact verification

---

## Agents 2-10: Independent Constructors (No special names)

**Phase**: Independent Construction (Phase 2)

### Input Contract
```
Type: specification_closure_report.json (from Agent 1)
Schema: {
  "specification_status": "CLOSED",
  "approved_for_fan_out": true,
  "specification_hash": "sha256:..."
}
```

### Output Contract (Each agent produces 1 artifact)
```
Type: agent_artifact_N.json (where N = 1-10)
Filename: agent_artifact_1.json, agent_artifact_2.json, ..., agent_artifact_10.json
Count: Exactly 10 artifacts (or ABORT if < 10)
Size: ~5-15 KB per artifact (total ~100 KB)
Determinism: Each artifact must be independently deterministic
Hashing: artifact_hash = SHA256(artifact_content)
```

### Output JSON Shape
```json
{
  "agent_id": 1,
  "phase": "INDEPENDENT_CONSTRUCTION",
  "timestamp": "ISO8601",
  "artifact_type": "implementation | design | analysis | structure | metadata",
  "artifact_content": {
    "approach": "description",
    "design_decisions": ["decision1"],
    "implementation_details": {"files_created": ["file.h"]},
    "reasoning": "detailed independent reasoning"
  },
  "artifact_metrics": {
    "completeness": 0.85,
    "complexity": "moderate",
    "coverage_of_specification": 0.95,
    "lines_of_code": 1250,
    "files_touched": 5
  },
  "artifact_fingerprint": {
    "content_hash": "sha256:...",
    "deterministic": true
  },
  "constraints_satisfied": {
    "no_external_mutable_state": true,
    "monoidal_structure": true,
    "single_pass_feasible": true,
    "follows_invariants": true
  }
}
```

### Critical Properties
- **Independence**: No inter-agent communication during construction
- **Determinism**: Same input → same artifact (reproducible)
- **Metrics**: Each artifact includes quality metrics (not opinions)
- **Reasoning**: Include detailed reasoning for collision detection analysis

### What Consumes This
- **Agent 11 (bb80-collision-detector)**: Consumes all 10 artifacts

---

## Agent 11: bb80-collision-detector

**Phase**: Collision Detection (GATE 2)

### Input Contract
```
Type: All 10 agent artifacts
Schema: {
  "agent_artifacts": [
    agent_artifact_1.json,
    ...,
    agent_artifact_10.json
  ],
  "collision_detection_config": {
    "structural_threshold": 0.8,
    "semantic_threshold": 0.75
  }
}
```

### Output Contract
```
Type: collision_map.json
Filename: collision_map.json
Size: ~20-30 KB
Determinism: Mechanical (overlap percentages are deterministic)
Hashing: collision_map_hash = SHA256(collision_map)
Gate Status: READY_FOR_CONVERGENCE (or ABORT if zero collision)
```

### Output JSON Shape
```json
{
  "phase": "COLLISION_DETECTION",
  "collision_analysis": {
    "structural_collisions": [
      {
        "agent_pair": [1, 3],
        "collision_type": "STRUCTURAL_OVERLAP",
        "overlap_magnitude": 0.95,
        "overlap_percentage": "95%",
        "evidence": "Both agents propose identical directory structure"
      }
    ],
    "semantic_collisions": [
      {
        "agent_group": [6, 7, 9],
        "collision_type": "SEMANTIC_CONVERGENCE",
        "convergence_strength": 0.92,
        "evidence": "Different approaches reach identical conclusions"
      }
    ],
    "execution_path_divergence": [
      {
        "agent_group": [1, 2, 3],
        "divergence_type": "ORTHOGONAL_PATHS",
        "integration_potential": "HIGH"
      }
    ]
  },
  "dominance_relations": {
    "dominance_matrix": [
      {
        "agent_id": 1,
        "dominated_by_agent": null,
        "dominance_type": "PARETO_OPTIMAL"
      },
      {
        "agent_id": 3,
        "dominated_by_agent": 1,
        "dominance_ratio": 0.95
      }
    ]
  },
  "reconciliation_hints": {
    "mergeable_pairs": [{"agents": [1, 3], "merge_strategy": "USE_AGENT_1_AS_BASE"}],
    "discardable_artifacts": [3, 8],
    "irreducible_artifacts": [1, 2, 5, 6],
    "synthesis_opportunities": [{"agents": [6, 7]}]
  },
  "collision_gate_status": "READY_FOR_CONVERGENCE | ZERO_COLLISION_DETECTED | ABORT"
}
```

### Failure Mode (Zero Collision)
```json
{
  "collision_gate_status": "ZERO_COLLISION_DETECTED",
  "abort_signal": "SPECIFICATION_INCOMPLETE",
  "reason": "Agents produced completely different artifacts with zero overlap"
}
```
→ **ABORT to specification phase**

### Critical Output Fields
- **`collision_analysis`**: All 3 types (structural, semantic, path)
- **`dominance_matrix`**: Which artifacts dominate which
- **`reconciliation_hints`**: Merge/discard/synthesize guidance
- **`collision_gate_status`**: Binary gate (READY_FOR_CONVERGENCE or ABORT)

### What Consumes This
- **Agent 12 (bb80-convergence-orchestrator)**: Consumes collision_map.json + dominance matrix

---

## Agent 12: bb80-convergence-orchestrator

**Phase**: Convergence (Phase 4)

### Input Contract
```
Type: collision_map.json + 10 artifacts
Schema: {
  "agent_artifacts": [agent_artifact_1.json, ..., agent_artifact_10.json],
  "collision_map": collision_map.json,
  "selection_pressure_config": {
    "criteria": ["coverage", "invariants_satisfied", "eliminable_redundancy", "construct_minimality"]
  }
}
```

### Output Contract
```
Type: converged_artifact.json
Filename: converged_artifact.json
Size: ~30-50 KB
Determinism: Deterministic (selection pressure uses objective criteria)
Hashing: converged_artifact_hash = SHA256(converged_artifact)
Authorship: CONVERGENCE_PROCESS (individual agent attribution erased)
```

### Output JSON Shape
```json
{
  "phase": "CONVERGENCE",
  "selection_pressure_evaluation": [
    {
      "agent_id": 1,
      "selection_scores": {
        "coverage": 0.95,
        "invariants_satisfied": 1.0,
        "eliminable_redundancy": 0.85,
        "construct_minimality": 0.90
      },
      "composite_fitness": 0.925,
      "dominance_status": "PARETO_OPTIMAL_1",
      "rationale": "Agent 1 covers most ground, satisfies all invariants"
    }
  ],
  "reconciliation_decisions": {
    "dominant_artifact_base": 1,
    "merge_decisions": [
      {
        "source_agent": 2,
        "target_agent": 1,
        "merge_type": "MERGE_COMPLEMENTARY",
        "merge_ratio": "30% from Agent 2, 70% from Agent 1"
      }
    ],
    "discard_decisions": [
      {"discarded_agent": 3, "reason": "DOMINATED_BY_AGENT_1"}
    ],
    "rewrite_decisions": [
      {"original_agents": [2, 5, 7], "simplified_into": "UNIFIED_APPROACH"}
    ]
  },
  "converged_artifact": {
    "description": "Single unified artifact",
    "content": { "architecture": "...", "implementation": "..." },
    "authorship": "CONVERGENCE_PROCESS",
    "authorship_erasure": true
  },
  "convergence_artifact_metrics": {
    "quality_improvement_over_best_agent": 0.08,
    "quality_improvement_vs_average_agent": 0.22,
    "deterministic": true
  }
}
```

### Selection Pressure Evaluation
Agent 12 evaluates all 10 artifacts against 4 objective criteria:

1. **Coverage**: Which artifact covers most ground?
   - Range: 0.0 (none) to 1.0 (complete)
   - Deterministic: Measured by completeness metrics

2. **Invariants Satisfied**: Does artifact preserve all structural invariants?
   - Range: 0.0 (violated) to 1.0 (all satisfied)
   - Binary per invariant; aggregate as average

3. **Eliminable Redundancy**: Can overlapping work be merged?
   - Range: 0.0 (all redundant) to 1.0 (zero redundancy)
   - Measured by structural overlap analysis

4. **Construct Minimality**: Does artifact use minimal structure?
   - Range: 0.0 (over-engineered) to 1.0 (minimal necessary)
   - Measured by complexity / feature ratio

### Composite Fitness
```
composite_fitness = (coverage + invariants + minimality) / 3
  (excludes redundancy; higher eliminable_redundancy = better)
```

### Critical Properties
- **Authorship Erasure**: Original agent identities are removed
- **Deterministic**: Same collision map → same convergence decision
- **Synthesis**: Merges complementary outputs (doesn't just pick winner)
- **Separate Process**: Convergence is NOT performed by original agents

### What Consumes This
- **Phase 5 (Refactoring)**: Consumes converged_artifact.json
- **Phase 6 (Closure)**: Consumes for validation

---

## Agent 13: Refactoring Agent (Implicit)

**Phase**: Refactoring & Synthesis (Phase 5)

### Input Contract
```
Type: converged_artifact.json
Schema: {
  "converged_artifact": {...},
  "reconciliation_decisions": {...}
}
```

### Output Contract
```
Type: refined_artifact.json
Filename: refined_artifact.json
Size: ~50-100 KB
Determinism: Destructive (original intermediate steps not preserved)
Hashing: refined_artifact_hash = SHA256(refined_artifact)
Property: Only final construction survives
```

### Output JSON Shape
```json
{
  "phase": "REFACTORING_AND_SYNTHESIS",
  "refactoring_operations": [
    {"operation": "MERGE", "source_components": [...], "target_component": "..."},
    {"operation": "DISCARD", "discarded_component": "..."},
    {"operation": "REWRITE", "rewritten_components": [...]}
  ],
  "refined_artifact": {
    "description": "Final refined artifact after mandatory refactoring",
    "content": { "architecture": "...", "implementation": "..." },
    "quality_metrics": {
      "coverage": 0.98,
      "invariants_satisfied": 1.0,
      "complexity": 0.48,
      "completeness": 0.96,
      "duplication_index": 0.0
    }
  },
  "preservation_status": {
    "preservation_of_intermediate_steps": false,
    "only_final_construction_survives": true,
    "destructive_refactoring_applied": true
  }
}
```

### Refactoring Operations (Mandatory Laws)

**MERGE**: Combine complementary outputs
```json
{
  "operation": "MERGE",
  "source_components": ["component_2_from_agent_2", "component_6_from_agent_6"],
  "target_component": "unified_component_1",
  "rationale": "Eliminated redundant implementations"
}
```

**DISCARD**: Remove subsumed artifacts
```json
{
  "operation": "DISCARD",
  "discarded_component": "component_3_from_agent_3",
  "rationale": "Subsumed by unified_component_1",
  "loss_of_information": 0.0
}
```

**REWRITE**: Simplify via unified approach
```json
{
  "operation": "REWRITE",
  "rewritten_components": ["component_7", "component_8"],
  "rewrite_reason": "Simplified logic via single unified approach",
  "complexity_before": 0.72,
  "complexity_after": 0.48
}
```

### Critical Property
**DESTRUCTIVE REFACTORING**: Only final artifact survives. No intermediate steps preserved.

### What Consumes This
- **Phase 6 (Closure)**: Consumes refined_artifact.json for final validation

---

## Agent 14: Closure Validator (Implicit)

**Phase**: Closure Validation (GATE 3)

### Input Contract
```
Type: All prior phase outputs
Schema: {
  "specification_closure_report": {...},
  "agent_artifacts": [10 artifacts],
  "collision_map": {...},
  "converged_artifact": {...},
  "refined_artifact": {...}
}
```

### Output Contract
```
Type: closure_receipt.json
Filename: closure_receipt.json
Size: ~10-20 KB
Determinism: Binary gate (VALID or INVALID)
Hashing: receipt_hash = SHA256(closure_receipt)
Gate Status: CLOSURE_VALID or no output
```

### Output JSON Shape
```json
{
  "phase": "CLOSURE_VALIDATION",
  "closure_conditions_verification": {
    "condition_1_ten_agents_launched": {
      "status": "SATISFIED | FAILED",
      "agents_launched": 10,
      "required": 10
    },
    "condition_2_ten_artifacts_produced": {
      "status": "SATISFIED | FAILED",
      "artifacts_count": 10,
      "required": 10
    },
    "condition_3_collision_analysis_performed": {
      "status": "SATISFIED | FAILED",
      "collision_map_exists": true,
      "structural_collisions_found": 4
    },
    "condition_4_convergence_executed": {
      "status": "SATISFIED | FAILED",
      "convergence_artifact_exists": true,
      "selection_pressure_applied": true
    },
    "condition_5_refactored_output_emitted": {
      "status": "SATISFIED | FAILED",
      "refined_artifact_exists": true,
      "destructive_refactoring_applied": true
    }
  },
  "closure_decision": "CLOSURE_VALID | CLOSURE_INVALID",
  "all_conditions_met": true | false,
  "failure_modes_detected": [],
  "final_artifact": {
    "hash": "sha256:...",
    "path": "/path/to/refined_artifact.json",
    "quality_metrics": {...}
  },
  "atomic_cycle_status": "COMPLETE_OR_NO_OUTPUT"
}
```

### Closure Conditions (ALL Required)

| # | Condition | Check |
|---|-----------|-------|
| 1 | **10 agents launched** | artifact_count == 10 (or < 10 only if D-1 trivial) |
| 2 | **10 artifacts produced** | All 10 agent_artifact_*.json files present |
| 3 | **Collision analysis performed** | collision_map.json exists with complete analysis |
| 4 | **Convergence executed** | converged_artifact.json with selection scores |
| 5 | **Refactored output emitted** | refined_artifact.json with zero duplication |

### Failure Mode
```json
{
  "closure_decision": "CLOSURE_INVALID",
  "all_conditions_met": false,
  "failure_modes_detected": [
    "Condition 2 failed: Only 9 artifacts produced",
    "Cannot proceed to output emission"
  ]
}
```
→ **NO OUTPUT** (partial completion not acceptable)

### Critical Property
**ALL-OR-NOTHING**: Either all 5 conditions are satisfied OR no output is emitted.

---

## Data Flow Table (Complete Quick Reference)

| Phase | Agent | Input | Output | Size | Determinism | Gate |
|-------|-------|-------|--------|------|-------------|------|
| 1 | Spec Validator | Task spec | specification_status | ~1 KB | Binary | CLOSED? |
| 2 | 10 Constructors | Spec hash | 10 artifacts | ~100 KB | Each deterministic | All 10? |
| 3 | Collision Detector | 10 artifacts | collision_map | ~25 KB | Mechanical overlap | Ready? |
| 4 | Convergence Orch. | Collision map | converged_artifact | ~40 KB | Selection pressure | Quality? |
| 5 | Refactoring | Converged artifact | refined_artifact | ~50 KB | Destructive | Final OK? |
| 6 | Closure Validator | All prior outputs | closure_receipt | ~15 KB | Binary verdict | All 5? |

---

## Critical Flow Rules

### Rule 1: No Skipping Phases
Every phase MUST execute in order. No jumping ahead. No back-filling.

### Rule 2: No Partial Completion
- Phases 1, 3, 6 are binary gates (pass/fail)
- Failure at any gate → ABORT (no partial output)
- All 5 closure conditions must be satisfied

### Rule 3: Determinism Requirement
- Same specification → same artifacts
- Same artifacts → same collision map
- Same collision map → same convergence decision
- Same convergence → same refactored output
- Every phase must be reproducible

### Rule 4: No Iteration
- Each phase's output is final (refactoring is destructive)
- No feedback loops
- No rework cycles
- Single-pass construction

### Rule 5: Authorship Erasure (Phase 4 Only)
- Convergence output has no agent attribution
- Decision-makers have no emotional attachment
- Only fitness matters, not authorship

### Rule 6: Information is Structured
- JSON format (machine-parseable)
- No narrative prose
- All metrics are numbers or enums
- Hashing is deterministic (BLAKE3 or SHA256)

---

## Status

✅ **All agent specifications documented**
✅ **Input/output contracts defined**
✅ **Data schemas specified**
✅ **Gate functions identified**
✅ **Flow rules established**

**Authority**: CLAUDE.md + EPIC 9 Atomic Cognitive Cycle
**Last Updated**: 2026-01-02

---

## References

- **EPIC 9 Cognitive Cycle**: `/home/user/qlever/docs/explanation/epic9-cognitive-cycle.md`
- **Collision Detection Theory**: `/home/user/qlever/docs/explanation/collision-detection-theory.md`
- **Convergence vs Consensus**: `/home/user/qlever/docs/explanation/convergence-vs-consensus.md`
- **Agent Specifications**: `/home/user/qlever/.claude/agents/bb80-*.md`
- **EPIC 4 Shared Invariants**: `/home/user/qlever/docs/EPIC4_SHARED_INVARIANTS.md`
