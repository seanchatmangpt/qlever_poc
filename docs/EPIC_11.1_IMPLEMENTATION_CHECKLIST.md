# EPIC 11.1 Implementation Checklist

**Convergence Agent**: bb80-convergence-orchestrator
**Timestamp**: 2026-01-02T22:30:00Z
**Status**: DETERMINISTIC EXECUTION GUIDE
**Target Audience**: Implementation Team

---

## Overview

This checklist provides deterministic step-by-step execution instructions for EPIC 11.1 workspace restructuring. All steps are mechanical with binary pass/fail criteria. No judgment calls required.

**Total Estimated Time**: 10-12 hours (1-2 working days)
**Rollback Strategy**: Fail-closed (any failure → full rollback)
**Validation Gates**: 28 total (5 pre-migration + 21 phase gates + 2 post-migration)

---

## PRE-MIGRATION: Preparation & Baseline Capture

### Pre-Gate 1: Environment Verification

**Objective**: Verify execution environment is ready

**Actions**:
```bash
# Verify Rust toolchain
rustc --version
# Expected: rustc 1.91.1 or newer

# Verify cargo installed
cargo --version
# Expected: cargo 1.91.1 or newer

# Verify git is clean
cd /home/user/qlever
git status
# Expected: clean working tree on branch claude/epic-11-1-closure-*

# Verify backup directory exists
mkdir -p /tmp/epic11.1-backups
```

**Pass Criteria**: ✅ All commands succeed (exit code 0)
**Failure Action**: ❌ Install missing tools, retry gate

---

### Pre-Gate 2: Baseline Testing

**Objective**: Capture baseline test results before migration

**Actions**:
```bash
# Test current qlever-verification workspace
cd /home/user/qlever/qlever-verification
cargo test --workspace --all-features > /tmp/epic11.1-baseline-tests.txt 2>&1

# Capture exit code
echo $? > /tmp/epic11.1-baseline-test-status.txt

# Count test results
grep -E "(test result:|passed)" /tmp/epic11.1-baseline-tests.txt
```

**Pass Criteria**: ✅ Exit code 0, all tests pass
**Failure Action**: ❌ DO NOT PROCEED - Fix failing tests first

---

### Pre-Gate 3: Baseline Artifact Capture

**Objective**: Generate deterministic baseline for parity validation

**Actions**:
```bash
cd /home/user/qlever/qlever-verification

# Clean build
cargo clean
cargo build --workspace --release

# Capture source hashes
find . -name "*.rs" -type f | sort | xargs blake3 > /tmp/epic11.1-baseline-source-hashes.txt

# Capture dependency tree
cargo tree --workspace --edges no-dev > /tmp/epic11.1-baseline-dep-tree.txt

# Capture package count
cargo metadata --format-version=1 | jq '.packages | length' > /tmp/epic11.1-baseline-package-count.txt

# Capture binary artifacts
find target/release -type f -executable | sort > /tmp/epic11.1-baseline-artifacts.txt
```

**Pass Criteria**: ✅ All files generated successfully
**Failure Action**: ❌ Retry capture commands

---

### Pre-Gate 4: Backup Creation (CRITICAL)

**Objective**: Create fail-safe rollback point

**Actions**:
```bash
cd /home/user/qlever

# Create timestamped backup
BACKUP_NAME="rust-backup-$(date +%s).tar.gz"
tar -czf "/tmp/epic11.1-backups/${BACKUP_NAME}" rust/ qlever-verification/ wasm/

# Verify backup created
ls -lh "/tmp/epic11.1-backups/${BACKUP_NAME}"

# Save backup path for rollback
echo "/tmp/epic11.1-backups/${BACKUP_NAME}" > /tmp/epic11.1-backup-path.txt
```

**Pass Criteria**: ✅ Backup file exists and is >10MB
**Failure Action**: ❌ Retry backup creation

---

### Pre-Gate 5: Feature Branch Creation

**Objective**: Isolate migration work from main branch

**Actions**:
```bash
cd /home/user/qlever

# Create feature branch
git checkout -b epic11.1-workspace-restructure

# Verify branch created
git branch --show-current
# Expected: epic11.1-workspace-restructure
```

