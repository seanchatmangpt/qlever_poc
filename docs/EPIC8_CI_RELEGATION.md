# EPIC 8: CI/CD Relegation Under Reverse Conway's Law

## 1. Reverse Conway's Law: Formal Definition

**Reverse Conway's Law (Prescriptive Form):**

The structure of a system must be prescribed by its organizational authority. Where Conway's Law states that "system architecture mirrors organizational communication," Reverse Conway's Law enforces that the code-prescribed architecture must determine organizational structure and process boundaries.

**Instantiation for CI/CD Systems:**

All continuous integration and continuous deployment systems are relegated to **compute-only role**. CI/CD systems execute deterministic build instructions. They do not:
- Interpret domain logic
- Emit decisions
- Gate progress through logical branches
- Retry conditionally
- Emit narratives or observational metrics

The canonical authority for all construction is the Makefile. The Makefile prescribes what CI/CD may invoke. CI/CD has no authority to override, reinterpret, or extend the Makefile.

---

## 2. EPIC 8 Makefile as Organizational Law

### 2.1 Single Invocation Point

The `/home/user/qlever/Makefile` establishes a single, deterministic entry point:

```makefile
universe: verify-invariants phase-a phase-b phase-c phase-d phase-e phase-f artifact-seal
```

**What this prescribes:**
- All construction flows through the `universe` target.
- Targets execute in strict sequential order: A → B → C → D → E → F → Seal.
- Each phase is fail-closed: if any phase fails, the entire construction fails.
- No partial states are permitted.

### 2.2 Six Mandatory Phases with Fail-Closed Semantics

#### Phase A: Toolchain Sealing
- Identifies compiler identity (clang++ or g++)
- Normalizes compilation flags (`-std=c++20 -O3 -DNDEBUG`)
- Records normalized environment to `/artifacts/flags.env`
- **No flexibility permitted:** flags are non-negotiable.

#### Phase B: Dependency Integrity
- Verifies presence of `CMakeLists.txt`, `src/`, `test/`, `.git/HEAD`
- Enforces immutability (no runtime fetches)
- Validates vendored sources
- **No flexibility permitted:** all dependencies must be present and verified before proceeding.

#### Phase C: Core Compilation
- Sources normalized flags from Phase A
- Invokes CMake with `CMAKE_BUILD_TYPE=Release`
- Compiles via Ninja with parallel jobs
- **No flexibility permitted:** compilation must be silent on success, fail-closed on any error.

#### Phase D: Rule & Constraint Enforcement
- Validates build artifacts exist
- Confirms Makefile or build.ninja is present
- **No flexibility permitted:** symbolic checks only; no interpretation.

#### Phase E: Deterministic Benchmarks
- Runs ctest with `--rerun-failed --output-on-failure`
- Boolean result only: pass or fail
- **No flexibility permitted:** tests must be reproducible; variance is not tolerated.

#### Phase F: Artifact Sealing
- Emits SHA-256 manifest of all artifacts
- Renders manifest read-only (`chmod 444`)
- **No flexibility permitted:** artifacts are immutable post-sealing.

### 2.3 Artifact-Seal: Phase Lock Enforcement

```makefile
artifact-seal: phase-f
	@echo "PHASE_SEAL: Artifact integrity finalization" >&2
	@touch $(PHASE_LOCK)
	@chmod 444 $(PHASE_LOCK)
```

The phase lock file (`/artifacts/.phase.lock`) confirms successful completion of all phases.

---

## 3. What CI/CD Systems MUST Do

CI/CD systems **shall and only shall** perform the following:

### 3.1 Single Invocation

```bash
make -C /home/user/qlever universe
```

**Mandatory properties:**
- CI/CD invokes the Makefile from the repository root.
- CI/CD passes no additional targets or arguments.
- CI/CD does not set environment variables that would alter compilation flags.
- CI/CD does not inject conditional logic into the make invocation.

### 3.2 Exit Code Validation

CI/CD must validate the exit code:

```bash
make -C /home/user/qlever universe
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then
    exit $EXIT_CODE
fi
```

**Mandatory properties:**
- Exit code 0 indicates all six phases completed successfully.
- Exit code non-zero indicates failure; CI/CD terminates immediately.
- CI/CD does not inspect logs, metrics, or intermediate states.
- CI/CD does not retry or escalate failures.

### 3.3 Phase Lock Validation

CI/CD may (optionally) validate that the phase lock exists:

