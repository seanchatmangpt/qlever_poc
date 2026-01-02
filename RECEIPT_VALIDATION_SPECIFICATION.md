# Receipt Validation Specification (BB80/20 + EPIC 4)

**Complete Receipt Validation Specification with Gates, Benchmarks, Guards, Hash Formats, and Validity Criteria**

---

## 1. Receipt Definition

A **receipt** is deterministic proof that work is correct. It comprises:
- Benchmark results (quantified metrics)
- State hashes (deterministic digests)
- Event logs (trace events)
- Guard validation results (checkpoint passes)

Receipts replace human consensus. If work passes all guards and benchmarks, it is VALID by definition. No narrative arguments permitted.

---

## 2. Benchmark Specifications

### 2.1 Benchmarks That Must Pass

#### 1. Ingress Throughput Benchmark (`ingress_throughput`)
**Location**: `benchmark/ingress_throughput.cpp`
**Purpose**: Measure data ingestion performance

**Metrics**:
- Throughput (MB/s)
- P99 Latency (nanoseconds)
- P95 Latency (nanoseconds)
- P50 Latency (nanoseconds)
- Mean Latency (nanoseconds)
- Standard Deviation (nanoseconds)

**Pass Criteria** (Variance Gate):
- Coefficient of Variation (CV) < 5.0%
- Formula: CV = (stddev / mean) × 100%
- Threshold is HARDCODED (not configurable)

#### 2. Query Latency Distribution Benchmark (`query_latency_distribution`)
**Location**: `benchmark/query_latency_distribution.cpp`
**Purpose**: Measure query execution latency distribution

**Metrics**:
- P99 Latency (nanoseconds)
- P95 Latency (nanoseconds)
- P50 Latency (nanoseconds)
- Mean Latency (nanoseconds)
- Min Latency (nanoseconds)
- Max Latency (nanoseconds)
- Standard Deviation (nanoseconds)

**Pass Criteria** (Variance Gate):
- Coefficient of Variation (CV) < 5.0%
- Same as Ingress Throughput

#### 3. Regression Detection Benchmark (EPIC 10.1)
**Location**: `src/engine/readPlane/PerformanceEnvelope.h`
**Purpose**: Detect performance regressions vs. baseline

**Thresholds**:
```
Latency CV Threshold       = 0.10 (10%)
Cache Hit Rate Threshold   = 0.05 (5%)
```

**Pass Criteria**:
- `stability.latency_coefficient_of_variation <= 0.10` AND
- `|bytes_hit_rate_change| <= 0.05` AND
- `|plan_hit_rate_change| <= 0.05`

**Regression Detection Logic**:
```cpp
is_regression = (CV > 0.10) OR (hit_rate_change > 5%)
```

### 2.2 Benchmark Execution Requirements

**Standard Benchmark Run Configuration**:
- Warmup runs: 1 (to stabilize caches)
- Measurement runs: 10 (sufficient sample size for statistical significance)
- Total time: ~5-10 minutes on quiet node

**Environment Requirements** (for variance <= 5%):
1. Single-tenant "quiet node" (bare metal)
2. CPU isolation: use `taskset` or `isolcpus` kernel parameter
3. Network isolation: disable network services
4. Consistent clock: disable CPU turbo boost
5. No other processes running

**Example**:
```bash
taskset -c 4-7 ./variance_gate.py --benchmark ./ingress_throughput \
  --warmup 1 --runs 10 --output ingress_throughput.receipt
```

---

## 3. Guard Specifications (Automatic Validation)

### 3.1 Guard Types (Deterministic Invariant Checks)

All guards are defined in `/home/user/qlever/src/engine/readPlane/GuardConfiguration.h`

#### Envelope-Based Guards (Deterministic Matching)
```
PLAN_HASH_MUST_MATCH              (0x01)
  ├─ QueryExecutionTree structure must match
  └─ Non-topology fields excluded (cost, cardinality, timing)

QUERY_FINGERPRINT_MUST_MATCH      (0x02)
  ├─ Query identity (EPIC 3)
  └─ Canonical serialization of all fingerprint fields

RESOURCE_ENVELOPE_MUST_MATCH      (0x04)
  ├─ Memory/cache behavior
  ├─ Bytes cache hits/misses
  ├─ Plan cache hits/misses
  └─ Negative cache hits

RESULT_SHAPE_MUST_MATCH           (0x08)
  ├─ Column count
  ├─ Column types (sorted)
  └─ Output format

RESULT_LENGTH_MUST_MATCH          (0x10)
  └─ Number of result rows
```