**Pass Criteria**: ✅ On correct feature branch
**Failure Action**: ❌ Delete branch, retry

---

## PHASE 1: Workspace Root Creation (1 hour)

### Phase 1.1: Create Workspace Cargo.toml

**Actions**:
```bash
cd /home/user/qlever

# Create rust/ directory if doesn't exist
mkdir -p rust

# Create workspace Cargo.toml
cat > rust/Cargo.toml << 'EOF'
[workspace]
resolver = "2"

members = [
    # Three public packages (first-class products)
    "qleverest",
    "qleverest-validation",
    "qleverest-wasm",

    # Internal verification subsystems (under qleverest-validation)
    "qleverest-validation/crates/qlever-kernel-runner",
    "qleverest-validation/crates/qlever-artifact-capture",
    "qleverest-validation/crates/qlever-digest-verifier",
    "qleverest-validation/crates/qlever-cache-verifier",
    "qleverest-validation/crates/qlever-replay-verifier",
    "qleverest-validation/crates/qlever-regression-verifier",
    "qleverest-validation/crates/qlever-epoch-verifier",
    "qleverest-validation/crates/qlever-simd-verifier",
    "qleverest-validation/crates/qlever-chaos-verifier",
    "qleverest-validation/crates/qlever-verification-harness",
    "qleverest-validation/crates/receipt_comparator",
    "qleverest-validation/crates/qlever-repro",
    "qleverest-validation/crates/qlever-witness",
]

[workspace.package]
version = "0.1.0"
edition = "2021"
rust-version = "1.91.1"
authors = ["QLever Contributors"]
license = "Apache-2.0"
repository = "https://github.com/seanchatmangpt/qlever"

[workspace.dependencies]
# Serialization
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
ciborium = "0.2"

# Hashing
blake3 = "1.5"

# Error Handling
thiserror = "1.0"
anyhow = "1.0"

# Async Runtime
tokio = { version = "1.0", features = ["full"] }
parking_lot = "0.12"

# CLI
clap = { version = "4.0", features = ["derive"] }

# Time
chrono = { version = "0.4", features = ["serde"] }

# UUID
uuid = { version = "1.0", features = ["v4"] }

# WASM
wasm-bindgen = "0.2.92"
wasm-bindgen-futures = "0.4.42"
web-sys = { version = "0.3", features = [
    "console", "Document", "Element", "Window", "Request",
    "Response", "Headers", "MessageEvent", "WebSocket",
] }
js-sys = "0.3"
async-trait = "0.1"
futures = "0.3"
log = "0.4"
wasm-logger = "0.2"
console_error_panic_hook = "0.1"

# FFI
libc = "0.2"

# Testing
proptest = "1.0"
tempfile = "3.0"
wasm-bindgen-test = "0.3"

[workspace.lints.rust]
unsafe_code = "deny"
missing_docs = "warn"
unused_must_use = "deny"

[workspace.lints.clippy]
correctness = "deny"
perf = "warn"
complexity = "warn"
unwrap_used = "warn"
expect_used = "warn"
panic = "warn"

[profile.dev]
opt-level = 1
incremental = true

[profile.release]
opt-level = 3
lto = "thin"
codegen-units = 16
strip = "debuginfo"

[profile.test]
opt-level = 1
EOF
```

**Gate 1: Validate Workspace Cargo.toml**:
```bash
cd /home/user/qlever/rust

# Verify workspace parses correctly
cargo metadata --format-version=1 > /tmp/epic11.1-workspace-metadata.json

# Count workspace members
cat /tmp/epic11.1-workspace-metadata.json | jq '.workspace_members | length'
# Expected: 16
```

**Pass Criteria**: ✅ `cargo metadata` succeeds, 16 members listed
**Failure Action**: ❌ Fix Cargo.toml syntax errors, retry gate

---

## PHASE 2: Create Top-Level Packages (1 hour)

### Phase 2.1: Create qleverest Package (Compute)

