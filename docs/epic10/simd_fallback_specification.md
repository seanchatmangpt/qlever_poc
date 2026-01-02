# EPIC 10.1 - SIMD Fallback Specification

**Document Type**: Architectural Specification
**Purpose**: Define explicit SIMD fallback path guaranteeing bit-identical correctness
**Spec Lock**: Section 3.4, Section 4.4
**Date**: 2025-01-02
**Agent**: Agent 5 (SIMD Equivalence Validation)

---

## 1. Introduction

This specification defines the **SIMD fallback path** for all SIMD-accelerated operations in QLever. The fallback path is **explicit** (not implicit), ensuring bit-identical correctness guarantees regardless of SIMD availability.

### 1.1 Specification Principles

1. **Explicit Fallback**: Fallback path is manually implemented, not compiler-generated.
2. **Bit-Identical Correctness**: SIMD and scalar paths produce identical observable outputs.
3. **No Operations Skipped**: Fallback implements 100% of SIMD functionality.
4. **No SIMD-Only Features**: Every SIMD optimization has a scalar equivalent.
5. **Deterministic Behavior**: Both paths are deterministic and portable.

---

## 2. Fallback Architecture

### 2.1 Dual-Path Design

```
┌─────────────────────────────────────────┐
│  Public API (SimdJsonIngressWrapper)    │
└───────────────┬─────────────────────────┘
                │
                ├──> SIMD Path (if available)
                │    ├─> simdjson library
                │    ├─> Vectorized operations
                │    └─> Return IngressResult
                │
                └──> Scalar Fallback Path (always available)
                     ├─> Native C++ implementation
                     ├─> Sequential operations
                     └─> Return IngressResult (bit-identical)
```

### 2.2 Fallback Detection

**Compile-Time Detection**:
```cpp
#if defined(__AVX2__) || defined(__SSE4_2__)
  #define QLEVER_SIMD_AVAILABLE 1
#else
  #define QLEVER_SIMD_AVAILABLE 0
#endif
```

**Runtime Detection** (future enhancement):
```cpp
bool simdAvailable() {
  return __builtin_cpu_supports("avx2") || __builtin_cpu_supports("sse4.2");
}
```

---

## 3. Operation-by-Operation Fallback Mapping

### 3.1 JSON Parsing (`parseJsonLd`)

#### SIMD Path
```cpp
IngressResult SimdJsonIngressWrapper::parseJsonLd(std::string_view json_input) noexcept {
  #if QLEVER_SIMD_AVAILABLE
    // Use simdjson::ondemand parser
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;

    auto error = parser.iterate(json_input).get(doc);
    if (error) {
      return IngressResult{translateError(error)};
    }

    // Extract fields, compute digest
    return buildResult(doc);
  #else
    return fallback_parse(json_input);
  #endif
}
```

#### Scalar Fallback
```cpp
IngressResult SimdJsonIngressWrapper::fallback_parse(std::string_view json_input) noexcept {
  // Native C++ JSON parser using standard library
  // - Manual state machine for JSON syntax
  // - Sequential character processing
  // - Identical error codes as SIMD path
  // - Identical digest computation

  IngressResult result;

  if (json_input.empty()) {
    result.error = IngressErrorCode::PARSE_ERROR_EMPTY_INPUT;
    return result;
  }

  // State machine variables
  enum class State { START, OBJECT, ARRAY, STRING, NUMBER, LITERAL };
  State state = State::START;

  // Parse character-by-character
  for (char c : json_input) {
    // ... state transitions ...
  }

  // Compute digest (identical to SIMD path)
  result.digest_sha256 = compute_digest(normalized, validation_mask, result.error);

  return result;
}
```

**Equivalence Guarantee**: Both paths produce identical `IngressResult::digest_sha256` for same input.

---

### 3.2 Structure Validation (`validateStructure`)

#### SIMD Path
- **Vectorized Bracket Matching**: Process 16 bytes at a time using SIMD
- **Quote Detection**: Vectorized quote pairing using bitmasks
- **Branchless Validation**: SIMD comparisons for syntax checking

#### Scalar Fallback
- **Sequential Bracket Matching**: Stack-based bracket/quote tracking
- **Character-by-Character Quote Detection**: Standard string iteration
- **Conditional Validation**: Traditional if/else error checks

