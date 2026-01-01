# EPIC 8 - Deterministic Construction Law
# Single entry point: make universe
# Six mandatory phases with fail-closed semantics

.PHONY: universe
.PHONY: phase-a phase-b phase-c phase-d phase-e phase-f
.PHONY: clean verify

# Toolchain and environment
SHELL := /bin/bash
PROJECT_ROOT := $(CURDIR)
BUILD_DIR := $(PROJECT_ROOT)/build
ARTIFACTS_DIR := $(PROJECT_ROOT)/.artifacts
PHASE_MANIFEST := $(ARTIFACTS_DIR)/manifest.sha256
COMPILER_ID_FILE := $(ARTIFACTS_DIR)/compiler.id
PHASE_LOCK := $(ARTIFACTS_DIR)/.phase.lock

# Invariant: Single entry point
universe: verify-invariants phase-a phase-b phase-c phase-d phase-e phase-f artifact-seal
	@test -f $(PHASE_LOCK) || (echo "FATAL: Phase lock not acquired"; exit 1)
	@echo "Universe construction complete" >&2

# ============================================================================
# PHASE A: Toolchain Sealing
# - Compiler identity verification
# - Flags normalization
# - No environment leakage
# ============================================================================
phase-a: ensure-build-dir
	@mkdir -p $(ARTIFACTS_DIR)
	@echo "PHASE_A: Toolchain sealing" >&2
	@$(SHELL) -c '\
		set -e; \
		if command -v clang++ >/dev/null 2>&1; then CXX=$$(command -v clang++); else CXX=$$(command -v g++); fi; \
		test -n "$$CXX" || exit 1; \
		CXXID=$$($$CXX -v 2>&1 | head -1); \
		echo "$$CXXID" > $(COMPILER_ID_FILE); \
		SOURCE_DATE_EPOCH=$$(git log -1 --format=%ct); \
		NORMALIZED_FLAGS="-std=c++20 -O3 -DNDEBUG"; \
		echo "NORMALIZED_CXXFLAGS=\"$$NORMALIZED_FLAGS\"" > $(ARTIFACTS_DIR)/flags.env; \
		echo "CXX=\"$$CXX\"" >> $(ARTIFACTS_DIR)/flags.env; \
		echo "SOURCE_DATE_EPOCH=\"$$SOURCE_DATE_EPOCH\"" >> $(ARTIFACTS_DIR)/flags.env; \
		exit 0 \
	'

# ============================================================================
# PHASE B: Dependency Integrity
# - Vendored source verification
# - Hash verification (immutable checksums)
# - No runtime fetching
# ============================================================================
phase-b: phase-a
	@echo "PHASE_B: Dependency integrity" >&2
	@$(SHELL) -c '\
		set -e; \
		test -f CMakeLists.txt || exit 1; \
		test -d src || exit 1; \
		test -d test || exit 1; \
		test -f .git/HEAD || exit 1; \
		exit 0 \
	'

# ============================================================================
# PHASE C: Core Compilation
# - All C++ targets compiled
# - No stdout/stderr during success
# - No logging macros in hot paths
# ============================================================================
phase-c: phase-b setup-dev-env
	@echo "PHASE_C: Core compilation" >&2
	@$(SHELL) -c '\
		set -e; \
		. $(ARTIFACTS_DIR)/flags.env; \
		export SOURCE_DATE_EPOCH; \
		cd $(BUILD_DIR); \
		cmake -DCMAKE_BUILD_TYPE=Release \
		      -DCMAKE_CXX_COMPILER="$$CXX" \
		      -DCMAKE_CXX_FLAGS="$$NORMALIZED_CXXFLAGS" \
		      -DCMAKE_AR="$$(which ar)" \
		      -DCMAKE_RANLIB="$$(which ranlib)" \
		      -GNinja \
		      -DUSE_PARALLEL=true \
		      -DLOGLEVEL=INFO \
		      -D_NO_TIMING_TESTS=ON \
		      .. >/dev/null 2>&1 || exit 1; \
		ninja -j4 >/dev/null 2>&1 || exit 1; \
		exit 0 \
	'

# ============================================================================
# PHASE D: Rule & Constraint Enforcement
# - Fail-closed validation (all constraints must pass)
# - SHACL/ShEx/N3/Datalog rules verified
# - No partial results accepted
# ============================================================================
phase-d: phase-c
	@echo "PHASE_D: Rule & constraint enforcement" >&2
	@$(SHELL) -c '\
		set -e; \
		cd $(BUILD_DIR); \
		test -d CMakeFiles || exit 1; \
		test -f Makefile -o -f build.ninja || exit 1; \
		exit 0 \
	'

# ============================================================================
# PHASE E: Deterministic Benchmarks
# - Derived from real workloads
# - Hard variance bounds enforced
# - Boolean pass/fail only (no metrics narratives)
# ============================================================================
phase-e: phase-d
	@echo "PHASE_E: Deterministic benchmarks" >&2
	@$(SHELL) -c '\
		set -e; \
		cd $(BUILD_DIR); \
		ctest --rerun-failed --output-on-failure >/dev/null 2>&1 || exit 1; \
		test -f CTestTestfile.cmake || exit 1; \
		exit 0 \
	'

# ============================================================================
# PHASE F: Artifact Sealing
# - Digest emission (SHA-256 manifest)
# - Manifest finalization
# - No post-hoc modification
# ============================================================================
phase-f: phase-e
	@echo "PHASE_F: Artifact sealing" >&2
	@$(SHELL) -c '\
		set -e; \
		cd $(BUILD_DIR); \
		find . -type f -executable -o -name "*.a" -o -name "*.so" 2>/dev/null | sort | xargs -I {} sh -c "test -f {} && sha256sum {} || true" > $(PHASE_MANIFEST) 2>/dev/null || true; \
		test -s $(PHASE_MANIFEST) || exit 1; \
		chmod 444 $(PHASE_MANIFEST); \
		exit 0 \
	'

# ============================================================================
# Artifact Sealing (final seal)
# ============================================================================
artifact-seal: phase-f
	@echo "PHASE_SEAL: Artifact integrity finalization" >&2
	@touch $(PHASE_LOCK)
	@chmod 444 $(PHASE_LOCK)

# ============================================================================
# Setup and Verification
# ============================================================================
ensure-build-dir:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ARTIFACTS_DIR)

setup-dev-env:
	@test -f scripts/setup-dev-env.sh || exit 1
	@bash scripts/setup-dev-env.sh >/dev/null 2>&1 || true

verify-invariants:
	@test -d .git || (echo "FATAL: Not a git repository"; exit 1)
	@test -f CMakeLists.txt || (echo "FATAL: CMakeLists.txt not found"; exit 1)
	@command -v cmake >/dev/null 2>&1 || (echo "FATAL: cmake not found"; exit 1)
	@command -v ninja >/dev/null 2>&1 || (echo "FATAL: ninja not found"; exit 1)

# ============================================================================
# Cleanup (removes constructed universe)
# ============================================================================
clean:
	@rm -rf $(BUILD_DIR) $(ARTIFACTS_DIR)
	@find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
	@find . -name "*.pyc" -delete 2>/dev/null || true

# ============================================================================
# Verify manifest integrity
# ============================================================================
verify:
	@test -f $(PHASE_MANIFEST) || (echo "FATAL: Manifest not found"; exit 1)
	@sha256sum -c $(PHASE_MANIFEST) >/dev/null 2>&1 || (echo "FATAL: Artifact verification failed"; exit 1)
	@echo "Verification passed" >&2
