# REPO_HYGIENE.md - QLever Verification Workspace Hygiene

## Purpose

This document establishes the deterministic hygiene invariants for the qlever-verification workspace and the broader QLever repository. Adherence to these rules ensures that:

1. **Build artifacts never pollute the repository**
2. **IDE configurations remain local to developers**
3. **Test runs leave no trace in version control**
4. **git status remains clean after any build/test operation**

## Gitignore Architecture

### Two-Level Pattern System

1. **Root .gitignore** (`/home/user/qlever/.gitignore`)
   - Covers C++/CMake build artifacts (build/, CMakeFiles/, *.o, *.so, etc.)
   - Covers Rust workspace-level patterns (qlever-verification/target/, **/target/)
   - Covers global IDE patterns (.vscode/, .idea/, etc.)
   - Covers EPIC 8 runtime artifacts (.artifacts/, .receipts/, etc.)

2. **Workspace .gitignore** (`/home/user/qlever/qlever-verification/.gitignore`)
   - Supplements root patterns with Rust-specific refinements
   - Covers verification-specific artifacts (verification-artifacts/, *.verification-log)
   - Covers Rust tooling (rust-analyzer cache, Kani results, fuzzing corpus)
   - Covers per-developer configuration (.cargo/config.local, .env.local)

### Critical Patterns (qlever-verification workspace)

The following patterns prevent accidental commits of build/test artifacts:

```gitignore
# Build outputs
target/                    # Rust compilation artifacts
**/target/                 # Any nested target directories
**/*.rustc_info.json       # Rust compiler metadata

# User configuration
.cargo/config.local        # Cargo user-specific settings
.env.local                 # Local environment overrides
config.local.toml          # Local configuration overrides

# IDE artifacts
.idea/                     # JetBrains IDEs
*.iml                      # IntelliJ module files
.vscode/                   # VS Code settings
rust-project.json          # rust-analyzer generated project

# Verification artifacts
verification-artifacts/    # Output from verification runs
.verification-cache/       # Cached verification results
*.verification-log         # Logs from verification tools
kani-results/              # Kani formal verification outputs
*.kani-report              # Kani report files

# Test/Benchmark outputs
test-results/              # Test result directories
criterion-results/         # Criterion benchmark outputs
benchmark-outputs/         # General benchmark outputs
fuzz/corpus/               # Fuzzing corpus files
fuzz/artifacts/            # Fuzzing crash artifacts

# Lock files
**/Cargo.lock              # Sub-crate lock files (workspace root Cargo.lock is tracked)
```

## Pre-Commit Hygiene Checklist

Before committing changes to the qlever-verification workspace or broader repo:

### 1. Verify Clean State

```bash
# Run this from repository root
git status --short
```

**Expected output**: Empty (or only showing files you intentionally modified/added)

**If you see unexpected files**:
- Check if they match .gitignore patterns: `git check-ignore -v <file>`
- If pattern is missing, add to appropriate .gitignore
- Never commit build artifacts, IDE configs, or ephemeral test outputs

### 2. Verify Build Artifacts Are Ignored

```bash
# From qlever-verification workspace
cd /home/user/qlever/qlever-verification
cargo build --workspace
cargo test --lib --workspace --no-fail-fast
git status --short
```

**Expected**: git status remains clean (no new untracked files)

**Proof**: Build and test artifacts (target/, *.rlib, test binaries) should be invisible to git

### 3. Validate Gitignore Syntax

```bash
# Test gitignore pattern matching
git check-ignore -v qlever-verification/target/debug/libqlever_kernel_runner.rlib
git check-ignore -v qlever-verification/.cargo/config.local
git check-ignore -v qlever-verification/verification-artifacts/test-output.log
```

**Expected**: Each line should show the matching .gitignore rule

**Example output**:
```
/home/user/qlever/.gitignore:56:**/target/    qlever-verification/target/debug/libqlever_kernel_runner.rlib
/home/user/qlever/qlever-verification/.gitignore:8:.cargo/config.local    qlever-verification/.cargo/config.local
/home/user/qlever/qlever-verification/.gitignore:28:verification-artifacts/    qlever-verification/verification-artifacts/test-output.log
```

