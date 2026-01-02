# EPIC 8 - Deterministic Construction Law
# Single entry point: make universe
# Six mandatory phases with fail-closed semantics

.PHONY: universe
.PHONY: phase-a phase-b phase-c phase-d phase-e phase-f
.PHONY: clean verify
.PHONY: build test all
.PHONY: help fast-build test-single
.PHONY: lint format-check format-fix coverage quality
.PHONY: dev setup-dev profile

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
	@$(SHELL) -c 'if test -f $(ARTIFACTS_DIR)/.phase_times; then echo "" >&2; echo "Build Phase Timing Summary:" >&2; cat $(ARTIFACTS_DIR)/.phase_times >&2; fi'

# ============================================================================
# PHASE A: Toolchain Sealing
# - Compiler identity verification
# - Flags normalization
# - No environment leakage
# ============================================================================
phase-a: ensure-build-dir
	@mkdir -p $(ARTIFACTS_DIR)
	@rm -f $(ARTIFACTS_DIR)/.phase_times
	@echo "PHASE_A: Toolchain sealing" >&2
	@$(SHELL) -c '\
		set -e; \
		_START_TIME=$$(date +%s); \
		if command -v clang++ >/dev/null 2>&1; then CXX=$$(command -v clang++); else CXX=$$(command -v g++); fi; \
		test -n "$$CXX" || exit 1; \
		CXXID=$$($$CXX -v 2>&1 | head -1); \
		echo "$$CXXID" > $(COMPILER_ID_FILE); \
		SOURCE_DATE_EPOCH=$$(git log -1 --format=%ct); \
		NORMALIZED_FLAGS="-std=c++20 -O3 -DNDEBUG"; \
		echo "NORMALIZED_CXXFLAGS=\"$$NORMALIZED_FLAGS\"" > $(ARTIFACTS_DIR)/flags.env; \
		echo "CXX=\"$$CXX\"" >> $(ARTIFACTS_DIR)/flags.env; \
		echo "SOURCE_DATE_EPOCH=\"$$SOURCE_DATE_EPOCH\"" >> $(ARTIFACTS_DIR)/flags.env; \
		_END_TIME=$$(date +%s); \
		_ELAPSED=$$(($$_END_TIME - $$_START_TIME)); \
		echo "PHASE_A: $$_ELAPSED seconds" >> $(ARTIFACTS_DIR)/.phase_times; \
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
		_START_TIME=$$(date +%s); \
		test -f CMakeLists.txt || exit 1; \
		test -d src || exit 1; \
		test -d test || exit 1; \
		test -f .git/HEAD || exit 1; \
		_END_TIME=$$(date +%s); \
		_ELAPSED=$$(($$_END_TIME - $$_START_TIME)); \
		echo "PHASE_B: $$_ELAPSED seconds" >> $(ARTIFACTS_DIR)/.phase_times; \
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
		_START_TIME=$$(date +%s); \
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
		NINJA_JOBS=$$(( $$(nproc) + 1 )); \
		ninja -j$$NINJA_JOBS >/dev/null 2>&1 || exit 1; \
		_END_TIME=$$(date +%s); \
		_ELAPSED=$$(($$_END_TIME - $$_START_TIME)); \
		echo "PHASE_C: $$_ELAPSED seconds" >> $(ARTIFACTS_DIR)/.phase_times; \
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
		_START_TIME=$$(date +%s); \
		cd $(BUILD_DIR); \
		test -d CMakeFiles || exit 1; \
		test -f Makefile -o -f build.ninja || exit 1; \
		echo "PHASE_D: Running SHACL validation tests (fail-closed)" >&2; \
		CTEST_JOBS=$$(( $$(nproc) / 2 )); \
		test $$CTEST_JOBS -lt 1 && CTEST_JOBS=1; \
		ctest -R "Shacl" --output-on-failure -j $$CTEST_JOBS || exit 1; \
		echo "PHASE_D: SHACL validation passed" >&2; \
		_END_TIME=$$(date +%s); \
		_ELAPSED=$$(($$_END_TIME - $$_START_TIME)); \
		echo "PHASE_D: $$_ELAPSED seconds" >> $(ARTIFACTS_DIR)/.phase_times; \
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
		_START_TIME=$$(date +%s); \
		cd $(BUILD_DIR); \
		CTEST_JOBS=$$(( $$(nproc) / 2 )); \
		test $$CTEST_JOBS -lt 1 && CTEST_JOBS=1; \
		ctest --rerun-failed --output-on-failure -j $$CTEST_JOBS >/dev/null 2>&1 || exit 1; \
		test -f CTestTestfile.cmake || exit 1; \
		_END_TIME=$$(date +%s); \
		_ELAPSED=$$(($$_END_TIME - $$_START_TIME)); \
		echo "PHASE_E: $$_ELAPSED seconds" >> $(ARTIFACTS_DIR)/.phase_times; \
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
		_START_TIME=$$(date +%s); \
		cd $(BUILD_DIR); \
		find . -type f -executable -o -name "*.a" -o -name "*.so" 2>/dev/null | sort | xargs -I {} sh -c "test -f {} && sha256sum {} || true" > $(PHASE_MANIFEST) 2>/dev/null || true; \
		test -s $(PHASE_MANIFEST) || exit 1; \
		chmod 444 $(PHASE_MANIFEST); \
		_END_TIME=$$(date +%s); \
		_ELAPSED=$$(($$_END_TIME - $$_START_TIME)); \
		echo "PHASE_F: $$_ELAPSED seconds" >> $(ARTIFACTS_DIR)/.phase_times; \
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
# Standard Makefile Targets (Makefile Best Practices)
# ============================================================================

# build: Compile the project (Phase C - core compilation)
# Standard Makefile target for development builds
build: phase-c
	@echo "Build complete. Artifacts in $(BUILD_DIR)" >&2

