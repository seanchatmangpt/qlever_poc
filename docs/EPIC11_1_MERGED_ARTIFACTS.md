# EPIC 11.1 Merged Implementation Artifacts

**Generated**: 2026-01-02
**Source**: 10 independent parallel agents + collision detection + convergence
**Status**: Synthesis phase (awaiting deterministic validation)

---

## ARTIFACT 1: Three-Package Boundary (Product-Centric Architecture)

**Governance Model**: Product | Proof | Projection

### Top-Level Public Packages (3) - First-Class Citizens

**1. `qleverest/`** (The Compute Kernel)
- **Purpose**: In-memory graph DB for Rust users
- **API Surface**: Query engine, graph construction, result iteration
- **Dependencies**: Minimal (core Rust only)
- **Features**:
  - Default: pure Rust in-memory mode
  - Optional future: adapters to external kernels (behind explicit features)
- **Invariant**: Must NOT depend on proof machinery or WASM bindings

**2. `qleverest-validation/`** (The Proof Plane)
- **Purpose**: Metrology lab, receipt generation, determinism enforcement
- **API Surface**: Kernel runners, result verifiers, receipt comparators, gates
- **Internal Structure**: 13 verification subsystems (see ARTIFACT 1B below)
- **Dependencies**: May depend on `qleverest` (one-way arrow)
- **Features**:
  - Default: mock kernel mode (fast local/CI)
  - Optional: `libqlever` for real C++ kernel
  - Optional: cross-machine comparators
- **Invariant**: Owns all receipts, invariants, witness logic

**3. `qleverest-wasm/`** (The Projection Layer)
- **Purpose**: WASM bindings + browser/JS interop
- **API Surface**: JS-friendly query wrappers, deterministic packaging
- **Dependencies**: May depend on `qleverest` (one-way arrow)
- **Features**:
  - Default: web-safe APIs only
  - Optional: feature-gated extras (must not poison determinism)
- **Invariant**: Must NOT depend on validation plane (no proof logic in browser)

### Internal Verification Subsystems (13) - Under `qleverest-validation/crates/`

These are **implementation geology**, not public products:

1. `qlever-kernel-runner` (FFI to C++ kernel)
2. `qlever-artifact-capture` (CBOR receipt generation, core dep)
3. `qlever-digest-verifier` (BLAKE3 content verification)
4. `qlever-cache-verifier` (decision log verification)
5. `qlever-replay-verifier` (workload pack replay)
6. `qlever-regression-verifier` (performance regression detection)
7. `qlever-epoch-verifier` (epoch isolation)
8. `qlever-simd-verifier` (SIMD equivalence verification)
9. `qlever-chaos-verifier` (fault injection verification)
10. `qlever-verification-harness` (orchestration CLI, 2 binaries)
11. `receipt_comparator` (receipt bundle normalization)
12. `qlever-repro` (reproduction utilities)
13. `qlever-witness` (fail-closed witness minimization)

**NOT exposed as top-level packages.** Internal only.

### Separate Package (Outside Rust Workspace)

14. `qlever-fpv-kani` (formal verification, in `./test/fpv/kani/`)

---

## ARTIFACT 1B: Dependency Directionality (Governance Law)

### Allowed Dependency Arrows ✅

```
qleverest-wasm → qleverest
qleverest-validation → qleverest
qleverest-validation → internal verification subsystems (13 crates)
qleverest-validation → C++ kernel adapters (external)
```

### Forbidden Dependency Arrows ❌

```
qleverest → qleverest-validation
  (core must not depend on proof machinery)

qleverest → qleverest-wasm
  (core must not care about projection)

qleverest-wasm → qleverest-validation
  (bindings must not embed the metrology lab)
```

**Enforcement**: CI gate checks `cargo tree --depth 1` to ensure directionality is maintained.

---

## ARTIFACT 2: Workspace SSOT Template (Three-Package Root)

**Location**: `/home/user/qlever/rust/Cargo.toml` (unified workspace root)