**Actions**:
```bash
cd /home/user/qlever/rust

# Create directory structure
mkdir -p qleverest/src

# Create stub Cargo.toml
cat > qleverest/Cargo.toml << 'EOF'
[package]
name = "qleverest"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true
repository.workspace = true
description = "QLever in-memory RDF/SPARQL graph database for Rust"

[dependencies]
serde = { workspace = true }
thiserror = { workspace = true }

[dev-dependencies]
proptest = { workspace = true }
tempfile = { workspace = true }
EOF

# Create stub lib.rs
cat > qleverest/src/lib.rs << 'EOF'
//! QLever in-memory RDF/SPARQL graph database
#![deny(unsafe_code)]
#![warn(missing_docs)]

/// Placeholder for qleverest compute kernel
pub fn placeholder() {}
EOF
```

**Gate 2: Validate qleverest Package**:
```bash
cd /home/user/qlever/rust
cargo check --package qleverest
```

**Pass Criteria**: ✅ Compilation succeeds (exit code 0)
**Failure Action**: ❌ Fix Cargo.toml/lib.rs errors, retry

---

### Phase 2.2: Create qleverest-validation Package (Proof)

**Actions**:
```bash
cd /home/user/qlever/rust

# Create directory structure
mkdir -p qleverest-validation/src
mkdir -p qleverest-validation/crates

# Create stub Cargo.toml
cat > qleverest-validation/Cargo.toml << 'EOF'
[package]
name = "qleverest-validation"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true
repository.workspace = true
description = "QLever verification and proof plane"

[dependencies]
serde = { workspace = true }
thiserror = { workspace = true }
qleverest = { path = "../qleverest" }

[dev-dependencies]
proptest = { workspace = true }
tempfile = { workspace = true }

[features]
default = ["mock"]
mock = []
libqlever = []
EOF

# Create stub lib.rs
cat > qleverest-validation/src/lib.rs << 'EOF'
//! QLever verification and proof plane
#![deny(unsafe_code)]
#![warn(missing_docs)]

/// Placeholder for qleverest-validation proof plane
pub fn placeholder() {}
EOF
```

**Gate 3: Validate qleverest-validation Package**:
```bash
cd /home/user/qlever/rust
cargo check --package qleverest-validation
```

**Pass Criteria**: ✅ Compilation succeeds
**Failure Action**: ❌ Fix errors, retry

---

### Phase 2.3: Create qleverest-wasm Package (Projection)

**Actions**:
```bash
cd /home/user/qlever/rust

# Create directory structure
mkdir -p qleverest-wasm/src

# Create stub Cargo.toml
cat > qleverest-wasm/Cargo.toml << 'EOF'
[package]
name = "qleverest-wasm"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true
repository.workspace = true
description = "QLever WebAssembly bindings"

[lib]
crate-type = ["cdylib", "rlib"]

[dependencies]
serde = { workspace = true }
thiserror = { workspace = true }
qleverest = { path = "../qleverest" }
wasm-bindgen = { workspace = true }

[dev-dependencies]
wasm-bindgen-test = { workspace = true }

[features]
default = []
EOF

# Create stub lib.rs
cat > qleverest-wasm/src/lib.rs << 'EOF'
//! QLever WebAssembly bindings
#![deny(unsafe_code)]
#![warn(missing_docs)]

/// Placeholder for qleverest-wasm projection layer
pub fn placeholder() {}
EOF
```

**Gate 4: Validate qleverest-wasm Package**:
```bash
cd /home/user/qlever/rust
cargo check --package qleverest-wasm
```

**Pass Criteria**: ✅ Compilation succeeds
**Failure Action**: ❌ Fix errors, retry

---

## PHASE 3: Move Verification Crates (2 hours)

### Phase 3.1: Move Internal Crates (13 total)

**For EACH crate** (qlever-kernel-runner, qlever-artifact-capture, etc.):

**Actions**:
```bash
# Example for qlever-artifact-capture
cd /home/user/qlever

# Move crate to new location
mv qlever-verification/qlever-artifact-capture rust/qleverest-validation/crates/

# Update Cargo.toml to inherit workspace
cd rust/qleverest-validation/crates/qlever-artifact-capture
# Edit Cargo.toml: replace hardcoded values with workspace = true

# Update path dependencies in Cargo.toml
# Example: path = "../qlever-artifact-capture" (adjust relative paths)
```

