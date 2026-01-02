# Dependency Management Strategy

QLever manages dependencies across multiple ecosystems with explicit version pinning, lock files, and security auditing. This document outlines the strategy and procedures.

## Table of Contents

1. [Ecosystem Overview](#ecosystem-overview)
2. [Version Pinning Strategy](#version-pinning-strategy)
3. [Lock File Management](#lock-file-management)
4. [Security Auditing](#security-auditing)
5. [Updating Dependencies](#updating-dependencies)
6. [Troubleshooting](#troubleshooting)

---

## Ecosystem Overview

QLever uses four dependency management systems:

| System | Language | Tool | Lock File | Status |
|--------|----------|------|-----------|--------|
| **Conan** | C++ (System) | conan | conanfile.txt (no lock) | ⚠ Needs conan.lock |
| **CMake FetchContent** | C++ (Vendored) | cmake | CMakeLists.txt | ✅ Git commits |
| **Cargo** | Rust | cargo | Cargo.lock | ✅ Present |
| **npm** | JavaScript | npm | package-lock.json | ✅ Present |

### Ecosystem Details

#### C++ System Dependencies (Conan)

**File**: `conanfile.txt`

**Current Dependencies**:
- boost/1.81.0 - Networking, async, containers
- icu/76.1 - Unicode/collation (REQUIRED)
- openssl/3.1.1 - TLS/SSL encryption
- zstd/1.5.5 - Compression
- jemalloc/5.3.0 - Memory allocator (currently disabled, Conan2 recipe broken)

**Version Bounds**:
- ⚠ Open-ended: Dependencies permit minor/patch version changes
- Example: `boost/1.81.0` allows >= 1.81.0, < 2.0.0 (semver compatible)

**Lock File Status**:
- ❌ NO conan.lock file currently
- **Action Required**: Generate with `conan lock create conanfile.txt`

**Install Command**:
```bash
# Using lock file (reproducible)
conan install . --lockfile=conan.lock --lockfile-partial

# Without lock (flexible, but less reproducible)
conan install .
```

#### C++ Vendored Dependencies (FetchContent)

**File**: `CMakeLists.txt` (lines 102-412)

**12 FetchContent declarations**:
- googletest (GIT_TAG: commit hash)
- nlohmann-json (URL_HASH: SHA3 verification)
- antlr (GIT_TAG: semantic version)
- range-v3 (GIT_TAG: custom fork)
- spatialjoin, ctre, abseil, s2, fsst, re2 (mixed pinning)
- simdjson (Git submodule, hardwired)

**Version Control**: Git commits + tags (immutable, deterministic)

**Lock Mechanism**: Git history provides reproducibility

**Update Procedure**:
```bash
# Update single dependency
cd build
git -C _deps/abseil-src fetch origin && \
  git -C _deps/abseil-src checkout <new_commit>
# Then update CMakeLists.txt GIT_TAG
```

#### Rust Dependencies (Cargo)

**Files**:
- `rust/Cargo.toml` - Dependency specifications
- `rust/Cargo.lock` - Lock file (checked in, tracked)

**Lock Status**: ✅ PRESENT, TRACKED

**Update Procedure**:
```bash
cd rust
cargo update                    # Update to allowed versions
cargo update --package <name>   # Update single package
git add Cargo.lock && git commit "chore(rust): Update dependencies"
```

**Audit**:
```bash
cd rust
cargo audit                     # Check for CVEs
cargo audit --deny warnings     # Fail on warnings
```

#### JavaScript Dependencies (npm)

**Files**:
- `js/package.json` - Dependency specifications with semver ranges
- `js/package-lock.json` - Lock file (checked in, tracked, 217KB)

**Lock Status**: ✅ PRESENT, TRACKED

**Update Procedure**:
```bash
cd js
npm update                      # Update to allowed versions
npm install <package>@latest    # Update single package
npm ci                          # Install from lock file (CI)
git add package-lock.json && git commit "chore(js): Update dependencies"
```

**Audit**:
```bash
cd js
npm audit                       # Report vulnerabilities
npm audit fix                   # Auto-fix compatible versions
npm audit fix --force           # Force breaking changes
```

---

## Version Pinning Strategy

### Rationale

**Goal**: Balance reproducibility with security updates

**Trade-off**:
- 100% locked: Requires manual updates for security patches
- 100% flexible: Non-deterministic rebuilds, dependency conflicts

**QLever Approach**: Lock files + semver ranges

### Version Bound Types

| Type | Example | Behavior | Use Case |
|------|---------|----------|----------|
| **Exact** | 1.81.0 | Only 1.81.0 | Conan (explicit) |
| **Patch** | ~1.81.0 | 1.81.x only | Critical security |
| **Minor** | ^1.81.0 | 1.x.y >= 1.81.0 | Standard (npm/Cargo) |
| **Major** | *1.x | Any 1.x | Rare, risky |

### Current Bounds

**Problematic Bounds**:
- boost/1.81.0 (no explicit upper, allows 1.82, 1.83, etc.)
- icu/76.1 (no upper bound)
- openssl/3.1.1 (no upper bound, but security-critical)

**Recommendation**: Add explicit upper bounds or use lock file

---

## Lock File Management

### What is a Lock File?

**Lock files** (Cargo.lock, package-lock.json, conan.lock) capture the exact versions used at a point in time. This ensures:

1. **Reproducibility**: `conan install --lockfile=conan.lock` installs exact versions
2. **Determinism**: Bit-for-bit identical binaries across builds
3. **Audit Trail**: Track when/why versions changed

### Lock File Procedures

#### Cargo (Rust)

✅ **ALREADY IMPLEMENTED**

```bash
# Lock file auto-generated on install
cargo build

# Update lock file
cargo update

# Upgrade specific package
cargo upgrade --package <name>

# Verify lock integrity
cargo tree --locked
```

#### npm (JavaScript)

✅ **ALREADY IMPLEMENTED**

```bash
# Lock file auto-generated on install
npm install

# Update lock file
npm update

# Use lock file in CI
npm ci              # Clean install (respects package-lock.json)
npm install --frozen-lockfile  # Fail if not up-to-date
```

#### Conan (C++)

❌ **NOT IMPLEMENTED** (requires action)

```bash
# Generate lock file from conanfile.txt
conan lock create conanfile.txt --lockfile=conan.lock

# Use lock file in CMake
cmake ... -DCONAN_LOCKFILE=conan.lock ...

# Track in git
git add conan.lock && git commit "chore: Conan dependency lock"

# Update lock file
conan lock create conanfile.txt --lockfile=conan.lock --update
```

---

## Security Auditing

### Automated Auditing (CI/CD)

**Workflow**: `.github/workflows/security-audit.yml`

Runs on:
- Push to master (dependency files changed)
- Weekly schedule (Mondays at 02:00 UTC)
- Manual trigger

**Checks**:
1. Rust: `cargo audit` (CVE database)
2. npm: `npm audit` (npm security advisory)
3. Python: `pip-audit` (for build/dev tools)
4. Conan: Lock file validation

**Output**:
- Console logs
- SBOM (Software Bill of Materials) artifact
- PR comments (if in PR)

### Manual Auditing

#### Cargo

```bash
cd rust
cargo audit                     # List vulnerabilities
cargo audit --deny warnings     # Fail on findings
cargo audit --ignore RUSTSEC-XXXX  # Suppress known issue
```

**Output**: CVE identifiers, severity, fix versions

#### npm

```bash
cd js
npm audit                       # Report vulnerabilities
npm audit --json | jq .         # Machine-readable format
npm audit fix                   # Auto-fix compatible versions
npm audit fix --force           # Allow breaking changes
```

**Levels**:
- low, moderate (usually safe)
- high, critical (action required)

#### Conan

Currently manual; automated checking in progress

```bash
# Check if new versions available
conan remote list

# Inspect dependencies
conan graph info conanfile.txt

# Check security advisories
# (Monitor: https://github.com/ConanDeps/conan-center-index)
```

---

## Updating Dependencies

### Process

1. **Identify Update Need**
   - Security advisory
   - Feature requirement
   - Maintenance (quarterly review)

2. **Test Locally**
   ```bash
   # Update to allowed versions
   cargo update           # Rust
   npm update             # JavaScript

   # Build and test
   make build && make test
   ```

3. **Verify Compatibility**
   - No breaking API changes
   - All tests pass
   - No new warnings

4. **Commit & Push**
   ```bash
   git commit -m "chore(deps): Update <package> to <version>"
   git push -u origin <feature-branch>
   ```

5. **CI/CD Validation**
   - All workflows passing
   - Code review approval
   - Merge to master

### Update Frequency

| Ecosystem | Frequency | Trigger |
|-----------|-----------|---------|
| **Conan** | Monthly | Security updates, major version releases |
| **Cargo** | Biweekly | `cargo update` on develop |
| **npm** | Biweekly | `npm update` on develop |
| **FetchContent** | Quarterly | Review for security & features |

### Known Issues & Workarounds

#### jemalloc (Conan)

**Status**: DISABLED - Conan2 recipe broken

**Workaround**:
- Falls back to system jemalloc if available
- Performance degradation: ~15-20% on IndexBuilder

**Resolution**: Monitor conan-center-index PR #18922

#### OpenSSL 3.1.1

**Status**: Current (June 2023 release)

**Recommendation**: Upgrade to 3.1.6 for security patches

**Update**: Edit conanfile.txt:
```
openssl/3.1.6
```

---

## Troubleshooting

### Issue: "Conan: No matching recipe"

**Cause**: Dependency version not in Conan Center

**Solution**:
1. Check available versions: `conan search openssl/*`
2. Use nearest available: `openssl/3.1.5`
3. Or pin to local recipe (advanced)

### Issue: "npm WARN deprecated"

**Cause**: Package marked as deprecated but still functional

**Solution**:
1. Check for maintained replacement
2. Or wait for auto-removal (usually safe)
3. Suppress: `npm install --legacy-peer-deps`

### Issue: "Cargo: Semver violation"

**Cause**: Dependency changed API between versions

**Solution**:
1. Investigate changelog for migration path
2. Update local code to match new API
3. Or pin to old version (`=` exact)

### Issue: "Build fails after update"

**Cause**: Version change introduced incompatibility

**Solution**:
1. Revert: `git revert <commit>`
2. Investigate: `git log --oneline <package>`
3. Update code to match new version, or
4. Pin to previous version + create issue

---

## Resources

- **Conan**: https://conan.io/
- **Cargo**: https://doc.rust-lang.org/cargo/
- **npm**: https://docs.npmjs.com/
- **CVE Database**: https://cve.mitre.org/
- **SBOM Standard**: https://www.ntia.gov/SBOM

---

## Contributing

When updating dependencies:

1. Use lock files for reproducibility
2. Run security audits (`make audit` if available)
3. Test thoroughly (local + CI)
4. Document breaking changes
5. Update this file if ecosystem coverage changes

See [CONTRIBUTING.md](CONTRIBUTING.md) for general guidelines.