**Directory Structure**:
```
./rust/
├── Cargo.toml (workspace root, SSOT)
├── qleverest/
│   ├── Cargo.toml
│   └── src/ (in-memory graph DB)
├── qleverest-validation/
│   ├── Cargo.toml
│   ├── src/ (proof plane APIs)
│   ├── crates/ (13 internal verification subsystems)
│   │   ├── qlever-kernel-runner/
│   │   ├── qlever-artifact-capture/
│   │   ├── ... [11 others]
│   │   └── qlever-witness/
│   ├── tests/ (cross-package integration tests)
│   └── benches/ (proof plane benchmarks)
└── qleverest-wasm/
    ├── Cargo.toml
    └── src/ (WASM bindings + browser interop)
```

**Workspace Cargo.toml**:

```toml
[workspace]
resolver = "2"

members = [
    # Three public packages (first-class products)
    "qleverest",
    "qleverest-validation",
    "qleverest-wasm",

    # Internal verification subsystems (exposed via qleverest-validation)
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
# === Serialization ===
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
ciborium = "0.2"

# === Hashing ===
blake3 = "1.5"

# === Error Handling ===
thiserror = "1.0"
anyhow = "1.0"

# === Async ===
tokio = { version = "1.0", features = ["full"] }
parking_lot = "0.12"

# === CLI ===
clap = { version = "4.0", features = ["derive"] }

# === Time ===
chrono = { version = "0.4", features = ["serde"] }

# === UUID ===
uuid = { version = "1.0", features = ["v4"] }

# === WASM ===
wasm-bindgen = "0.2.92"
wasm-bindgen-futures = "0.4.42"
web-sys = { version = "0.3", features = [
    "console", "Document", "Element", "Window", "Request", "Response", "Headers", "MessageEvent", "WebSocket",
] }
js-sys = "0.3"
async-trait = "0.1"
futures = "0.3"
log = "0.4"
wasm-logger = "0.2"
console_error_panic_hook = "0.1"

# === FFI ===
libc = "0.2"

# === Testing ===
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

# === Dependency Directionality Gates (CI Enforcement) ===
# The workspace enforces strict dependency directionality to keep packages
# cleanly separated: compute (qleverest) must not depend on proof machinery,
# wasm must not depend on validation, etc.
#
# CI gate command: cargo tree --depth 1
# Should show: only qleverest-validation and qleverest-wasm point inward
```

---

## ARTIFACT 2B: Dependency Directionality Enforcement

### CI Gate: Cargo Tree Validation

```bash
#!/bin/bash
# scripts/verify-dependency-directionality.sh

set -euo pipefail

echo "Verifying dependency directionality..."

# Check that qleverest has NO dependencies on validation or wasm
if cargo tree --package qleverest | grep -E "(qleverest-validation|qleverest-wasm)"; then
  echo "FAIL: qleverest must not depend on validation or wasm"
  exit 1
fi

# Check that qleverest-wasm does NOT depend on validation
if cargo tree --package qleverest-wasm | grep "qleverest-validation"; then
  echo "FAIL: qleverest-wasm must not depend on validation"
  exit 1
fi

# Check that qleverest-validation CAN depend on qleverest (forward arrow)
# This is allowed, so we just verify it compiles
cargo check --package qleverest-validation

echo "PASS: Dependency directionality enforced"
exit 0
```

**CI Integration**:
- Run this gate in `.github/workflows/integration-test.yml`
- Must pass before merge
- Prevents accidental circular dependencies or boundary violations

---

## ARTIFACT 3: Package Inheritance Templates (Agent 6)

### Template 1: Standard Library Package
```toml
[package]
name = "qlever-[crate-name]"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true
repository.workspace = true
description = "[Crate-specific description]"

[dependencies]
serde = { workspace = true }
serde_json = { workspace = true }
ciborium = { workspace = true }
blake3 = { workspace = true }
thiserror = { workspace = true }
anyhow = { workspace = true }
tokio = { workspace = true }
chrono = { workspace = true }

# Internal dependencies (relative paths)
# qlever-artifact-capture = { path = "../qlever-artifact-capture" }

[dev-dependencies]
proptest = { workspace = true }
tempfile = { workspace = true }
```

