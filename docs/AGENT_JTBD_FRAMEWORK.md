---
diataxis_type: reference
title: "Agent JTBDs (Jobs To Be Done) — Coding Agent Objectives & Solutions"
description: "Map coding agent objectives to QLever tools, documentation, and entrypoints. Solve specific jobs without iteration."
audience: agents
status: complete
last_updated: 2026-01-02
difficulty: advanced
estimated_time: "15 minutes"
prerequisites:
  - "CLAUDE.md (operational model)"
  - "README.md (agent-native guide)"
  - ".claude/agents/README.md (agent lifecycle)"
related_docs:
  - "docs/AGENT_DOCUMENTATION_GUIDE.md (discovery patterns)"
  - "docs/INDEX.md (Diataxis overview)"
keywords:
  - "agent objectives"
  - "JTBD framework"
  - "coding agent jobs"
  - "task entrypoints"
semantic_tags:
  - "agent-workflow/objectives"
  - "agent-tools/job-mapping"
  - "methodology/jtbd"
agent_priority: high
search_boost: 2.5
invariants:
  - "Each JTBD has exactly one canonical tool/entrypoint"
  - "No JTBD requires iteration (specification closure prerequisite)"
  - "Each JTBD maps to concrete paths and commands"
specifications:
  - "CLAUDE.md (atomic cognitive cycle)"
  - "AGENT_DOCUMENTATION_GUIDE.md (discovery patterns)"
---

# Agent JTBDs (Jobs To Be Done)

**Purpose:** Help coding agents accomplish their work by mapping objectives to tools, documentation, and proof entrypoints.

**Principle:** Each job has exactly one canonical solution. No iteration.

---

## JTBD #1: Understand System Architecture

**Agent Need:** "I need to understand QLever's structure, invariants, and design patterns before implementing anything."

### Solution Path

**Step 1: Read Authority Documents (5 min)**
```bash
cat /home/user/qlever/CLAUDE.md          # Operational model
cat /home/user/qlever/README.md          # Agent-native navigation
```

**Step 2: Learn Invariants (10 min)**
```bash
cat /home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md  # 11 architectural invariants
cat /home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md        # Mathematical proof
```

**Step 3: Map Modules (5 min)**
- Read README.md § "3. NAVIGATION GRAPH"
- Reference: `/home/user/qlever/src/engine/Operation.h` (Strategy pattern root)

**Step 4: Understand Verification (5 min)**
- Read README.md § "5. VERIFICATION PLANES"
- Reference: `/home/user/qlever/docs/EPIC4_SHARED_INVARIANTS.md` (5+ shared invariants)

**Success Criteria:**
- [ ] Understand 11 architectural invariants
- [ ] Understand monoidal composition law
- [ ] Know module topology (engine, util, parser, index)
- [ ] Know verification planes (8 types)

**Estimated Time:** 25 minutes
**Tool:** cat + grep (no code changes)
**Success Proof:** Can explain invariants 1-5 without consulting docs

---

## JTBD #2: Validate Specification (Gate)

**Agent Need:** "I need to verify the task is fully specified before starting implementation."

### Solution Path

**Canonical Tool:** `bb80-specification-validator` agent

**Step 1: Run Specification Validator**
```bash
Task: bb80-specification-validator → Output: CLOSED or INCOMPLETE
```

**Step 2: If CLOSED:**
```
✅ Proceed to JTBD #3 (Build & Test Environment)
```

**Step 3: If INCOMPLETE:**
```
⛔ Abort: Return to specification phase
Query: rg 'Specification Closure.*INCOMPLETE' docs/EPIC*.md
Fix: Identify missing specification + reopen validator
```

**Expected Output:**
```
Specification Status: CLOSED
Ambiguities Resolved: 0/0
Design Freedom: Zero
Implementation Readiness: READY
```

**Success Criteria:**
- [ ] Validator returns CLOSED
- [ ] Zero unresolved ambiguities
- [ ] Task specification is formal (not narrative)