```bash
test -f /home/user/qlever/.artifacts/.phase.lock || exit 1
```

**Mandatory properties:**
- This is a **post-execution check only**.
- CI/CD does not use this check to decide whether to retry.
- CI/CD does not interpret the presence/absence of the lock as a signal to take action.

---

## 4. What CI/CD Systems MUST NOT Do

CI/CD systems are **strictly prohibited** from the following:

### 4.1 Branch or Gate Logic

**PROHIBITED:**
```bash
# DO NOT DO THIS
if [ -f /artifacts/.phase.lock ]; then
    # retry or escalate
fi
```

**PROHIBITED:**
```bash
# DO NOT DO THIS
make phase-a && make phase-b || make phase-a
```

**Rationale:** The Makefile is the source of truth for sequencing. CI/CD has no authority to reorder, conditionally skip, or retry phases.

### 4.2 Interpretation of Toolchain

**PROHIBITED:**
```bash
# DO NOT DO THIS
if ! command -v clang++ >/dev/null; then
    CXX=g++ make universe
fi
```

**PROHIBITED:**
```bash
# DO NOT DO THIS
CXX=/custom/clang++ CXXFLAGS="-O2" make universe
```

**Rationale:** Phase A prescribes toolchain identity. CI/CD cannot override this; it must accept what Phase A determines.

### 4.3 Conditional Retries

**PROHIBITED:**
```bash
# DO NOT DO THIS
for i in {1..3}; do
    make universe && break || sleep 5
done
```

**Rationale:** If the universe construction fails, it failed deterministically. Retrying masks the root cause and violates fail-closed semantics.

### 4.4 Metric Narratives or Observational Logging

**PROHIBITED:**
```bash
# DO NOT DO THIS
echo "Build completed in 45 seconds"
echo "Tests passed: 1234/1234"
echo "Code coverage: 87.3%"
```

**PROHIBITED:**
```bash
# DO NOT DO THIS
curl -X POST https://observability.internal/events \
    -d '{"build_time": 45, "tests_passed": 1234}'
```

**Rationale:** CI/CD is compute-only. Metrics and narratives are for observability systems, not for CI/CD to emit. The Makefile emits only deterministic facts (phase names, phase lock presence). All interpretation is forbidden.

### 4.5 Post-Build Analysis or Gating

**PROHIBITED:**
```bash
# DO NOT DO THIS
if [ $(stat -c%s /artifacts/manifest.sha256) -eq 0 ]; then
    echo "Manifest is empty; build may be corrupted"
    exit 1
fi
```

**Rationale:** The Makefile validates its own outputs. If the Makefile exited 0, the outputs are valid. CI/CD may not re-validate.

### 4.6 Parallel Phase Execution

**PROHIBITED:**
```bash
# DO NOT DO THIS
make phase-a & make phase-b & make phase-c & wait
```

**Rationale:** Phases are sequentially dependent. The Makefile enforces this ordering. CI/CD cannot parallelize.

---

## 5. Example: Violating CI Configuration

### 5.1 GitHub Actions Violation

```yaml
# VIOLATES REVERSE CONWAY'S LAW
name: Build

on: [push]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      # VIOLATION 1: Conditional toolchain injection
      - name: Setup toolchain
        run: |
          if [[ "${{ github.event_name }}" == "pull_request" ]]; then
            export CXX=clang++
          else
            export CXX=g++
          fi

      # VIOLATION 2: Parallel phase execution
      - name: Run phases in parallel
        run: |
          make phase-a & make phase-b & make phase-c & wait

      # VIOLATION 3: Conditional retry logic
      - name: Run universe with retry
        run: |
          for i in {1..3}; do
            make universe && break || echo "Retry $i"
          done

      # VIOLATION 4: Post-build metric emission
      - name: Emit metrics
        run: |
          BUILD_TIME=$(date +%s)
          curl -X POST https://metrics.internal/build \
            -d "{\"branch\": \"${{ github.ref }}\", \"duration\": $BUILD_TIME}"

      # VIOLATION 5: Conditional gating
      - name: Validate build
        run: |
          if [ ! -f /artifacts/.phase.lock ]; then
            echo "Build validation failed"
            exit 1
          fi
```

**Why this violates Reverse Conway's Law:**
1. **Line 17-23:** CI decides the toolchain; Phase A is ignored.
2. **Line 26-28:** CI parallelizes phases that must be sequential.
3. **Line 31-35:** CI retries; if Phase E fails deterministically, retrying masks the failure.
4. **Line 38-42:** CI emits metrics; CI/CD is compute-only, not observability.
5. **Line 45-48:** CI re-validates the phase lock; the Makefile already validated this.

