---
diataxis_type: reference
title: "Agent Documentation Navigation Guide"
description: "How agents discover, parse, and leverage Diataxis-structured documentation for specification closure, invariant validation, and verification proof construction"
audience: agents
status: complete
last_updated: 2026-01-02
difficulty: advanced
estimated_time: "10 minutes"
prerequisites:
  - "CLAUDE.md (operational model)"
  - ".claude/agents/README.md (agent lifecycle)"
related_docs:
  - "docs/INDEX.md (Diataxis structure)"
  - "docs/DIATAXIS_METADATA_SPEC.md (metadata format)"
  - "docs/QLEVEREST_THESIS_DOCUMENTATION.md (invariants)"
keywords:
  - "agent documentation discovery"
  - "Diataxis navigation"
  - "semantic search"
  - "invariant lookups"
  - "agent-priority filtering"
semantic_tags:
  - "agent-workflow/discovery"
  - "documentation/diataxis"
  - "agent-tools/query"
agent_priority: critical
search_boost: 3.0
invariants:
  - "All agent-facing docs MUST have agent_priority field"
  - "semantic_tags enable efficient keyword search"
  - "invariants field must list preserved laws"
  - "specifications field must reference specs with explicit status"
specifications:
  - "DIATAXIS_METADATA_SPEC.md (metadata closure)"
  - "CLAUDE.md (operational model)"
  - "EPIC9_cognitive_cycle.md (atomic cycle)"
---

# Agent Documentation Navigation Guide

**Purpose:** Enable autonomous agents to efficiently discover, validate, and leverage QLever documentation for specification closure, invariant validation, and verification proof construction.

---

## 1. AGENT DOCUMENTATION QUERY PATTERNS

Agents should use these queries to discover relevant documentation without iteration.

### Query Pattern 1: Find Critical Docs for Current Phase

**Fan-Out Phase** (specification closure):
```bash
rg 'agent_priority: critical' docs/ --type md | grep -E '(EPIC|SPECIFICATION|INVARIANT)'
```

**Expected results:** Specification closure docs with closure status.

**Independent Construction Phase** (implementation):
```bash
rg 'agent_priority: high' docs/ --type md | grep -E 'invariant|preservation|law'
```

**Expected results:** Invariant validation docs and proof strategies.

**Collision Detection Phase** (EPIC 9):
```bash
rg 'semantic_tags.*collision' docs/ --type md
```

**Expected results:** Collision detection theory, convergence law docs.

### Query Pattern 2: Find Specs with Closure Status

```bash
find docs -name "EPIC*.md" -o -name "*SPECIFICATION*.md" | \
  xargs grep -l "Specification Closure.*CLOSED"
```

**Expected:** Documents with formal closure markers.

### Query Pattern 3: Find Invariant Definitions

```bash
rg 'invariants:' docs/ --type md | head -20
```

**Returns:** All docs listing preserved invariants (11 architectural + 5+ shared).

### Query Pattern 4: Find Documentation by Verification Plane

```bash
# Determinism Plane
rg 'Determinism Plane|deterministic receipt' docs/ --type md

# Specification Closure Plane
rg 'Specification Closure Plane|CLOSED' docs/ --type md

# Fail-Closed Enforcement Plane
rg 'Fail-Closed|fail-closed|3-layer' docs/ --type md

# Invariant Preservation Plane
rg 'Architectural Invariant|11 invariant' docs/ --type md

# Collision Detection Plane (EPIC 9)
rg 'Collision Detection Plane|structural.*semantic.*path' docs/ --type md

# Convergence Plane (EPIC 9)
rg 'Convergence Plane|selection pressure' docs/ --type md
```

### Query Pattern 5: Find Docs for Specific Semantic Tags

```bash
# For operation strategy pattern
rg 'semantic_tags.*operation.*pattern' docs/ --type md

# For monoidal composition
rg 'semantic_tags.*monoidal' docs/ --type md

# For epoch isolation
rg 'semantic_tags.*epoch' docs/ --type md

# For hot-path semantics
rg 'semantic_tags.*hot-path' docs/ --type md
```

---

## 2. DOCUMENTATION STRUCTURE BY AGENT PHASE