#### Epoch-Based Guards (Epoch Binding Validation)
```
EPOCH_MUST_NOT_CHANGE             (0x20)
  └─ Forbid epoch transition during replay

EPOCH_MANIFEST_MUST_MATCH         (0x40)
  └─ Manifest hash must remain constant
```

#### Composite Guard Sets
```
ENVELOPE_STRICT = PLAN_HASH_MUST_MATCH | QUERY_FINGERPRINT_MUST_MATCH |
                  RESOURCE_ENVELOPE_MUST_MATCH | RESULT_SHAPE_MUST_MATCH |
                  RESULT_LENGTH_MUST_MATCH

EPOCH_STRICT = EPOCH_MUST_NOT_CHANGE | EPOCH_MANIFEST_MUST_MATCH

ALL_GUARDS_STRICT = ENVELOPE_STRICT | EPOCH_STRICT  (DEFAULT)
```

### 3.2 Guard Configuration

**Default Configuration** (Fail-Closed, No Tolerance):
```
active_guards               = ALL_GUARDS_STRICT
abort_strategy              = ABORT_IMMEDIATELY
abort_timeout_ms            = 1000
envelope_match_threshold    = 1.0 (100%, no fuzzy matching)
log_guard_violations        = false
collect_guard_metrics       = true
max_logged_violations       = 100
```

**Canonical Serialization** (for hashing):
```
Binary Format (23 bytes total):
- active_guards         (4 bytes, big-endian)
- abort_strategy        (1 byte)
- abort_timeout_ms      (4 bytes, big-endian)
- envelope_match_threshold (8 bytes, fixed-point: value * 1,000,000)
- log_guard_violations  (1 byte, 0x00/0x01)
- collect_guard_metrics (1 byte, 0x00/0x01)
- max_logged_violations (4 bytes, big-endian)
```

### 3.3 Guard Validation Rules

**How Guards Work**:
1. Guard is **ACTIVE** if `(active_guards & GuardRuleType) == GuardRuleType`
2. Guard is **CHECKED** by comparing expected vs. actual envelope component
3. Guard is **VIOLATED** if hashes differ
4. On violation: `abort_strategy` is executed immediately
5. Result: ABORT and fail-closed, OR log and abort

**Abort Strategies**:
```
ABORT_IMMEDIATELY = 0x00  ← Default (no logging overhead)
LOG_AND_ABORT     = 0x01  (log violation details, then abort)
ALERT_AND_ABORT   = 0x02  (send alert, log, then abort)
```

**Validation Constraints**:
```
envelope_match_threshold ∈ [0.0, 1.0]  (must be valid)
abort_timeout_ms > 0                    (must be positive)
```

---

## 4. Hash Digest Formats (SHA-256 Specification)

### 4.1 Hash Types and Specifications

All hashes are **SHA-256**, output as **64-character lowercase hex strings** (256 bits = 32 bytes).

#### Digest Hashes:

**1. query_fingerprint_sha256**
- Input: Serialized QueryFingerprint (all fields)
- Format: `"FP:" + epoch_id + ":" + epoch_manifest_sha256 + ":" + raw_query_sha256 + ...`
- Output: 64-char hex string
- Use: Identify query uniqueness across epochs

**2. plan_hash**
- Input: Canonical serialization of QueryExecutionTree topology
- Format: `"PLAN:" + plan_cache_key + ":" + operation_descriptors.join("|")`
- Excludes: cost estimates, cardinality, timing
- Includes: operator sequence, variable bindings, join keys, scan patterns
- Output: 64-char hex string
- Use: Validate execution plan structure

**3. resource_signature**
- Input: Canonical serialization of ResourceMetrics
- Format: `"RSRC:" + bytes_cache_hits + ":" + plan_cache_hits + ":" + negative_cache_hits + ":" + memory_pages_accessed + ":" + result_bytes_written`
- Output: 64-char hex string
- Use: Validate memory/cache behavior