**Estimated Time:** 2-5 minutes
**Blocking Condition:** Cannot proceed to implementation if INCOMPLETE

---

## JTBD #3: Build & Test Environment

**Agent Need:** "I need to verify the build system works and discover test entrypoints."

### Solution Path

**Step 1: Verify Environment**
```bash
cmake --version        # ✓ 3.27+
clang++ --version      # ✓ 16.0+
conan --version        # ✓ 2.x
```

**Step 2: Run Build**
```bash
cd /home/user/qlever
make build             # Deterministic release build
```

**Step 3: Discover Tests**
```bash
ctest -N               # List all 3,453+ tests
ctest -N | grep -E 'engine|parser|index'  # Filter by subsystem
```

**Step 4: Run Core Tests**
```bash
ctest --output-on-failure -j$(nproc)  # Run all unit tests
```

**Success Criteria:**
- [ ] Build completes (or ICU error if cloud environment)
- [ ] ctest discovers all tests
- [ ] Unit tests pass

**Estimated Time:** 5-10 minutes (build time varies)
**Reference:** README.md § "4. ENTRYPOINTS"

---

## JTBD #4: Find Code for Task

**Agent Need:** "I need to locate the code that implements X, understand its contract, and find relevant tests."

### Solution Path

**Step 1: Identify Module**
- Read README.md § "3. NAVIGATION GRAPH"
- Module: src/engine/, src/index/, src/parser/, src/util/?

**Step 2: Find Critical File**
```bash
# Example: Find Query Planner
rg 'class QueryPlanner' src/engine/ --type h

# Example: Find Operation base
rg 'class Operation' src/engine/ --type h
```

**Step 3: Read Header Contract**
```bash
# Read Operation.h to understand strategy pattern
cat src/engine/Operation.h | head -150
```

**Step 4: Find Tests**
```bash
# Find tests for module
find test -name "*Operation*" -o -name "*QueryPlanner*"
```

**Step 5: Check Verification**
- Read README.md § "6. PROJECT STRUCTURE ANALYSIS" → find module
- Section lists: Primary files, Contracts/specs, Tests, Failure modes

**Success Criteria:**
- [ ] Located source file(s)
- [ ] Understood public interface (header)
- [ ] Found test file(s)
- [ ] Know failure modes (from README)

**Estimated Time:** 5 minutes
**Reference:** README.md § "10. QUERY MAP" (rg patterns)

---

## JTBD #5: Understand Contract Surface

**Agent Need:** "I need to know what public APIs/contracts define this component's behavior."

### Solution Path

**Step 1: Read Contract Definition**
```bash
# Example: Query execution contract
cat src/engine/Operation.h | grep -A20 "class Operation"

# Example: Index contract
cat src/index/Index.h | grep -A20 "class Index"
```

**Step 2: Check Reference Docs**
```bash
# Example: SPARQL support contract
cat docs/reference/sparql.md | head -50

# Example: JSON-LD ingress contract
cat docs/reference/jsonld-ingress-contract.md
```

**Step 3: Verify Against Tests**
```bash
# Find tests validating the contract
rg 'TEST_F.*Operation' test/engine/ --type cpp | head -10
```

**Success Criteria:**
- [ ] Understand public interface
- [ ] Know all parameters/return types
- [ ] Know error handling semantics
- [ ] Know thread-safety model (if applicable)

**Estimated Time:** 10 minutes
**Reference:** README.md § "11. CONTRACT SURFACES" (9 stable interfaces)

---

## JTBD #6: Validate Code Against Invariants

**Agent Need:** "I need to ensure my code preserves all 11 architectural invariants and monoidal composition."

### Solution Path

**Canonical Tool:** `bb80-invariant-validator` agent

**Step 1: Check 11 Architectural Invariants**
```bash
cat /home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md | grep "^## Invariant"
```

