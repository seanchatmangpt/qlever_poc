# EPIC 4: Shared Invariant Contract

This document defines the minimal set of invariants that all 6 EPIC 4 subsystems must obey. No subsystem may violate any rule here. Violations are fail-closed (execution aborts, artifact emitted).

---

## 1. CORE AXIOMS (Non-Negotiable)

1. **Epoch Binding:** Every artifact MUST be keyed with `EpochKey = (epochId, manifestSha256)`. Cross-epoch contamination is a fatal error.
2. **Determinism Filter:** Workload capture ONLY includes deterministic queries. Non-deterministic queries are logged separately as `EXCLUDED`.
3. **Query Fingerprint Immutability:** QueryFingerprint values (shape_sha256, params_sha256) MUST NOT change across subsystems.
4. **Atomic Visibility:** All writes to shared artifacts MUST be atomic or guarded by locks. No torn reads.
5. **Failure = Abort:** Any non-determinism, ambiguity, or inconsistency → execution aborts with artifact, never silent fallback.

---

## 2. CANONICAL DATA ARTIFACTS

All subsystems produce and consume these 6 artifacts. All artifacts MUST be:
- Deterministic (same inputs → same bytes)
- Hashable (SHA-256 digest)
- Portable (no pointer values, thread IDs, or wall-clock times)
- Versioned (v1.0 baseline)

### Artifact 1: WorkloadRecord (Per-Query)

```cpp
struct WorkloadRecord {
  // Identity
  uint64_t sequence_id;              // Global monotonic counter
  uint64_t capture_timestamp_epoch;  // Normalized to epoch, not wall-clock

  // Execution Context
  EpochKey epoch_key;                // (epochId, manifestSha256)
  QueryFingerprint query_fp;         // Complete query identity

  // Execution Metadata
  std::string execution_class;       // "CACHE_HIT_BYTES" | "CACHE_HIT_PLAN" | "CACHE_MISS" | "RECOMPILE"
  uint64_t observed_latency_ns;      // Wall-clock time (must be removed before replay)

  // Replay-Relevant Parameters (ONLY)
  std::map<std::string, std::string> replay_params; // Parameters that affect execution path

  // Digest for Reproducibility
  std::string fingerprint_sha256;    // SHA256(query_fp serialized)
};
```

**Constraints:**
- `capture_timestamp_epoch` must be deterministic (e.g., epoch counter, not wall-clock)
- `observed_latency_ns` must NOT be used for correctness decisions
- `replay_params` must be a subset of QueryFingerprint (already normalized)
- All serialized to JSON-LD for portability

### Artifact 2: WorkloadManifest (Ordered Collection)

```cpp
struct WorkloadManifest {
  // Metadata
  std::string id;                        // Unique ID for this workload (uuid)
  std::string format_version;            // "v1.0"
  std::string captured_on_hostname;      // For audit (not for correctness)
  uint64_t epoch_id;                     // All records MUST have same epoch
  std::string epoch_manifest_sha256;     // Proof of reproducibility

  // Sequence Proof
  std::vector<WorkloadRecord> records;   // Ordered, immutable after creation
  std::string manifest_digest;           // SHA256(all records concatenated)

  // Statistics (Informational Only)
  struct {
    uint64_t total_records;
    uint64_t deterministic_count;
    uint64_t nondeterministic_excluded;
    std::map<std::string, uint64_t> execution_class_counts;
  } stats;
};
```

**Constraints:**
- Once created, manifest is immutable (append-only)
- `manifest_digest` must be reproducible on another machine
- All records must share single epoch (no cross-epoch workloads)

### Artifact 3: ReplayResult (Per Execution)

```cpp
struct ReplayResult {
  // Identity
  uint64_t replay_run_id;          // UUID for this replay execution
  uint64_t workload_record_id;     // Which WorkloadRecord was replayed

  // Replay Context
  EpochKey replayed_on_epoch;      // Epoch on replay machine (may differ from capture epoch)
  std::string replayed_on_hostname;

  // Execution Outcome
  std::string execution_status;    // "SUCCESS" | "DIVERGENCE" | "ABORT"
  std::string execution_digest;    // ExecutionDigest.digest_hash

  // Comparison to Expected
  std::string expected_digest;     // From capture machine (for comparison)
  bool digest_matches;             // True iff hashes equal

  // Trace (Optional, for Debugging)
  std::vector<ExecutionTraceEvent> trace_events; // Timestamped events (wall-clock, for inspection only)
};
```

**Constraints:**
- Comparison must be digest-based, not result-based
- If `digest_matches == false` → execution ABORTS (fail-closed)
- Trace events may contain wall-clock time (purely informational)

### Artifact 4: ExecutionDigest (Deterministic Hash)