**4. result_length_hash**
- Input: Result row count (as string)
- Format: `std::to_string(result_size)`
- Output: SHA256(size_string) → 64-char hex string
- Use: Validate result cardinality

**5. result_shape_hash**
- Input: Canonical serialization of ResultMetadata
- Format: `"RESULT:" + column_count + ":" + row_count + ":" + output_format + ":" + sorted_column_types.join(",")`
- Excludes: row content
- Includes: column count, types (sorted), output format
- Output: 64-char hex string
- Use: Validate result structure

**6. result_structure_digest** (ResultDigest, separate subsystem)
- Input: Column count, sorted column types, output format
- Format: `"STRUCT:" + column_count + ":" + output_format + ":" + sorted_types.join(",")`
- Output: SHA-256 (32-byte binary) → 64-char hex via `hexEncode()`
- Use: Validate result schema without content

**7. result_content_digest** (ResultDigest, separate subsystem)
- Input: Canonical binary serialization of result rows
- Canonical Format:
  ```
  FOR EACH ROW:
    ROW_MARKER (1 byte = 0xFE)
    FOR EACH COLUMN:
      ID value (8 bytes, little-endian uint64_t)
    END
    ROW_TERMINATOR (1 byte = 0xFF)
  ```
- No floating-point arithmetic in critical path
- Output: SHA-256 (32-byte binary) → 64-char hex
- Use: Validate result content determinism

**8. digest_hash** (Final Composite Hash)
- Input: Concatenation of all component hashes
- Formula: `SHA256(query_fingerprint_sha256 || plan_hash || resource_signature || result_length_hash || result_shape_hash)`
- Order is CRITICAL for determinism
- Output: 64-char hex string
- Use: Single proof of entire execution envelope

### 4.2 Hash Format Details

**Hex Encoding** (all hashes):
```
Input:  32-byte binary digest
Output: 64-character lowercase hex string
        Example: a7f3c2d8e1b4f9a3c6d5e8f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0
```

**Hex Decoding** (for validation):
```
Input:  64-character hex string
Output: 32-byte binary digest
Validation: Length must be exactly 64, all characters must be hex digits [0-9a-fA-F]
```

**Canonical Byte Ordering**:
- All multi-byte integers: **big-endian** (network byte order)
- Exception: Result content serialization uses **little-endian** for uint64_t IDs
- Floating-point: **NEVER used** in deterministic paths (use fixed-point instead)

### 4.3 Integrity Verification

**digest_hash Verification**:
```cpp
bool isValid() {
  // All 5 component hashes must be 64-char hex strings
  return isValidHex(query_fingerprint_sha256) &&
         isValidHex(plan_hash) &&
         isValidHex(resource_signature) &&
         isValidHex(result_length_hash) &&
         isValidHex(result_shape_hash) &&
         isValidHex(digest_hash);
}

bool verifyIntegrity() {
  // Recompute digest_hash and compare
  expected = SHA256(query_fp || plan || resource || result_len || result_shape)
  return digest_hash == expected;
}
```

**Determinism Verification** (ResultDigest):
```cpp
bool verifyDeterminism(const Result& result, int iterations = 10) {
  // Compute reference digests
  reference_structure = computeStructureDigest(result, "JSON");
  reference_content = computeContentDigest(result);

  // Verify all iterations produce identical digests
  for (i = 1; i < iterations; ++i) {
    current_structure = computeStructureDigest(result, "JSON");
    current_content = computeContentDigest(result);
    if (current_structure != reference_structure ||
        current_content != reference_content) {
      return false;  // Non-deterministic!
    }
  }
  return true;
}
```

**SIMD Equivalence Verification**:
```cpp
bool verifySimdEquivalence(const Result& result_simd_on,
                           const Result& result_simd_off) {
  // Compute digests for both results
  digest_on_struct = computeStructureDigest(result_simd_on, "JSON");
  digest_on_content = computeContentDigest(result_simd_on);
  digest_off_struct = computeStructureDigest(result_simd_off, "JSON");
  digest_off_content = computeContentDigest(result_simd_off);

  // Both must match
  return (digest_on_struct == digest_off_struct) &&
         (digest_on_content == digest_off_content);
}
```

