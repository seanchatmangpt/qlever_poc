# Agent Phase Flow: Visual Diagrams & Examples

**Date**: 2026-01-02
**Purpose**: Visual representation of data flows between agent phases

---

## Phase Flow Diagram (ASCII)

```
┌─────────────────────────────────────────────────────────────────┐
│                     TASK SPECIFICATION                          │
│                    (Plain Language)                             │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│  PHASE 1: SPECIFICATION CLOSURE (Agent: bb80-specification-validator)
│                                                                   │
│  Input:  Task description, constraints, scope                    │
│  Process: Verify full formalization, zero ambiguity              │
│  Output: specification_status = CLOSED | INCOMPLETE              │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │ OUTPUT: specification_closure_report.json                    │ │
│  │ {                                                            │ │
│  │   "specification_status": "CLOSED",       ← BINARY GATE     │ │
│  │   "approved_for_fan_out": true,                             │ │
│  │   "specification_hash": "sha256:..."                        │ │
│  │ }                                                            │ │
│  └─────────────────────────────────────────────────────────────┘ │
└────────────────────────────┬────────────────────────────────────┘
                             │
                    ┌────────┴────────┐
                    │                 │
        CLOSED (✓)  │ specification_  │  INCOMPLETE (✗)
                    │ status = CLOSED │
                    │                 │
                    ▼                 ▼
              ┌─────────┐        ┌──────────┐
              │ PROCEED │        │ ABORT    │
              │ Phase 2 │        │ Iterate  │
              └────┬────┘        │ Spec     │
                   │             └──────────┘
                   ▼
┌──────────────────────────────────────────────────────────────────┐
│  PHASE 2: PARALLEL AGENT CONSTRUCTION                            │
│           (10 Independent Agents - Agents 2-11)                 │
│                                                                   │
│  Input:  specification_hash + CLOSED gate signal                 │
│  Process: 10 agents work in parallel (NO COORDINATION)           │
│  Output: 10 independent artifacts (agent_artifact_1..10.json)    │
│                                                                   │
│  AGENT 1        AGENT 2        AGENT 3  ...   AGENT 10           │
│  ┌────────┐    ┌────────┐    ┌────────┐      ┌────────┐         │
│  │Artifact│    │Artifact│    │Artifact│      │Artifact│         │
│  │1.json  │    │2.json  │    │3.json  │      │10.json │         │
│  │        │    │        │    │        │      │        │         │
│  │{       │    │{       │    │{       │      │{       │         │
│  │ content│    │ content│    │ content│      │ content│         │
│  │ metrics│    │ metrics│    │ metrics│      │ metrics│         │
│  │ hash   │    │ hash   │    │ hash   │      │ hash   │         │
│  │}       │    │}       │    │}       │      │}       │         │
│  └────────┘    └────────┘    └────────┘      └────────┘         │
│       │              │             │               │             │
│       └──────────────┴─────────────┴───────────────┘             │
│                      ALL 10 COLLECTED                            │
│                                                                   │
└────────────────────────────┬────────────────────────────────────┘
                             │
                    ┌────────┴────────┐
                    │                 │
              10 artifacts  │ All 10   │  < 10 artifacts
              (✓)           │ present? │  (✗)
                            │         │
                            ▼         ▼
                       ┌─────────┐  ┌──────────┐
                       │PROCEED  │  │ ABORT    │
                       │Phase 3  │  │ Re-fan   │
                       └────┬────┘  │ out      │
                            │       └──────────┘
                            ▼
┌──────────────────────────────────────────────────────────────────┐
│  PHASE 3: COLLISION DETECTION                                    │
│           (Agent: bb80-collision-detector)                       │
│                                                                   │
│  Input:  All 10 artifacts                                        │
│  Process: Analyze structural, semantic, path overlaps            │
│  Output:  collision_map.json with dominance matrix               │
│                                                                   │
│  COLLISION ANALYSIS:                                             │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ Structural Collisions:                                  │    │
│  │   Agent 1 & Agent 3: 95% overlap (identical structure) │    │
│  │   Agent 2 & Agent 5: 87% overlap (similar metadata)   │    │
│  │                                                         │    │
│  │ Semantic Collisions:                                    │    │
│  │   Agents 6, 7, 9: Different approaches → same design  │    │
│  │                                                         │    │
│  │ Path Divergence:                                        │    │
│  │   Agents 1-3 diverged but reconverge at Phase 4        │    │
│  │                                                         │    │
│  │ DOMINANCE MATRIX:                                       │    │
│  │   Agent 1: PARETO_OPTIMAL (no one dominates it)        │    │
│  │   Agent 3: DOMINATED_BY_AGENT_1 (95% overlap)          │    │
│  │                                                         │    │
│  │ RECONCILIATION HINTS:                                   │    │
│  │   Merge: Agents [1,3] → use Agent 1 as base            │    │
│  │   Discard: Agents [3, 8]                               │    │
│  │   Synthesize: Agents [6, 7] → integrate orthogonal     │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ OUTPUT: collision_map.json                              │    │
│  │ {                                                       │    │
│  │   "collision_gate_status": "READY_FOR_CONVERGENCE"     │    │
│  │ }                                                       │    │
│  └─────────────────────────────────────────────────────────┘    │
└────────────────────────────┬────────────────────────────────────┘
                             │
                    ┌────────┴────────┐
                    │                 │
        COLLISIONS  │ collision data  │ ZERO COLLISIONS
        FOUND (✓)   │ exists?         │ (✗)
                    │                 │
                    ▼                 ▼
              ┌─────────┐        ┌──────────────┐
              │PROCEED  │        │ ABORT        │
              │Phase 4  │        │ Specification│
              └────┬────┘        │ incomplete   │
                   │             └──────────────┘
                   ▼
┌──────────────────────────────────────────────────────────────────┐
│  PHASE 4: CONVERGENCE                                            │
│           (Agent: bb80-convergence-orchestrator)                 │
│           [SEPARATE PROCESS - not original agents]               │
│                                                                   │
│  Input:  collision_map + all 10 artifacts                        │
│  Process: Apply SELECTION PRESSURE (4 objective criteria)        │
│  Output:  converged_artifact.json (merged, synthesized)          │
│                                                                   │
│  SELECTION PRESSURE EVALUATION:                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                                 Coverage   Invariants   │   │
│  │ Agent   Redundancy   Minimality  0.0-1.0   0.0-1.0      │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │   1      0.85         0.90         0.95      1.0        │   │
│  │   2      0.92         0.88         0.78      1.0        │   │
│  │   3      0.78         0.91         0.93      1.0        │   │
│  │   ...                                                    │   │
│  │  10      0.65         0.72         0.60      0.95       │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  DOMINANCE RANKING:                                              │
│  1. PARETO_OPTIMAL_1 (Agent 1): Covers most, satisfies all     │
│  2. PARETO_OPTIMAL_2 (Agent 2): Complementary to Agent 1       │
│  3. DOMINATED_BY_1 (Agent 3): Subsumed by Agent 1              │
│  ... (others evaluated)                                         │
│                                                                  │
│  RECONCILIATION DECISIONS:                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Base: Agent 1 (dominant)                                │   │
│  │ Merge: Agent 2 (30%) + Agent 6 (10%) into Agent 1      │   │
│  │ Discard: Agents 3, 8 (dominated)                       │   │
│  │ Rewrite: Agents 2, 5, 7 (simplified via Agent 1)       │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ OUTPUT: converged_artifact.json                          │   │
│  │ {                                                        │   │
│  │   "converged_artifact": {... merged content ...},       │   │
│  │   "authorship": "CONVERGENCE_PROCESS",     ← No agent   │   │
│  │   "authorship_erasure": true                            │   │
│  │ }                                                        │   │
│  └──────────────────────────────────────────────────────────┘   │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│  PHASE 5: REFACTORING & SYNTHESIS                                │
│                                                                   │
│  Input:  converged_artifact.json                                 │
│  Process: Mandatory refactoring laws (DESTRUCTIVE)               │
│  Output:  refined_artifact.json                                  │
│                                                                   │
│  OPERATIONS:                                                      │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │ MERGE:    component_2 + component_6 → unified_1         │    │
│  │ DISCARD:  component_3 (subsumed)                        │    │
│  │ REWRITE:  components_7,8 → simplified_7_8               │    │
│  │                                                          │    │
│  │ PRESERVATION: None (destructive refactoring)            │    │
│  │ RESULT:       Only final artifact survives              │    │
│  └──────────────────────────────────────────────────────────┘    │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │ OUTPUT: refined_artifact.json                            │    │
│  │ {                                                        │    │
│  │   "refined_artifact": {... final content ...},          │    │
│  │   "duplication_index": 0.0,     ← No redundancy         │    │
│  │   "only_final_construction_survives": true              │    │
│  │ }                                                        │    │
│  └──────────────────────────────────────────────────────────┘    │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│  PHASE 6: CLOSURE VALIDATION (GATE)                              │
│                                                                   │
│  Input:  All prior phase outputs                                 │
│  Process: Verify ALL 5 closure conditions                        │
│  Output:  closure_receipt.json (VALID or INVALID)                │
│                                                                   │
│  CLOSURE CONDITIONS CHECKLIST:                                    │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │ ✓ Condition 1: 10 agents launched (10 == 10) SATISFIED   │  │
│  │ ✓ Condition 2: 10 artifacts produced (10 == 10) SATISFIED │  │
│  │ ✓ Condition 3: Collision analysis performed SATISFIED     │  │
│  │ ✓ Condition 4: Convergence executed SATISFIED            │  │
│  │ ✓ Condition 5: Refactored output emitted SATISFIED       │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                   │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │ OUTPUT: closure_receipt.json                              │  │
│  │ {                                                         │  │
│  │   "closure_decision": "CLOSURE_VALID",   ← BINARY GATE   │  │
│  │   "all_conditions_met": true,                            │  │
│  │   "final_artifact": {                                     │  │
│  │     "hash": "sha256:...",                                │  │
│  │     "path": "/path/to/refined_artifact.json"             │  │
│  │   }                                                       │  │
│  │ }                                                         │  │
│  └────────────────────────────────────────────────────────────┘  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                    ┌────────┴────────┐
                    │                 │
          ALL 5 OK  │ All closure     │ ANY FAILED
          (✓)       │ conditions      │ (✗)
                    │ met?            │
                    ▼                 ▼
              ┌──────────┐        ┌──────────┐
              │ EMIT:    │        │ NO       │
              │refined_  │        │ OUTPUT   │
              │artifact  │        │(Fail-    │
              │          │        │ Closed)  │
              └──────────┘        └──────────┘
                    │
                    ▼
           ┌──────────────────┐
           │ IMPLEMENTATION   │
           │ READY            │
           └──────────────────┘
```

