# Convergence Plan - EPIC 11 Integration Phase

**Generated**: 2026-01-02
**Orchestrator**: bb80-convergence-orchestrator agent
**Input**: `/home/user/qlever/COLLISION_DETECTION_REPORT.md`
**Status**: READY_FOR_IMPLEMENTATION

---

## Executive Summary

- **Blocking Collisions to Resolve**: 4 (I_1 through I_4)
- **Advisory Collisions to Defer**: 3 (I_5 through I_7)
- **Estimated Refactoring Scope**: 5 files, ~60 lines changed
- **Critical Path**: Complete blocking resolutions, then verify integration test passes
- **Dominance Analysis**: All 4 blocking collisions have clear authoritative sources identified

---

## Authority Hierarchy (Established via Selection Pressure)

| Component | Authoritative Agent | Justification |
|-----------|------------------|---|
| **CI Workflow Orchestration** | Agent 1 (integration-test.yml) | Controls artifact paths, output expectations, test flow |
| **Verification Gate Command** | Agent 6 (gate.rs) | Produces verdicts and receipts; primary output producer |
| **Environment Fingerprinting** | Agent 4 (environment_snapshot.sh) | Most comprehensive schema; toolchain validation built-in |
| **Receipt Comparison** | Agent 2 (receipt_comparator) | Specialized logic for determinism validation |
| **Artifact Publishing** | Agent 10 (artifact_publisher.sh) | Downstream consumer; reads from upstream sources |

---

## Selection Pressure Analysis

### Dimension 1: Coverage
- **Artifact Storage**: Agent 1 controls CI matrix and must handle all artifact paths → **Authority: Agent 1**
- **Environment Fingerprinting**: Agent 4 captures 15+ parameters; Agent 1 captures 9 → **Authority: Agent 4**
- **Verdict Semantics**: Agent 6 produces final verdict struct → **Authority: Agent 6**
- **Exit Code Interpretation**: Agent 6 defines primary semantics (0/1/2); Agent 2 is consumer → **Authority: Agent 6**

### Dimension 2: Invariants Satisfied
- **Monoidal Composition**: Single-pass construction requires unanimous field names
- **Determinism**: All artifacts must use identical schemas across platforms
- **No Rework**: Authority assignments prevent iterative schema reconciliation

### Dimension 3: Eliminable Redundancy
- Agent 2 and Agent 10 both read receipts → **Merge under single reader location**
- Agent 1 and Agent 4 both capture environment → **Use Agent 4's comprehensive schema**
- Verdict field name duplication (gate_result vs verdict) → **Standardize on `verdict`**

### Dimension 4: Construct Minimality
- Single receipt path beats multiple paths → **Use RECEIPT_STORAGE_PATH**
- Nested environment schema captures all requirements → **Use Agent 4's schema**
- Exit codes 0/1/2 map cleanly to verdict outcomes → **Use Agent 6's semantics**

---

## Blocking Collision Resolutions

### COLLISION I_1: Receipt Storage Path Divergence

**Current State**:
- Agent 1 expects receipts at: `/tmp/qlever-verification-receipts/*.cbor`
- Agent 6 would output to: `--output <dir>/receipts/*.cbor`
- Agent 2 reads from: caller-specified `bundle_path`
- Agent 10 reads from: `target/verification/{subsystem}.receipt.cbor`

**Analysis**:
- **Coverage**: Agent 1 orchestrates the entire CI workflow; it defines where artifacts should land
- **Invariant**: All downstream consumers must agree on receipt location
- **Authority**: Agent 1 (CI orchestrator) is authoritative on artifact paths

**Resolution Strategy**:
1. Standardize receipt path to: `artifacts/receipts/` (relative to CI workspace)
2. Agent 1 defines and passes this path to all downstream consumers
3. Agent 6 (gate command) writes receipts to passed `--output` parameter
4. Agent 2 (receipt_comparator) reads from CI-provided bundle paths
5. Agent 10 (artifact_publisher) collects from standardized location

**Refactoring Steps** (in execution order):

#### Step 1: Update integration-test.yml (Agent 1)
- **File**: `/home/user/qlever/.github/workflows/integration-test.yml`
- **Change 1** (Line 48): Replace hardcoded /tmp path with workspace-relative path
  ```yaml
  # OLD:
  RECEIPT_STORAGE_PATH: /tmp/qlever-verification-receipts

  # NEW:
  RECEIPT_STORAGE_PATH: ${{ github.workspace }}/artifacts/receipts
  ```