---

## 5. Event Log Validation Rules

### 5.1 Event Log Format

**Event Types** (from ExecutionTraceDigest.h):
```
QUERY_START           - Query execution begins
CACHE_LOOKUP          - Cache lookup performed
PLAN_COMPILE          - Query plan compilation
EXECUTION_START       - Plan execution begins
EXECUTION_END         - Plan execution completes
RESULT_SERIALIZE      - Result serialization
QUERY_COMPLETE        - Query execution complete
```

**Event Structure**:
```cpp
struct ExecutionTraceEvent {
  TraceEventType event_type;      // Which event
  uint64_t wall_clock_ns;          // Wall-clock time (EXCLUDED from digest)
  std::string description;          // Human-readable detail

  // JSON-LD with explicit "excluded from hash" marker
  {
    "@type": "ExecutionTraceEvent",
    "event_type": "QUERY_START",
    "wall_clock_ns": 1234567890,
    "wall_clock_ns@excluded_from_hash": true,  ← Explicit marker
    "description": "..."
  }
}
```

**Key Invariant**: Wall-clock times are **EXCLUDED from all digest computations**. They are informational only for debugging and observability.

### 5.2 Event Log Validation Rules

**Valid Event Log Requirements**:
1. **Ordered Sequencing**: Events must be in execution order
   - QUERY_START must precede QUERY_COMPLETE
   - CACHE_LOOKUP before EXECUTION_START
   - EXECUTION_START before EXECUTION_END

2. **Complete Event Chains**: For each successful query:
   - QUERY_START must exist
   - QUERY_COMPLETE must exist
   - Time delta (wall_clock_ns) must be > 0

3. **Event Latency Extraction** (for PerformanceEnvelope):
   ```cpp
   uint64_t latency_ns = event[QUERY_COMPLETE].wall_clock_ns -
                         event[QUERY_START].wall_clock_ns;
   // Round to microsecond precision
   latency_ns = (latency_ns / 1000) * 1000;
   ```

4. **Latency Distribution Calculation**:
   ```
   Sort all latencies_ns
   p50 = sorted[n * 50 / 100]
   p95 = sorted[n * 95 / 100]
   p99 = sorted[n * 99 / 100]
   max = sorted.back()
   ```

5. **Stability Metrics from Events**:
   - Mean latency = Σlatencies / count
   - Stddev = √(Σ(latency - mean)² / count)
   - CV = stddev / mean

### 5.3 Event Log Integrity

**Event Validation**:
- Each event's wall_clock_ns must be > 0
- For sequences: wall_clock_ns must be monotonically increasing
- Description field must be UTF-8 valid (no malformed escapes)

**Epoch Transition Detection**:
```cpp
EpochKey first_epoch;
bool transition_detected = false;

for (const auto& result : replay_results) {
  if (result.replayed_on_epoch.epoch_id != first_epoch.epoch_id &&
      !transition_detected) {
    transition_detected = true;
    envelope.epoch_transition_detected = true;
  }
}
```

---

## 6. Receipt VALID vs. INVALID Criteria

### 6.1 VALID Receipt (All Conditions Required)

A receipt is **VALID** IFF **ALL** of the following hold:

1. **All Hashes Present and Valid**
   ```
   isValidHex(query_fingerprint_sha256) ∧
   isValidHex(plan_hash) ∧
   isValidHex(resource_signature) ∧
   isValidHex(result_length_hash) ∧
   isValidHex(result_shape_hash) ∧
   isValidHex(digest_hash)
   ```

2. **Digest Integrity**
   ```
   digest_hash == SHA256(query_fp || plan || resource || result_len || result_shape)
   ```

3. **All Active Guards Pass**
   ```
   For each GuardRuleType in active_guards:
     expected_hash == actual_hash
   ```

4. **Benchmarks Pass**
   ```
   CV(P99 latency, 10 runs) < 5.0%  [Variance Gate]
   latency_coefficient_of_variation <= 10%  [Regression Gate]
   |bytes_hit_rate_change| <= 5%  [Cache Gate]
   |plan_hit_rate_change| <= 5%   [Cache Gate]
   ```