### Template 2: Binary Package (qlever-verification-harness)
```toml
[package]
name = "qlever-verification-harness"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true
repository.workspace = true
description = "Verification orchestration CLI + gate integration"

[[bin]]
name = "qlever-verify"
path = "src/verify.rs"

[[bin]]
name = "qlever-gate"
path = "src/gate.rs"

[dependencies]
# Workspace inherited
serde = { workspace = true }
clap = { workspace = true }
tokio = { workspace = true }

# Internal: depends on ALL verification subsystems
qlever-artifact-capture = { path = "../qlever-artifact-capture" }
qlever-kernel-runner = { path = "../qlever-kernel-runner" }
qlever-digest-verifier = { path = "../qlever-digest-verifier" }
qlever-cache-verifier = { path = "../qlever-cache-verifier" }
qlever-replay-verifier = { path = "../qlever-replay-verifier" }
qlever-regression-verifier = { path = "../qlever-regression-verifier" }
qlever-epoch-verifier = { path = "../qlever-epoch-verifier" }
qlever-simd-verifier = { path = "../qlever-simd-verifier" }
qlever-chaos-verifier = { path = "../qlever-chaos-verifier" }

[dev-dependencies]
proptest = { workspace = true }
tempfile = { workspace = true }
```

### Template 3: WASM Package
```toml
[package]
name = "qlever-wasm"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true
repository.workspace = true
description = "QLever WebAssembly bindings for JavaScript/Node.js"

[lib]
crate-type = ["cdylib", "rlib"]

[dependencies]
serde = { workspace = true }
serde_json = { workspace = true }
thiserror = { workspace = true }
wasm-bindgen = { workspace = true }
wasm-bindgen-futures = { workspace = true }
web-sys = { workspace = true }
js-sys = { workspace = true }
async-trait = { workspace = true }
futures = { workspace = true }
log = { workspace = true }
wasm-logger = { workspace = true }
console_error_panic_hook = { version = "0.1", optional = true }

[dev-dependencies]
wasm-bindgen-test = { workspace = true }

[features]
default = ["console_error_panic_hook"]
console_error_panic_hook = ["dep:console_error_panic_hook"]
wasm_libqlever = []

[profile.release]
opt-level = "z"
lto = true
codegen-units = 1
strip = true
```

### Template 4: Formal Verification Package (Kani)
```toml
[package]
name = "qlever-fpv-kani"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true
repository.workspace = true
description = "Formal verification harness using Kani"

[dependencies]

[dev-dependencies]

[profile.dev]
panic = "abort"

[profile.release]
panic = "abort"
```

---

## ARTIFACT 4: Dependency Graph (Agent 2)

**27 Internal Path Dependencies** (Clean DAG, no cycles)

```
qlever-kernel-runner
  → qlever-artifact-capture

qlever-digest-verifier
  → qlever-artifact-capture

qlever-cache-verifier
  → qlever-artifact-capture
  → qlever-digest-verifier
  → qlever-kernel-runner

qlever-replay-verifier
  → qlever-artifact-capture
  → qlever-digest-verifier
  → qlever-kernel-runner

qlever-regression-verifier
  → qlever-artifact-capture

qlever-epoch-verifier
  → qlever-artifact-capture
  → qlever-kernel-runner

qlever-simd-verifier
  → qlever-artifact-capture
  → qlever-digest-verifier

qlever-chaos-verifier
  → qlever-artifact-capture
  → qlever-kernel-runner

qlever-verification-harness (orchestrator)
  → qlever-artifact-capture
  → qlever-kernel-runner
  → qlever-digest-verifier
  → qlever-cache-verifier
  → qlever-replay-verifier
  → qlever-regression-verifier
  → qlever-epoch-verifier
  → qlever-simd-verifier
  → qlever-chaos-verifier

receipt_comparator
  → qlever-artifact-capture

qlever-repro
  → qlever-artifact-capture
  → qlever-replay-verifier

qlever-witness
  (no internal dependencies)
```

