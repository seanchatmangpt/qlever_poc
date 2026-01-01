# EPIC 7 — SIMD Ingress Architecture Guide

## System Overview

```
Input JSON-LD
     ↓
[SimdJsonIngressWrapper]
     ↓
[SIMD Parsing Path] ← uses simdjson::ondemand
     ↓                       ↓
  [Normalize]          [Fallback C++ Parser]
     ↓                       ↓
     ├─ Canonical Order ─→ [Digest Computation]
     ├─ UTF-8 NFC ────────→ (IngressDigest::compute)
     ├─ Whitespace Removal ↓
     └─ Number Canon ──→ IngressResult {error, digest}
                              ↓
                        Error Code (no exception)
```

## SIMD Techniques Employed

1. **Structural Scanning** (simdjson)
   - SIMD byte-level classification of whitespace, delimiters
   - 64-byte chunks processed in parallel
   - ~10x faster than byte-at-a-time parsing

2. **Quote/Escape Detection** (simdjson)
   - Vectorized quote pairing with backslash tracking
   - Identifies strings in parallel across SIMD lanes

3. **Number Validation** (simdjson)
   - SIMD parallel digit classification
   - Floating-point normalization deferred to cold-path

4. **Nested Structure Validation** (simdjson)
   - Branchless brace/bracket pairing via lookup tables
   - O(n) depth validation without recursion

5. **Error Classification** (bitmasks)
   - Validation failures represented as bitmasks (not strings)
   - No string formatting in hot-path

## Error Handling (No Exceptions)

All errors propagated via `IngressErrorCode`:
- Parse errors (1-99): syntax, UTF-8, depth, type mismatches
- JSON-LD validation (100-199): @context, @id, @type errors
- Structural validation (200-299): type checks, constraints
- Resource errors (300-399): memory, timeouts, I/O
- Implementation errors (400-499): unsupported formats, internal state

## Determinism Guarantee

All ingress operations produce deterministic digests:
1. Canonical JSON-LD (alphabetical fields, no whitespace)
2. SIMD validation bitmask
3. Error code

Combined via deterministic serialization (little-endian encoding) and SHA256 hashing.

Result: Same input document → identical SHA256 digest across all machines and runs.

