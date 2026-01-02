# Toolchain Pinning Specification
## Agent 4: Deterministic Environment Requirements

**Status**: INTEGRATION PHASE - EPIC 11
**Purpose**: Establish immutable toolchain constraints for Rust cache verification
**Principle**: Determinism over flexibility. Lock versions, not ranges.

---

## 1. Rust Toolchain Pinning

### 1.1 Current Locked Versions
- **rustc**: `1.91.1 (ed61e7d7e 2025-11-07)`
- **cargo**: `1.91.1 (ea2d97820 2025-10-10)`
- **Channel**: `stable`

### 1.2 Rust Toolchain File
**File**: `rust-toolchain.toml` (currently not present - using system default)

**Recommended content** (if creating):
```toml
[toolchain]
channel = "1.91.1"
components = ["rustfmt", "clippy"]
targets = ["x86_64-unknown-linux-gnu"]
```

**Decision**: Currently relying on documented minimum version (1.91.0+). If determinism issues arise, create `rust-toolchain.toml` with exact version pin.

### 1.3 Rust Version Constraints
- **Minimum**: `1.91.0` (hard requirement)
- **Recommended**: `1.91.1` (tested and verified)
- **Maximum**: None (forward compatibility assumed for minor versions)

---

## 2. C++ Toolchain Pinning

### 2.1 CMake Requirements
- **Minimum Version**: `3.27.0` (enforced in CMakeLists.txt line 1)
- **Current Version**: `3.28.3`
- **Standard**: C++20 (CMAKE_CXX_STANDARD 20, line 10)

### 2.2 C++ Compiler Requirements

#### GCC (Primary)
- **Minimum**: `11.0.0` (enforced in CMakeLists.txt line 48-49)
- **Current**: `13.3.0`
- **Coroutines Flag**: `-fcoroutines` (auto-applied for GCC >11.0)

#### Clang/LLVM (Alternative)
- **Minimum**: `16.0.0` (enforced in CMakeLists.txt line 55-56)
- **Current**: `18.1.3`
- **Range-v3 Note**: Clang 16 requires range-v3; Clang 17+ uses std::ranges

### 2.3 C++ Standard Library
- **glibc Version**: `2.39` (current)
- **Minimum**: `2.31+` (recommended for C++20 support)

---

## 3. System Dependencies Pinning

### 3.1 Required System Libraries
Per EPIC 10.2, only these system libraries are permitted:

1. **ICU** (International Components for Unicode)
   - **Minimum**: `60.0`
   - **Components**: `uc`, `i18n`
   - **REQUIRED**: Yes (build fails if missing - CMakeLists.txt lines 185-208)
   - **Dev Headers**: Must be present (`libicu-dev` package)

2. **libc** (glibc)
   - **Version**: `2.39` (current)
   - **Minimum**: `2.31+`

3. **libm** (math library)
   - Provided by glibc

### 3.2 Optional System Libraries (with fallbacks)
- **jemalloc**: Performance benefit, not required (CMakeLists.txt lines 214-237)
- **zstd**: Required but can be vendored (CMakeLists.txt lines 240-246)
- **Boost**: `1.81+` required (CMakeLists.txt line 252)
- **OpenSSL**: Required (CMakeLists.txt line 259)

### 3.3 Vendored Dependencies (Git Submodules/FetchContent)
All other dependencies are vendored via CMake FetchContent:
- Google Test (line 124)
- nlohmann-json (line 134)
- ANTLR4 C++ runtime (line 146)
- range-v3 (line 160)
- libspatialjoin (line 169)
- Abseil (line 334)
- S2 Geometry (line 344)
- FSST (line 394)
- RE2 (line 404)
- CTRE (line 322)
- **simdjson** (vendored at `vendors/simdjson` - lines 493-507, EPIC 7/10.2 sealed)

---

## 4. Environment Variables Affecting Determinism

### 4.1 Rust Compilation Flags
```bash
# RUSTFLAGS - affects code generation and cache keys
export RUSTFLAGS=""  # Default: empty (no additional flags)

# CARGO_BUILD_TARGET - target triple
export CARGO_BUILD_TARGET="x86_64-unknown-linux-gnu"  # Default for x86_64 Linux

# CARGO_INCREMENTAL - disable for deterministic builds
export CARGO_INCREMENTAL=0  # Recommended for CI
```

### 4.2 C++ Compilation Flags
```bash
# CFLAGS / CXXFLAGS - additional compiler flags
export CFLAGS=""
export CXXFLAGS=""

# CMake build type affects flags
CMAKE_BUILD_TYPE=Release  # -O3 optimization
CMAKE_BUILD_TYPE=Debug    # Debug symbols, no optimization
CMAKE_BUILD_TYPE=ASAN     # Address sanitizer
```

### 4.3 CMake Variables
```bash
# Additional compiler flags (CMakeLists.txt line 286)
-DADDITIONAL_COMPILER_FLAGS=""

# Additional linker flags (CMakeLists.txt lines 295-296)
-DADDITIONAL_LINKER_FLAGS=""

# Precompiled headers (CMakeLists.txt line 30)
-DUSE_PRECOMPILED_HEADERS=ON  # Default: ON
```