### PHASE 1: Fan-Out (Specification Closure Gate)

**Agent Type:** `bb80-specification-validator`

**Required Documents:**
1. `/home/user/qlever/CLAUDE.md` — Operational meta-model authority
2. `/home/user/qlever/docs/DIATAXIS_METADATA_SPEC.md` — Document format specification
3. Relevant EPIC spec files (EPIC 4, 7, 8, 10.x) — Domain specification closure

**Query Strategy:**
```bash
# Find all specifications with status
rg 'status:.*complete' docs/ -A1 | grep -E '(EPIC|SPECIFICATION|Closure)'

# Verify zero ambiguities
rg 'ambiguities.*0/0|Specification Closure.*CLOSED' docs/ --type md
```

**Success Criteria:**
- All referenced EPIC specs found with status=complete or status=closed
- Zero unresolved ambiguities per spec document
- Specification closure gates all subsequent work

### PHASE 2: Independent Construction (Invariant Validation)

**Agent Type:** `bb80-invariant-validator`

**Required Documents:**
1. `/home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md` — 11 architectural invariants
2. `/home/user/qlever/docs/EPIC4_SHARED_INVARIANTS.md` — 5+ shared invariants (epoch, determinism, atomicity, etc.)
3. `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md` — Mathematical proof of monoidal composition
4. Reference docs (api.md, sparql.md, shacl-compliance.md) — Contract surfaces

**Query Strategy:**
```bash
# Find all invariant definitions
rg 'Invariant.*:' docs/ --type md | grep -E '(1\.|2\.|3\.|Operation|IdTable|Cache|Join|Filter|Epoch|Atomic|Fail)'

# Find monoidal composition requirements
rg 'associativity|identity|closure' docs/ --type md
```

**Success Criteria:**
- All 11 architectural invariants understood
- All 5+ shared invariants understood
- Monoidal composition proof understood
- Single-pass construction feasibility confirmed

### PHASE 3: Collision Detection (EPIC 9)

**Agent Type:** `bb80-collision-detector`

**Required Documents:**
1. `/home/user/qlever/CLAUDE.md` — Collision Semantics section
2. `/home/user/qlever/docs/epic-10-3/EPIC_10_3_CONVERGENCE_SUMMARY.md` — Convergence example
3. `/home/user/qlever/docs/explanation/collision-detection-theory.md` — Theory

**Query Strategy:**
```bash
# Find collision detection definitions
rg 'structural.*overlap|semantic.*overlap|execution.*path.*divergence' docs/ --type md

# Find convergence law documentation
rg 'selection pressure|coverage|invariant.*preservation|minimality' docs/ --type md
```

**Success Criteria:**
- Collision matrix generated from 10 agent artifacts
- Structural overlaps identified
- Semantic overlaps identified
- Execution path divergences detected

### PHASE 4: Convergence (EPIC 9)

**Agent Type:** `bb80-convergence-orchestrator`

**Required Documents:**
1. `/home/user/qlever/docs/explanation/convergence-vs-consensus.md` — Selection pressure law
2. `/home/user/qlever/CLAUDE.md` — Convergence Law section
3. Any agent receipt documents (PATCH_*.md, AGENT*_RECEIPT.md) — Example convergence

**Query Strategy:**
```bash
# Find convergence criteria
rg 'selection pressure|coverage|minimality|dominance' docs/ --type md

# Find example convergence artifacts
find docs/epic-10-3 -name "AGENT*RECEIPT.md" -o -name "CONVERGENCE*.md"
```

**Success Criteria:**
- Convergence criteria understood (coverage, invariants, minimality, dominance)
- Separate reconciliation process confirmed (not voting)
- Final artifact synthesized with agent authorship erased

### PHASE 5: Refactoring & Synthesis

**Agent Type:** All agents (cooperative)

**Required Documents:**
1. `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md` — No rework law
2. All relevant reference docs for artifact type
3. Receipt validation docs (deterministic-receipts skill)

**Query Strategy:**
```bash
# Verify no rework needed
rg 'monoidal|no backtracking|no rework|single-pass' docs/ --type md

# Find receipt validation requirements
rg 'deterministic.*receipt|benchmarks|event.*log|hash' docs/ --type md
```