**Equivalence Guarantee**: Both paths detect same structural errors and return identical `IngressErrorCode`.

---

### 3.3 Canonical Normalization (`normalizeJsonLd`)

#### SIMD Path
- **Vectorized Field Sorting**: SIMD-accelerated string comparison
- **Whitespace Removal**: Vectorized space detection and removal
- **UTF-8 NFC Normalization**: SIMD Unicode processing (if available)

#### Scalar Fallback
```cpp
IngressResult SimdJsonIngressWrapper::normalizeJsonLd(
    std::string_view json_input, std::string& normalized_output) noexcept {

  // Parse into in-memory structure
  JsonObject obj = parse_to_object(json_input);

  // Sort fields alphabetically (deterministic)
  std::vector<std::pair<std::string, JsonValue>> fields;
  for (const auto& [key, value] : obj) {
    fields.emplace_back(key, value);
  }
  std::sort(fields.begin(), fields.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  // Serialize to canonical form
  normalized_output = serialize_canonical(fields);

  // Compute digest
  IngressResult result;
  result.digest_sha256 = compute_digest(normalized_output, validation_mask, result.error);

  return result;
}
```

**Equivalence Guarantee**: Both paths produce identical `normalized_output` string (bit-for-bit).

---

### 3.4 Digest Computation (`compute_digest`)

#### SIMD Path (if available)
- **Vectorized SHA256**: Hardware-accelerated SHA256 (Intel SHA extensions)
- **Parallel Hashing**: SIMD-accelerated digest computation

#### Scalar Fallback
```cpp
std::string SimdJsonIngressWrapper::compute_digest(
    std::string_view normalized_json,
    uint32_t validation_mask,
    IngressErrorCode error_code) noexcept {

  // Use standard SHA256 implementation (OpenSSL or built-in)
  return IngressDigest::hex_encode(
      IngressDigest::compute(normalized_json, validation_mask, error_code)
  );
}
```

**Equivalence Guarantee**: Both paths produce identical 64-character hex digest (SHA256 is deterministic).

---

## 4. Constraint Boundaries

### 4.1 Prohibited Behaviors

❌ **Do NOT permit semantic equivalence only**
- Semantic equivalence (e.g., `1.0 == 1.00`) is NOT sufficient
- Outputs must be **bit-identical** (exact byte match)

❌ **Do NOT skip any operation in fallback path**
- Fallback must implement 100% of functionality
- No "good enough" approximations

❌ **Do NOT use SIMD intrinsics without scalar fallback**
- Every `_mm_*` intrinsic must have a scalar alternative
- Compilation must succeed with `-mno-sse -mno-avx`

❌ **Do NOT optimize away correctness checks in SIMD path**
- SIMD path must have same error detection as scalar
- No "fast and loose" SIMD implementations

---

### 4.2 Required Invariants

✅ **Digest Invariant**: `digest(SIMD, input) == digest(Scalar, input)` for all inputs

✅ **Error Invariant**: `error_code(SIMD, input) == error_code(Scalar, input)` for all inputs

✅ **Bytes Invariant**: `normalize(SIMD, input) == normalize(Scalar, input)` (byte-identical)

✅ **Determinism Invariant**: `f(input) == f(input)` for all calls (no randomness, no wall-clock)

---

## 5. Fallback Selection Logic

### 5.1 Compile-Time Selection

```cpp
IngressResult parseJsonLd(std::string_view input) noexcept {
  #if QLEVER_SIMD_AVAILABLE
    // Try SIMD path first
    IngressResult result = simd_parse(input);

    // On SIMD parse failure (not error, but failure to use SIMD),
    // fall back to scalar
    if (result.error == IngressErrorCode::SIMD_UNAVAILABLE) {
      return fallback_parse(input);
    }

    return result;
  #else
    // No SIMD available, use scalar path
    return fallback_parse(input);
  #endif
}
```

### 5.2 Runtime Selection (Future)

For future runtime SIMD detection:

```cpp
IngressResult parseJsonLd(std::string_view input) noexcept {
  static bool simd_available = detectSimdSupport();

  if (simd_available) {
    return simd_parse(input);
  } else {
    return fallback_parse(input);
  }
}
```

---

## 6. Testing Requirements

### 6.1 Equivalence Testing