---

## Data Flow Example (Concrete Scenario)

### Scenario: Documentation Structure Task

#### Phase 1 Input
```
Task: Design documentation structure for QLever project
Constraints:
  - Follow Diataxis framework (explanation, how-to, reference, tutorial)
  - Organize for agent discoverability
  - Support metadata annotations
Domain: Markdown + directory structure
```

#### Phase 1 Output
```json
{
  "specification_status": "CLOSED",
  "ambiguity_count": 0,
  "closure_confidence": 1.0,
  "specification_hash": "sha256:abc123def456",
  "approved_for_fan_out": true
}
```
→ **GATE PASSES** → Proceed to Phase 2

#### Phase 2 Outputs (Example Agents 1 and 3)

**Agent 1**:
```json
{
  "agent_id": 1,
  "phase": "INDEPENDENT_CONSTRUCTION",
  "artifact_type": "structure",
  "artifact_content": {
    "approach": "Create hierarchical directory structure matching Diataxis",
    "design_decisions": [
      "Use /docs/explanation/, /docs/how-to/, /docs/reference/, /docs/tutorials/",
      "Metadata in YAML frontmatter",
      "Agent discovery via .claude/agents/ registry"
    ],
    "implementation_details": {
      "files_created": [
        "docs/explanation/overview.md",
        "docs/how-to/setup.md",
        "docs/reference/api.md",
        "docs/tutorials/01-quickstart.md"
      ]
    }
  },
  "artifact_metrics": {
    "completeness": 0.95,
    "complexity": "moderate",
    "coverage_of_specification": 0.98,
    "lines_of_code": 2500,
    "files_touched": 24
  },
  "artifact_fingerprint": {
    "content_hash": "sha256:agent1_hash",
    "deterministic": true
  }
}
```