5. **Event Log Valid**
   ```
   Events present for each query:
     - QUERY_START exists
     - QUERY_COMPLETE exists
     - wall_clock_ns delta > 0
     - All times > 0
   ```

6. **Determinism Verified**
   ```
   Same result computed N times → N identical digests
   ```

7. **No Narrative Arguments**
   ```
   Proof is quantitative, not subjective
   Receipt contains only benchmarks, hashes, guards
   ```

### 6.2 INVALID Receipt (Any Condition Fails)

A receipt is **INVALID** if **ANY** of the following hold:

1. **Hash Missing or Malformed**
   ```
   Any of the 6 required hashes is empty, not 64 chars, or non-hex
   ```

2. **Digest Integrity Failed**
   ```
   Recomputed digest_hash ≠ provided digest_hash
   (Indicates data corruption or tampering)
   ```

3. **Guard Violation**
   ```
   Any active guard fails:
     actual_hash ≠ expected_hash
   ⟹ abort_strategy triggered immediately
   ```

4. **Benchmark Failed**
   ```
   CV(P99 latency) >= 5.0%  [Variance Gate]
   OR
   latency_cv > 10%  [Regression Gate]
   OR
   |hit_rate_change| > 5%  [Cache Gate]
   ```

5. **Event Log Incomplete or Malformed**
   ```
   Missing QUERY_START or QUERY_COMPLETE for any query
   OR wall_clock_ns not monotonically increasing
   OR wall_clock_ns delta <= 0
   ```

6. **Determinism Not Verified**
   ```
   Same result produced N times with different digests
   (Indicates non-deterministic implementation)
   ```

7. **Narrative Arguments Present**
   ```
   Receipt contains statements like:
     "This looks good"
     "I believe this is correct"
     "Probably works"
   ⟹ Abort immediately, reject receipt
   ```

### 6.3 Receipt Comparison Outcomes

After computing two ExecutionDigests and comparing:

```cpp
enum DigestComparisonOutcome {
  MATCH,                      ← VALID (same digest, same epoch)
  DIVERGENCE,                 ← INVALID (different digests)
  CROSS_EPOCH_REPRODUCIBLE    ← VALID (same digest, different epoch)
}

bool isSuccess() {
  return outcome == MATCH || outcome == CROSS_EPOCH_REPRODUCIBLE;
}

bool isDivergence() {
  return outcome == DIVERGENCE;
}
```

**Divergence Details Logged**:
- If plan_hash differs: "plan_hash differs"
- If result_length_hash differs: "result_length differs"
- If result_shape_hash differs: "result_shape differs"
- If resource_signature differs: "resource_signature differs"

---

## 7. Receipt Validation Workflow

### 7.1 Validation Entry Point

**Script**: `/home/user/qlever/scripts/validate-receipts.sh`

```bash
validate-receipts.sh <manifest_file>
```

**Input Format** (manifest file):
```
<expected_hash1> <filepath1>
<expected_hash2> <filepath2>
...
```

**Validation Logic**:
```bash
FOR EACH line in manifest:
  1. Parse: expected_hash, filepath
  2. Verify: file exists and is readable
  3. Compute: current_hash = SHA256(file)
  4. Compare: current_hash == expected_hash
  5. On mismatch: Exit 1 immediately (atomic failure)
6. If all pass: Exit 0
```

**Output**:
- Exit 0: Success, all digests match
- Exit 1: Failure, digest mismatch or validation error
- No output on success (quiet on pass, loud on fail)

### 7.2 Receipt Validator Agent (bb80-receipt-validator)

**Agent Role**: Validate implementations using deterministic receipts

**Inputs**:
- Artifact digests
- Benchmark results
- Guard evaluations
- Event logs

**Validation Steps**:
1. Require concrete proof (benchmarks, state hashes, event logs)
2. Validate against deterministic guards
3. Reject narrative arguments completely
4. Certify work once valid (no reiteration)

**Abort Conditions**:
- Proof missing → ABORT
- Any guard fails → ABORT
- Narrative argument → ABORT
- Ambiguity in instruction → ABORT

