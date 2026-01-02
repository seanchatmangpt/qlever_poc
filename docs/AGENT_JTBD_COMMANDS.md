---
diataxis_type: reference
title: "Agent JTBD Command Cheat Sheet"
description: "Copy-paste commands for each job agents need to do. No iteration, deterministic execution."
audience: agents
status: complete
last_updated: 2026-01-02
difficulty: intermediate
estimated_time: "5 minutes"
prerequisites:
  - "AGENT_JTBD_FRAMEWORK.md (full JTBD descriptions)"
related_docs:
  - "README.md (Quick Start Commands)"
  - "how-to/quick-start.md (copy-paste examples)"
keywords:
  - "agent commands"
  - "copy-paste ready"
  - "deterministic execution"
semantic_tags:
  - "agent-tools/commands"
  - "workflow/quick-reference"
agent_priority: critical
search_boost: 2.0
---

# Agent JTBD Command Cheat Sheet

**Purpose:** Copy-paste commands for agent JTBDs. No iteration.

**All commands are deterministic** (same input → same output).

---

## JTBD #1: Understand Architecture

```bash
# Read operational model (5 min)
cat /home/user/qlever/CLAUDE.md | head -100

# Read agent-native guide (5 min)
cat /home/user/qlever/README.md | head -50

# Learn invariants (10 min)
cat /home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md | grep "^## Invariant" -A2

# Learn monoidal law (5 min)
cat /home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md | head -50
```

---

## JTBD #2: Validate Specification

```bash
# Validate spec closure (returns CLOSED or INCOMPLETE)
# Task tool with subagent_type=bb80-specification-validator
Task: bb80-specification-validator → Check output for status

# If INCOMPLETE: find missing spec
rg 'Specification Closure.*INCOMPLETE' docs/EPIC*.md
```

---

## JTBD #3: Build & Test Environment

```bash
# Verify CMake
cmake --version

# Verify Compiler
clang++ --version

# Verify Conan
conan --version

# Build (deterministic release)
cd /home/user/qlever && make build

# Or direct CMake
mkdir -p build && cd build && \
  cmake -DCMAKE_BUILD_TYPE=Release -GNinja .. && \
  cmake --build . -j$(nproc)

# Discover all tests
ctest -N | wc -l  # Should show 3,453+

# Run unit tests
ctest --output-on-failure -j$(nproc)
```

---

## JTBD #4: Find Code for Task

```bash
# Find critical files by pattern
rg 'class Operation' src/engine/ --type h
rg 'class Index' src/index/ --type h
rg 'class SparqlParser' src/parser/ --type h

# Find related tests
find test -name "*Operation*" -type f
find test -name "*Index*" -type f

# Check file sizes
wc -l src/engine/Operation.h src/engine/QueryPlanner.h
```

---

## JTBD #5: Understand Contract Surface

```bash
# Read header file
head -150 src/engine/Operation.h

# Find all public methods
rg 'public:' src/engine/Operation.h -A50

# Check reference docs
cat docs/reference/api.md | head -50
cat docs/reference/sparql.md | head -50

# Find contract tests
rg 'TEST_F.*Operation' test/engine/ --type cpp | head -10
```

---

## JTBD #6: Validate Code Against Invariants

```bash
# List all 11 invariants
cat /home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md | grep "^## Invariant" -A3

# Check for monoidal composition (no rework)
rg 'monoidal|single-pass' docs/ --type md | grep -i "law\|composition"

# Run invariant validator
# Task tool with subagent_type=bb80-invariant-validator
Task: bb80-invariant-validator → Check output for PRESERVED or VIOLATED
```

---

## JTBD #7: Detect & Fix Regressions

```bash
# Build regression gate
cd /home/user/qlever && cmake --build build --target RegressionGate

# Run benchmark
./build/ingress_throughput > /tmp/current_perf.json 2>&1

# Run regression gate
./build/RegressionGate \
  --baseline benchmark/regression/baseline_performance.json \
  --current /tmp/current_perf.json

# Expected: EXIT CODE 0 (PASS)
# Thresholds: ±10% latency, ±5% cache hit rate
```

---

## JTBD #8: Run Deterministic Tests

```bash
# Run regression detector tests
ctest -R "RegressionDetector" --output-on-failure

# Run chaos/entropy tests
ctest -R "EntropyInjection" --output-on-failure

# Run all unit tests (core)
ctest -R "^[A-Z][a-zA-Z]*Test$" --output-on-failure

# Expected: All tests PASS (determinism verified)
```

