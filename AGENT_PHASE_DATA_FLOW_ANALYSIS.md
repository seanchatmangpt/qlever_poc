# Agent Execution Phases: Data Flow Analysis

**Date**: 2026-01-02
**Authority**: BB80/20 + EPIC 9 Specification
**Status**: Complete Analysis

## Overview

This document defines the information flows between the 6 agent execution phases in the EPIC 9 atomic cognitive cycle. Each phase is a structural invariant; information must flow deterministically between phases.

---

## Phase Structure

```
Phase 1: Specification Closure (GATE)
   ↓ [OUTPUTS: specification_closure_report.json]
Phase 2: Parallel Agent Construction (Fan-Out + Independent Work)
   ↓ [OUTPUTS: 10 independent agent_artifact_*.json files]
Phase 3: Collision Detection
   ↓ [CONSUMES: all 10 artifacts, OUTPUTS: collision_map.json]
Phase 4: Convergence via Selection Pressure
   ↓ [CONSUMES: collision_map.json + 10 artifacts, OUTPUTS: converged_artifact.json]
Phase 5: Refactoring & Synthesis
   ↓ [CONSUMES: converged_artifact.json, OUTPUTS: refined_artifact.json]
Phase 6: Closure Validation
   ↓ [CONSUMES: all phase outputs, OUTPUTS: closure_receipt.json]
```

---

## Phase 1: Specification Closure (Gate)

**Agent**: `bb80-specification-validator`

**Purpose**: Verify that the domain is fully formalized with zero degrees of freedom for design choice.

### Input

```json
{
  "task_specification": {
    "description": "Plain-language task description",
    "constraints": ["constraint 1", "constraint 2"],
    "scope": "Clearly defined scope or INCOMPLETE",
    "domain": "C++20 | RDF/SPARQL | CMake | etc."
  }
}
```

### Output: `specification_closure_report.json`

```json
{
  "phase": "SPECIFICATION_CLOSURE",
  "timestamp": "2026-01-02T12:00:00Z",
  "specification_status": "CLOSED | INCOMPLETE",
  "closure_verdict": {
    "is_closed": true,
    "ambiguity_count": 0,
    "design_choices_remaining": 0,
    "iteration_required": false
  },
  "findings": {
    "gaps": [],
    "ambiguities": [],
    "open_questions": [],
    "design_degrees_of_freedom": []
  },
  "closure_confidence": 1.0,
  "approved_for_fan_out": true,
  "abort_reason": null,
  "determinism_proof": {
    "domain_is_formalized": true,
    "formalization_completeness": 1.0,
    "single_valid_approach": true
  }
}
```

**Critical Output Field**: `specification_status`
- Value: **CLOSED** → Proceed to Phase 2 (Fan-Out)
- Value: **INCOMPLETE** → ABORT, return to specification iteration

**Gate Function**: Specification closure gates the entire cognitive cycle. Without closure, parallel agents cannot spawn deterministically.

---

## Phase 2: Parallel Agent Construction (Fan-Out + Independent Work)

**Agent(s)**: 10 independent agents (no special naming; they report back artifacts)

**Purpose**: Spawn 10 agents in parallel, each producing independent artifact without inter-agent coordination.

### Input (from Phase 1)

```json
{
  "specification_closure_report": {
    "specification_status": "CLOSED",
    "approved_for_fan_out": true
  }
}
```

### Output: 10 Independent Artifacts (`agent_artifact_1.json` through `agent_artifact_10.json`)

Each artifact has this schema:

```json
{
  "phase": "INDEPENDENT_CONSTRUCTION",
  "agent_id": 1,
  "timestamp": "2026-01-02T12:00:00Z",
  "task_context": {
    "specification_hash": "sha256:...",
    "task_id": "unique-task-id"
  },
  "artifact_type": "implementation | design | analysis | structure | metadata",
  "artifact_content": {
    "approach": "Description of the independent approach",
    "design_decisions": [
      "Decision 1",
      "Decision 2"
    ],
    "implementation_details": {
      "files_created": ["file1.h", "file2.cpp"],
      "code_structure": "Hierarchical structure description",
      "key_invariants": ["Invariant 1", "Invariant 2"]
    },
    "reasoning": "Detailed independent reasoning"
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

**Critical Properties of Phase 2 Output**:
- **10 artifacts minimum** (or fewer only if task is trivial per D-1 whitelist)
- **Artifacts are independent** (no inter-agent coordination)
- **Each artifact includes reasoning** (for collision detection to analyze)
- **Artifacts include metrics** (for convergence selection pressure)
- **No authorship bias in this phase** (agents produce work, not opinions)

**Gate Transition**: All 10 artifacts must be produced before Phase 3 begins. Incomplete fan-out → ABORT.

---

## Phase 3: Collision Detection

**Agent**: `bb80-collision-detector`

**Purpose**: Identify structural, semantic, and execution path collisions across all 10 artifacts.

### Input

```json
{
  "agent_artifacts": [
    // All 10 artifacts from Phase 2
    {
      "agent_id": 1,
      "artifact_content": { ... }
    },
    // ... through agent 10
  ],
  "collision_detection_config": {
    "structural_threshold": 0.8,  // 80% overlap = structural collision
    "semantic_threshold": 0.75,   // 75% semantic agreement = collision
    "divergence_analysis": true
  }
}
```

### Output: `collision_map.json`

```json
{
  "phase": "COLLISION_DETECTION",
  "timestamp": "2026-01-02T12:30:00Z",
  "specification_hash": "sha256:...",
  "collision_analysis": {
    "total_artifacts": 10,
    "structural_collisions": [
      {
        "agent_pair": [1, 3],
        "collision_type": "STRUCTURAL_OVERLAP",
        "overlap_magnitude": 0.95,
        "overlap_percentage": "95%",
        "evidence": "Both agents propose identical directory structure",
        "artifacts_involved": ["agent_artifact_1.json", "agent_artifact_3.json"]
      },
      {
        "agent_pair": [2, 5],
        "collision_type": "STRUCTURAL_OVERLAP",
        "overlap_magnitude": 0.87,
        "overlap_percentage": "87%",
        "evidence": "Similar metadata format with minor field ordering differences",
        "artifacts_involved": ["agent_artifact_2.json", "agent_artifact_5.json"]
      }
    ],
    "semantic_collisions": [
      {
        "agent_group": [6, 7, 9],
        "collision_type": "SEMANTIC_CONVERGENCE",
        "convergence_strength": 0.92,
        "evidence": "Different approaches reach identical conclusions about system design",
        "reasoning_paths_analyzed": 3
      }
    ],
    "execution_path_divergence": [
      {
        "agent_group": [1, 2, 3],
        "divergence_phase": "INDEPENDENT_CONSTRUCTION",
        "reconvergence_point": "CONVERGENCE_PHASE",
        "divergence_type": "ORTHOGONAL_PATHS",
        "integration_potential": "HIGH",
        "evidence": "Agents explored different problem aspects but outputs are complementary"
      }
    ]
  },
  "collision_signal": {
    "high_collision_count": true,
    "collision_ratio": 0.78,
    "signal_interpretation": "Specification is clear; agents consistently interpret constraints"
  },
  "dominance_relations": {
    "dominance_matrix": [
      {
        "dominant_agent": 1,
        "dominated_by_agent": null,
        "dominance_type": "PARETO_OPTIMAL",
        "reason": "Covers most ground, satisfies all invariants, minimal structure"
      },
      {
        "agent_id": 3,
        "dominated_by_agent": 1,
        "dominance_ratio": 0.95,
        "reason": "Agent 1's output subsumes Agent 3 with 95% structural equivalence"
      }
    ]
  },
  "reconciliation_hints": {
    "mergeable_pairs": [
      {
        "agents": [1, 3],
        "merge_strategy": "USE_AGENT_1_AS_BASE",
        "reason": "Agents 1 and 3 are structurally identical; Agent 1 has better documentation"
      }
    ],
    "discardable_artifacts": [3, 8],
    "irreducible_artifacts": [1, 2, 5, 6],
    "synthesis_opportunities": [
      {
        "agents": [6, 7],
        "synthesis_benefit": "Merge Agent 6 directory structure with Agent 7 metadata format",
        "integrated_coverage": 0.98
      }
    ]
  },
  "collision_gate_status": "READY_FOR_CONVERGENCE",
  "collision_fingerprint": {
    "collision_map_hash": "sha256:...",
    "deterministic": true
  }
}
```

**Critical Output Fields**:
- **`collision_analysis`**: All three collision types (structural, semantic, path divergence)
- **`dominance_relations`**: Which artifacts dominate/are dominated by others
- **`reconciliation_hints`**: Guidance for convergence phase (merge, discard, synthesize)
- **`collision_gate_status`**: Must be "READY_FOR_CONVERGENCE" to proceed

**Gate Function**: Collision detection gates convergence. Without collision data, convergence logic is undefined (you have no evidence for selection pressure).

**Special Case - Zero Collision**:
```json
{
  "collision_gate_status": "ZERO_COLLISION_DETECTED",
  "abort_signal": "SPECIFICATION_INCOMPLETE",
  "reason": "Agents produced completely different artifacts with zero overlap. Specification ambiguity likely."
}
```
→ **ABORT to specification phase**

---

## Phase 4: Convergence via Selection Pressure

**Agent**: `bb80-convergence-orchestrator`

**Purpose**: Execute selection pressure (4 objective criteria) and reconciliation to produce single converged artifact.

### Input

```json
{
  "agent_artifacts": [
    // All 10 artifacts from Phase 2
  ],
  "collision_map": {
    // Full collision_map.json from Phase 3
  },
  "selection_pressure_config": {
    "criteria": [
      "coverage",
      "invariants_satisfied",
      "eliminable_redundancy",
      "construct_minimality"
    ]
  }
}
```

### Output: `converged_artifact.json`

```json
{
  "phase": "CONVERGENCE",
  "timestamp": "2026-01-02T13:00:00Z",
  "specification_hash": "sha256:...",
  "convergence_process": {
    "input_artifact_count": 10,
    "collision_map_hash": "sha256:...",
    "reconciliation_process": "SELECTION_PRESSURE + SYNTHESIS"
  },
  "selection_pressure_evaluation": {
    "artifacts_evaluated": [
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
        "rationale": "Agent 1 covers most ground, satisfies all invariants, minimal overcomplexity"
      },
      {
        "agent_id": 2,
        "selection_scores": {
          "coverage": 0.78,
          "invariants_satisfied": 1.0,
          "eliminable_redundancy": 0.92,
          "construct_minimality": 0.88
        },
        "composite_fitness": 0.895,
        "dominance_status": "PARETO_OPTIMAL_2",
        "rationale": "Agent 2 has high redundancy elimination; can be merged with Agent 1"
      },
      {
        "agent_id": 3,
        "selection_scores": {
          "coverage": 0.93,
          "invariants_satisfied": 1.0,
          "eliminable_redundancy": 0.78,
          "construct_minimality": 0.91
        },
        "composite_fitness": 0.905,
        "dominance_status": "DOMINATED_BY_AGENT_1",
        "rationale": "Agent 3 is subsumed by Agent 1 (95% structural equivalence)"
      }
    ]
  },
  "reconciliation_decisions": {
    "dominant_artifact_base": 1,
    "merge_decisions": [
      {
        "source_agent": 2,
        "target_agent": 1,
        "merge_type": "MERGE_COMPLEMENTARY",
        "merge_ratio": "30% from Agent 2, 70% from Agent 1",
        "reason": "Agent 2's metadata format complements Agent 1's structure"
      },
      {
        "source_agent": 6,
        "target_agent": 1,
        "merge_type": "MERGE_ORTHOGONAL",
        "merge_ratio": "10% new content from Agent 6",
        "reason": "Agent 6 explored optimization aspects not covered by Agent 1"
      }
    ],
    "discard_decisions": [
      {
        "discarded_agent": 3,
        "reason": "DOMINATED_BY_AGENT_1",
        "loss_of_information": 0.0
      },
      {
        "discarded_agent": 8,
        "reason": "SUBSUMED_BY_MERGED_OUTPUT",
        "loss_of_information": 0.0
      }
    ],
    "rewrite_decisions": [
      {
        "original_agents": [2, 5, 7],
        "rewrite_reason": "SIMPLIFY_REDUNDANT_APPROACHES",
        "simplified_into": "UNIFIED_APPROACH_IN_FINAL_ARTIFACT",
        "complexity_reduction": "40%"
      }
    ]
  },
  "converged_artifact": {
    "description": "Single unified artifact produced by convergence process",
    "content": {
      "architecture": "Merged architecture from Agents 1, 2, 6",
      "implementation": "Unified implementation synthesized from all sources",
      "quality_metrics": {
        "coverage": 0.98,
        "invariants_satisfied": 1.0,
        "complexity": "moderate",
        "completeness": 0.96
      }
    },
    "authorship": "CONVERGENCE_PROCESS",
    "authorship_erasure": true,
    "original_agent_attribution": [
      "Agent 1: Base structure (70%)",
      "Agent 2: Metadata enrichment (15%)",
      "Agent 6: Optimization insights (10%)",
      "Agents 3,4,5,7,8,9,10: Evaluated and merged into above (5%)"
    ]
  },
  "convergence_artifact_metrics": {
    "quality_improvement_over_best_agent": 0.08,
    "quality_improvement_vs_average_agent": 0.22,
    "deterministic": true,
    "reproducible": true
  },
  "convergence_fingerprint": {
    "converged_artifact_hash": "sha256:...",
    "convergence_process_hash": "sha256:...",
    "deterministic": true
  }
}
```

**Critical Output Fields**:
- **`selection_pressure_evaluation`**: Objective fitness scores for each artifact
- **`reconciliation_decisions`**: Which artifacts to merge, discard, rewrite
- **`converged_artifact`**: The single unified output
- **`authorship_erasure`**: True (individual agent contributions anonymized in final form)

**Key Property**: Convergence is performed by a **separate process** (not the original 10 agents). Decision-makers have no emotional attachment to original artifacts.

---

## Phase 5: Refactoring & Synthesis

**Agent(s)**: Implicit in convergence output refinement

**Purpose**: Apply mandatory refactoring laws (merge, discard, rewrite).

### Input

```json
{
  "converged_artifact": {
    // Full converged_artifact.json from Phase 4
  }
}
```

### Output: `refined_artifact.json`

```json
{
  "phase": "REFACTORING_AND_SYNTHESIS",
  "timestamp": "2026-01-02T13:30:00Z",
  "specification_hash": "sha256:...",
  "refactoring_operations": [
    {
      "operation": "MERGE",
      "source_components": ["component_2_from_agent_2", "component_6_from_agent_6"],
      "target_component": "unified_component_1",
      "rationale": "Eliminated redundant implementations"
    },
    {
      "operation": "DISCARD",
      "discarded_component": "component_3_from_agent_3",
      "rationale": "Subsumed by unified_component_1"
    },
    {
      "operation": "REWRITE",
      "rewritten_components": ["component_7", "component_8"],
      "rewrite_reason": "Simplified logic via single unified approach",
      "complexity_before": 0.72,
      "complexity_after": 0.48
    }
  ],
  "refined_artifact": {
    "description": "Final refined artifact after mandatory refactoring",
    "content": {
      // Fully refined, synthesized content
    },
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
  },
  "refinement_fingerprint": {
    "refined_artifact_hash": "sha256:...",
    "deterministic": true
  }
}
```

**Critical Property**: **DESTRUCTIVE REFACTORING**
- Only the final artifact survives
- Intermediate steps are not preserved
- All rework is incorporated; no traces of iteration remain

---

## Phase 6: Closure Validation

**Agent(s)**: Implicit receipt validator

**Purpose**: Verify all phases completed; emit final deterministic receipt.

### Input

```json
{
  "specification_closure_report": { ... },
  "agent_artifacts": [ ... 10 total ... ],
  "collision_map": { ... },
  "converged_artifact": { ... },
  "refined_artifact": { ... }
}
```

### Output: `closure_receipt.json`

```json
{
  "phase": "CLOSURE_VALIDATION",
  "timestamp": "2026-01-02T14:00:00Z",
  "closure_conditions_verification": {
    "condition_1_ten_agents_launched": {
      "status": "SATISFIED",
      "agents_launched": 10,
      "required": 10,
      "evidence": "artifact_count == 10"
    },
    "condition_2_ten_artifacts_produced": {
      "status": "SATISFIED",
      "artifacts_count": 10,
      "required": 10,
      "evidence": "All 10 agent_artifact_*.json files present"
    },
    "condition_3_collision_analysis_performed": {
      "status": "SATISFIED",
      "collision_map_exists": true,
      "structural_collisions_found": 4,
      "semantic_collisions_found": 3,
      "path_divergence_found": 2,
      "evidence": "collision_map.json with complete analysis"
    },
    "condition_4_convergence_executed": {
      "status": "SATISFIED",
      "convergence_artifact_exists": true,
      "selection_pressure_applied": true,
      "dominance_analysis_completed": true,
      "reconciliation_process_completed": true,
      "evidence": "converged_artifact.json with selection scores"
    },
    "condition_5_refactored_output_emitted": {
      "status": "SATISFIED",
      "refined_artifact_exists": true,
      "destructive_refactoring_applied": true,
      "only_final_survives": true,
      "evidence": "refined_artifact.json with zero duplication"
    }
  },
  "closure_decision": "CLOSURE_VALID",
  "all_conditions_met": true,
  "failure_modes_detected": [],
  "final_artifact": {
    "hash": "sha256:...",
    "path": "/path/to/refined_artifact.json",
    "quality_metrics": {
      "coverage": 0.98,
      "invariants_satisfied": 1.0,
      "deterministic": true,
      "reproducible": true
    }
  },
  "receipt_metadata": {
    "specification_hash": "sha256:...",
    "collision_map_hash": "sha256:...",
    "converged_artifact_hash": "sha256:...",
    "refined_artifact_hash": "sha256:...",
    "receipt_hash": "sha256:..."
  },
  "atomic_cycle_status": "COMPLETE_OR_NO_OUTPUT",
  "output_ready_for_implementation": true
}
```

**Closure Conditions (ALL Required)**:
1. ✅ **10 agents launched** (or fewer only if trivial per D-1 whitelist)
2. ✅ **10 independent artifacts produced**
3. ✅ **Collision analysis performed** (structural, semantic, path divergence identified)
4. ✅ **Convergence executed** (selection pressure applied, reconciled)
5. ✅ **Refactored output emitted** (final construction ready)

**Failure Mode**: Failure at **any point** → **`closure_decision: "CLOSURE_INVALID"` → NO OUTPUT**

Partial completion is NOT acceptable. Either all 5 conditions are satisfied or the cycle fails entirely.

---

## Critical Data Flows Summary

### Phase 1 → Phase 2

**Must Flow**:
- `specification_status: "CLOSED"` (binary gate: CLOSED or INCOMPLETE)
- `approved_for_fan_out: true`
- `specification_hash` (for artifact fingerprinting in Phase 2)

**Format**: JSON
**Size**: ~1 KB
**Determinism**: Binary decision

### Phase 2 → Phase 3

**Must Flow**:
- All 10 `agent_artifact_*.json` files
- Each artifact's `artifact_content`, `artifact_metrics`, `artifact_fingerprint`
- Each artifact's reasoning and design decisions

**Format**: 10 × JSON files (each ~5-15 KB)
**Size**: ~100 KB total
**Determinism**: Each artifact is deterministically reproducible

### Phase 3 → Phase 4

**Must Flow**:
- `collision_map.json` with all collision analysis
- `dominance_relations` matrix (which artifacts dominate which)
- `reconciliation_hints` (merge/discard/synthesize guidance)
- Original 10 artifacts (needed for detailed convergence evaluation)

**Format**: JSON
**Size**: ~20-30 KB
**Determinism**: Collision detection is mechanical (overlap percentages, not subjective)

### Phase 4 → Phase 5

**Must Flow**:
- `converged_artifact.json` with selection pressure scores
- `reconciliation_decisions` (specific merge/discard/rewrite decisions)
- Original 10 artifacts (for refactoring reference)

**Format**: JSON
**Size**: ~30-50 KB
**Determinism**: Selection pressure is deterministic (objective criteria)

### Phase 5 → Phase 6

**Must Flow**:
- `refined_artifact.json` (final output)
- `refactoring_operations` (proof of refactoring)
- All prior phase outputs (for closure validation)

**Format**: JSON
**Size**: ~50-100 KB total
**Determinism**: All operations are deterministic

### Phase 6 Output

**Must Emit**:
- `closure_receipt.json` verifying all 5 conditions
- Hash of final artifact (`refined_artifact_hash`)
- Hash of entire convergence process (`receipt_hash`)

**Format**: JSON
**Size**: ~10-20 KB
**Determinism**: Binary verdict (CLOSURE_VALID or no output)

---

## Minimal Information Flow (Critical Path)

**Absolute Minimum** for each phase to proceed:

| Phase | Consumes | Produces |
|-------|----------|----------|
| 1: Specification | Task specification | `specification_status` (CLOSED\|INCOMPLETE) |
| 2: Construction | `specification_status: CLOSED` + spec hash | 10 artifact hashes + metrics |
| 3: Collision Detection | 10 artifacts + hashes | `collision_map.json` + dominance matrix |
| 4: Convergence | collision map + dominance matrix | `converged_artifact` + selection scores |
| 5: Refactoring | converged artifact | `refined_artifact` + operation log |
| 6: Closure | All prior outputs | `closure_receipt.json` (VALID\|INVALID) |

**Information Must Be**:
- **Deterministic**: Same inputs → same outputs (always)
- **Portable**: No pointers, thread IDs, or wall-clock times
- **Hashable**: Every artifact has SHA256 fingerprint
- **Structured**: JSON format, not prose narratives
- **Machine-parseable**: Consumed by next phase programmatically

---

## JSON Schema Summary

### Artifact Schema (Phase 2)

```json
{
  "phase": "string",
  "agent_id": "number",
  "timestamp": "ISO8601",
  "artifact_type": "enum: implementation|design|analysis|structure|metadata",
  "artifact_content": { "any": "object" },
  "artifact_metrics": {
    "completeness": "number 0-1",
    "complexity": "string",
    "coverage_of_specification": "number 0-1",
    "lines_of_code": "number",
    "files_touched": "number"
  },
  "artifact_fingerprint": {
    "content_hash": "sha256",
    "deterministic": "boolean"
  },
  "constraints_satisfied": {
    "no_external_mutable_state": "boolean",
    "monoidal_structure": "boolean",
    "single_pass_feasible": "boolean",
    "follows_invariants": "boolean"
  }
}
```

### Collision Map Schema (Phase 3)

```json
{
  "phase": "COLLISION_DETECTION",
  "collision_analysis": {
    "structural_collisions": [
      {
        "agent_pair": "[number, number]",
        "collision_type": "STRUCTURAL_OVERLAP",
        "overlap_magnitude": "number 0-1",
        "evidence": "string"
      }
    ],
    "semantic_collisions": [...],
    "execution_path_divergence": [...]
  },
  "dominance_relations": {
    "dominance_matrix": [
      {
        "agent_id": "number",
        "dominated_by_agent": "number | null",
        "dominance_type": "enum: PARETO_OPTIMAL | DOMINATED_BY_AGENT_X"
      }
    ]
  },
  "reconciliation_hints": {
    "mergeable_pairs": [...],
    "discardable_artifacts": "number[]",
    "irreducible_artifacts": "number[]"
  }
}
```

### Convergence Artifact Schema (Phase 4)

```json
{
  "phase": "CONVERGENCE",
  "selection_pressure_evaluation": [
    {
      "agent_id": "number",
      "selection_scores": {
        "coverage": "number",
        "invariants_satisfied": "number",
        "eliminable_redundancy": "number",
        "construct_minimality": "number"
      },
      "composite_fitness": "number",
      "dominance_status": "string"
    }
  ],
  "reconciliation_decisions": {
    "dominant_artifact_base": "number",
    "merge_decisions": [...],
    "discard_decisions": [...],
    "rewrite_decisions": [...]
  },
  "converged_artifact": {
    "content": { "any": "object" },
    "authorship": "CONVERGENCE_PROCESS",
    "original_agent_attribution": "string"
  }
}
```

### Closure Receipt Schema (Phase 6)

```json
{
  "phase": "CLOSURE_VALIDATION",
  "closure_conditions_verification": {
    "condition_1_ten_agents_launched": { "status": "SATISFIED | FAILED" },
    "condition_2_ten_artifacts_produced": { "status": "SATISFIED | FAILED" },
    "condition_3_collision_analysis_performed": { "status": "SATISFIED | FAILED" },
    "condition_4_convergence_executed": { "status": "SATISFIED | FAILED" },
    "condition_5_refactored_output_emitted": { "status": "SATISFIED | FAILED" }
  },
  "closure_decision": "CLOSURE_VALID | CLOSURE_INVALID",
  "all_conditions_met": "boolean",
  "final_artifact": {
    "hash": "sha256",
    "path": "string"
  }
}
```

---

## Gate Functions (Non-Negotiable)

### Gate 1: Specification Closure Gates Fan-Out
```
If specification_status != "CLOSED":
  → ABORT (return to specification phase)