---

## 8. Receipt Serialization Formats

### 8.1 JSON-LD Format (Portable)

**ExecutionDigest JSON-LD**:
```json
{
  "@context": "https://qlever.cs.uni-freiburg.de/readplane/v1#",
  "@type": "ExecutionDigest",
  "format_version": "1.0",
  "query_fingerprint_sha256": "a7f3c2d8e1b4f9a3c6d5e8f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0",
  "plan_hash": "b8e4d3a9f2c5e8d1f4a7b0c3d6e9f2a5c8d1e4f7a0b3c6d9e2f5a8b1c4d7e0",
  "resource_signature": "c9f5e4b0g3d6f9e2a5b8c1d4e7f0a3b6c9d2e5f8a1b4c7d0e3f6a9b2c5d8e1",
  "result_length_hash": "d0g6f5c1h4e7g0f3b6c9d2e5f8a1b4c7d0e3f6a9b2c5d8e1f4a7b0c3d6e9f2",
  "result_shape_hash": "e1h7g6d2i5f8h1g4c7d0e3f6a9b2c5d8e1f4a7b0c3d6e9f2a5b8c1d4e7f0a3",
  "digest_hash": "f2i8h7e3j6g9i2h5d8e1f4a7b0c3d6e9f2a5b8c1d4e7f0a3b6c9d2e5f8a1b4"
}
```

**EnvelopeDiff JSON-LD**:
```json
{
  "@context": "https://qlever.cs.uni-freiburg.de/readplane/v1#",
  "@type": "EnvelopeDiff",
  "format_version": "1.0",
  "is_identical": false,
  "classification": "PLAN_DIVERGENCE",
  "digest1_hash": "a7f3c2d8e1b4f9a3c6d5e8f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0",
  "digest2_hash": "b8e4d3a9f2c5e8d1f4a7b0c3d6e9f2a5c8d1e4f7a0b3c6d9e2f5a8b1c4d7e0",
  "fingerprint_differs": false,
  "plan_changed": true,
  "resource_envelope_changed": false,
  "result_shape_changed": false,
  "result_length_changed": false,
  "differing_components": [
    {
      "component": "plan_hash",
      "before": "b8e4d3a9f2c5e8d1f4a7b0c3d6e9f2a5c8d1e4f7a0b3c6d9e2f5a8b1c4d7e0",
      "after": "c9f5e4b0g3d6f9e2a5b8c1d4e7f0a3b6c9d2e5f8a1b4c7d0e3f6a9b2c5d8e1"
    }
  ]
}
```

**GuardConfiguration JSON-LD**:
```json
{
  "@type": "GuardConfiguration",
  "abortStrategy": "ABORT_IMMEDIATELY",
  "abortTimeoutMs": 1000,
  "activeGuards": "ALL_GUARDS_STRICT",
  "collectGuardMetrics": true,
  "envelopeMatchThreshold": 1.0,
  "logGuardViolations": false,
  "maxLoggedViolations": 100
}
```

**PerformanceEnvelope JSON-LD**:
```json
{
  "@context": "https://qlever.cs.uni-freiburg.de/epic4/v1.0",
  "@type": "PerformanceEnvelope",
  "cache_stats": {
    "@type": "CacheEfficiencyStats",
    "bytes_hit_rate": 0.85,
    "bytes_hits": 8500,
    "bytes_misses": 1500,
    "neg_hits": 42,
    "plan_hit_rate": 0.92,
    "plan_hits": 920,
    "plan_misses": 80
  },
  "epoch_key": { "epoch_id": 5, "epoch_manifest_sha256": "..." },
  "epoch_transition_detected": false,
  "latency_stats": {
    "@type": "LatencyDistribution",
    "max_ns": 2500000,
    "p50_ns": 1200000,
    "p95_ns": 1950000,
    "p99_ns": 2400000
  },
  "stability": {
    "@type": "StabilityMetrics",
    "epoch_transition_overhead_pct": 0.0,
    "latency_coefficient_of_variation": 0.035,
    "plan_reuse_rate": 0.92
  },
  "variance_bounds": {
    "@type": "VarianceBounds",
    "acceptable_cache_hit_rate_change": 5,
    "acceptable_latency_variance_pct": 10,
    "is_regression": false
  },
  "workload_id": "workload_001"
}
```