Every SIMD operation must have a corresponding equivalence test:

```cpp
TEST(SimdEquivalenceTest, OperationName) {
  auto simd_result = simd_wrapper.operation(input);
  auto scalar_result = scalar_wrapper.fallback_operation(input);

  EXPECT_EQ(simd_result.digest_sha256, scalar_result.digest_sha256);
  EXPECT_EQ(simd_result.error, scalar_result.error);
}
```

### 6.2 Determinism Testing

Both paths must pass 100-iteration determinism tests:

```cpp
TEST(DeterminismTest, SimdPath) {
  auto first = simd_wrapper.operation(input);
  for (int i = 0; i < 99; ++i) {
    auto result = simd_wrapper.operation(input);
    EXPECT_EQ(result.digest_sha256, first.digest_sha256);
  }
}
```

---

## 7. Implementation Guidelines

### 7.1 Adding New SIMD Operation

**Steps** (in order):

1. **Implement Scalar Fallback First**
   - Write fallback implementation
   - Test for correctness
   - Establish baseline digest values

2. **Implement SIMD Optimization**
   - Add SIMD intrinsics
   - Compile-time or runtime selection
   - Test for equivalence with fallback

3. **Add Equivalence Tests**
   - Create `TEST(SimdEquivalenceTest, NewOperation)`
   - Verify digest matches
   - Verify error codes match

4. **Document Fallback**
   - Update this specification
   - Document SIMD technique and fallback mapping

### 7.2 Code Structure Template

```cpp
IngressResult SimdJsonIngressWrapper::newOperation(Args... args) noexcept {
  #if QLEVER_SIMD_AVAILABLE
    return simd_newOperation(args...);
  #else
    return fallback_newOperation(args...);
  #endif
}

// SIMD implementation
IngressResult SimdJsonIngressWrapper::simd_newOperation(Args... args) noexcept {
  // Use SIMD intrinsics
  // Return IngressResult with digest
}

// Scalar fallback
IngressResult SimdJsonIngressWrapper::fallback_newOperation(Args... args) noexcept {
  // Use standard library
  // Return IngressResult with digest (bit-identical to SIMD)
}
```

---

## 8. Platform Support Matrix

| Platform | Architecture | SIMD Support | Fallback Required | Status |
|----------|--------------|--------------|-------------------|--------|
| Linux x86-64 | x86-64 | SSE4.2, AVX2 | No (SIMD available) | ✅ Tested |
| Linux x86-64 | x86-64 (no SIMD) | None | Yes (scalar only) | ✅ Tested |
| Linux ARM64 | aarch64 | NEON | No (NEON available) | 🔄 Future |
| Linux ARM64 | aarch64 (no NEON) | None | Yes (scalar only) | 🔄 Future |
| macOS x86-64 | x86-64 | SSE4.2, AVX2 | No (SIMD available) | 🔄 Future |
| macOS ARM64 | Apple Silicon | NEON | No (NEON available) | 🔄 Future |
| Windows x86-64 | x86-64 | SSE4.2, AVX2 | No (SIMD available) | 🔄 Future |

**Guarantee**: Scalar fallback works on **all platforms**, ensuring portability.

---

## 9. Performance Characteristics

### 9.1 Expected Speedup (SIMD vs Scalar)

| Operation | SIMD Speedup | Measurement Method |
|-----------|--------------|-------------------|
| JSON Parsing | 2-4x | simdjson benchmarks |
| Structure Validation | 3-5x | Vectorized bracket matching |
| Normalization | 1.5-2x | Field sorting, whitespace removal |
| Digest Computation | 1.2-2x | SHA extensions (if available) |

**Note**: Performance is **separate from equivalence**. Equivalence testing does not measure performance.

### 9.2 Fallback Performance

- **Scalar fallback is slower** than SIMD (expected)
- **Scalar fallback is still correct** (bit-identical outputs)
- **Scalar fallback is always available** (portability guarantee)

---

## 10. Hot Path Compliance

### 10.1 Section 4.4 Requirements

Both SIMD and scalar paths comply with **hot path silence**:

- ✅ No exceptions thrown
- ✅ No heap allocation in critical path (pre-allocated buffers)
- ✅ No logging or diagnostic output
- ✅ Error codes only (no error messages)