**Gate 5-17: Validate Each Crate** (repeat for all 13 crates):
```bash
cd /home/user/qlever/rust
cargo check --package qlever-artifact-capture
cargo check --package qlever-kernel-runner
cargo check --package qlever-digest-verifier
cargo check --package qlever-cache-verifier
cargo check --package qlever-replay-verifier
cargo check --package qlever-regression-verifier
cargo check --package qlever-epoch-verifier
cargo check --package qlever-simd-verifier
cargo check --package qlever-chaos-verifier
cargo check --package qlever-verification-harness
cargo check --package receipt_comparator
cargo check --package qlever-repro
cargo check --package qlever-witness
```

**Pass Criteria**: ✅ All 13 crates compile successfully
**Failure Action**: ❌ Fix path dependencies, retry failing crate

---

## PHASE 4: Update Workspace Dependencies (1 hour)

### Phase 4.1: Ensure Workspace Inheritance

**Actions**:
```bash
cd /home/user/qlever/rust

# For each crate Cargo.toml, ensure dependencies use workspace = true
# Example:
# [dependencies]
# serde = { workspace = true }
# thiserror = { workspace = true }

# Verify no duplicate dependency versions
cargo tree --workspace --duplicates
```

**Gate 18: Validate Dependency Resolution**:
```bash
cd /home/user/qlever/rust

# Verify dependency tree is acyclic
cargo tree --workspace --edges no-dev > /tmp/epic11.1-post-move-dep-tree.txt

# Compare to baseline (only paths should differ)
diff -u /tmp/epic11.1-baseline-dep-tree.txt /tmp/epic11.1-post-move-dep-tree.txt | grep -v "^[<>] " | wc -l
# Expected: ~0 (only path differences, same topology)

# Verify package count
cargo metadata --format-version=1 | jq '.packages | length'
# Expected: 16
```

**Pass Criteria**: ✅ No circular dependencies, 16 packages
**Failure Action**: ❌ Fix dependency cycles, retry

---

## PHASE 5: CI/Scripts Updates (2 hours)

### Phase 5.1: Update CI Workflows

**Actions**:
```bash
# Update .github/workflows/integration-test.yml
# Replace: qlever-verification/ → rust/
# Replace: workspaces: "qlever-verification -> target" → "rust -> target"

# Example sed commands:
cd /home/user/qlever
sed -i 's|qlever-verification/|rust/|g' .github/workflows/*.yml
```

**Gate 19: Validate CI Syntax**:
```bash
# If actionlint available:
actionlint .github/workflows/*.yml || echo "SKIP: actionlint not installed"
```

**Pass Criteria**: ✅ No syntax errors (or actionlint not available)
**Failure Action**: ❌ Fix YAML syntax, retry

---

### Phase 5.2: Update Scripts

**Actions**:
```bash
cd /home/user/qlever

# Update scripts/artifact_publisher.sh
# Replace hardcoded paths: qlever-verification/ → rust/qleverest-validation/

# Update scripts/environment_snapshot.sh
# Replace paths similarly

# Create dependency directionality enforcement script
cat > rust/scripts/verify-dependency-directionality.sh << 'EOF'
#!/bin/bash
set -euo pipefail

echo "Verifying dependency directionality..."

cd /home/user/qlever/rust

# Check qleverest has NO deps on validation or wasm
if cargo tree --package qleverest | grep -E "(qleverest-validation|qleverest-wasm)"; then
  echo "FAIL: qleverest must not depend on validation or wasm"
  exit 1
fi

# Check qleverest-wasm does NOT depend on validation
if cargo tree --package qleverest-wasm | grep "qleverest-validation"; then
  echo "FAIL: qleverest-wasm must not depend on validation"
  exit 1
fi

# Check qleverest-validation CAN depend on qleverest (allowed)
cargo check --package qleverest-validation

echo "PASS: Dependency directionality enforced"
exit 0
EOF

chmod +x rust/scripts/verify-dependency-directionality.sh
```