**Success Criteria:**
- Output is dominant (covers most ground)
- All invariants preserved
- Zero redundancy in final artifact
- Minimal structure necessary

### PHASE 6: Closure (Deterministic Receipt Validation)

**Agent Type:** `bb80-receipt-validator`

**Required Documents:**
1. `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md` — Proof of single-pass
2. `/home/user/qlever/benchmark/regression/README.md` — Regression detection specs
3. Any specification closure docs (all EPIC*.md files)

**Query Strategy:**
```bash
# Find receipt requirements
rg 'benchmarks|guards|event.*log|digest|SHA256' docs/ --type md

# Verify all phases completed
rg 'Fan-Out|Independent.*Construction|Collision|Convergence|Refactoring|Closure' docs/ --type md
```

**Success Criteria:**
- All 6 phases completed (or no output)
- Deterministic receipt generated
- Benchmarks validate invariants
- Guards satisfied (boolean pass/fail)

---

## 3. SEMANTIC TAG INDEX (QUICK LOOKUP)

Agents can search docs by semantic tags for fast discovery:

```yaml
agent-workflow:
  - discovery
  - specification-closure
  - invariant-validation
  - collision-detection
  - convergence
  - refactoring
  - closure

documentation:
  - diataxis
  - specification
  - invariant
  - verification-plane
  - agent-guide

methodology:
  - big-bang-80-20
  - epic-9
  - monoidal-composition
  - specification-closure
  - deterministic-receipts

principles:
  - single-pass-construction
  - determinism
  - fail-closed
  - atomicity
  - monoidal-law

domains:
  - query-execution
  - indexing
  - parsing
  - ingress
  - http-protocol
```

**Query example:**
```bash
rg 'semantic_tags.*specification-closure' docs/ --type md
```

---

## 4. AGENT PRIORITY INDEX

Agents should prioritize docs by `agent_priority` field:

| Priority | Meaning | Action | When |
|----------|---------|--------|------|
| **critical** | Must read before phase start | Load immediately | Always |
| **high** | Strongly recommended | Load if available | Most cases |
| **medium** | Useful context | Load on demand | Referenced |
| **low** | Optional background | Skip unless blocked | Rarely |

**Critical docs (must load):**
- `/home/user/qlever/CLAUDE.md`
- `/home/user/qlever/README.md` (new agent-native version)
- `docs/QLEVEREST_THESIS_DOCUMENTATION.md` (invariants)
- `docs/MONOIDAL_CONSTRUCTION_LAW.md` (proof)
- `docs/EPIC*.md` (specifications)
- `.claude/agents/README.md` (agent lifecycle)

**High-priority docs (load when available):**
- Reference docs (api.md, sparql.md, etc.)
- Specification closure docs (EPIC*_SPECIFICATION_CLOSURE.md)
- Invariant docs (EPIC*_SHARED_INVARIANTS.md)
- Convergence docs (EPIC_10_3_CONVERGENCE_SUMMARY.md)

---

## 5. VERIFICATION PROOF LOOKUP

Agents can find highest-value tests and benchmarks by verification plane:

```bash
# Find determinism proof docs
rg 'Determinism Plane|RegressionDetector' docs/ --type md

# Find fail-closed enforcement docs
rg 'Fail-Closed.*3-layer|EntropyInjection' docs/ --type md

# Find invariant preservation docs
rg '11.*Architectural.*Invariant|QLEVEREST_THESIS' docs/ --type md

# Find EPIC 9 collision/convergence docs
rg 'Collision Detection Plane|Convergence Plane' docs/ --type md

# Find EPIC 10.1 regression detection docs
rg 'Regression Detection Plane|RegressionGate' docs/ --type md
```

---

## 6. METADATA FIELD PARSING (FOR AGENTS)

Agents should parse frontmatter YAML to extract:

### Required Fields (always extract)
- `diataxis_type` — Document classification (tutorial | how-to | explanation | reference)
- `agent_priority` — Load priority (critical | high | medium | low)
- `status` — Document freshness (draft | complete | deprecated | archived)