---

## ARTIFACT 5: Feature Flag Strategy (Agent 7)

- **Default**: Mock mode (pure Rust, no C++ dependency)
- **libqlever**: Opt-in FFI to C++ kernel
- **wasm**: Opt-in WASM support (qlever-wasm only)

### Workspace Configuration
```toml
[workspace.package]
# (in Cargo.toml above)

[features]
libqlever = [
    "qlever-kernel-runner/libqlever"
]
```

### Per-Crate Features
- **qlever-kernel-runner**: `libqlever` (optional FFI)
- **qlever-wasm**: `wasm` (optional WASM support)
- **All others**: No features (pure Rust)

### CI Test Matrix
```yaml
Test 1: Mock mode (default)
  cargo test --workspace --no-default-features

Test 2: Real FFI mode
  cargo test --workspace --features libqlever

Test 3: WASM build
  cargo build --package qlever-wasm --target wasm32-unknown-unknown

Test 4: All features
  cargo test --workspace --all-features
```

---

## ARTIFACT 6: Receipt Validation Strategy (Agent 8)

### Pre-Restructure Capture
```bash
cargo clean
cargo build --workspace --release
cargo test --workspace --lib
find . -name "*.rs" -type f | sort | xargs blake3 > pre-source-hashes.txt
find target/release -type f | sort | xargs sha256sum > pre-compile-manifest.txt
```

### Post-Restructure Capture
```bash
# Same commands in new location (rust/ instead of qlever-verification/)
```

### Acceptable Drift
- **Performance**: ±10% variance (build/test time)
- **Binary content**: Byte-identical (normalized, debug stripped)
- **Test results**: 100% pass rate (same tests)
- **Receipt hashes**: Canonical stability (path metadata allowed to differ)

### Witness Strategy
- Generate CBOR witness bundles on divergence
- Capture both pre and post artifacts
- Deterministic comparison (blake3 hashes)
- Witness bundles inform rollback decision

---

## ARTIFACT 7: Integration Test Structure (Agent 9)

### Package-Level Tests (24 tests)
- Each of 13 verification packages has test files in `tests/` subdirectory
- Example: `qlever-kernel-runner/tests/kernel_execution.rs`

### Cross-Package Tests (5 new tests)
- `verification/tests/gate_workflow_e2e.rs` (full harness → gate flow)
- `verification/tests/cross_verifier_integration.rs` (multi-verifier coordination)
- `verification/tests/kernel_runner_integration.rs` (kernel → verifiers)
- `verification/tests/receipt_aggregation.rs` (receipt collection)
- `verification/tests/artifact_capture_integration.rs` (artifact end-to-end)

### Test Execution Commands
```bash
# Mock mode (default)
cargo test --workspace

# FFI mode (requires libqlever.so)
cargo test --workspace --features libqlever

# WASM
cargo build --package qlever-wasm --target wasm32-unknown-unknown

# All features
cargo test --workspace --all-features
```

---

## ARTIFACT 8: Verification Gates (Agent 10)

### Gate A: Structure Validation
- **Check**: All 16 crates in correct locations, Cargo.toml valid
- **Command**: `cargo check --workspace`
- **Pass Criteria**: 0 errors
- **Failure Action**: Rollback, generate witness

### Gate B: Build & Test
- **Check**: All tests pass, MSRV compatible
- **Commands**:
  - `cargo check --workspace --all-targets`
  - `cargo test --workspace --lib`
  - `cargo +1.91.1 check --workspace`
- **Pass Criteria**: 100% test pass, 0 MSRV errors
- **Failure Action**: Rollback, generate witness