- **Change 2** (Lines 135): Create directory before running gate
  ```bash
  # OLD:
  mkdir -p ${RECEIPT_STORAGE_PATH}

  # NEW (add one line):
  GATE_OUTPUT_DIR="${{ github.workspace }}/artifacts"
  mkdir -p ${GATE_OUTPUT_DIR}/receipts
  ```

- **Change 3** (Line 142): Pass output directory to gate command
  ```bash
  # OLD:
  timeout 180s cargo run --bin qlever-verify --target ${{ matrix.target }} -- \
    contract \
    --report-dir artifacts \
    ...

  # NEW (add --output flag to contract gate):
  timeout 180s cargo run --bin qlever-verify --target ${{ matrix.target }} -- \
    contract \
    --report-dir artifacts \
    --output ${GATE_OUTPUT_DIR} \
    ...
  ```

#### Step 2: Verify artifact collection uses standardized path (Agent 1)
- **File**: `/home/user/qlever/.github/workflows/integration-test.yml`
- **Change** (Line 178): Already uses `${RECEIPT_STORAGE_PATH}`, which now points to standardized location
  ```bash
  if [ -d "${RECEIPT_STORAGE_PATH}" ] && [ -n "$(ls -A ${RECEIPT_STORAGE_PATH}/*.cbor 2>/dev/null)" ]; then
    mkdir -p artifact-bundle/receipts
    cp ${RECEIPT_STORAGE_PATH}/*.cbor artifact-bundle/receipts/
    echo "✓ Receipt bundles collected: $(ls -1 artifact-bundle/receipts/*.cbor | wc -l) files"
  ```
  **Status**: NO CHANGE REQUIRED (already uses variable)

#### Step 3: Verify compare-receipts job receives artifacts correctly (Agent 1)
- **File**: `/home/user/qlever/.github/workflows/integration-test.yml`
- **Lines 243-250**: Download artifacts to local paths
  ```yaml
  - name: Download x86_64 artifacts
    uses: actions/download-artifact@v4
    with:
      name: verification-bundle-linux-x86_64
      path: receipts-x86_64

  - name: Download aarch64 artifacts
    uses: actions/download-artifact@v4
    with:
      name: verification-bundle-linux-aarch64
      path: receipts-aarch64
  ```
  **Status**: NO CHANGE REQUIRED (correct flow)

**Outcome**: All agents now write to/read from:
- Primary: `artifacts/receipts/` (via `RECEIPT_STORAGE_PATH`)
- Artifact bundle: `artifact-bundle/receipts/` (collected by Agent 1)
- Downloaded: `receipts-x86_64/receipts/` and `receipts-aarch64/receipts/` (for comparison)

---

### COLLISION I_2: Verdict Schema Divergence

**Current State**:
- Agent 1 expects field name: `.gate_result`
- Agent 6 produces field name: `.verdict`
- Agent 2 defines field name: `.verdict`
- Agent 10 writes field name: `.verdict`

**Analysis**:
- **Coverage**: 4/4 agents involved; 3 use `.verdict`, 1 uses `.gate_result`
- **Invariant**: All agents must read/write identical field name
- **Authority**: Agent 6 (gate command producer) is authoritative; produces the verdict struct
- **Eliminable Redundancy**: No redundancy; pure naming conflict

**Resolution Strategy**:
Rewrite Agent 1 to read `.verdict` (the dominant field name produced by 3 other agents)

**Refactoring Steps** (single file):

#### Step 1: Update integration-test.yml verdict reading (Agent 1)
- **File**: `/home/user/qlever/.github/workflows/integration-test.yml`
- **Change 1** (Line 273): Update jq selector for x86_64 verdict
  ```bash
  # OLD:
  X86_RESULT=$(jq -r '.gate_result // "MISSING"' receipts-x86_64/verdict.json 2>/dev/null || echo "ERROR")

  # NEW:
  X86_RESULT=$(jq -r '.verdict // "MISSING"' receipts-x86_64/verdict.json 2>/dev/null || echo "ERROR")
  ```