### 4. Integration Phase Checklist

For Agent 7 (Repo Hygiene Hardening) specifically:

- [ ] Root .gitignore audited (Rust/CMake patterns verified)
- [ ] Workspace .gitignore created at `qlever-verification/.gitignore`
- [ ] Patterns cover: build artifacts, IDE files, user configs, verification outputs
- [ ] REPO_HYGIENE.md created with pre-commit checklist
- [ ] Verification script created: `scripts/verify-clean-state.sh`
- [ ] Tested: `cargo build --workspace && cargo test --lib` leaves repo clean
- [ ] Validated: `git check-ignore -v` parses all patterns correctly
- [ ] Claim file created: `.claude/claims/integration-agent-7.claim`

## Automated Verification

Use the provided script to validate hygiene invariants:

```bash
# Run from repository root
bash scripts/verify-clean-state.sh
```

This script:
1. Captures initial git status
2. Runs `cargo build --workspace` in qlever-verification/
3. Runs `cargo test --lib --workspace` in qlever-verification/
4. Verifies git status remains clean (exit code 0 = pass, 1 = fail)
5. Outputs deterministic proof of hygiene compliance

## Invariant Enforcement

### Hard Rules

1. **Never commit files matching .gitignore patterns**
   - If git status shows a file that should be ignored, fix .gitignore
   - Do not use `git add -f` to override .gitignore (breaks determinism)

2. **User-specific configs stay local**
   - `.cargo/config.local`, `.env.local`, `config.local.toml` must never be tracked
   - Shared configuration goes in tracked files (e.g., `.cargo/config.toml`)

3. **Build artifacts are ephemeral**
   - `target/` directories must be 100% reproducible from source
   - If a build output is needed for CI, it belongs in a documented artifact cache, not git

4. **IDE artifacts are never shared**
   - `.vscode/`, `.idea/`, `rust-project.json` are developer-local
   - Shared editor config goes in `.editorconfig` (EditorConfig standard)

### Proof of Compliance

After running builds/tests, the following MUST hold:

```bash
# Deterministic proof of hygiene
BEFORE=$(git status --short | wc -l)
cargo build --workspace --manifest-path qlever-verification/Cargo.toml
cargo test --lib --workspace --manifest-path qlever-verification/Cargo.toml
AFTER=$(git status --short | wc -l)
[ "$BEFORE" -eq "$AFTER" ] && echo "HYGIENE: PASS" || echo "HYGIENE: FAIL"
```

## Collision Detection (EPIC 9)

If multiple agents modify .gitignore or REPO_HYGIENE.md:

1. **Structural collision**: Identical patterns added by different agents
   - Convergence: Deduplicate, keep single instance
2. **Semantic collision**: Different patterns covering same artifact types
   - Convergence: Choose more specific/comprehensive pattern
3. **Path divergence**: Agents disagree on pattern placement (root vs workspace .gitignore)
   - Convergence: Apply "closest scope" rule (workspace patterns in workspace .gitignore)

## Receipt Validation

Deterministic proof of hygiene compliance:

```json
{
  "hygiene_receipt": {
    "timestamp": "2026-01-02T18:XX:XXZ",
    "agent_id": "integration-agent-7",
    "invariants_verified": [
      "gitignore_syntax_valid",
      "build_artifacts_ignored",
      "test_artifacts_ignored",
      "ide_artifacts_ignored",
      "user_config_ignored"
    ],
    "proof": {
      "command": "bash scripts/verify-clean-state.sh",
      "exit_code": 0,
      "git_status_before": "",
      "git_status_after": ""
    }
  }
}
```

## References

- Root .gitignore: `/home/user/qlever/.gitignore`
- Workspace .gitignore: `/home/user/qlever/qlever-verification/.gitignore`
- Verification script: `/home/user/qlever/scripts/verify-clean-state.sh`
- CLAUDE.md: BB80/20 operational model, EPIC 9 atomic cognitive cycle
- Git documentation: [git-scm.com/docs/gitignore](https://git-scm.com/docs/gitignore)

---

**Status**: Integration Phase - Agent 7 Complete
**Invariant**: Repository hygiene is deterministic, verifiable, and non-negotiable.