```cpp
struct ExecutionDigest {
  // Execution Identity
  std::string query_fingerprint_sha256;   // QueryFingerprint hash
  std::string plan_hash;                  // Hash of compiled QueryExecutionTree

  // Resource Envelope
  std::string resource_signature;         // Hash of:
                                          // - Memory pages accessed
                                          // - Cache tier utilization
                                          // - Result serialization length

  // Result Identity (NOT result content)
  std::string result_length_hash;         // SHA256(result.size())
  std::string result_shape_hash;          // SHA256(result structure, not content)

  // Final Digest
  std::string digest_hash;                // SHA256(concatenate all above)
};
```

**Constraints:**
- `digest_hash` is reproducible: same query + same epoch + same machine state → same hash
- Must NOT include result content (only result structure)
- Must NOT include wall-clock times
- Must include plan hash (detects plan reuse)

### Artifact 5: CacheDecision (Per Cache Event)

```cpp
struct CacheDecision {
  // Context
  uint64_t record_sequence_id;         // Which WorkloadRecord triggered this decision
  EpochKey epoch_key;
  std::string decision_type;           // "BYTES_HIT" | "BYTES_MISS" | "PLAN_HIT" | "PLAN_MISS" | "NEG_HIT" | "NEG_MISS"

  // Decision Factors (Machine-Readable)
  struct {
    bool is_deterministic;             // Passed DeterminismClassifier
    bool meets_frequency_threshold;    // >= configured threshold
    bool is_in_top_k_shapes;           // Within top-K shapes
    uint64_t observed_frequency;       // How many times seen
    std::string admission_reason;      // "FREQ_THRESHOLD_MET" | "TOP_K" | "DETERMINISM_FAIL" etc.
  } factors;

  // Outcome
  bool admitted_to_cache;              // Final decision
};
```

**Constraints:**
- All factors must be machine-readable (structured, not prose)
- No "judgment calls" — all decisions must be algorithmic
- Decision log must be queryable (for correctness proofs)

### Artifact 6: PerformanceEnvelope (Aggregated Over Workload)

```cpp
struct PerformanceEnvelope {
  // Identity
  std::string workload_id;           // Which WorkloadManifest
  std::string epoch_key;

  // Latency Distribution (Deterministic)
  struct {
    uint64_t p50_ns;
    uint64_t p95_ns;
    uint64_t p99_ns;
    uint64_t max_ns;
  } latency_stats;

  // Cache Efficiency
  struct {
    uint64_t bytes_hits;
    uint64_t bytes_misses;
    uint64_t plan_hits;
    uint64_t plan_misses;
    uint64_t neg_hits;
    float bytes_hit_rate;
    float plan_hit_rate;
  } cache_stats;

  // Stability Metrics (for Regression Detection)
  struct {
    float latency_coefficient_of_variation;  // σ / μ
    float plan_reuse_rate;
    float epoch_transition_overhead_pct;
  } stability;

  // Variance Bounds (For CI)
  struct {
    uint64_t acceptable_latency_variance_pct;    // Default: 10%
    uint64_t acceptable_cache_hit_rate_change;   // Default: 5%
    bool is_regression;                          // True if any metric exceeds bound
  } variance_bounds;
};
```

**Constraints:**
- All latencies are wall-clock (informational, not for correctness)
- Variance bounds are pre-defined (not human-interpreted)
- Regression detection is automatic in CI

---

## 3. EPOCH BINDING RULES

**Rule 1: Strict Epoch Isolation**
```
All cache operations MUST check:
  if (query.epoch_key != cache_entry.epoch_key) {
    ABORT with CacheDecision { admitted = false, reason = "EPOCH_MISMATCH" }
  }
```

**Rule 2: Workload ← Single Epoch**
```
A WorkloadManifest MUST have a single epoch_id.
If a record's epoch differs: ABORT immediately.
```

**Rule 3: Replay Epoch Binding**
```
Replay can happen on a DIFFERENT epoch (e.g., capture on epoch 5, replay on epoch 6).
Comparison is STILL by ExecutionDigest (deterministic, epoch-independent).
If digest matches but epoch differs: Log as "CROSS_EPOCH_REPRODUCIBLE" (SUCCESS with note).
```

---

## 4. DETERMINISM REQUIREMENTS

**Determinism Classification (Inherited from EPIC 3):**
- Query is deterministic IFF DeterminismClassifier.isDeterministic() == true
- Non-deterministic queries: NOW(), RAND(), UUID(), BNODE(), SERVICE

**Workload Capture Rule:**
```
if (!query.is_deterministic) {
  Record in EXCLUDED section of WorkloadManifest
  do NOT create WorkloadRecord
  do NOT create CacheDecision
}
```