**Agent 3**:
```json
{
  "agent_id": 3,
  "phase": "INDEPENDENT_CONSTRUCTION",
  "artifact_type": "structure",
  "artifact_content": {
    "approach": "Create Diataxis structure with same directory layout",
    "design_decisions": [
      "Use /docs/explanation/, /docs/how-to/, /docs/reference/, /docs/tutorials/",
      "Metadata in YAML frontmatter",
      "Agent discovery via registry"
    ],
    "implementation_details": {
      "files_created": [
        "docs/explanation/overview.md",
        "docs/how-to/setup.md",
        "docs/reference/api.md",
        "docs/tutorials/01-quickstart.md"
      ]
    }
  },
  "artifact_metrics": {
    "completeness": 0.93,
    "complexity": "moderate",
    "coverage_of_specification": 0.96,
    "lines_of_code": 2400,
    "files_touched": 24
  },
  "artifact_fingerprint": {
    "content_hash": "sha256:agent3_hash",
    "deterministic": true
  }
}
```

#### Phase 3 Output (Collision Detection)
```json
{
  "phase": "COLLISION_DETECTION",
  "collision_analysis": {
    "structural_collisions": [
      {
        "agent_pair": [1, 3],
        "collision_type": "STRUCTURAL_OVERLAP",
        "overlap_magnitude": 0.94,
        "overlap_percentage": "94%",
        "evidence": "Both propose identical directory structure: explanation, how-to, reference, tutorials"
      }
    ],
    "semantic_collisions": [
      {
        "agent_group": [1, 3, 5],
        "collision_type": "SEMANTIC_CONVERGENCE",
        "convergence_strength": 0.97,
        "evidence": "All three agents independently converge on Diataxis + metadata approach"
      }
    ]
  },
  "dominance_relations": {
    "dominance_matrix": [
      {
        "agent_id": 1,
        "dominated_by_agent": null,
        "dominance_type": "PARETO_OPTIMAL_1",
        "reason": "Highest coverage (0.98), complete specification satisfaction"
      },
      {
        "agent_id": 3,
        "dominated_by_agent": 1,
        "dominance_ratio": 0.94,
        "reason": "Structurally identical to Agent 1 (94% overlap)"
      }
    ]
  },
  "reconciliation_hints": {
    "mergeable_pairs": [
      {
        "agents": [1, 3],
        "merge_strategy": "USE_AGENT_1_AS_BASE",
        "reason": "94% structural overlap; Agent 1 has additional documentation depth"
      }
    ],
    "discardable_artifacts": [3, 8, 9],
    "irreducible_artifacts": [1, 2, 5, 6],
    "synthesis_opportunities": [
      {
        "agents": [1, 4],
        "synthesis_benefit": "Merge Agent 1's directory structure with Agent 4's metadata schema design"
      }
    ]
  },
  "collision_gate_status": "READY_FOR_CONVERGENCE"
}
```
→ **COLLISION DETECTED** → Proceed to Phase 4

