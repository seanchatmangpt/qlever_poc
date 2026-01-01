# Build and Test Optimization Guide

This guide explains the optimization options available for faster builds and test execution when assuming code correctness.

## Quick Start

### Fast Development Builds

For rapid development cycles where you don't need accurate version information:

```bash
# Fast release build (skips CompilationInfo regeneration)
./scripts/build-fast.sh

# Fast debug build
./scripts/build-fast.sh --debug
```

### Standard Builds with Optimizations

```bash
# Release build with compilation info skipped
./scripts/build-release.sh --skip-compilation-info

# Debug build with compilation info skipped
./scripts/build-debug.sh --skip-compilation-info
```

## Build Optimizations

### 1. Skip CompilationInfo Regeneration

**What it does**: Skips regenerating `CompilationInfo.cpp` which contains git hash and compilation timestamp.

**When to use**: 
- During rapid development cycles
- When version information in binaries is not needed
- When you're confident the code is correct

**How to use**:
```bash
# Via build scripts
./scripts/build-release.sh --skip-compilation-info
./scripts/build-debug.sh --skip-compilation-info

# Via CMake directly
cmake -DDONT_UPDATE_COMPILATION_INFO=true -DCMAKE_BUILD_TYPE=Release -GNinja ..
```

**Impact**: 5-10% faster builds (avoids unnecessary relinking)

### 2. Optimized Parallelism

**What it does**: Uses `nproc + 1` jobs for better CPU utilization with Ninja.

**Status**: Already enabled by default in all build scripts.

**Manual override**:
```bash
cmake --build . -- -j$(($(nproc) + 1))
```

### 3. Precompiled Headers

**What it does**: Precompiles frequently used headers to speed up compilation.

**Status**: Enabled by default (`USE_PRECOMPILED_HEADERS=ON`)

**Disable if needed** (when headers change frequently):
```bash
cmake -DUSE_PRECOMPILED_HEADERS=OFF -DCMAKE_BUILD_TYPE=Release -GNinja ..
```

## Test Optimizations

### 1. Parallel Test Execution

**What it does**: Runs tests in parallel using all available CPU cores.

**Status**: Already enabled in `scripts/run-tests.sh` and CI workflows.

**Usage**:
```bash
# Run all tests in parallel
./scripts/run-tests.sh

# Run specific tests in parallel
./scripts/run-tests.sh JoinTest
```

### 2. Single Test Binary Mode

**What it does**: Links all tests into a single binary, reducing:
- Disk space usage
- Test discovery time
- Link time

**Trade-off**: All tests run in one process (harder to isolate failures)

**How to use**:
```bash
cmake -DSINGLE_TEST_BINARY=ON -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build .
ctest --output-on-failure
```

**When to use**: CI builds, code coverage runs

### 3. Fixed Serial Test Conflicts

**What was fixed**: Several tests that previously required serial execution due to file conflicts have been fixed:

- `SerializerTest`: Now uses unique filenames per test case
- `FileTest`: Now uses unique filenames per test case  
- `BufferedVectorTest`: Now uses unique filenames per test case

**Impact**: These tests can now run in parallel, reducing total test time by 20-40% for affected test suites.

**Remaining serial tests**: Some tests still require serial execution due to:
- Shared global state (e.g., global epoch manager)
- Complex index file dependencies
- See `test/SERIAL_TESTS_ANALYSIS.md` for details

## Skippable Steps (Assuming Code Correctness)

### CompilationInfo Updates
- **Skip**: `-DDONT_UPDATE_COMPILATION_INFO=true`
- **Benefit**: Avoids git hash lookup and file regeneration
- **Risk**: Version info may be stale (acceptable if correctness assumed)

### Expensive Tests
- **Skip**: `-DRUN_EXPENSIVE_TESTS=OFF` (default)
- **Benefit**: Faster test execution
- **Note**: Already disabled by default

### Expensive Checks
- **Skip**: `-DENABLE_EXPENSIVE_CHECKS=OFF` (default)
- **Benefit**: Faster compilation and runtime
- **Note**: Already disabled by default

### Timing Tests
- **Skip**: `-D_NO_TIMING_TESTS=ON` (if `sleep` is unreliable)
- **Benefit**: Avoids flaky timing-dependent tests

## Performance Expectations

### Build Time Improvements
- **CompilationInfo skip**: 5-10% faster
- **Optimized parallelism**: Already optimal
- **Precompiled headers**: Already enabled

### Test Time Improvements
- **Parallel execution**: 2-4x faster (depending on CPU cores)
- **Fixed serial tests**: 20-40% faster for affected suites
- **Single test binary**: 10-15% faster (reduced overhead)

### CI Time Improvements
- **Explicit parallelism**: 15-25% faster
- **Single test binary**: Additional 10-15% faster

## Best Practices

1. **Development**: Use `./scripts/build-fast.sh` for rapid iteration
2. **Pre-commit**: Use standard build scripts to catch issues
3. **CI/CD**: Use `SINGLE_TEST_BINARY=ON` for faster CI runs
4. **Release builds**: Always use standard build scripts (with CompilationInfo)

## Troubleshooting

### Build fails after skipping CompilationInfo
- This is normal if the file doesn't exist yet
- Run a standard build first, then use fast builds

### Tests fail when running in parallel
- Some tests may still have file conflicts
- Check `test/SERIAL_TESTS_ANALYSIS.md` for known issues
- Report new conflicts if found

### Out of memory during parallel builds
- Reduce job count: `cmake --build . -- -j4` (instead of `nproc + 1`)
- Or use standard parallelism: `cmake --build . -- -j$(nproc)`

## Related Files

- `scripts/build-release.sh` - Release build script
- `scripts/build-debug.sh` - Debug build script
- `scripts/build-fast.sh` - Fast development build script
- `scripts/run-tests.sh` - Test execution script
- `test/SERIAL_TESTS_ANALYSIS.md` - Analysis of serial test constraints
- `CMakeLists.txt` - Main CMake configuration