**11 Invariants Checklist:**
- [ ] 1. Operation Strategy Pattern (all ops inherit Operation.h base)
- [ ] 2. Column-Major IdTable (cache-friendly layout)
- [ ] 3. Variable-to-Column Mapping (VariableToColumnMap struct)
- [ ] 4. Cache Key Uniqueness (epoch + manifest)
- [ ] 5. Runtime Information Tree (cost estimates)
- [ ] 6. Type Information (UNDEF tracking)
- [ ] 7. Tree Composition (DAG structure)
- [ ] 8. Join Operator Invariants
- [ ] 9. Aggregation Operator Invariants
- [ ] 10. Filter Operator Invariants
- [ ] 11. Optional (LEFT OUTER JOIN) Invariants

**Step 2: Verify Monoidal Composition**
```bash
# No rework allowed
rg 'monoidal|single-pass|no.*iteration' docs/ --type md | grep -i "law\|composition"
```

**Step 3: Run Invariant Validator**
```bash
Task: bb80-invariant-validator → Output: INVARIANTS_PRESERVED or VIOLATED
```

**Step 4: If VIOLATED**
```
⛔ Abort: Rewrite code to preserve invariants
No backtracking allowed: single-pass construction is mandatory
```

**Success Criteria:**
- [ ] Code preserves all 11 invariants
- [ ] Monoidal composition verified (no rework)
- [ ] Invariant validator passes

**Estimated Time:** 10 minutes (per code change)
**Blocking Condition:** Cannot merge without invariant preservation

---

## JTBD #7: Detect & Fix Regressions

**Agent Need:** "I need to ensure my changes don't regress latency, cache hit rates, or other performance metrics."

### Solution Path

**Canonical Tool:** `./RegressionGate` (EPIC 10.1)

**Step 1: Build Benchmarks**
```bash
cmake --build build --target RegressionGate ingress_throughput
```

**Step 2: Get Current Metrics**
```bash
./build/ingress_throughput > /tmp/current_perf.json 2>&1
```

**Step 3: Run Regression Gate**
```bash
./RegressionGate \
  --baseline benchmark/regression/baseline_performance.json \
  --current /tmp/current_perf.json
```

**Expected Output:**
```
=== REGRESSION GATE REPORT ===

Baseline metrics:
  Mean latency: 5200000 ns
  Bytes hit rate: 75.5%

Current metrics:
  Mean latency: 5300000 ns (±1.9%)
  Bytes hit rate: 75.4%

Result: NO REGRESSION (within ±10% latency, ±5% cache thresholds)
EXIT CODE: 0 (PASS)
```

**Step 4: If Regression Detected (EXIT CODE: 1)**
```
⛔ Merge blocked: Rewrite to optimize
Performance SLA violated:
  - Latency variance: 12.5% (threshold: ±10%)
  - Cache drift: 6.2% (threshold: ±5%)
```

**Success Criteria:**
- [ ] RegressionGate exits with 0 (PASS)
- [ ] Latency within ±10%
- [ ] Cache hit rates within ±5%
- [ ] Baseline metrics preserved

**Estimated Time:** 5 minutes (gate execution)
**Reference:** README.md § "5. Regression Detection Plane (EPIC 10.1)"
**Baseline Authority:** `/home/user/qlever/benchmark/regression/baseline_performance.json`

---

## JTBD #8: Run Deterministic Tests

**Agent Need:** "I need to verify results are deterministic (same input → same output across builds)."

### Solution Path

**Canonical Tool:** Determinism test suite

**Step 1: Run Regression Detector Tests**
```bash
ctest -R "RegressionDetector" --output-on-failure
```

**Step 2: Run Chaos/Entropy Tests**
```bash
ctest -R "EntropyInjection" --output-on-failure
```

**Expected Output:**
```
✓ RegressionDetectorTest.cpp (20+ test cases)
✓ EntropyInjectionHarnessTest.cpp (26+ chaos cases)

All tests passed: Determinism verified
```