---

## JTBD #9: Discover Specification by Phase

```bash
# PHASE 1: Spec Closure
rg 'Specification Closure.*CLOSED' docs/EPIC*.md

# PHASE 2: Invariant Validation
cat /home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md

# PHASE 3: Collision Detection (EPIC 9)
cat /home/user/qlever/docs/explanation/collision-detection-theory.md | head -50

# PHASE 4: Convergence (EPIC 9)
cat /home/user/qlever/docs/explanation/convergence-vs-consensus.md | head -50

# PHASE 5: Refactoring
cat /home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md | head -50

# PHASE 6: Closure
rg 'deterministic.*receipt|bb80-receipt-validator' docs/ --type md
```

---

## JTBD #10: Find Verification Proof for Claim

```bash
# Determinism Proof
grep -A5 "Determinism Proof" /home/user/qlever/README.md
ctest -R "RegressionDetector" --output-on-failure

# Monoidal Composition Proof
grep -A5 "Monoidal Composition Proof" /home/user/qlever/README.md
ctest -R "Operation" --output-on-failure

# Epoch Isolation Proof
ctest -R "EpochIntegrationTest" --output-on-failure

# Fail-Closed Enforcement Proof
ctest -R "EntropyInjection" --output-on-failure

# Conformance Proof
ctest -R "W3C|Shacl|N3" --output-on-failure

# All proofs
cat /home/user/qlever/README.md | grep "^### .* Proof"
```

---

## JTBD #11: Collision Detection (EPIC 9)

```bash
# After all 10 agents complete independently
# Invoke collision detector
# Task tool with subagent_type=bb80-collision-detector
Task: bb80-collision-detector \
  --artifacts agent1.json agent2.json ... agent10.json \
  → Output: collision_matrix.json

# Check collision matrix
cat collision_matrix.json | jq '.overlaps | length'

# Structural overlaps
cat collision_matrix.json | jq '.structural'

# Semantic overlaps
cat collision_matrix.json | jq '.semantic'

# Path divergences
cat collision_matrix.json | jq '.path_divergence'
```

---

## JTBD #12: Converge Solutions (EPIC 9)

```bash
# After collision detection, invoke convergence
# Task tool with subagent_type=bb80-convergence-orchestrator
Task: bb80-convergence-orchestrator \
  --artifacts agent1.json agent2.json ... agent10.json \
  --collision_matrix collision_matrix.json \
  → Output: final_artifact.json

# Verify final artifact
cat final_artifact.json | jq '.invariants | length'  # Should be 11+

# Check coverage
cat final_artifact.json | jq '.coverage_percentage'

# Validate all invariants
cat final_artifact.json | jq '.invariants_satisfied'
```

---

## JTBD #13: Generate Deterministic Proof

```bash
# Run all tests
ctest --output-on-failure 2>&1 | tee test_results.log

# Run regression gate
./build/RegressionGate \
  --baseline benchmark/regression/baseline_performance.json \
  --current /tmp/current_perf.json \
  --verbose > regression_report.json

# Generate receipt
cat > deterministic_receipt.json << 'EOF'
{
  "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "benchmarks": $(cat regression_report.json),
  "tests": $(grep "passed\|failed" test_results.log),
  "digests": {
    "code": "$(sha256sum $(git ls-files src/) | cut -d' ' -f1)"
  },
  "invariants_count": 11,
  "status": "deterministic"
}
EOF

# Validate receipt
# Task tool with subagent_type=bb80-receipt-validator
Task: bb80-receipt-validator \
  --receipt deterministic_receipt.json \
  → Output: RECEIPT_VALID or RECEIPT_INVALID
```

---

## JTBD #14: Commit Work to Git

```bash
# Stage changes
git add .

# Show what's staged
git diff --cached --stat

# Commit with message
git commit -m "feat(engine): Add feature X

Explain why this change is needed.
Preserves all 11 architectural invariants.
Regression gate: No regression detected.
EPIC 10.1 requirement."

# Push to feature branch
git push -u origin claude/<feature>-SESSION_ID

# Check CI gates
git log --oneline -1
# Output: Shows commit hash, CI gates will run automatically

# Monitor CI gates (in GitHub)
# All 18 gates must pass before merge
```