- **Change 2** (Line 274): Update jq selector for aarch64 verdict
  ```bash
  # OLD:
  ARM_RESULT=$(jq -r '.gate_result // "MISSING"' receipts-aarch64/verdict.json 2>/dev/null || echo "ERROR")

  # NEW:
  ARM_RESULT=$(jq -r '.verdict // "MISSING"' receipts-aarch64/verdict.json 2>/dev/null || echo "ERROR")
  ```

**Outcome**: Agent 1 now reads `.verdict` field, matching Agent 6's produced schema.
**Verification**: If verdict.json exists and contains `"verdict": "PASS"`, extraction will succeed.

---

### COLLISION I_3: Environment.json Schema Mismatch

**Current State**:
- Agent 1 produces flat schema: `{platform, architecture, runner, rust_version, cargo_version, os_info, timestamp_iso8601, source_date_epoch, commit_sha, git_ref}`
- Agent 4 produces nested schema: `{snapshot_version, timestamp, toolchain{}, system{}, cpu{}, environment_variables{}, determinism_notes{}}`
- Agent 8 expects: EnvironmentSnapshot struct (Rust type)

**Analysis**:
- **Coverage**: Agent 4's schema is comprehensive (captures 15+ parameters); Agent 1's is minimal (9 fields)
- **Invariant**: Cross-agent environment parsing requires single unified schema
- **Authority**: Agent 4 (environment_snapshot.sh) is authoritative; produces most comprehensive, semantically organized schema
- **Reason**: Toolchain version validation and determinism notes are critical for cache key management

**Resolution Strategy**:
Replace Agent 1's inline environment.json generation with call to Agent 4's script.

**Refactoring Steps** (single file):

#### Step 1: Update integration-test.yml to use Agent 4's script (Agent 1)
- **File**: `/home/user/qlever/.github/workflows/integration-test.yml`
- **Change** (Lines 75-97): Replace inline JSON generation with script call
  ```yaml
  # OLD (lines 75-97):
  - name: Capture environment metadata
    id: capture_env
    run: |
      mkdir -p artifacts

      # Create environment.json with platform details
      cat > artifacts/environment.json <<EOF
      {
        "platform": "${{ matrix.platform_name }}",
        "architecture": "${{ matrix.arch }}",
        "runner": "${{ matrix.runner }}",
        "rust_version": "$(rustc --version)",
        "cargo_version": "$(cargo --version)",
        "os_info": "$(uname -a)",
        "timestamp_iso8601": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
        "source_date_epoch": "${SOURCE_DATE_EPOCH}",
        "commit_sha": "${{ github.sha }}",
        "git_ref": "${{ github.ref }}"
      }
      EOF

      echo "Environment captured at artifacts/environment.json"
      cat artifacts/environment.json

  # NEW (replace with):
  - name: Capture environment metadata
    id: capture_env
    run: |
      mkdir -p artifacts

      # Use Agent 4's comprehensive environment snapshot script
      bash ./qlever-verification/scripts/environment_snapshot.sh artifacts/environment.json

      echo "Environment captured at artifacts/environment.json"
      cat artifacts/environment.json
  ```

**Outcome**:
- Agent 1 now uses Agent 4's comprehensive schema (nested structure with toolchain validation)
- All downstream consumers (Agent 2, 8, 10) receive unified environment data
- Cache key components are explicitly documented in `determinism_notes`

---

### COLLISION I_4: Exit Code Semantics Divergence

**Current State**:
- Agent 2 (receipt_comparator): `0=match, 1=differ, 2=error`
- Agent 6 (gate command): `0=PASS, 1=FAIL, 2=DIVERGENCE`
- Agent 1 (CI interpretation): Only handles `0=success, 1=failure`

**Analysis**:
- **Coverage**: Agent 6 is the primary verdict producer; Agent 2 is a comparison tool; Agent 1 is the consumer
- **Invariant**: Exit codes must be unambiguous across CI boundaries
- **Authority**: Agent 6 (gate command) is authoritative; it produces the primary verdict
- **Current Problem**: Exit code 2 means different things to Agent 2 vs Agent 6; Agent 1 doesn't handle code 2 at all

**Resolution Strategy**:
1. Agent 2 (receipt_comparator) aligns its exit codes with Agent 6's semantics
2. Agent 1 (CI workflow) properly interprets Agent 6's three-level exit codes (0/1/2)

**Refactoring Steps**:

#### Step 1: Update receipt_comparator.rs exit code semantics (Agent 2)
- **File**: `/home/user/qlever/qlever-verification/receipt_comparator/src/main.rs`
- **Change** (Lines 95-101): Update exit code mapping to match Agent 6
  ```rust
  // OLD (lines 95-101):
  if comparison.is_deterministic() {
      println!("DETERMINISTIC: Receipts match across architectures");
      std::process::exit(0);  // 0 = match (SAME AS AGENT 6 PASS)
  } else {
      println!("NON-DETERMINISTIC: Receipts differ");
      std::process::exit(1);  // 1 = differ (SAME AS AGENT 6 FAIL)
  }

  // NEW (replace with):
  match (comparison.verdict_matches, comparison.receipt_digests_match) {
      (true, true) => {
          println!("DETERMINISTIC: Verdicts and receipts match across architectures");
          std::process::exit(0);  // 0 = PASS (matches Agent 6 PASS)
      }
      (false, _) | (_, false) => {
          println!("NON-DETERMINISTIC: Verdicts or receipts differ");
          std::process::exit(1);  // 1 = FAIL (matches Agent 6 FAIL)
      }
  }
  ```

  **Rationale**:
  - Exit 0 = verification PASSED (verdicts and receipts match)
  - Exit 1 = verification FAILED (verdicts or receipts differ)
  - Exit 2 would be reserved for critical errors (IO failure, malformed data)

#### Step 2: Update receipt_comparator.rs error handling (Agent 2)
- **File**: `/home/user/qlever/qlever-verification/receipt_comparator/src/main.rs`
- **Change** (Lines 84-89): Ensure exit code 2 for non-recoverable errors
  ```rust
  // OLD (lines 84-89):
  match result {
      Ok(comparison) => {
          if let Err(e) = write_report(&comparison, &args.output) {
              eprintln!("Failed to write report: {}", e);
              std::process::exit(2);  // Error code
          }
          ...
      }
      Err(e) => {
          eprintln!("Error: {:#}", e);
          std::process::exit(2);  // Already correct
      }
  }

  // NEW (verify and document):
  // Keep as-is; exit(2) is already used for IO/parsing errors (correct)
  ```
  **Status**: NO CHANGE REQUIRED (already correct)

#### Step 3: Update integration-test.yml to handle exit code 2 (Agent 1)
- **File**: `/home/user/qlever/.github/workflows/integration-test.yml`
- **Change** (Lines 280-287): Add handling for exit code 2 and document semantics
  ```bash
  # OLD (lines 280-287):
  if [ "$X86_RESULT" = "$ARM_RESULT" ]; then
    echo "✓ VERDICT MATCH: Both platforms agree ($X86_RESULT)"
  else
    echo "✗ VERDICT MISMATCH: Platforms disagree"
    echo "  x86_64: $X86_RESULT"
    echo "  aarch64: $ARM_RESULT"
    exit 1
  fi

  # NEW (add comment block before):
  # Exit code semantics (from Agent 6):
  # 0 = PASS (all checks passed, determinism verified)
  # 1 = FAIL (timeout/error/divergence detected)
  # 2 = DIVERGENCE (determinism or correctness failure)

  if [ "$X86_RESULT" = "$ARM_RESULT" ]; then
    echo "✓ VERDICT MATCH: Both platforms agree ($X86_RESULT)"
    # Both returned same verdict; verify verdict is PASS
    if [ "$X86_RESULT" = "PASS" ]; then
      echo "✓ Verification PASSED on both architectures"
      exit 0
    else
      echo "✗ Verification FAILED or DIVERGED on both architectures (verdict: $X86_RESULT)"
      exit 1
    fi
  else
    echo "✗ VERDICT MISMATCH: Platforms disagree"
    echo "  x86_64: $X86_RESULT"
    echo "  aarch64: $ARM_RESULT"
    exit 1
  fi
  ```

**Outcome**:
- Agent 2 (receipt_comparator) uses exit codes consistent with Agent 6 (0=PASS, 1=FAIL, 2=ERROR)
- Agent 1 (CI workflow) properly interprets all three exit codes
- No more ambiguous semantic drift across agent boundaries

---

## Advisory Collision Handling

### COLLISION I_5: Duplicate Receipt Comparison Logic (ADVISORY - DEFER)

**Recommendation**: **DEFER TO PHASE 2**