**Step 3: Generate Deterministic Receipt**
```bash
# After all tests pass:
# Receipt is auto-generated with SHA256 hash of artifacts
cat /tmp/deterministic_receipt.json
```

**Step 4: Verify Receipt**
```bash
Task: bb80-receipt-validator → Output: PROOF_VALID or PROOF_FAILED
```

**Success Criteria:**
- [ ] Determinism tests pass (same input → same digest)
- [ ] Deterministic receipt generated
- [ ] Receipt validator passes
- [ ] No silent corruption detected

**Estimated Time:** 5 minutes (test execution)
**Reference:** README.md § "12. Determinism Proof (Invariant 1)"
**Test Authority:** `/home/user/qlever/test/engine/RegressionDetectorTest.cpp`

---

## JTBD #9: Discover Specification by Phase

**Agent Need:** "I need to find the right specification for my current phase (fan-out, invariants, collision, convergence, etc.)."

### Solution Path

**Phase-Specific Discovery:**

**PHASE 1: Fan-Out (Specification Closure)**
```bash
# Find spec closure docs
rg 'Specification Closure.*CLOSED' docs/EPIC*.md
# Or use agent guide:
cat docs/AGENT_DOCUMENTATION_GUIDE.md | grep -A10 "PHASE 1"
```

**PHASE 2: Independent Construction (Invariant Validation)**
```bash
# Find invariant docs
rg 'invariants:' docs/ --type md
# Specific: QLEVEREST_THESIS_DOCUMENTATION.md
```

**PHASE 3: Collision Detection (EPIC 9)**
```bash
# Find collision theory
rg 'Collision Detection Plane' docs/ --type md
cat docs/explanation/collision-detection-theory.md
```

**PHASE 4: Convergence (EPIC 9)**
```bash
# Find convergence law
rg 'Convergence Plane|selection pressure' docs/ --type md
cat docs/explanation/convergence-vs-consensus.md
```

**PHASE 5: Refactoring & Synthesis**
```bash
# Single-pass only: no rework
cat docs/MONOIDAL_CONSTRUCTION_LAW.md
```

**PHASE 6: Closure (Deterministic Receipt)**
```bash
# Find receipt validation
rg 'deterministic.*receipt|bb80-receipt-validator' docs/ --type md
```

**Success Criteria:**
- [ ] Found phase-specific spec
- [ ] Spec has status=COMPLETE or CLOSED
- [ ] Know exactly what to do next

**Estimated Time:** 2 minutes
**Reference:** docs/AGENT_DOCUMENTATION_GUIDE.md § "2. DOCUMENTATION STRUCTURE BY AGENT PHASE"

---

## JTBD #10: Find Verification Proof for Claim

**Agent Need:** "I need to prove that invariant X is satisfied, for claim Y in the spec."

### Solution Path

**Step 1: Identify Which Invariant**
```bash
# Example: Prove determinism
# Invariant 1: Same input → same output

rg 'Determinism Proof' docs/README.md
```

**Step 2: Find Proof Location**
- Test file: `/home/user/qlever/test/engine/RegressionDetectorTest.cpp`
- Gate tool: `./RegressionGate`
- Baseline: `/home/user/qlever/benchmark/regression/baseline_performance.json`

**Step 3: Run Proof Test**
```bash
ctest -R "RegressionDetector" --output-on-failure
```

**Step 4: Interpret Results**
```
✓ All 20+ test cases pass → INVARIANT SATISFIED
✗ Any test fails → INVARIANT VIOLATED
```

**8 Verification Planes (Pick One):**

| Plane | Proof Test | Location |
|-------|-----------|----------|
| Determinism | RegressionDetectorTest.cpp | `test/engine/` |
| Monoidal Composition | OperationTest.cpp (all ops) | `test/engine/` |
| Epoch Isolation | EpochIntegrationTest.cpp | `test/integration/` |
| Fail-Closed | EntropyInjectionHarnessTest.cpp | `test/chaos/` |
| Hot-Path Silence | clang-tidy lint.yml | `.github/workflows/` |
| Conformance | W3CShaclTestSuiteTest.cpp | `test/engine/shacl/` |
| Invariant Preservation | All operation tests | `test/engine/` |
| Regression Detection | RegressionGate | `benchmark/regression/` |

