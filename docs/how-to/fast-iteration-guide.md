# QLever Fast Iteration Guide

This guide explains how to achieve per-second rebuilds when developing QLever, enabling efficient iteration on specific components.

## Quick Start (10-second setup)

```bash
# One-time setup (install ccache)
sudo apt-get install ccache

# Start fast iterations
cd /home/user/qlever
./scripts/fast-build.sh ServerMain build

# Rebuild after changes
./scripts/rebuild.sh ServerMain

# Watch for changes and auto-rebuild
./scripts/fast-build.sh ServerMain watch
```

## Expected Build Times

### First Build
- **Full build** (cold cache): 10-15 minutes
- **With ccache** (warm): 5-8 minutes

### Incremental Rebuilds
- **Single file change** (ccache hit): 2-5 seconds
- **Header change** (rebuild few dependents): 10-30 seconds
- **Large refactor** (rebuild many targets): 1-3 minutes

### With RAM Disk
- **Single file change**: 1-2 seconds
- Requires: `USE_RAMDISK=yes ./scripts/fast-build.sh ServerMain build`

## Caching Strategies

### 1. ccache (Compiler Cache)

**What it does**: Caches compiled object files, enabling instant rebuilds when code hasn't changed.

**Setup** (automatic):
```bash
./scripts/fast-build.sh ServerMain build
```

**Statistics**:
```bash
ccache -s
```

**Clear cache** (if corrupted):
```bash
ccache -C
rm -rf ~/.cache/ccache/*
```

**Configuration**:
- Location: `~/.cache/ccache/`
- Size: 10GB (configurable via `CCACHE_SIZE`)
- Hash sloppiness: Allows rebuilds without recompilation if implementation unchanged

### 2. Ninja Incremental Build

**What it does**: Tracks dependencies and only recompiles affected files.

**Advantages**:
- Header changes are detected automatically
- Minimal rebuild of dependent targets
- Built-in parallelization (-j flag)

**Usage**:
```bash
cd build
ninja ServerMain -j8      # Rebuild only what changed
ninja ServerMain -j1      # Sequential (for debugging)
```

### 3. RAM Disk (Optional, for Ultra-Fast Builds)

**What it does**: Builds in RAM instead of disk, providing ~2-3x speedup.

**Usage**:
```bash
USE_RAMDISK=yes ./scripts/fast-build.sh ServerMain build
```

**Requirements**:
- ~4GB free RAM
- Linux with tmpfs support (standard on all Linux systems)

**Caveats**:
- Build artifacts lost on reboot
- Requires `sudo` access to mount `/mnt/qlever-ramdisk`
- Use only during development, not for CI/CD

## Per-Second Iteration Workflow

### Scenario: Fixing a Single Source File

1. **Initial build** (cold cache, ~10 minutes):
```bash
./scripts/fast-build.sh ServerMain build
```

2. **Edit source file**:
```bash
vim src/engine/GroupBy.cpp
```

3. **Rebuild** (~3-5 seconds with warm ccache):
```bash
./scripts/rebuild.sh ServerMain
```

4. **Watch for changes** (hands-free iteration):
```bash
./scripts/fast-build.sh ServerMain watch
```

### Scenario: Modifying a Header File

1. **Edit header**:
```bash
vim src/engine/GroupBy.h
```

2. **Rebuild** (~10-30 seconds, rebuilds all dependents):
```bash
./scripts/rebuild.sh ServerMain
```

3. **Monitor progress**:
```bash
./scripts/rebuild.sh ServerMain VERBOSE=1
```

### Scenario: Index Builder Development

```bash
# First time
./scripts/fast-build.sh IndexBuilderMain build

# Iterative changes
./scripts/rebuild.sh IndexBuilderMain

# Watch mode
./scripts/fast-build.sh IndexBuilderMain watch
```

## Advanced: Manual Ninja Commands

**For maximum control**, use Ninja directly:

```bash
cd build

# Rebuild single target
ninja ServerMain -j8

# Rebuild with verbose output
ninja ServerMain -j8 -v

# List all targets
ninja -t targets all | head -20

# Analyze rebuild graph
ninja -t graph ServerMain | dot -Tpng > graph.png

# Dry run (show what would be rebuilt)
ninja -n ServerMain
```

## Troubleshooting

### Builds Not Using Cache

**Problem**: Rebuild takes as long as fresh build

**Solution**: Check cache stats
```bash
ccache -s
```

**Common causes**:
1. Modified system includes (ccache skips if compiler changes)
2. Absolute paths in build (use `CCACHE_BASEDIR`)
3. Different compile flags between builds

**Fix**:
```bash
rm -rf build/
./scripts/fast-build.sh ServerMain build
```

### Build Fails After ccache

**Problem**: Compilation succeeds but cached artifact fails

**Solution**: Clear ccache and rebuild
```bash
ccache -C
./scripts/rebuild.sh ServerMain
```

### RAM Disk Issues

**Problem**: "Permission denied" when creating RAM disk

**Solution**: Use /tmp instead
```bash
# Scripts auto-fallback to /tmp if /mnt fails
USE_RAMDISK=yes ./scripts/fast-build.sh ServerMain build
```

**Check RAM disk status**:
```bash
mount | grep tmpfs
df -h | grep tmpfs
```

### Out of Disk Space

**Problem**: Build fails with "No space left on device"

**Solution**: Clean ccache
```bash
ccache -C           # Clear all cache
rm -rf ~/.cache/ccache/*
```

**Monitor disk usage**:
```bash
du -sh ~/.cache/ccache/
df -h
```

## CMake Configuration for Fast Builds

The build system is already configured for fast iteration:

```cmake
# Precompiled headers (10-30% speedup)
target_precompile_headers(engine PRIVATE ...)

# ccache integration (automatic if installed)
CMAKE_C_COMPILER_LAUNCHER = ccache
CMAKE_CXX_COMPILER_LAUNCHER = ccache

# Optimized for development
BUILD_TESTING = OFF  # Skip building all tests
SINGLE_TEST_BINARY = ON  # Single test binary (faster linking)
```

## Performance Analysis

### Measuring Build Times

```bash
# Time a single rebuild
time ./scripts/rebuild.sh ServerMain

# Profile CMake configuration
cmake ... --debug-output

# Analyze Ninja build
ninja -d stats
```

### ccache Hit Rate Target

- **Ideal**: >90% hit rate after initial builds
- **Good**: >75% hit rate
- **Poor**: <50% hit rate (investigate configuration)

**Check hit rate**:
```bash
ccache -s | grep "cache hit rate"
```

## Continuous Development Workflow

### Recommended Setup

1. **Terminal 1** (Watch & Build):
```bash
./scripts/fast-build.sh ServerMain watch
```

2. **Terminal 2** (Edit Code):
```bash
vim src/engine/GroupBy.cpp
# Terminal 1 automatically rebuilds
```

3. **Terminal 3** (Run Tests):
```bash
cd build && ctest -R "GroupBy" --output-on-failure
```

### IDE Integration

**VS Code** (with CMake extension):
1. Install CMake extension
2. Select preset: `conan-release`
3. Build with Ctrl+Shift+B
4. Test with Ctrl+Shift+T

**CLion**:
1. Open CMakeLists.txt
2. CMake cache should auto-populate
3. Build → Build Project (Ctrl+F9)
4. Run → Run with ccache preset

## Summary Table

| Scenario | Build Time | Setup |
|----------|-----------|-------|
| Fresh build (no cache) | 10-15 min | `./scripts/fast-build.sh` |
| Warm ccache rebuild | 5-8 min | None (automatic) |
| Single file change | 2-5 sec | None |
| Header change | 10-30 sec | None |
| With RAM disk | -50% | `USE_RAMDISK=yes` |
| Watch mode | 0 sec (auto) | `./scripts/fast-build.sh watch` |

## See Also

- [Build System Documentation](../architecture/build-system.md)
- [CMake Configuration](../../CMakeLists.txt)
- [Development Environment Setup](../how-to/claude-code-setup.md)