#### Phase 4 Output (Convergence)
```json
{
  "phase": "CONVERGENCE",
  "selection_pressure_evaluation": [
    {
      "agent_id": 1,
      "selection_scores": {
        "coverage": 0.98,
        "invariants_satisfied": 1.0,
        "eliminable_redundancy": 0.88,
        "construct_minimality": 0.92
      },
      "composite_fitness": 0.9447,
      "dominance_status": "PARETO_OPTIMAL_1",
      "rationale": "Agent 1 covers most ground, satisfies all invariants"
    },
    {
      "agent_id": 3,
      "selection_scores": {
        "coverage": 0.96,
        "invariants_satisfied": 1.0,
        "eliminable_redundancy": 0.94,
        "construct_minimality": 0.91
      },
      "composite_fitness": 0.9283,
      "dominance_status": "DOMINATED_BY_AGENT_1",
      "rationale": "Subsumed by Agent 1 (94% structural equivalence)"
    },
    {
      "agent_id": 4,
      "selection_scores": {
        "coverage": 0.75,
        "invariants_satisfied": 1.0,
        "eliminable_redundancy": 0.86,
        "construct_minimality": 0.88
      },
      "composite_fitness": 0.8316,
      "dominance_status": "PARETO_OPTIMAL_2",
      "rationale": "Metadata schema is orthogonal to Agent 1's directory structure"
    }
  ],
  "reconciliation_decisions": {
    "dominant_artifact_base": 1,
    "merge_decisions": [
      {
        "source_agent": 4,
        "target_agent": 1,
        "merge_type": "MERGE_ORTHOGONAL",
        "merge_ratio": "15% from Agent 4 (metadata), 85% from Agent 1 (structure)",
        "reason": "Agent 4's metadata schema enhances Agent 1's directory structure"
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
        "reason": "SUBSUMED_BY_MERGED_OUTPUT"
      }
    ],
    "rewrite_decisions": [
      {
        "original_agents": [2, 5, 7],
        "rewrite_reason": "SIMPLIFY_REDUNDANT_APPROACHES",
        "simplified_into": "UNIFIED_STRUCTURE_WITH_METADATA"
      }
    ]
  },
  "converged_artifact": {
    "description": "Unified documentation structure: Diataxis directory layout + metadata schema",
    "content": {
      "directory_structure": [
        "/docs/explanation/ - Conceptual guides",
        "/docs/how-to/ - Task-oriented guides",
        "/docs/reference/ - API/syntax references",
        "/docs/tutorials/ - Learning-oriented step-by-step"
      ],
      "metadata_format": {
        "diataxis_type": "explanation | how-to | reference | tutorial",
        "title": "Human-readable title",
        "description": "Short description for agent discovery",
        "audience": "Intended audience",
        "status": "complete | draft | needs_review"
      }
    },
    "authorship": "CONVERGENCE_PROCESS"
  }
}
```
→ **CONVERGED** → Proceed to Phase 5