**Success Criteria:**
- [ ] Found proof test
- [ ] Test passes
- [ ] Invariant is proven satisfied

**Estimated Time:** 3 minutes
**Reference:** README.md § "12. PROOF INDEX"

---

## JTBD #11: Collaborate with Other Agents (Collision Detection)

**Agent Need:** "I need to detect conflicts with other agents' work and converge on a solution."

### Solution Path

**Canonical Tool:** `bb80-collision-detector` agent (EPIC 9)

**Step 1: After All 10 Agents Complete**
```
Input: 10 independent artifacts
Tool: bb80-collision-detector
Output: Collision matrix (structural, semantic, path overlaps)
```

**Step 2: Analyze Collision Matrix**
```bash
# Structural overlap: Same file/function, different implementations
# Semantic overlap: Different approach, same goal
# Path divergence: Independent agents reconverge at convergence point
```

**Step 3: If No Collisions**
```
→ Proceed to Convergence Phase (JTBD #12)
```

**Step 4: If Collisions Detected**
```
→ Collision is REQUIRED signal for convergence
→ Multiple agents = better coverage
→ Invoke Convergence Phase with collision data
```

**Success Criteria:**
- [ ] Collision detection complete
- [ ] Structural overlaps identified
- [ ] Semantic overlaps identified
- [ ] Ready for convergence

**Estimated Time:** 5 minutes
**Reference:** CLAUDE.md § "Collision Semantics"
**Theory:** `docs/explanation/collision-detection-theory.md`

---

## JTBD #12: Converge on Final Solution (EPIC 9)

**Agent Need:** "I need to reconcile multiple agent artifacts and synthesize a single final solution."

### Solution Path

**Canonical Tool:** `bb80-convergence-orchestrator` agent (EPIC 9)

**Step 1: Receive Collision Matrix**
```
From: bb80-collision-detector
Input: 10 artifacts + overlap analysis
```

**Step 2: Apply Selection Pressure**
```
Decision Criteria:
1. Coverage: Which artifact covers most ground?
2. Invariants: Does artifact preserve all 11 invariants?
3. Redundancy: Can overlapping work be merged?
4. Minimality: Does artifact use minimal structure?
```

**Step 3: Synthesize Final Artifact**
```
Actions:
- Merge multiple agents' outputs (compatible pieces)
- Discard entire agents' work (dominated)
- Rewrite (better alternative discovered)

Result: ONE final artifact (authorship erased)
```

**Step 4: Validate Final Artifact**
```bash
Task: bb80-receipt-validator → Output: PROOF_VALID
```

**Success Criteria:**
- [ ] Coverage maximized (artifact covers most ground)
- [ ] All invariants preserved
- [ ] Zero redundancy
- [ ] Minimal structure achieved
- [ ] Receipt validator passes

**Estimated Time:** 10 minutes
**Reference:** docs/explanation/convergence-vs-consensus.md
**Example:** `docs/epic-10-3/EPIC_10_3_CONVERGENCE_SUMMARY.md`

---

## JTBD #13: Generate Deterministic Proof (Receipt)

**Agent Need:** "I need to generate proof that my work satisfies all invariants (benchmarks + event log + hash)."

### Solution Path

**Canonical Tool:** `bb80-receipt-validator` agent

**Step 1: Gather Evidence**
```bash
# Benchmarks
./RegressionGate --baseline baseline.json --current current.json

# Test results
ctest --output-on-failure 2>&1 | tee test_output.log

# State digests
sha256sum *.json *.txt
```