**Cache Admission Rule:**
```
// BytesCache: DETERMINISTIC REQUIRED
if (shouldAdmitBytes(query)) {
  if (!query.is_deterministic) {
    ABORT with "DETERMINISM_REQUIRED_VIOLATION"
  }
}

// PlanCache: DETERMINISTIC NOT REQUIRED
if (shouldAdmitPlan(query)) {
  // No determinism check; plans reuse across deterministic/non-deterministic
}

// NegativeCache: DETERMINISTIC REQUIRED
if (shouldAdmitEmpty(query)) {
  if (!query.is_deterministic) {
    ABORT with "DETERMINISM_REQUIRED_VIOLATION"
  }
}
```

---

## 5. HASHING & DIGEST RULES

**Deterministic Serialization:**
- All structs serialized to JSON-LD with canonical ordering (fields alphabetical)
- Floating-point values rounded to 6 decimals (for stability)
- Timestamps excluded from digest computation (marked with `@excluded_from_hash`)

**ExecutionDigest Computation:**
```
digest = SHA256(
  query_fingerprint_sha256 ||
  plan_hash ||
  resource_signature ||
  result_length_hash ||
  result_shape_hash
)
```

**Manifest Digest Computation:**
```
manifest_digest = SHA256(
  concatenate all WorkloadRecord serialized ||
  epoch_id ||
  epoch_manifest_sha256
)
```

---

## 6. FAILURE SEMANTICS (Fail-Closed)

**Non-Determinism Detected:**
```
→ ABORT query execution
→ Emit Artifact: { error: "NON_DETERMINISTIC_DETECTED", query_fp: ... }
→ Do NOT return partial results
→ Do NOT fall back to non-caching
```

**Cache Ambiguity (e.g., Multiple Valid Plans):**
```
→ ABORT with CacheDecision { admitted = false, reason = "PLAN_AMBIGUITY" }
→ Do NOT pick one arbitrarily
```

**Replay Divergence (ExecutionDigest Mismatch):**
```
→ ABORT with ReplayResult { status: "DIVERGENCE", divergence_artifact: ... }
→ Do NOT ignore divergence
→ Do NOT report "close enough"
```

**Epoch Mismatch in Cache:**
```
→ Treat as MISS (not as corruption)
→ Log with CacheDecision { reason = "EPOCH_MISMATCH" }
→ Continue normally (cache refresh)
```

---

## 7. ATOMIC VISIBILITY & THREAD SAFETY

**Shared Mutable State:**
1. WorkloadManifest (append-only, readers may read partial)
2. CacheDecision log (append-only, readers may read partial)
3. PerformanceEnvelope stats (atomic updates per counter)

**Synchronization:**
- WorkloadManifest: Protected by RWLock (writers exclusive)
- CacheDecision log: Protected by lock-free queue (MPMC)
- PerformanceEnvelope: std::atomic<> per counter, no global lock

**Reader Guarantees:**
- Readers of WorkloadManifest see consistent prefix
- Readers of CacheDecision log see all decisions up to their read time
- PerformanceEnvelope counters are always consistent (atomic)

---

## 8. NO SUBSYSTEM MAY ASSUME ORDER

**Invalid (Serialized):**
```cpp
// WRONG: Capture subsystem assumes it runs before Replay
if (capture_finished()) {
  start_replay();  // INVALID: Assumes ordering
}
```

**Valid (Independent):**
```cpp
// CORRECT: Each subsystem publishes artifacts; others consume
publish(WorkloadManifest);
spawn_replay_agent(WorkloadManifest);  // Independent, runs in parallel
```

---

## 9. SUMMARY: Convergence Points

All 6 subsystems MUST converge via:

| Artifact | Producer | Consumers |
|----------|----------|-----------|
| WorkloadRecord | Capture | Replay, Benchmarks, Observability |
| WorkloadManifest | Capture | Replay, Benchmarks, Observability |
| ReplayResult | Replay | Correctness Boundary, Benchmarks, Observability |
| ExecutionDigest | Execution Envelope | Replay, Benchmarks, Correctness Boundary |
| CacheDecision | Cache Correctness | Benchmarks, Observability |
| PerformanceEnvelope | Benchmarks | Observability, CI Regression Detection |

**No Ordering Dependency Between Producers and Consumers**—all artifacts are immutable after creation.

---

## 10. INVARIANT VIOLATION CHECKLIST

Before any subsystem commits code, verify:

- [ ] All epochs are bound via EpochKey (epoch_id + manifest hash)
- [ ] Non-deterministic queries are excluded from capture
- [ ] All serialized structures have deterministic field ordering
- [ ] No artifact contains pointer values, thread IDs, or uncontrolled wall-clock times
- [ ] All cache decisions are logged as CacheDecision structs
- [ ] Replay divergence causes ABORT (not silent fallback)
- [ ] All writes to shared artifacts are atomic or locked
- [ ] No subsystem assumes execution order
- [ ] Failure modes result in artifacts (no silent failures)

---

**Status:** Baseline v1.0
**Last Updated:** 2026-01-01
**Next Review:** After Subsystem 1 completion
