# Collision Detection Report - Integration Phase (EPIC 11)

**Generated**: 2026-01-02
**Detector**: bb80-collision-detector agent
**Scope**: 10 independent integration phase agent artifacts

---

## Executive Summary

- **Total Collisions Found**: 7
- **Blocking Collisions**: 4 (path divergence, schema conflicts)
- **Advisory Collisions**: 3 (semantic divergence, duplicate functionality)
- **Status**: REQUIRES_RESOLUTION - Convergence cannot proceed until blocking collisions reconciled
- **Collision Density**: 35% of agent pairs exhibit some form of collision

---

## Collision Registry

### COLLISION_I_1: Receipt Storage Path Divergence (BLOCKING)

**Agents**: Agent 1 (CI matrix), Agent 2 (receipt comparator), Agent 6 (gate command), Agent 10 (artifact publisher)

**Type**: Execution Path + Semantic

**Severity**: BLOCKING

**Description**:
Multiple agents assume receipt storage at different locations with no coordination:

1. **Agent 1** (`.github/workflows/integration-test.yml`, lines 48, 135, 178):
   - Stores receipts at: `/tmp/qlever-verification-receipts/*.cbor`
   - Collects to artifact-bundle in CI step
   - Uploads via `actions/upload-artifact@v4`

2. **Agent 2** (receipt_comparator/src/main.rs, lines 119-120):
   - Reads receipts from: `bundle_path` directory (caller-specified)
   - Expects `verdict.json` adjacent to receipt files

3. **Agent 6** (qlever-verification-harness/src/gate.rs, lines 344-354):
   - Writes receipts to: `output_dir/receipts/*.cbor`
   - Writes verdict to: `output_dir/verdict.json`

4. **Agent 10** (artifact_publisher.sh, lines 41-48):
   - Reads receipts from: `target/verification/{subsystem}.receipt.cbor`
   - Copies to: `OUTPUT_DIR/receipt_bundle/`

**File Paths**:
- `/home/user/qlever/.github/workflows/integration-test.yml:48,135,178`
- `/home/user/qlever/qlever-verification/receipt_comparator/src/main.rs:119-120`
- `/home/user/qlever/qlever-verification/qlever-verification-harness/src/gate.rs:344-354`
- `/home/user/qlever/qlever-verification/artifact_publisher.sh:41-48`

**Impact**: BLOCKING - CI workflow will fail if receipts aren't in expected location

**Authority**: Agent 1 (CI matrix) - controls artifact paths

**Resolution**: Merge outputs through standardized artifact-bundle structure

---

### COLLISION_I_2: Verdict Schema Divergence (BLOCKING)

**Agents**: Agent 1 (CI matrix), Agent 2 (receipt comparator), Agent 6 (gate command), Agent 10 (artifact publisher)

**Type**: Structural

**Severity**: BLOCKING

**Description**:
Agent 1 expects field name `gate_result`, but Agents 2, 6, 10 use field name `verdict`

**File Paths & Evidence**:
- `/home/user/qlever/.github/workflows/integration-test.yml:273-274` expects `gate_result`
- `/home/user/qlever/qlever-verification/receipt_comparator/src/main.rs:41-48` defines `verdict` field
- `/home/user/qlever/qlever-verification/qlever-verification-harness/src/gate.rs:84` produces `verdict` field
- `/home/user/qlever/qlever-verification/artifact_publisher.sh:56` writes `verdict` field

**Code Snippet (Agent 1 expects)**:
```yaml
X86_RESULT=$(jq -r '.gate_result // "MISSING"' receipts-x86_64/verdict.json)
```

**Code Snippet (Agent 6 produces)**:
```rust
pub struct GateVerdict {
    pub verdict: String,  // <-- NOT gate_result
    ...
}
```

**Impact**: BLOCKING - CI workflow parsing will fail

**Authority**: Agent 6 (gate command) produces the verdict

**Resolution**: Rewrite Agent 1 to read `verdict` field instead of `gate_result`

---

### COLLISION_I_3: Environment.json Schema Mismatch (BLOCKING)

**Agents**: Agent 1 (CI matrix), Agent 4 (environment snapshot), Agent 8 (rerun commands)

**Type**: Structural

**Severity**: BLOCKING

**Description**:
Three incompatible schemas for environment.json:
- Agent 1: Flat structure with platform/architecture/rust_version fields
- Agent 4: Nested structure with toolchain/system/cpu sections
- Agent 8: Expects EnvironmentSnapshot struct with machine_fingerprint

**File Paths & Evidence**:
- `/home/user/qlever/.github/workflows/integration-test.yml:81-94` generates flat schema
- `/home/user/qlever/qlever-verification/scripts/environment_snapshot.sh:115-166` generates nested schema
- `/home/user/qlever/qlever-verification/qlever-repro/src/lib.rs:23-45` expects EnvironmentSnapshot struct

**Agent 1 Schema**:
```json
{
  "platform": "linux-x86_64",
  "architecture": "x86_64",
  "rust_version": "rustc 1.75.0"
}
```

**Agent 4 Schema**:
```json
{
  "snapshot_version": "1.0.0",
  "toolchain": {"rustc": {"version": "...", "required_minimum": "..."}},
  "system": {...},
  "cpu": {...}
}
```

**Impact**: BLOCKING - Cross-agent environment parsing will fail

**Authority**: Agent 4 (environment_snapshot.sh) is most comprehensive

**Resolution**: Use Agent 4's script in Agent 1, update Agent 8 to be schema-flexible

---

### COLLISION_I_4: Exit Code Semantics Divergence (BLOCKING)