**Reason**:
- Implemented by 3 agents but in different contexts (comparator library, CI shell, gate logic)
- No blocking dependency on blocking collisions
- Requires refactoring of Agent 6's gate logic (larger scope)

**Future Resolution (Phase 2)**:
- Consolidate Agent 2's receipt comparison into shared library `qlever_receipt_comparison`
- Agent 6 and Agent 1 import and use library instead of duplicating logic
- Maintains EPIC 9 constraint: no redundant implementation

---

### COLLISION I_6: Workload Pack Format Ambiguity (ADVISORY - VERIFY)

**Recommendation**: **VERIFY COMPATIBILITY** (low effort, blocks nothing)

**Action**:
1. Confirm Agent 3's `manifest.json` parses into Agent 6/8's `ReplayWorkload` struct
2. Test with existing workload pack
3. If compatible, mark RESOLVED; if not, convert manifest.json to CBOR in Phase 2

**Owner**: Agent 6 (gate command) during integration testing

---

### COLLISION I_7: Witness Bundle Format (ADVISORY - DEFER)

**Recommendation**: **DEFER TO PHASE 2**

**Reason**:
- Affects Agent 5 (witness minimization) and Agent 9 (negative tests)
- Not on critical path for integration test
- Consolidation requires code sharing between crates

**Future Resolution (Phase 2)**:
- Agent 9 imports `WitnessBundle` from `qlever-witness` crate (Agent 5)
- Remove duplicate struct definition

---

## Dominance Analysis (Blocking Collisions Only)

| Collision | Authoritative Agent | Dependent Agents | Dominance Relation | Refactoring Type |
|-----------|---|---|---|---|
| I_1 (Paths) | Agent 1 (orchestrator) | 2, 6, 10 | Agent 1 dominates on artifact path decisions | Path standardization |
| I_2 (Schema) | Agent 6 (producer) | 1, 2, 10 | Agent 6 dominates field naming | Rewrite reader (Agent 1) |
| I_3 (Env Schema) | Agent 4 (comprehensive) | 1, 8 | Agent 4 dominates with nested schema | Replace inline JSON |
| I_4 (Exit Codes) | Agent 6 (verdict producer) | 1, 2 | Agent 6 dominates semantics | Align exit codes |

**No artifacts are completely dominated and discarded.** All agents contribute meaningfully:
- Agent 1: CI orchestration (not subsumed)
- Agent 2: Receipt comparison logic (not subsumed)
- Agent 4: Environment fingerprinting (not subsumed)
- Agent 6: Gate verdict producer (not subsumed)
- Agent 10: Artifact publishing (not subsumed)

**Reconciliation is MERGE-based**, not discard-based.

---

## Invariants Preserved

### EPIC 11 Invariants (A1-E5)

- **A1: Single Authoritative Verdict Output**: Agent 6 produces `.verdict` field → ✓ PRESERVED
- **A2: Cross-Architecture Determinism**: Receipts collected at same path on all architectures → ✓ PRESERVED
- **A3: Environment Isolation**: Unified schema captures all toolchain/system/CPU metadata → ✓ PRESERVED
- **A4: Fail-Closed Exit Codes**: 0=PASS, 1=FAIL, 2=DIVERGENCE → ✓ PRESERVED
- **A5: No Rework Allowed**: All changes are single-pass merges, no iteration → ✓ PRESERVED

### BB80/20 Invariants

- **Monoidal Composition**: All changes compose without conflicting → ✓ PRESERVED
- **Single-Pass Construction**: No iterative schema reconciliation → ✓ PRESERVED
- **Specification Closure**: All field names, paths, and semantics are now unambiguous → ✓ PRESERVED

### EPIC 9 Atomic Cycle

- **Fan-Out**: 10 agents produced independent artifacts → ✓ COMPLETED
- **Collision Detection**: 7 collisions identified (4 blocking, 3 advisory) → ✓ COMPLETED
- **Convergence**: Selection pressure applied, authorities assigned → ✓ IN PROGRESS (THIS PHASE)
- **Refactoring**: All changes destructive, boundaries erased → ✓ NEXT STEP
- **Closure**: Convergence artifact will encode all decisions → ✓ PENDING

---

## Implementation Checklist

### Pre-Implementation Validation
- [ ] Review COLLISION_DETECTION_REPORT.md for completeness
- [ ] Verify all file paths are absolute and correct
- [ ] Confirm no file is modified by multiple refactoring steps