Else:
  → Proceed to Phase 2 (Fan-Out)
```

### Gate 2: All 10 Artifacts Required
```
If agent_artifact_count < 10:
  → ABORT (fan-out incomplete)
Else:
  → Proceed to Phase 3 (Collision Detection)
```

### Gate 3: Collision Analysis Gates Convergence
```
If collision_map is empty (zero overlap):
  → ABORT (specification likely incomplete)
Else:
  → Proceed to Phase 4 (Convergence)
```

### Gate 4: All Closure Conditions Required
```
For each condition in [1, 2, 3, 4, 5]:
  If condition_status != "SATISFIED":
    → closure_decision = "CLOSURE_INVALID"
    → NO OUTPUT
If all conditions satisfied:
  → closure_decision = "CLOSURE_VALID"
  → Output: refined_artifact.json
```

---

## Example Data Flow (Complete Cycle)

### Phase 1 Output (10 bytes minimum)
```json
{"specification_status": "CLOSED"}
```
↓

### Phase 2 Output (10 artifacts, ~100 KB)
```json
[agent_artifact_1.json, agent_artifact_2.json, ..., agent_artifact_10.json]
```
↓

### Phase 3 Output (~25 KB)
```json
{
  "collision_analysis": {
    "structural_collisions": 4,
    "semantic_collisions": 3,
    "execution_path_divergence": 2
  },
  "dominance_matrix": [...]
}
```
↓

### Phase 4 Output (~40 KB)
```json
{
  "selection_pressure_evaluation": [...],
  "reconciliation_decisions": {...},
  "converged_artifact": {...}
}
```
↓

### Phase 5 Output (~50 KB)
```json
{
  "refactoring_operations": [...],
  "refined_artifact": {...},
  "preservation_status": {"only_final_construction_survives": true}
}
```
↓

### Phase 6 Output (~15 KB)
```json
{
  "closure_decision": "CLOSURE_VALID",
  "all_conditions_met": true,
  "final_artifact": {"hash": "sha256:..."}
}
```

---

## Key Invariants

1. **Binary Gates**: Phases 1, 3, 6 have binary go/no-go decisions
2. **No Iteration**: Each phase produces final output (refactoring is destructive)
3. **Determinism**: All hashing and evaluation is deterministic (reproducible)
4. **Authorship Erasure**: Phase 4 output has no agent attribution
5. **All-or-Nothing Closure**: Phase 6 succeeds only if all 5 conditions met
6. **Structured Format**: All information is JSON (machine-parseable)
7. **No Prose**: No narrative descriptions required; all data is metrics and hashes

---

## Status

✅ **Analysis Complete**
✅ **All phase dependencies documented**
✅ **Data formats specified (JSON)**
✅ **Minimal information flow identified**
✅ **Gate functions defined**
✅ **Ready for implementation**

**Authority**: BB80/20 + EPIC 9 Specification Closure
**Last Updated**: 2026-01-02