### 8.2 Manifest Receipt Format

**File**: `.receipt` or `.receipt.json`

**Contents**:
- Benchmark results (quantified metrics)
- Digest hashes (65 fields)
- Guard status (PASS/FAIL)
- Event log entries (with wall-clock times marked excluded)
- Regression report (if applicable)
- Implementation hash (SHA256 of code/script)

**Example Structure**:
```
# EXECUTION_DIGEST_RECEIPT
query_fingerprint_sha256=a7f3c2d8e1b4f9a3c6d5e8f1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0
plan_hash=b8e4d3a9f2c5e8d1f4a7b0c3d6e9f2a5c8d1e4f7a0b3c6d9e2f5a8b1c4d7e0
resource_signature=c9f5e4b0g3d6f9e2a5b8c1d4e7f0a3b6c9d2e5f8a1b4c7d0e3f6a9b2c5d8e1
result_length_hash=d0g6f5c1h4e7g0f3b6c9d2e5f8a1b4c7d0e3f6a9b2c5d8e1f4a7b0c3d6e9f2
result_shape_hash=e1h7g6d2i5f8h1g4c7d0e3f6a9b2c5d8e1f4a7b0c3d6e9f2a5b8c1d4e7f0a3
digest_hash=f2i8h7e3j6g9i2h5d8e1f4a7b0c3d6e9f2a5b8c1d4e7f0a3b6c9d2e5f8a1b4

# BENCHMARK_RESULTS
variance_gate_cv=1.210%
variance_gate_status=PASS
p99_latency_ns=1240000
regression_status=NO_REGRESSION
latency_cv_baseline=0.042
latency_cv_current=0.035

# GUARD_STATUS
PLAN_HASH_MUST_MATCH=PASS
QUERY_FINGERPRINT_MUST_MATCH=PASS
RESOURCE_ENVELOPE_MUST_MATCH=PASS
RESULT_SHAPE_MUST_MATCH=PASS
RESULT_LENGTH_MUST_MATCH=PASS
EPOCH_MUST_NOT_CHANGE=PASS
EPOCH_MANIFEST_MUST_MATCH=PASS

# EVENT_LOG (wall-clock times excluded from validation)
QUERY_START wall_clock_ns=1234567890
CACHE_LOOKUP wall_clock_ns=1234567891
EXECUTION_START wall_clock_ns=1234567892
EXECUTION_END wall_clock_ns=1234568092
QUERY_COMPLETE wall_clock_ns=1234568093

# IMPLEMENTATION_HASH
code_hash=7a8f3e9c4b2d1f5e9a3c7b2d4f8e1a5c9d2f6e3a7b1c4d8e2f5a9c3d6e0f4a
script_hash=6b9e4f0d5c3e2g6f0b4d8c3e5g9f2b6d0e3g7f4a8b2c5d9e3f6a0c4d7e1f5a9

# VALIDATION_STATUS
receipt_valid=true
all_guards_pass=true
benchmarks_pass=true
digest_verified=true
determinism_verified=true
```

---

## 9. State Reconstruction from Receipts

### 9.1 State Reconstruction Invariant

**EPIC 4 Principle**: State is fully reconstructible from events, snapshots, and hashes.

**Reconstruction Steps**:
1. Load ExecutionDigest (contains all component hashes)
2. Load PerformanceEnvelope (contains latency distribution and cache stats)
3. Load ExecutionTraceEvents (contains event sequence, wall-clock times)
4. From events: reconstruct latency statistics via percentile calculation
5. From digests: verify integrity (recompute and compare)
6. From guards: validate all constraints (plan, fingerprint, resource, result)