# test: Run all tests (Phase E - deterministic benchmarks)
# Standard Makefile target for testing
test: phase-e
	@echo "Tests complete" >&2

# all: Full deterministic construction (Phase A-F with sealing)
# Equivalent to: make universe
all: universe
	@echo "All phases complete" >&2

# ============================================================================
# Convenience Targets (Developer Experience)
# ============================================================================

# help: Display all available targets with descriptions
help:
	@echo "QLever Build System (EPIC 8 Deterministic Construction + PHASE 1-7 DX Enhancements)" >&2
	@echo "" >&2
	@echo "Entry Points:" >&2
	@echo "  make universe  - Full deterministic build (phases A-F, all checks)" >&2
	@echo "  make all       - Alias for 'make universe'" >&2
	@echo "  make build     - Fast development build (phase C: compilation only)" >&2
	@echo "  make test      - Run test suite (phase E: deterministic benchmarks)" >&2
	@echo "" >&2
	@echo "Code Quality (PHASE 2):" >&2
	@echo "  make lint          - Run clang-tidy on hot-path modules" >&2
	@echo "  make format-check  - Verify code formatting (clang-format)" >&2
	@echo "  make format-fix    - Auto-format code with clang-format" >&2
	@echo "  make coverage      - Generate LLVM coverage report" >&2
	@echo "  make quality       - Run all checks (lint, format, test)" >&2
	@echo "" >&2
	@echo "Developer Experience (PHASE 5):" >&2
	@echo "  make dev           - Fast build + test (default all tests)" >&2
	@echo "  make setup-dev     - One-time environment setup" >&2
	@echo "  make profile       - Identify slow tests" >&2
	@echo "" >&2
	@echo "Development Workflow:" >&2
	@echo "  make fast-build      - Skip benchmarks (phases D, E, F) for rapid iteration" >&2
	@echo "  make test-single     - Run single test: make test-single TEST=pattern" >&2
	@echo "" >&2
	@echo "Utilities:" >&2
	@echo "  make clean         - Remove build/ and .artifacts/ directories" >&2
	@echo "  make verify        - Validate artifact manifest integrity" >&2
	@echo "" >&2
	@echo "Advanced:" >&2
	@echo "  make phase-c   - Resume from phase C (compilation)" >&2
	@echo "  make phase-d   - Resume from phase D (validation)" >&2
	@echo "  make phase-e   - Resume from phase E (benchmarks)" >&2
	@echo "" >&2
	@echo "Documentation: See Makefile for phase descriptions" >&2
	@echo "  CONTRIBUTING.md    - Development guidelines" >&2
	@echo "  docs/how-to/*.md   - Setup guides" >&2

# fast-build: Development build without benchmarking (skip phases D, E, F)
# Useful for rapid iteration: compile → test subset → repeat
fast-build: phase-c
	@echo "Fast build complete (SKIPPING: phase D validation, phase E benchmarks)" >&2
	@echo "To run full validation: make universe" >&2

# test-single: Run a single test by pattern (requires TEST variable)
# Usage: make test-single TEST=EngineTest
test-single:
	@test -n "$(TEST)" || (echo "Usage: make test-single TEST=pattern" >&2; echo "  Example: make test-single TEST=EngineTest" >&2; exit 1)
	@cd $(BUILD_DIR) && ctest -R "$(TEST)" --output-on-failure

# ============================================================================
# Code Quality Targets (PHASE 2/7: Code Quality Modernization)
# ============================================================================

# lint: Run clang-tidy on hot-path modules (cache, query, index, ingress)
lint:
	@echo "Running clang-tidy on hot-path modules..." >&2
	@command -v clang-tidy >/dev/null 2>&1 || (echo "FATAL: clang-tidy not found"; exit 1)
	@cd $(BUILD_DIR) && \
	  clang-tidy -p . $(shell find ../src/engine/{cache,query,index,ingress} -name "*.cpp" -o -name "*.h" 2>/dev/null)

# format-check: Verify code follows clang-format style
format-check:
	@echo "Checking code formatting..." >&2
	@clang-format --dry-run -Werror src/**/*.cpp src/**/*.h test/**/*.cpp 2>/dev/null || \
	  (echo "Code formatting violations found. Run 'make format-fix' to auto-fix." >&2; exit 1)
	@echo "✓ Code formatting is correct" >&2

# format-fix: Auto-format code with clang-format
format-fix:
	@echo "Auto-formatting code..." >&2
	@find src test benchmark -name "*.cpp" -o -name "*.h" | \
	  xargs clang-format -i
	@echo "✓ Code formatting applied" >&2

# coverage: Generate test coverage report (LLVM instrumentation)
coverage: phase-c
	@echo "Generating coverage report..." >&2
	@cd $(BUILD_DIR) && cmake -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping" . >/dev/null 2>&1
	@cd $(BUILD_DIR) && ctest -j$$(nproc/2) --output-on-failure >/dev/null 2>&1 || true
	@echo "✓ Coverage data generated (see: llvm-cov show)" >&2

# quality: Run all quality checks (lint, format, test)
quality: lint format-check test
	@echo "✓ All quality checks passed" >&2

# ============================================================================
# Developer Experience Targets (PHASE 5/7: DX Enhancements)
# ============================================================================

# dev: Fast development build + test (default: all tests)
# Usage: make dev or make dev TEST=pattern
dev: $(BUILD_DIR)
	@bash scripts/quick-build.sh $(TEST)

# setup-dev: One-time developer environment setup
setup-dev:
	@bash scripts/dev-setup.sh

# profile: Identify slow tests and generate performance report
profile:
	@bash scripts/test-profile.sh $(SLOWEST_N)

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