**Gate 20: Validate Scripts**:
```bash
cd /home/user/qlever

# Test dependency directionality gate
bash rust/scripts/verify-dependency-directionality.sh
```

**Pass Criteria**: ✅ Script exits 0 (all directionality rules satisfied)
**Failure Action**: ❌ Fix forbidden dependencies, retry

---

## PHASE 6: Testing & Validation (2 hours)

### Phase 6.1: Unit Tests

**Actions**:
```bash
cd /home/user/qlever/rust

# Run all unit tests
cargo test --workspace --lib > /tmp/epic11.1-post-migration-tests.txt 2>&1

# Capture exit code
echo $? > /tmp/epic11.1-post-migration-test-status.txt
```

**Gate 21: Validate Unit Tests**:
```bash
# Compare to baseline
diff /tmp/epic11.1-baseline-test-status.txt /tmp/epic11.1-post-migration-test-status.txt
# Expected: identical (both should be 0)

# Count test results
grep -E "(test result:|passed)" /tmp/epic11.1-post-migration-tests.txt
```

**Pass Criteria**: ✅ All tests pass (100%), same count as baseline
**Failure Action**: ❌ DO NOT PROCEED - Investigate test failures

---

### Phase 6.2: MSRV Validation

**Actions**:
```bash
cd /home/user/qlever/rust

# Check with MSRV toolchain
cargo +1.91.1 check --workspace --all-targets
```

**Gate 22: Validate MSRV**:
**Pass Criteria**: ✅ Exit code 0 (all packages compatible with 1.91.1)
**Failure Action**: ❌ Fix incompatibilities, retry

---

### Phase 6.3: Feature Flag Matrix

**Actions**:
```bash
cd /home/user/qlever/rust

# Test mock mode
cargo test --workspace --features mock

# Test all features
cargo test --workspace --all-features
```

**Gate 23: Validate Features**:
**Pass Criteria**: ✅ All feature combinations pass tests
**Failure Action**: ❌ Fix feature-gated code, retry

---

### Phase 6.4: Binary Artifact Parity

**Actions**:
```bash
cd /home/user/qlever/rust

# Clean build
cargo clean
cargo build --workspace --release

# Capture post-migration artifacts
find target/release -type f -executable | sort > /tmp/epic11.1-post-migration-artifacts.txt

# Compare to baseline
diff /tmp/epic11.1-baseline-artifacts.txt /tmp/epic11.1-post-migration-artifacts.txt
```

**Gate 24: Validate Artifacts**:
**Pass Criteria**: ✅ Same artifact names (paths may differ)
**Failure Action**: ❌ Investigate missing/extra binaries

---

## PHASE 7: Documentation Updates (1 hour)

### Phase 7.1: Update Architecture Documentation

**Actions**:
```bash
# Update docs/explanation/architecture.md
# Add section on Rust workspace structure (3 public packages, 13 internal)
```

**Gate 25: Validate Architecture Doc**:
```bash
# Verify file exists and is non-empty
ls -l docs/explanation/architecture.md
```

**Pass Criteria**: ✅ File exists
**Failure Action**: ❌ Create/update file, retry

---

### Phase 7.2: Update CONTRIBUTING.md

**Actions**:
```bash
# Add workspace instructions to CONTRIBUTING.md
# Document how to add new verification crates
```

**Gate 26: Validate CONTRIBUTING**:
```bash
ls -l CONTRIBUTING.md
```

**Pass Criteria**: ✅ File exists
**Failure Action**: ❌ Update file, retry

---

### Phase 7.3: Create How-To Guide

**Actions**:
```bash
# Create docs/how-to/add-verification-crate.md
# Step-by-step guide for creating new crates
```

**Gate 27: Validate How-To**:
```bash
ls -l docs/how-to/add-verification-crate.md
```

**Pass Criteria**: ✅ File exists
**Failure Action**: ❌ Create file, retry

---

## POST-MIGRATION: Final Validation

### Post-Gate 1: Link Validation

**Actions**:
```bash
# Check for broken documentation links
grep -r "qlever-verification/" docs/ || echo "No stale paths found"
```