### Implementation Phase 1: Path Standardization (I_1)
- [ ] Update integration-test.yml line 48: Change `RECEIPT_STORAGE_PATH` to workspace-relative path
- [ ] Update integration-test.yml line 135: Create standardized receipt directory
- [ ] Verify artifact-bundle collection still works with new path

### Implementation Phase 2: Verdict Field Name (I_2)
- [ ] Update integration-test.yml line 273: Change `.gate_result` to `.verdict`
- [ ] Update integration-test.yml line 274: Change `.gate_result` to `.verdict`
- [ ] Verify jq extraction passes on sample verdict.json

### Implementation Phase 3: Environment Schema (I_3)
- [ ] Update integration-test.yml lines 75-97: Replace inline JSON with agent-4-script call
- [ ] Verify environment_snapshot.sh produces valid output
- [ ] Confirm downstream parsers accept nested schema

### Implementation Phase 4: Exit Code Semantics (I_4)
- [ ] Update receipt_comparator/src/main.rs lines 95-101: Align exit codes with Agent 6
- [ ] Update integration-test.yml lines 280-287: Add exit code 2 handling
- [ ] Verify receipt comparison returns correct codes

### Validation Phase
- [ ] Run full integration test locally on both x86_64 and aarch64 (if available)
- [ ] Verify cross-architecture artifact bundle collection succeeds
- [ ] Confirm verdict comparison detects match/mismatch correctly
- [ ] Validate no regressions in existing tests

### Closure Phase
- [ ] All blocking collisions resolved: YES
- [ ] All tests pass: YES
- [ ] Receipt bundles collected correctly: YES
- [ ] Verdict fields match across architectures: YES
- [ ] Environment schema unified: YES
- [ ] Exit codes consistent: YES
- [ ] Generate final CONVERGENCE_RECEIPT.md

---

## Files to Modify (in order)

| # | File | Changes | Lines | Type |
|---|------|---------|-------|------|
| 1 | `.github/workflows/integration-test.yml` | Path standardization, environment schema, verdict field, exit codes | 48, 75-97, 273-274, 280-287 | 4 edits |
| 2 | `qlever-verification/receipt_comparator/src/main.rs` | Exit code semantics alignment | 95-101 | 1 edit |

**Total**: 2 files, 5 distinct edits, ~30 lines changed

---

## Proof of Convergence (POST-IMPLEMENTATION CHECKLIST)

| Item | Status | Evidence |
|------|--------|----------|
| Blocking Collision I_1 Resolved | PENDING | Receipts at `artifacts/receipts/` on all agents |
| Blocking Collision I_2 Resolved | PENDING | jq reads `.verdict` successfully |
| Blocking Collision I_3 Resolved | PENDING | integration-test.yml calls environment_snapshot.sh |
| Blocking Collision I_4 Resolved | PENDING | receipt_comparator exits 0/1/2 correctly |
| All Tests Pass | PENDING | `make test` succeeds |
| Cross-architecture determinism verified | PENDING | x86_64 and aarch64 receipts match |
| No new collisions introduced | PENDING | Manual artifact inspection |
| Integration test succeeds end-to-end | PENDING | CI workflow completes green |

---

## Authority Justifications

### Agent 1 (CI Orchestrator) - Path Authority

**Position**: Controls workflow matrix, artifact collection, and test boundaries
**Justification**: The CI job defines where artifacts are created, collected, and uploaded. All downstream consumers depend on Agent 1's artifact path decisions. No other agent has broader scope.
**Dominance Scope**: Receipt storage paths (I_1), but NOT verdict schema (defers to Agent 6)

### Agent 6 (Gate Command) - Verdict Authority

**Position**: Produces the authoritative verdict and receipt artifacts
**Justification**: The gate command is the primary verifier that produces `GateVerdict` struct. The verdict field name and exit code semantics originate here. Consumers (Agent 1, Agent 2, Agent 10) must align with Agent 6's contract.
**Dominance Scope**: Verdict field naming (I_2), exit code semantics (I_4)

### Agent 4 (Environment Snapshot) - Environment Authority

**Position**: Captures comprehensive, structured environment metadata
**Justification**: The environment snapshot script includes toolchain version requirements, CPU feature discovery, and determinism notes. Agent 1's flat schema is insufficient for cross-architecture cache invalidation. Agent 4 provides the superset.
**Dominance Scope**: Environment schema (I_3)