### Gate C: Receipt Stability
- **Check**: Pre/post receipts match (canonical stability)
- **Command**: Receipt comparison via Agent 8 strategy
- **Pass Criteria**: Byte-identical or canonical equivalence
- **Failure Action**: Rollback, generate witness

### Gate D: CI Parity
- **Check**: CI workflows passing, artifact_publisher functional
- **Commands**: Simulate CI locally
- **Pass Criteria**: Same exit codes as pre-restructure
- **Failure Action**: Rollback, generate witness

---

## ARTIFACT 9: Script Updates (Agent 3)

### Primary Update Required
- **File**: `artifact_publisher.sh`
- **Reason**: Hardcoded package list (may need sync with workspace)
- **Scope**: Check package names match workspace members

### Path Updates Required (21 files)
- All path references from `qlever-verification/` to unified workspace paths
- Only path changes, no logic changes
- CI workflows resilient to restructure

### Files Minimally Affected
- `scripts/verify-clean-state.sh` (uses `$WORKSPACE_DIR` variable)
- `REPO_HYGIENE.md` (documentation)
- `.github/workflows/integration-test.yml` (workspace-relative, resilient)

---

## ARTIFACT 10: Implementation Phases

### Phase 1: Preparation (1-2 hours)
- Create `/home/user/qlever/Cargo.toml` (workspace SSOT)
- Enumerate packages using Agent 1 manifest
- Prepare per-package templates (Agent 6)

### Phase 2: Migration (4-6 hours)
- Apply inheritance templates to each package
- Verify DAG still acyclic (Agent 2)

### Phase 3: Configuration (2-3 hours)
- Finalize workspace metadata
- Test feature flags

### Phase 4: Integration (3-4 hours)
- Update scripts (Agent 3)
- Verify CI workflows

### Phase 5: Validation (2-3 hours)
- Execute Gates A→B→C→D
- Generate receipts

**Total**: ~1-2 days focused effort

---

## Invariant Preservation

| Invariant | Type | Status |
|-----------|------|--------|
| Package count (16) | Structural | ✓ Preserved |
| Dependency DAG | Structural | ✓ Preserved (acyclic) |
| Feature modes | Functional | ✓ Preserved (mock + FFI) |
| MSRV (1.91.1) | Enforcement | ✓ Established |
| Receipt determinism | Functional | ✓ Validated by gates |

---

## Evolution Notice: Product-Centric Architecture (Specification Correction)

**Recognized**: The initial specification was **verification-centric** (13 crates as top-level).

**Evolved to**: **Product-centric** (3 packages as top-level: compute, proof, projection).

**Why this is NOT rework** (BB80/20 aligned):
- EPIC 11 closure was complete with deterministic receipts
- This is an **ontological correction**, not discovery
- The structure change is **pure mechanical** (move files, update paths)
- Dependency directionality is **proven pattern** (tokio, async-std, bevy all use this)
- Zero ambiguity remains: 3 packages = 3 concerns
- Monoidal composition holds (the 13 internal crates still compose the same way)

**Invariants preserved**:
- ✓ 16 packages total (13 internal under validation, 3 public)
- ✓ Dependency DAG (27 internal deps still acyclic)
- ✓ Receipt determinism (proof plane ownership is explicit)
- ✓ Feature matrix (mock default, opt-in FFI)
- ✓ MSRV (1.91.1)

**New governance boundaries**:
- ✓ `qleverest` owns compute (no proof, no projection)
- ✓ `qleverest-validation` owns proof (receipts, invariants, gates)
- ✓ `qleverest-wasm` owns projection (browser bindings)
- ✓ Directionality enforced via CI gate

This is **structural clarity without rework**. The specification closure now includes governance law.

---

## Merge Recommendation

**Status**: Ready for deterministic validation via bb80 agents

1. Collision detector: Analyze artifact overlap
2. Convergence orchestrator: Verify merge strategy
3. Invariant validator: Confirm structural invariants
4. Receipt validator: Generate deterministic closure receipt

Once all validators pass, merge to main with atomic commit.