**Pass Criteria**: ✅ No stale paths in documentation
**Failure Action**: ❌ Update docs, retry

---

### Post-Gate 2: Receipt Generation

**Actions**:
```bash
cd /home/user/qlever

# Generate deterministic receipt
bash rust/scripts/generate_verification_receipt.sh post-migration > /tmp/epic11.1-final-receipt.json
```

**Gate 28: Validate Receipt**:
```bash
# Verify receipt is valid JSON
jq . /tmp/epic11.1-final-receipt.json > /dev/null
```

**Pass Criteria**: ✅ Receipt is valid JSON
**Failure Action**: ❌ Fix receipt script, retry

---

## COMMIT & PUSH

### Commit Changes

**Actions**:
```bash
cd /home/user/qlever

# Stage all changes
git add .

# Commit with deterministic message
git commit -m "$(cat <<'EOF'
feat(EPIC 11.1): Restructure Rust workspace to product-centric architecture

Unified workspace restructuring complete:
- 3 public packages: qleverest, qleverest-validation, qleverest-wasm
- 13 internal verification crates under qleverest-validation/crates/
- Dependency directionality enforced via CI gate
- All tests passing (100%)
- MSRV 1.91.1 validated
- Documentation updated

EPIC 11.1 CLOSED. Specification complete. Zero ambiguities.
EOF
)"

# Push to remote
git push -u origin epic11.1-workspace-restructure
```

**Final Gate: Validate Push**:
**Pass Criteria**: ✅ Push succeeds
**Failure Action**: ❌ Resolve conflicts, retry push

---

## ROLLBACK PROCEDURE

**When to Rollback**: ANY gate failure triggers immediate rollback

**Rollback Steps**:
```bash
cd /home/user/qlever

# Step 1: Restore from backup
BACKUP_PATH=$(cat /tmp/epic11.1-backup-path.txt)
tar -xzf "${BACKUP_PATH}"

# Step 2: Hard reset git
git reset --hard HEAD
git clean -fd

# Step 3: Verify restoration
cd qlever-verification
cargo test --workspace
# Expected: same results as baseline

# Step 4: Generate rollback receipt
cat > /tmp/epic11.1-rollback-receipt.json <<EOF
{
  "rollback_timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "failed_phase": "PHASE_NAME",
  "failed_gate": "GATE_NUMBER",
  "error_message": "ERROR_DESCRIPTION",
  "backup_restored": true,
  "verification_status": "baseline_tests_pass"
}
EOF
```

---

## SUCCESS CRITERIA SUMMARY

**All gates must pass (28/28)**:
- ✅ Pre-Migration Gates: 5/5
- ✅ Phase 1 Gates: 1/1
- ✅ Phase 2 Gates: 3/3
- ✅ Phase 3 Gates: 13/13
- ✅ Phase 4 Gates: 1/1
- ✅ Phase 5 Gates: 2/2
- ✅ Phase 6 Gates: 4/4
- ✅ Phase 7 Gates: 3/3
- ✅ Post-Migration Gates: 2/2

**Final Validation**:
- ✅ All 16 packages compile
- ✅ All tests pass (100%)
- ✅ MSRV check passes (1.91.1)
- ✅ Dependency directionality enforced
- ✅ Documentation updated
- ✅ Receipt generated

**Deterministic Execution**: COMPLETE ✅

---

## Document Metadata

- **Type**: Implementation Checklist (Deterministic Execution Guide)
- **Convergence Agent**: bb80-convergence-orchestrator
- **Input**: Final Merged Specification (EPIC_11.1_FINAL_MERGED_SPECIFICATION.md)
- **Output**: Step-by-step execution checklist (this document)
- **Gates**: 28 total (binary pass/fail)
- **Rollback**: Fail-closed (tarball restore)
- **Deterministic**: YES (mechanical transformations only)
- **Document Version**: 1.0 (immutable)
- **Generated**: 2026-01-02T22:30:00Z
- **Hash**: BLAKE3(this_document_content)

---

**END OF IMPLEMENTATION CHECKLIST**