#### Phase 5 Output (Refactoring)
```json
{
  "phase": "REFACTORING_AND_SYNTHESIS",
  "refactoring_operations": [
    {
      "operation": "MERGE",
      "source_components": ["agent_4_metadata_schema"],
      "target_component": "unified_documentation_structure",
      "rationale": "Integrated metadata annotations into directory structure"
    },
    {
      "operation": "DISCARD",
      "discarded_component": "agent_3_duplicate_structure",
      "rationale": "Eliminated 94% redundant directory proposal"
    },
    {
      "operation": "REWRITE",
      "rewritten_components": ["agents_2_5_7_subcomponents"],
      "rewrite_reason": "Unified multiple metadata approaches into single schema"
    }
  ],
  "refined_artifact": {
    "description": "Final documentation structure with metadata support",
    "content": {
      "structure": {
        "docs/explanation/": "Conceptual guides",
        "docs/how-to/": "Task-oriented guides",
        "docs/reference/": "API/syntax references",
        "docs/tutorials/": "Learning step-by-step",
        ".claude/agents/": "Agent definitions with metadata"
      },
      "metadata_schema": {
        "diataxis_type": "enum",
        "title": "string",
        "description": "string",
        "audience": "string",
        "status": "enum",
        "tags": "array"
      }
    },
    "quality_metrics": {
      "coverage": 0.98,
      "invariants_satisfied": 1.0,
      "complexity": 0.45,
      "completeness": 0.97,
      "duplication_index": 0.0
    }
  },
  "preservation_status": {
    "preservation_of_intermediate_steps": false,
    "only_final_construction_survives": true
  }
}
```
→ **REFINED** → Proceed to Phase 6

#### Phase 6 Output (Closure)
```json
{
  "phase": "CLOSURE_VALIDATION",
  "closure_conditions_verification": {
    "condition_1_ten_agents_launched": {
      "status": "SATISFIED",
      "agents_launched": 10,
      "required": 10
    },
    "condition_2_ten_artifacts_produced": {
      "status": "SATISFIED",
      "artifacts_count": 10,
      "required": 10
    },
    "condition_3_collision_analysis_performed": {
      "status": "SATISFIED",
      "collision_map_exists": true,
      "structural_collisions_found": 4,
      "semantic_collisions_found": 3
    },
    "condition_4_convergence_executed": {
      "status": "SATISFIED",
      "convergence_artifact_exists": true,
      "selection_pressure_applied": true,
      "dominance_analysis_completed": true
    },
    "condition_5_refactored_output_emitted": {
      "status": "SATISFIED",
      "refined_artifact_exists": true,
      "destructive_refactoring_applied": true,
      "duplication_index": 0.0
    }
  },
  "closure_decision": "CLOSURE_VALID",
  "all_conditions_met": true,
  "failure_modes_detected": [],
  "final_artifact": {
    "hash": "sha256:final_artifact_hash",
    "path": "/home/user/qlever/docs/structure/refined_artifact.json",
    "quality_metrics": {
      "coverage": 0.98,
      "invariants_satisfied": 1.0,
      "deterministic": true,
      "reproducible": true
    }
  },
  "atomic_cycle_status": "COMPLETE"
}
```
→ **CLOSURE VALID** → **OUTPUT READY FOR IMPLEMENTATION**

---

## Phase Transition State Machine