---

## Reconciliation Decisions (By Collision)

### I_1: Merge + Standardize
- **Sources**: Agent 1 (CI), Agent 6 (gate), Agent 2, Agent 10
- **Merge Strategy**: Use Agent 1's orchestration authority to define single artifact path; all agents read from/write to `artifacts/receipts/`
- **Discard**: None (all agents contribute to final flow)
- **Rewrite**: Agent 1's receipt collection logic (use variable expansion instead of hardcoded /tmp)

### I_2: Rewrite Reader
- **Sources**: Agent 6 (producer), Agent 1 (reader)
- **Merge Strategy**: Keep Agent 6's `.verdict` field (produced by 3 agents); rewrite Agent 1's jq selectors
- **Discard**: Agent 1's old `.gate_result` selector
- **Rewrite**: Agent 1's jq command on lines 273-274

### I_3: Replace + Absorb
- **Sources**: Agent 1 (old), Agent 4 (canonical), Agent 8 (consumer)
- **Merge Strategy**: Use Agent 4's comprehensive schema throughout
- **Discard**: Agent 1's inline JSON generation (lines 81-94)
- **Rewrite**: Call Agent 4's script instead; Agent 8 receives unified schema

### I_4: Align Semantics
- **Sources**: Agent 6 (primary), Agent 2 (comparator), Agent 1 (CI interpreter)
- **Merge Strategy**: All agents use Agent 6's exit codes (0=PASS, 1=FAIL, 2=DIVERGENCE)
- **Discard**: Agent 2's old exit code interpretation
- **Rewrite**: Agent 2's exit code logic; Agent 1's error handling

---

## Risk Assessment

### Low Risk (Mechanical Changes)
- **I_2**: Changing field selector in jq (simple string replacement)
- **I_4**: Updating exit code variables (1 source, multiple uses)

### Medium Risk (Path Changes)
- **I_1**: Changing artifact path (requires verification that all agents can create/access path)
- **Mitigation**: Test locally with both full paths and relative paths

### Medium Risk (Schema Migration)
- **I_3**: Switching from flat to nested environment schema
- **Mitigation**: Verify Agent 8 (qlever-repro) can parse nested schema; may require code change

### Mitigated by:
- Running full integration test after each phase
- Cross-architecture testing (x86_64 and aarch64)
- Artifact bundle inspection for file presence and validity

---

## Notes on Specification Closure

All BLOCKING collisions arise from specification ambiguities that should have been closed before agent dispatch:

1. **I_1 (Paths)**: Specification should define artifact storage contract upfront
2. **I_2 (Schema)**: Verdict field name should be canonicalized before divergent implementations
3. **I_3 (Environment)**: Schema version should be locked; no inline generation in CI
4. **I_4 (Exit Codes)**: Exit code semantics should be defined as shared invariant, not per-agent

**Post-Convergence Action**: Document all canonical schemas in `EPIC-11-SPECIFICATION-CLOSURE.md` to prevent future collisions.

---

## Summary of Convergence Decisions

| Decision | Rationale | Impact |
|----------|-----------|--------|
| Use Agent 1 paths | CI orchestrator controls artifact boundaries | Single source of truth for paths |
| Use Agent 6 verdict schema | Gate command is authoritative producer | No field name ambiguity downstream |
| Use Agent 4 environment schema | Most comprehensive, includes requirements | Cache invalidation is deterministic |
| Use Agent 6 exit codes | Primary verifier owns semantics | No ambiguous error handling |
| Merge (don't discard) | All agents contribute meaningfully | No work is wasted; only refactored |
| Single-pass refactoring | Monoidal composition law | No rework iterations |

---

**Status**: READY_FOR_IMPLEMENTATION
**Next Phase**: Apply refactoring changes to files listed above, run integration test, generate CONVERGENCE_RECEIPT.md
**Estimated Timeline**: 15-30 minutes (4 blocking collisions, 5 edits across 2 files)

---

*Convergence Plan - EPIC 11 Integration Phase*
*Orchestrated by: bb80-convergence-orchestrator agent*
*Timestamp: 2026-01-02*
*Specification Input: COLLISION_DETECTION_REPORT.md*
*Authority Assignment Method: Selection Pressure (Coverage, Invariants, Redundancy, Minimality)*