**Full State Reconstruction Example**:
```cpp
// 1. Load receipt components
ExecutionDigest digest = loadDigestFromReceipt();
PerformanceEnvelope envelope = loadEnvelopeFromReceipt();
std::vector<ExecutionTraceEvent> events = loadEventsFromReceipt();

// 2. Verify digest integrity
assert(digest.verifyIntegrity());

// 3. Reconstruct latency distribution from events
std::vector<uint64_t> latencies;
for (size_t i = 0; i < events.size(); ++i) {
  if (events[i].event_type == TraceEventType::QUERY_START) {
    for (size_t j = i+1; j < events.size(); ++j) {
      if (events[j].event_type == TraceEventType::QUERY_COMPLETE) {
        uint64_t latency_ns = events[j].wall_clock_ns - events[i].wall_clock_ns;
        latencies.push_back(latency_ns);
        break;
      }
    }
  }
}

// 4. Verify reconstructed latency matches envelope
std::sort(latencies.begin(), latencies.end());
assert(latencies[latencies.size() * 99 / 100] == envelope.latency_stats.p99_ns);

// 5. Verify guards (requires original envelope components)
GuardConfiguration config = loadGuardConfigFromReceipt();
assert(validateGuards(digest, config));

// Full state reconstructed and verified ✓
```

### 9.2 No Mutable External State

**EPIC 4 Constraint**: No mutable external state permitted.

**Implication**:
- Receipts are immutable once created
- State reconstruction never modifies receipt
- All computations are deterministic and repeatable
- No side effects allowed in receipt validation

---

## 10. Summary: Complete Receipt Validation Gates

### Gate 1: Hash Validation
```
All 6 component hashes present ✓
All 64-char hex format ✓
digest_hash integrity verified ✓
```

### Gate 2: Guard Validation
```
PLAN_HASH_MUST_MATCH ✓
QUERY_FINGERPRINT_MUST_MATCH ✓
RESOURCE_ENVELOPE_MUST_MATCH ✓
RESULT_SHAPE_MUST_MATCH ✓
RESULT_LENGTH_MUST_MATCH ✓
EPOCH_MUST_NOT_CHANGE ✓
EPOCH_MANIFEST_MUST_MATCH ✓
```

### Gate 3: Benchmark Validation
```
Variance Gate: CV(P99 latency) < 5.0% ✓
Regression Gate: CV <= 10% ✓
Cache Gate: |hit_rate_change| <= 5% ✓
```

### Gate 4: Event Log Validation
```
QUERY_START exists ✓
QUERY_COMPLETE exists ✓
wall_clock_ns delta > 0 ✓
All times > 0 ✓
```

### Gate 5: Determinism Verification
```
Same result computed N times → N identical digests ✓
```

### Gate 6: State Reconstruction
```
Full state reconstructible from events/hashes ✓
No external mutable state required ✓
```

### Gate 7: No Narratives
```
Proof is purely quantitative ✓
No subjective arguments ✓
```

**FINAL RESULT**:
- **VALID**: All 7 gates pass → Receipt accepted, work correct by definition
- **INVALID**: Any gate fails → Abort immediately, reject receipt

---

## References

- **Receipt Validator Agent**: `/home/user/qlever/.claude/agents/bb80-receipt-validator.md`
- **Deterministic Receipts Skill**: `/home/user/qlever/.claude/skills/bb80-deterministic-receipts/SKILL.md`
- **Validation Script**: `/home/user/qlever/scripts/validate-receipts.sh`
- **Guard Configuration**: `/home/user/qlever/src/engine/readPlane/GuardConfiguration.h`
- **Execution Digest**: `/home/user/qlever/src/engine/readPlane/ExecutionDigest.h`
- **Result Digest**: `/home/user/qlever/src/engine/ingress/ResultDigest.h`
- **Performance Envelope**: `/home/user/qlever/src/engine/readPlane/PerformanceEnvelope.h`
- **Execution Trace**: `/home/user/qlever/src/engine/readPlane/ExecutionTraceDigest.h`
- **Variance Gate README**: `/home/user/qlever/benchmark/VARIANCE_GATE_README.md`
- **Big Bang 80/20**: `/home/user/qlever/CLAUDE.md` (Section: "Big Bang 80/20: Latent-Space Priming")

---

**Document Version**: 1.0 (Complete)
**Last Updated**: 2026-01-02
**Author**: Extracted from BB80/20 + EPIC 4 Specification
**Status**: SPECIFICATION CLOSURE - Ready for Implementation