---

## 6. Example: Compliant CI Configuration

### 6.1 GitHub Actions Compliance

```yaml
# COMPLIES WITH REVERSE CONWAY'S LAW
name: Build

on: [push]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      # COMPLIANT: Single invocation of make universe
      - name: Universe construction
        run: make -C /home/user/qlever universe
        # No environment variables set
        # No conditional logic
        # No retry loop
        # Single target: universe

      # COMPLIANT: Exit code validation only
      - name: Verify phase lock
        run: |
          EXIT_CODE=$?
          if [ $EXIT_CODE -ne 0 ]; then
            echo "Universe construction failed with exit code $EXIT_CODE"
            exit $EXIT_CODE
          fi
```

**Why this complies:**
1. **Line 16:** Single invocation: `make universe`.
2. **No conditional toolchain injection:** CI trusts Phase A.
3. **No parallel execution:** Phases execute sequentially via Makefile.
4. **No retry logic:** If it fails, it failed deterministically.
5. **No metric emission:** CI/CD is silent on success.
6. **No re-validation:** The Makefile is authoritative.

### 6.2 GitLab CI Compliance

```yaml
# COMPLIES WITH REVERSE CONWAY'S LAW
build:
  stage: build
  script:
    - make -C /home/user/qlever universe
  # No artifacts collection
  # No metrics reporting
  # No conditional logic
  # No environment variable override
```

**Why this complies:**
1. Single invocation: `make universe`.
2. No logic beyond invoking the Makefile.
3. GitLab's implicit exit code validation handles the rest.

### 6.3 Jenkins Compliance

```groovy
// COMPLIES WITH REVERSE CONWAY'S LAW
pipeline {
    agent any

    stages {
        stage('Universe Construction') {
            steps {
                sh 'make -C /home/user/qlever universe'
            }
        }
    }

    // No post actions
    // No metrics collection
    // No conditional gating
}
```

**Why this complies:**
1. Single step: invoke `make universe`.
2. Jenkins' implicit exit code validation handles success/failure.
3. No post-build logic.

---

## 7. Enforcement

### 7.1 CI/CD Configuration Review

All CI/CD pipelines (GitHub Actions, GitLab CI, Jenkins, CircleCI, etc.) must be audited against this document.

**Checklist:**
- [ ] Single invocation: `make universe`
- [ ] No environment variable overrides (CXX, CXXFLAGS, CMAKE_BUILD_TYPE, etc.)
- [ ] No conditional branching based on git event, branch name, or PR status
- [ ] No retry loops
- [ ] No metric emission or observability hooks
- [ ] No post-build validation or re-verification
- [ ] No parallel phase execution

### 7.2 Non-Compliance Consequences

CI/CD pipelines that violate this policy must be corrected immediately. Non-compliant pipelines:
- Obscure root causes of failures
- Introduce non-determinism
- Violate the organizational structure prescribed by the Makefile
- Breach Reverse Conway's Law

---

## 8. References

- **Makefile (Authority):** `/home/user/qlever/Makefile`
- **Phase Documentation:** Makefile comments (lines 1-172)
- **Artifact Manifest:** `/home/user/qlever/.artifacts/manifest.sha256`
- **Phase Lock:** `/home/user/qlever/.artifacts/.phase.lock`

---

## Appendix: Glossary

| Term | Definition |
|------|-----------|
| **Reverse Conway's Law** | Prescriptive principle: code architecture determines organizational structure and process boundaries. |
| **Compute-Only Role** | CI/CD performs deterministic execution only; no branching, interpretation, or decision-making. |
| **Fail-Closed Semantics** | If any phase fails, the entire construction fails; no partial states permitted. |
| **Universe Target** | Single entry point in Makefile; represents complete construction from toolchain sealing through artifact finalization. |
| **Phase Lock** | File (`/.artifacts/.phase.lock`) indicating successful completion of all phases. |
| **Artifact Manifest** | SHA-256 hashes of all constructed binaries and libraries; immutable and read-only post-sealing. |
| **Narrative** | Observable metrics, logging messages, or performance descriptions emitted by CI/CD; prohibited. |

---

**Document Authority:** EPIC 8 - CI/CD Relegation
**Effective Date:** 2026-01-01
**Status:** Prescriptive (Mandatory Compliance)