### 10.2 Error Handling

```cpp
// ❌ WRONG (throws exception)
if (parse_error) {
  throw std::runtime_error("Parse failed");
}

// ✅ CORRECT (returns error code)
if (parse_error) {
  result.error = IngressErrorCode::PARSE_ERROR_SYNTAX;
  return result;
}
```

---

## 11. Debugging and Diagnostics

### 11.1 Fallback Logging (DEBUG builds only)

```cpp
#ifdef QLEVER_DEBUG
  #define SIMD_FALLBACK_LOG(msg) std::cerr << "[SIMD FALLBACK] " << msg << "\n"
#else
  #define SIMD_FALLBACK_LOG(msg) ((void)0)
#endif

IngressResult fallback_parse(std::string_view input) noexcept {
  SIMD_FALLBACK_LOG("Using scalar fallback for parseJsonLd");
  // ... implementation ...
}
```

**Note**: Logging only in DEBUG builds, not in RELEASE (hot path silence).

---

## 12. Validation Checklist

Before merging SIMD-accelerated code:

- [ ] Scalar fallback implemented
- [ ] Equivalence tests added
- [ ] Determinism tests pass (100 iterations)
- [ ] Digest matches verified
- [ ] Error codes consistent
- [ ] Canonical bytes identical
- [ ] Hot path silence maintained (no exceptions, no logging)
- [ ] Documentation updated (this spec)
- [ ] CI tests pass on all platforms

---

## 13. Failure Modes and Recovery

### 13.1 SIMD Unavailable at Compile Time

**Scenario**: Compiled with `-mno-sse -mno-avx`

**Behavior**:
```cpp
#if QLEVER_SIMD_AVAILABLE
  // This block not compiled
#else
  return fallback_parse(input);  // Always use scalar
#endif
```

**Result**: Program runs correctly using scalar fallback.

---

### 13.2 SIMD Instruction Fault at Runtime

**Scenario**: CPU does not support required SIMD instructions

**Behavior**:
- Runtime detection (if implemented) selects scalar path
- No crashes, no undefined behavior

**Result**: Program runs correctly using scalar fallback.

---

### 13.3 Digest Mismatch (Bug Detected)

**Scenario**: Equivalence test fails

**Behavior**:
```
EXPECT_EQ(simd_result.digest_sha256, scalar_result.digest_sha256);
// FAILS: Digests differ!
```

**Action**:
1. ❌ **Block merge** - SIMD equivalence violated
2. 🔍 **Debug** - Identify divergence point
3. 🛠️ **Fix** - Correct SIMD or scalar implementation
4. ✅ **Re-test** - Verify equivalence restored

---

## 14. Future Enhancements

### 14.1 Runtime SIMD Detection

Implement `cpuid`-based runtime detection:

```cpp
bool hasAVX2() {
  return __builtin_cpu_supports("avx2");
}

bool hasSSE42() {
  return __builtin_cpu_supports("sse4.2");
}
```

### 14.2 Multi-Level Fallback

Support graceful degradation:

```
AVX-512 → AVX2 → SSE4.2 → Scalar
```

Each level provides bit-identical results, with varying performance.

---

## 15. References

- **EPIC 10.1 Specification**: Section 3.4 (SIMD Equivalence)
- **EPIC 10.1 Specification**: Section 6.3 (Validation Artifact)
- **EPIC 10.1 Specification**: Section 4.4 (Hot Path Silence)
- **simdjson Documentation**: https://github.com/simdjson/simdjson
- **Intel Intrinsics Guide**: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/

---

## 16. Conclusion

The SIMD fallback specification ensures:

1. ✅ **Bit-Identical Correctness** - SIMD and scalar produce identical outputs
2. ✅ **Portability** - Scalar fallback works on all platforms
3. ✅ **Explicit Design** - No compiler magic, manual control
4. ✅ **Testability** - Equivalence tests validate correctness
5. ✅ **Hot Path Compliance** - No exceptions, no logging

**Status**: ✅ **SPECIFICATION COMPLETE**

All SIMD operations must conform to this specification. Non-compliance is a **blocker** for merge.

---

**Agent 5 Deliverable**: ✅ **COMPLETE**

This specification, combined with the test suite and validation report, completes Agent 5's partition for EPIC 10.1.