**Step 2: Generate Receipt**
```bash
# Receipt includes:
cat > deterministic_receipt.json << EOF
{
  "timestamp": "2026-01-02T18:30:00Z",
  "benchmarks": { ... benchmarks from gate ... },
  "tests": { ... all test results ... },
  "digests": { ... SHA256 hashes ... },
  "guards": { ... guard satisfaction checks ... },
  "invariants": [ ... 11 architectural invariants ... ],
  "proof_status": "DETERMINISTIC"
}
EOF
```

**Step 3: Validate Receipt**
```bash
Task: bb80-receipt-validator \
  --receipt deterministic_receipt.json \
  --baseline baseline.json \
  --invariants 11 \
  → Output: RECEIPT_VALID or RECEIPT_INVALID
```

**Step 4: If RECEIPT_VALID**
```
✅ Work is proven deterministic
✅ Can be merged
✅ No iteration allowed (receipt blocks rework)
```

**Step 5: If RECEIPT_INVALID**
```
⛔ Abort: Receipt validation failed
⛔ Rework required (but within same specification)
⛔ Do NOT iterate on specification
```

**Success Criteria:**
- [ ] Receipt generated with all evidence
- [ ] Benchmarks pass (±10% latency, ±5% cache)
- [ ] Tests pass (all 3,453+ unit tests)
- [ ] Digests valid (SHA256 hashes match)
- [ ] Guards satisfied (boolean true)
- [ ] Receipt validator passes

**Estimated Time:** 5 minutes
**Reference:** README.md § "PROOF INDEX" (test list)
**Receipt Tool:** `bb80-receipt-validator` agent
**Authority:** `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md`

---

## JTBD #14: Commit Work to Git

**Agent Need:** "I need to commit my work with proper message format and ensure CI gates pass."

### Solution Path

**Step 1: Stage Changes**
```bash
git add <modified-files>
```

**Step 2: Write Commit Message (Conventional Commits)**
```bash
git commit -m "$(cat <<'EOF'
feat(module): Short description of what changed

Body: Explain why (not what, that's in the diff)
- Reference EPIC number if applicable
- Reference any specs or invariants
- Keep it factual

Example:
  feat(engine): Add caching to QueryPlanner

  Reduce query replan overhead by caching parsed trees.
  Preserves all 11 invariants (cache key includes epoch).
  Regression gate: ±2% latency improvement.
  EPIC 10.1 requirement.
EOF
)"
```

**Step 3: Push to Feature Branch**
```bash
git push -u origin claude/<feature>-<SESSION_ID>
```

**Step 4: Monitor CI Gates**
```
GitHub Actions (18 workflows) check:
✅ clang-format (Google style)
✅ clang-tidy (hot-path enforcement)
✅ codespell (documentation)
✅ code-coverage (>70% new code)
✅ regression-gate (±10% latency)
✅ conformance (W3C specs)
✅ ... (15 more gates)
```

**Step 5: If All Gates Pass**
```
✅ Ready for merge
PR can be reviewed and merged to master
```

**Step 6: If Any Gate Fails**
```
⛔ Merge blocked
Fix the failing check and push again
(no force-push to master)
```

**Success Criteria:**
- [ ] Commit message follows Conventional Commits
- [ ] All 18 CI gates pass
- [ ] Code coverage >70% (new code)
- [ ] Regression gate: no regression
- [ ] Ready for merge

**Estimated Time:** 5 minutes
**Reference:** CONTRIBUTING.md (commit guidelines)
**CI Gates:** `.github/workflows/` (18 total)

---

## QUICK REFERENCE: JTBD → TOOL MAPPING