---

## 5. CPU Architecture and Features

### 5.1 Supported Architectures
- **Primary**: `x86_64` (64-bit Intel/AMD)
- **Secondary**: `aarch64` (64-bit ARM) - assumed supported by Rust

### 5.2 CPU Feature Flags (x86_64)
Current environment supports:
- **AVX2**: Yes
- **AVX512F/DQ/BW/VL**: Yes
- **POPCNT**: Yes
- **AES-NI**: Yes
- **SHA-NI**: Yes

**Note**: CPU feature flags do NOT affect Rust cache keys by default (unless explicitly set in RUSTFLAGS with `-C target-cpu=...`)

### 5.3 Target CPU Settings
```bash
# Default: generic x86_64 (portable)
RUSTFLAGS="-C target-cpu=x86-64"

# Optimized for native CPU (NOT deterministic across machines)
RUSTFLAGS="-C target-cpu=native"  # AVOID in CI

# Specific baseline (recommended for determinism)
RUSTFLAGS="-C target-cpu=x86-64-v3"  # AVX2, FMA, BMI2
```

---

## 6. Deterministic Build Requirements

### 6.1 Cache Key Components
The following components MUST match for Rust cache reuse:
1. **rustc version** (exact match)
2. **RUSTFLAGS** (exact match)
3. **Cargo.toml dependencies** (hash match)
4. **Source code** (hash match)
5. **Target triple** (exact match)

### 6.2 Non-Deterministic Factors (to avoid)
- **Build timestamps** (not included in cache keys)
- **Absolute paths** (normalized by Cargo)
- **User/hostname** (not included in cache keys)
- **CPU features** (unless explicitly set in RUSTFLAGS)

### 6.3 CI Environment Standardization
For CI builds, enforce:
```bash
export CARGO_INCREMENTAL=0
export RUSTFLAGS="-C target-cpu=x86-64"
export CARGO_BUILD_TARGET="x86_64-unknown-linux-gnu"
```

---

## 7. Verification Scripts

### 7.1 Environment Snapshot Script
**File**: `qlever-verification/scripts/environment_snapshot.sh`

**Purpose**: Capture current toolchain environment as JSON

**Usage**:
```bash
./qlever-verification/scripts/environment_snapshot.sh [output_file]
# Default output: environment.json
```

**Validation**:
- Script is idempotent (safe to run multiple times)
- Outputs valid JSON (validated with `jq` if available)
- Supports x86_64 and aarch64 Linux

### 7.2 Environment JSON Template
**File**: `qlever-verification/environment.json.template`

**Purpose**: Template for CI systems to fill with actual environment data

**Usage**: CI jobs copy this template and substitute placeholder values

---

## 8. Change Control Protocol

### 8.1 Toolchain Version Updates
**Process**:
1. Propose new version in GitHub issue
2. Test on clean environment (no cached artifacts)
3. Run full test suite (all 289+ tests)
4. Update this document (TOOLCHAIN_PINNING.md)
5. Update rust-toolchain.toml (if exists)
6. Document in CHANGELOG

### 8.2 Breaking Changes
**Definition**: Any change that invalidates existing Rust cache

**Examples**:
- rustc version upgrade
- RUSTFLAGS modification
- Target triple change

**Mitigation**: Clear cache (`cargo clean`) before merging

---

## 9. Platform Support Matrix

| Platform       | Architecture | rustc  | CMake  | GCC   | Clang | Status   |
|----------------|--------------|--------|--------|-------|-------|----------|
| Linux (Ubuntu) | x86_64       | 1.91.1 | 3.28.3 | 13.3  | 18.1  | Primary  |
| Linux (Ubuntu) | aarch64      | 1.91+  | 3.27+  | 11.0+ | 16.0+ | Untested |
| macOS          | x86_64       | 1.91+  | 3.27+  | N/A   | 16.0+ | Untested |
| macOS          | aarch64      | 1.91+  | 3.27+  | N/A   | 16.0+ | Untested |

**Primary Platform**: Linux x86_64 (Ubuntu 24.04, glibc 2.39)

---

## 10. References

- **CMakeLists.txt**: `/home/user/qlever/CMakeLists.txt` (lines 1-552)
- **Rust Version**: Output of `rustc --version`
- **CMake Version**: Output of `cmake --version`
- **EPIC 10.2**: Reproducible builds, system library constraints
- **EPIC 11**: Multi-agent integration, cache verification subsystem

---

## Document Metadata

- **Agent**: 4 (Integration Phase)
- **Slice**: Toolchain pinning + environment fingerprint
- **Status**: COMPLETE
- **Proof**: environment_snapshot.sh produces valid JSON on x86_64 Linux
- **Claim**: `.claude/claims/integration-agent-4.claim`
- **Last Updated**: 2026-01-02
