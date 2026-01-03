# QLever Build System - 80/20 First Principles
# Three essential commands: build, test, benchmark

.PHONY: setup configure build test benchmark clean validate-testcontainers

BUILD_DIR := build

# setup: One-time development environment setup
setup:
	@bash scripts/setup-dev-env.sh

# configure: Configure CMake (run after setup or when CMakeLists.txt changes)
configure:
	@cmake -B $(BUILD_DIR) -GNinja -DCMAKE_BUILD_TYPE=Release ..

# build: Compile all C++ code (assumes CMake already configured)
build:
	@test -f $(BUILD_DIR)/build.ninja || (echo "FATAL: CMake not configured. Run: make configure"; exit 1)
	@cd $(BUILD_DIR) && ninja -j$$(nproc)

# test: Run all tests
test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure -j$$(nproc)

# benchmark: Run all benchmark executables
benchmark: build
	@cd $(BUILD_DIR)/benchmark && \
		for exe in BenchmarkExamples JoinAlgorithmBenchmark ParallelMergeBenchmark \
			GroupByHashMapBenchmark ConstructBenchmark ConstructAdvancedBenchmark \
			N3BenchmarkTest RdfParserBenchmark EpochBenchmark EpochManifestBenchmark \
			CanonicalBenchmark ReadCacheBench RegressionGate ingress_throughput \
			query_latency_distribution FFIGatekeeperBenchmark; do \
			if [ -f $$exe ]; then \
				echo "Running $$exe..."; \
				./$$exe || true; \
			fi; \
		done

# clean: Remove build directory
clean:
	@rm -rf $(BUILD_DIR)

# validate-testcontainers: Validate build process in clean containers using Testcontainers
# Tests TRIZ implementation: prerequisites ensure first-time success
validate-testcontainers:
	@echo "Validating build process with Testcontainers..." >&2
	@command -v python3 >/dev/null 2>&1 || (echo "FATAL: python3 not found"; exit 1)
	@command -v docker >/dev/null 2>&1 || (echo "FATAL: docker not found"; exit 1)
	@python3 -m pip install -q -r requirements-testcontainers.txt 2>/dev/null || (echo "Installing testcontainers..."; python3 -m pip install testcontainers docker)
	@python3 scripts/validate-with-testcontainers.py