---

## QUICK LOOKUP TABLE

**By JTBD:**

| JTBD | Command | Output |
|------|---------|--------|
| #1 | `cat /home/user/qlever/CLAUDE.md | head -100` | Operational model |
| #2 | `Task: bb80-specification-validator` | CLOSED or INCOMPLETE |
| #3 | `make build && ctest -N` | Build success + test count |
| #4 | `rg 'class Operation' src/engine/` | File path |
| #5 | `cat src/engine/Operation.h \| head -150` | Public interface |
| #6 | `Task: bb80-invariant-validator` | PRESERVED or VIOLATED |
| #7 | `./build/RegressionGate --baseline ... --current ...` | EXIT 0 (PASS) or 1 (FAIL) |
| #8 | `ctest -R "RegressionDetector"` | All tests PASS |
| #9 | `rg 'Specification Closure.*CLOSED' docs/EPIC*.md` | Spec file path |
| #10 | `ctest -R "Determinism"` | Test PASS or FAIL |
| #11 | `Task: bb80-collision-detector` | collision_matrix.json |
| #12 | `Task: bb80-convergence-orchestrator` | final_artifact.json |
| #13 | `Task: bb80-receipt-validator` | RECEIPT_VALID or INVALID |
| #14 | `git push -u origin claude/<feature>-SESSION_ID` | CI gates pass/fail |

**By Tool:**

| Tool | JTBD | Command |
|------|------|---------|
| cat | #1, #9 | `cat /path/to/doc` |
| rg | #4, #9, #10 | `rg 'pattern' docs/ --type md` |
| make | #3 | `make build` |
| ctest | #3, #8, #10 | `ctest -R "pattern"` |
| RegressionGate | #7 | `./build/RegressionGate ...` |
| bb80-specification-validator | #2 | `Task: bb80-specification-validator` |
| bb80-invariant-validator | #6 | `Task: bb80-invariant-validator` |
| bb80-collision-detector | #11 | `Task: bb80-collision-detector` |
| bb80-convergence-orchestrator | #12 | `Task: bb80-convergence-orchestrator` |
| bb80-receipt-validator | #13 | `Task: bb80-receipt-validator` |
| git | #14 | `git add . && git commit && git push` |

---

## COMMAND PATTERNS

**Pattern: Find something**
```bash
rg 'keyword' docs/ --type md | head -20
rg 'keyword' src/ --type h | head -20
find test -name "*Pattern*" -type f
```

**Pattern: Verify something**
```bash
ctest -R "test_name" --output-on-failure
./RegressionGate --baseline baseline.json --current current.json
Task: bb80-invariant-validator
```

**Pattern: Read something**
```bash
cat /path/to/file | head -50        # First 50 lines
cat /path/to/file | grep "keyword"  # Search within
wc -l /path/to/file                 # Line count
```

**Pattern: Commit something**
```bash
git add files...
git commit -m "type(scope): description"
git push -u origin claude/<feature>-SESSION_ID
```

---

## EXIT CODES

**Expected success codes:**
- `0` → Operation succeeded
- `ctest PASSED` → All tests passed
- `RegressionGate EXIT 0` → No regression detected
- `Receipt VALID` → Deterministic proof accepted

**Expected failure codes:**
- `1` → Operation failed (needs fixing)
- `ctest FAILED` → Tests failed (rewrite code)
- `RegressionGate EXIT 1` → Regression detected (optimize)
- `Receipt INVALID` → Proof failed (gather more evidence)

---

## NEXT STEPS (FOR AGENT)

**After understanding this cheat sheet:**
1. Read full JTBD framework: `docs/AGENT_JTBD_FRAMEWORK.md`
2. Start with JTBD #1: Understand Architecture
3. Follow JTBDs in order (no skipping phases)
4. Use commands as templates (copy-paste ready)
5. All commands are deterministic (same input → same output)

**If command fails:**
- Check if prerequisites are met
- Read full JTBD description for context
- Do not skip phases
- Do not iterate (specification closure is prerequisite)

---

## REFERENCES

**Full Framework:** `docs/AGENT_JTBD_FRAMEWORK.md`
**Agent Guide:** `docs/AGENT_DOCUMENTATION_GUIDE.md`
**Quick Commands:** `docs/how-to/quick-start.md`
**Authority:** `/home/user/qlever/CLAUDE.md`