```
START
  │
  ├─→ SPECIFICATION_CLOSURE
  │         │
  │         ├─→ specification_status = "CLOSED" ✓
  │         │        │
  │         │        └─→ approved_for_fan_out = true
  │         │               │
  │         │               └─→ FAN_OUT
  │         │
  │         └─→ specification_status = "INCOMPLETE" ✗
  │                  │
  │                  └─→ ABORT (retry specification phase)
  │
  ├─→ FAN_OUT (Phase 2)
  │         │
  │         ├─→ 10 artifacts collected ✓
  │         │        │
  │         │        └─→ COLLISION_DETECTION
  │         │
  │         └─→ < 10 artifacts ✗
  │              │
  │              └─→ ABORT (retry fan-out)
  │
  ├─→ COLLISION_DETECTION (Phase 3)
  │         │
  │         ├─→ collision_data found ✓
  │         │        │
  │         │        └─→ CONVERGENCE
  │         │
  │         └─→ zero_collision ✗
  │              │
  │              └─→ ABORT (specification incomplete)
  │
  ├─→ CONVERGENCE (Phase 4)
  │         │
  │         └─→ converged_artifact produced
  │              │
  │              └─→ REFACTORING
  │
  ├─→ REFACTORING (Phase 5)
  │         │
  │         └─→ refined_artifact produced
  │              │
  │              └─→ CLOSURE_VALIDATION
  │
  ├─→ CLOSURE_VALIDATION (Phase 6 - FINAL GATE)
  │         │
  │         ├─→ all_conditions_met = true ✓
  │         │        │
  │         │        └─→ emit refined_artifact
  │         │               │
  │         │               └─→ END (success)
  │         │
  │         └─→ any_condition_failed ✗
  │              │
  │              └─→ NO OUTPUT (fail-closed)
  │                     │
  │                     └─→ END (failure)
  │
  └─→ END
```

---

## Information Decay Diagram

Shows how information transforms through phases:

```
PHASE 1: Task Specification (1000 chars)
         ↓ [Specification Closure Analysis]
         ↓ [Determinism verification]

         OUTPUT: specification_status (1 byte: CLOSED | INCOMPLETE)

PHASE 2: specification_status ✓ (1 byte)
         + specification_hash (32 bytes SHA256)
         ↓ [10 agents construct independently]
         ↓ [Each produces artifact with metrics + reasoning]

         OUTPUT: 10 artifacts (100 KB total)

PHASE 3: 10 artifacts (100 KB)
         ↓ [Collision detection: mechanical overlap analysis]
         ↓ [No subjective judgment]

         OUTPUT: collision_map (25 KB)
         + dominance_matrix
         + reconciliation_hints

PHASE 4: collision_map (25 KB)
         + 10 artifacts (100 KB)
         ↓ [Selection pressure: objective criteria]
         ↓ [Synthesis: merge complementary outputs]

         OUTPUT: converged_artifact (40 KB)
         [Authorship erased; only fitness matters]

PHASE 5: converged_artifact (40 KB)
         ↓ [Refactoring laws: MERGE/DISCARD/REWRITE]
         ↓ [Destructive: intermediate steps not preserved]

         OUTPUT: refined_artifact (50 KB)
         [Only final construction survives]

PHASE 6: All prior outputs
         ↓ [Verify all 5 closure conditions]
         ↓ [Binary verdict: valid or invalid]

         OUTPUT: closure_receipt (15 KB)
         [All-or-nothing: CLOSURE_VALID or NO OUTPUT]
```

---

## Error Recovery Paths

### Path 1: Specification Incomplete
```
Phase 1 detected ambiguity
    ↓
specification_status = "INCOMPLETE"
    ↓
ABORT to specification phase
    ↓
Refine specification (remove ambiguity)
    ↓
Return to Phase 1
```

### Path 2: Insufficient Artifacts
```
Phase 2 completed with < 10 artifacts
    ↓
Fan-out is incomplete
    ↓
ABORT fan-out
    ↓
Re-run 10 agents (or identify/fix blockers)
    ↓
Retry Phase 2
```

### Path 3: Zero Collision
```
Phase 3 detected zero overlap
    ↓
collision_gate_status = "ZERO_COLLISION_DETECTED"
    ↓
ABORT
    ↓
Specification is ambiguous (different interpretations)
    ↓
Return to Phase 1 (specification refinement)
```

### Path 4: Closure Failure
```
Phase 6 detected unsatisfied condition (e.g., only 9 artifacts)
    ↓
closure_decision = "CLOSURE_INVALID"
    ↓
all_conditions_met = false
    ↓
NO OUTPUT (fail-closed)
    ↓
Identify which condition failed
    ↓
Retry appropriate phase (Phase 2, 3, or 4)
```

---

## Status

✅ **Visual diagrams complete**
✅ **Concrete example scenario detailed**
✅ **State machine documented**
✅ **Error recovery paths identified**

**Authority**: EPIC 9 Atomic Cognitive Cycle
**Last Updated**: 2026-01-02