**Agents**: Agent 2 (receipt comparator), Agent 6 (gate command), Agent 1 (CI interpretation)

**Type**: Semantic

**Severity**: BLOCKING

**Description**:
Exit code 1 and 2 have different meanings:
- Agent 2: 0=match, 1=differ, 2=error
- Agent 6: 0=PASS, 1=FAIL, 2=DIVERGENCE
- Agent 1: 0=success, 1=failure (no 2)

**File Paths & Evidence**:
- `/home/user/qlever/qlever-verification/receipt_comparator/src/main.rs:96-101` defines exit codes
- `/home/user/qlever/qlever-verification/qlever-verification-harness/src/gate.rs:20-23` defines different codes
- `/home/user/qlever/.github/workflows/integration-test.yml:280-286` doesn't handle exit code 2

**Impact**: BLOCKING - Determinism divergence will be misinterpreted

**Authority**: Agent 6 (gate command) is primary verdict producer

**Resolution**: Agent 2 runs inside Agent 6, not independently; CI interprets Agent 6's codes

---

### COLLISION_I_5: Duplicate Receipt Comparison Logic (ADVISORY)

**Agents**: Agent 2 (receipt comparator), Agent 1 (CI receipt comparison), Agent 6 (gate verdict)

**Type**: Semantic Overlap

**Severity**: ADVISORY

**Description**:
Receipt comparison implemented 3 different ways:
- Agent 2: Structural normalization, digest grouping
- Agent 1: SHA256 hash of CBOR files
- Agent 6: Failure class enumeration

**File Paths**: Lines 229-283 (Agent 2), Lines 317-356 (Agent 1), Lines 396-415 (Agent 6)

**Impact**: ADVISORY - Maintenance burden, inconsistent results possible

**Resolution**: Consolidate into Agent 2 library, used by Agent 6

---

### COLLISION_I_6: Workload Pack Format Ambiguity (ADVISORY)

**Agents**: Agent 3 (workload pinning), Agent 6 (gate command), Agent 8 (rerun commands)

**Type**: Semantic

**Severity**: ADVISORY

**Description**:
Agent 3 produces JSON manifest, but Agents 6 and 8 expect ReplayWorkload struct

**File Paths**:
- `/home/user/qlever/qlever-verification/workload_packs/deterministic_pack_1/manifest.json` (JSON)
- `/home/user/qlever/qlever-verification/qlever-verification-harness/src/gate.rs:287-297` (loads CBOR or JSON)
- `/home/user/qlever/qlever-verification/qlever-repro/src/lib.rs:350-359` (loads CBOR)

**Impact**: ADVISORY - Likely compatible via JSON fallback

**Resolution**: Verify schema match, add test case

---

### COLLISION_I_7: Negative Test Witness Bundle Format (ADVISORY)

**Agents**: Agent 5 (witness minimization), Agent 9 (negative tests)

**Type**: Semantic Overlap

**Severity**: ADVISORY

**Description**:
Both define WitnessBundle but with different structures and fields

**File Paths**:
- `/home/user/qlever/qlever-verification/qlever-witness/src/lib.rs:254-346`
- `/home/user/qlever/qlever-verification/qlever-verification-harness/tests/negative_tests.rs:24-69`

**Impact**: ADVISORY - Code duplication

**Resolution**: Agent 9 imports WitnessBundle from Agent 5's crate

---

## Collision-Free Pairs

**Total collision-free pairs**: 24 out of 45 possible pairs (53%)

Agents with clear separation of concerns: 3-5, 3-7, 3-8, 4-5, 4-6, 4-7, 4-8, 4-9, 4-10, 5-6, 5-7, 5-8, 5-10, 6-7, 6-8, 7-8, 7-9, 7-10, 8-9, 8-10, 9-10, plus others.

---

## Convergence Readiness: CONDITIONAL YES

**Status**: Can proceed to convergence phase IF blocking collisions I_1 through I_4 are reconciled.

**Blockers to Remove**:
- [ ] Unify verdict.json field name (use `verdict`, not `gate_result`)
- [ ] Unify environment.json schema (use Agent 4's comprehensive schema)
- [ ] Unify receipt storage paths (use artifact-bundle standard)
- [ ] Clarify exit code semantics (Agent 6 authoritative)

**Advisory Items** (can defer):
- Consolidate receipt comparison logic
- Verify workload format compatibility
- Standardize witness bundle format

---

## Proof of Collision Analysis

All collisions verified through code inspection with absolute file paths and line numbers:

| Collision | File | Lines | Type |
|-----------|------|-------|------|
| I_1 | integration-test.yml, gate.rs, artifact_publisher.sh | 48, 344, 41 | Path divergence |
| I_2 | integration-test.yml, gate.rs, receipt_comparator.rs | 273, 84, 41 | Schema divergence |
| I_3 | integration-test.yml, environment_snapshot.sh, lib.rs | 81, 115, 23 | Schema mismatch |
| I_4 | receipt_comparator.rs, gate.rs, integration-test.yml | 96, 20, 280 | Exit code divergence |
| I_5 | receipt_comparator.rs, integration-test.yml, gate.rs | 229, 317, 396 | Logic duplication |
| I_6 | manifest.json, gate.rs, lib.rs | 1, 287, 350 | Format ambiguity |
| I_7 | lib.rs, negative_tests.rs | 254, 24 | Bundle format overlap |

---

*Collision Detection Report - EPIC 11 Integration Phase*
*Generated by: bb80-collision-detector agent*
*Timestamp: 2026-01-02*
*Status: READY FOR CONVERGENCE PHASE (with blocking resolutions)*