### Recommended Fields (conditional extraction)
- `prerequisites` — What to read first
- `related_docs` — Related doc paths
- `semantic_tags` — Search keywords
- `invariants` — Preserved laws

### Validation Rules
1. **All critical docs MUST have `status: complete`**
2. **All agent-facing docs MUST have `agent_priority` field**
3. **All specification docs MUST have `specifications` field listing related specs**
4. **All EPIC specs MUST have closure status marker (e.g., "Specification Closure: CLOSED")**

---

## 7. DOCUMENT DISCOVERY WITHOUT ITERATION

### Goal: Find Spec for Unknown Domain

**Strategy:**
1. Check `/home/user/qlever/CLAUDE.md` for domain mention
2. Search `docs/EPIC*.md` files for `agent_priority: critical` + matching keywords
3. Check `status` field — must be `complete` or `closed`
4. If not found → abort with specification incomplete error (no partial knowledge)

**Example: Finding JSON-LD Ingress Contract**
```bash
rg 'JSON-LD|ingress|SHACL.*ShEx.*N3.*Datalog' docs/EPIC*.md | head -5

# Expected: docs/EPIC10.1_CANONICAL_RESULT_SERIALIZATION.md or
#           docs/reference/jsonld-ingress-contract.md

# Verify status
grep "^status:" docs/reference/jsonld-ingress-contract.md
# Expected: status: complete
```

---

## 8. AGENT DECISION TREE

Use this tree to navigate documentation discovery:

```
Start Task
  ↓
Read CLAUDE.md (operational model)
  ↓
Is specification closed?
  ├─ No → Query EPIC*.md for spec + run bb80-specification-validator
  │ └─ Spec status = COMPLETE? → Proceed
  │ └─ Spec status ≠ COMPLETE? → Abort (specification incomplete)
  │
  └─ Yes → Proceed to phase-specific docs
      ↓
      Which phase?
      ├─ Fan-Out (Spec Closure)
      │ └─ Load: DIATAXIS_METADATA_SPEC.md + EPIC files + CLAUDE.md
      │
      ├─ Independent Construction (Invariants)
      │ └─ Load: QLEVEREST_THESIS_DOCUMENTATION.md + MONOIDAL_CONSTRUCTION_LAW.md
      │
      ├─ Collision Detection (EPIC 9)
      │ └─ Load: collision-detection-theory.md + CONVERGENCE_SUMMARY.md
      │
      ├─ Convergence (EPIC 9)
      │ └─ Load: convergence-vs-consensus.md + EPIC_10_3_CONVERGENCE_SUMMARY.md
      │
      ├─ Refactoring (Single-Pass)
      │ └─ Load: MONOIDAL_CONSTRUCTION_LAW.md + invariant docs
      │
      └─ Closure (Receipt Validation)
          └─ Load: deterministic-receipts docs + benchmark specs
```

---

## REFERENCES

**Authority documents:**
- `/home/user/qlever/CLAUDE.md` — Operational model
- `/home/user/qlever/docs/INDEX.md` — Diataxis structure overview
- `/home/user/qlever/docs/DIATAXIS_METADATA_SPEC.md` — Metadata format specification
- `/home/user/qlever/README.md` — Agent-native navigation guide

**Core references for agents:**
- `docs/QLEVEREST_THESIS_DOCUMENTATION.md` — Invariants
- `docs/MONOIDAL_CONSTRUCTION_LAW.md` — Mathematical proof
- `docs/EPIC4_SHARED_INVARIANTS.md` — Epoch/determinism/atomicity laws
- `docs/EPIC7_SHARED_INVARIANTS.md` — SIMD/ingress laws
- `docs/EPIC8_SPECIFICATION_CLOSURE.md` — Deterministic construction
- `docs/explanation/epic9-cognitive-cycle.md` — Atomic cognitive cycle
- `docs/epic-10-3/EPIC_10_3_CONVERGENCE_SUMMARY.md` — Collision detection example

**Verification references:**
- `test/engine/RegressionDetectorTest.cpp` — Determinism proofs
- `test/chaos/EntropyInjectionHarnessTest.cpp` — Fail-closed proofs
- `test/integration/` — Phase lifecycle proofs
- `benchmark/regression/README.md` — Regression detection specs