| JTBD | Tool | Time | Status |
|------|------|------|--------|
| #1 Understand Architecture | cat + README | 25m | Always |
| #2 Validate Specification | bb80-specification-validator | 5m | Gate (required) |
| #3 Build & Test | make + ctest | 5-10m | Always |
| #4 Find Code | rg + grep | 5m | Per feature |
| #5 Understand Contract | cat + rg | 10m | Per module |
| #6 Validate Invariants | bb80-invariant-validator | 10m | Gate (required) |
| #7 Detect Regressions | ./RegressionGate | 5m | Gate (required) |
| #8 Run Deterministic Tests | ctest (regression suite) | 5m | Gate (required) |
| #9 Discover Spec by Phase | rg + AGENT_DOCUMENTATION_GUIDE | 2m | Per phase |
| #10 Find Proof | rg + ctest | 3m | Per claim |
| #11 Collision Detection | bb80-collision-detector | 5m | EPIC 9 (required) |
| #12 Converge Solutions | bb80-convergence-orchestrator | 10m | EPIC 9 (required) |
| #13 Generate Proof | bb80-receipt-validator | 5m | Gate (required) |
| #14 Commit Work | git + CI gates | 5m | Always |

---

## JTBD WORKFLOW: COMPLETE AGENT CYCLE

```
Start Task
  ↓
JTBD #1: Understand Architecture (README + CLAUDE.md)
  ↓
JTBD #2: Validate Specification (bb80-specification-validator) → CLOSED?
  ├─ No → Abort (spec incomplete)
  └─ Yes → Continue
  ↓
JTBD #3: Build & Test Environment (make + ctest)
  ↓
JTBD #9: Discover Spec by Phase (AGENT_DOCUMENTATION_GUIDE)
  ↓
JTBD #4: Find Code (rg patterns)
  ↓
JTBD #5: Understand Contract (read headers + tests)
  ↓
JTBD #6: Validate Invariants (bb80-invariant-validator)
  ├─ Violated → Abort (rewrite preserving invariants)
  └─ Preserved → Continue
  ↓
[Implement code for JTBD]
  ↓
JTBD #7: Detect Regressions (RegressionGate) → PASS?
  ├─ No → Abort (optimize)
  └─ Yes → Continue
  ↓
JTBD #8: Run Deterministic Tests (ctest regression suite) → PASS?
  ├─ No → Abort (fix failures)
  └─ Yes → Continue
  ↓
JTBD #10: Find Proof for Claims (ctest specific suites)
  ↓
JTBD #11: Collision Detection (bb80-collision-detector)
  ↓
JTBD #12: Converge Solutions (bb80-convergence-orchestrator)
  ↓
JTBD #13: Generate Proof (bb80-receipt-validator) → VALID?
  ├─ No → Abort (gather more evidence)
  └─ Yes → Continue
  ↓
JTBD #14: Commit Work (git + CI gates)
  ↓
All Gates Pass?
  ├─ No → Fix and retry (still single specification)
  └─ Yes → Done ✅
```

---

## REFERENCES

**Authority Documents:**
- `/home/user/qlever/CLAUDE.md` — Operational model
- `/home/user/qlever/README.md` — Agent-native guide
- `/home/user/qlever/docs/AGENT_DOCUMENTATION_GUIDE.md` — Phase-specific discovery

**Canonical Tools:**
- `bb80-specification-validator` — Gate specification closure
- `bb80-invariant-validator` — Enforce invariant preservation
- `bb80-collision-detector` — Detect overlap (EPIC 9)
- `bb80-convergence-orchestrator` — Reconcile artifacts (EPIC 9)
- `bb80-receipt-validator` — Validate deterministic proof
- `./RegressionGate` — Detect performance regressions (EPIC 10.1)
- `ctest` — Run test suite (3,453+ tests)
- `make build` — Deterministic release build

**Key Paths:**
- Build entrypoints: `/home/user/qlever/CMakeLists.txt`
- Test entrypoints: `/home/user/qlever/test/CMakeLists.txt`
- Benchmark entrypoints: `/home/user/qlever/benchmark/CMakeLists.txt`
- Agent configuration: `/home/user/qlever/.claude/agents/README.md`

**Verification Authorities:**
- Invariants: `/home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md` (11 invariants)
- Proofs: `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md` (mathematical)
- Regressions: `/home/user/qlever/benchmark/regression/baseline_performance.json`
- Tests: `/home/user/qlever/test/` (3,453+ test definitions)
