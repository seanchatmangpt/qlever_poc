# EPIC 1.1 Integration Tests - Quick Reference

## Running the Tests

```bash
# One-time setup (if not done)
./scripts/setup-dev-env.sh

# Build
./scripts/build-release.sh
cd build

# Run all EPIC 1.1 tests
ctest -R EpochContractIntegrationTest --output-on-failure

# Run single test
ctest -R EpochContractIntegrationTest.FullContractWorkflow --output-on-failure

# Run with timing
ctest -R EpochContractIntegrationTest --output-on-failure --verbose --progress

# Run all tests in parallel
ctest -R EpochContractIntegrationTest -j$(nproc) --output-on-failure
```

---

## Test Scenarios at a Glance

| # | Test Name | Purpose | Key Assertion |
|---|-----------|---------|---------------|
| 1 | `FullContractWorkflow` | Complete INIT→INGEST→SEAL→SERVE cycle | Manifest stable, immutable |
| 2 | `IngressPipelineIntegration` | External data ingestion with audit | Token enforcement, audit trail |
| 3 | `DeterministicCaching` | Cache hit/miss on manifest hash | Same hash = Cache hit |
| 4 | `AtomicPromotionSafety` | Concurrent queries during promotion | No stale epoch binding |
| 5 | `ManifestReproducibility` | Identical inputs = Same manifest | E1 manifest == E2 manifest |
| 6 | `CacheInvalidationHookFiring` | Hook fired with correct data | Event captures old/new |
| 7 | `RapidPromotionStressTest` | 50 cycles × 20 queries = 1,000 ops | Consistent under stress |
| 8 | `DisasterRecoveryRollback` | Rollback on validation failure | Data restored, queries work |

---

## Architecture Summary

```
┌────────────────────────────────────────────────────┐
│ EPIC 1.1: Contract + Receipt + Atomic Promote      │
├────────────────────────────────────────────────────┤
│                                                     │
│ 1. CONTRACT (EpochManifest)                         │
│    ├── rdfTriplesSha256                            │
│    ├── inferenceRulesSha256                        │
│    ├── shapeSchemasSha256                          │
│    └── indexParametersSha256                       │
│         → Deterministic hash value                 │
│                                                     │
│ 2. RECEIPT (ManifestHash)                          │
│    ├── Cryptographic proof of data                 │
│    ├── Uniquely identifies epoch                   │
│    └── Cache key component                         │
│                                                     │
│ 3. ATOMIC PROMOTE (Epoch Transition)               │
│    ├── All queries bind before transition          │
│    ├── New queries see new epoch                   │
│    ├── No mixed-epoch results                      │
│    └── Zero-downtime operation                     │
│                                                     │
│ 4. CACHE SYSTEM (Result Storage)                   │
│    ├── Key: queryText | manifestHash               │
│    ├── Hit: Same hash = reuse result               │
│    ├── Miss: Different hash = recompute            │
│    └── Invalidation: hooks fire on transition      │
│                                                     │
└────────────────────────────────────────────────────┘
```

---

## Key Classes & Structs

### ManifestHash
**Purpose:** Cache key identifier
```cpp
ManifestHash hash("data");        // Create
hash1 == hash2                    // Compare
hash.toString()                   // Stringify
```

### EpochManifest
**Purpose:** Immutable data contract
```cpp
EpochManifest m(0, rdfHash, rulesHash, shapesHash, indexHash);
m.getHash()                       // Get receipt
m.epochId                         // Which epoch
m.createdAt                       // When created
```

### SimulatedCacheSystem
**Purpose:** Deterministic caching
```cpp
// Try cache hit
std::string result;
bool hit = cacheSystem_->tryGetCached(query, hash, result);

// Store result
cacheSystem_->cachingQueryResult(query, hash, result);

// Handle invalidation
cacheSystem_->registerInvalidationHandler([](const auto& event) {
  // old/new manifests, epochIds, timestamp
});

// Invalidate
cacheSystem_->invalidateOnPromotion(oldManifest, newManifest);
```

### SimulatedIngressPipeline
**Purpose:** External data ingestion
```cpp
// Ingest data (requires INGEST epoch)
bool ok = ingressPipeline_->ingestRdfData(rdfContent, tripleCount);

// Check audit trail
const auto& trail = ingressPipeline_->getAuditTrail();

// Verify writes blocked
bool allowed = ingressPipeline_->tryAdHocWrite();
```

### RealWorldDataLoader
**Purpose:** Test data
```cpp
std::string foaf = RealWorldDataLoader::getFoafSampleData();
std::string shapes = RealWorldDataLoader::getShapesData();
std::string rules = RealWorldDataLoader::getInferenceRules();
ManifestHash h = RealWorldDataLoader::getDataHash(data);
```

---

## Common Test Patterns

### Pattern 1: Full Epoch Lifecycle
```cpp
// INIT (default)
ASSERT_EQ(getEpochState(), EpochState::INIT);

// INGEST
transitionToIngest();
ingressPipeline_->ingestRdfData(data, count);

// SEAL
transitionToSeal();
EpochManifest manifest = createManifest(hash1, hash2, hash3, hash4);

// SERVE
transitionToServe();
EpochId epoch = getEpochId();

// Query works
auto epochForQuery = globalEpochManager.acquire()->getCurrentEpochIdForQuery();
```

### Pattern 2: Cache Testing
```cpp
// Build epoch A with manifest A
transitionToIngest();
// ... ingest data ...
transitionToSeal();
EpochManifest mA = createManifest(...);
transitionToServe();
ManifestHash hA = mA.getHash();

// Query: miss → store
cacheSystem_->cachingQueryResult(query, hA, resultA);

// Promote to B
restartEpoch();
// ... build epoch B with manifest B ...
EpochManifest mB = createManifest(...);
ManifestHash hB = mB.getHash();

// Cache invalidated
cacheSystem_->invalidateOnPromotion(mA, mB);

// Query: miss (different hash)
// Store with different hash
cacheSystem_->cachingQueryResult(query, hB, resultB);

// Later: If C has same manifest as A (hC == hA)
// Query would hit cache (same hash)
bool hit = cacheSystem_->tryGetCached(query, hC, resultC);
EXPECT_TRUE(hit);  // Cache reuse!
```

### Pattern 3: Atomic Promotion
```cpp
// Setup: Epoch N in SERVE
// ... transition and serve ...
EpochId epochN = getEpochId();

// Launch 10 concurrent queries
std::vector<std::thread> threads;
std::vector<EpochId> capturedEpochs;

for (int i = 0; i < 10; ++i) {
  threads.emplace_back([&capturedEpochs, epochN]() {
    // Capture epoch at start
    EpochId captured = getCurrentEpochIdForQuery();

    // Simulate execution (overlaps with promotion)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    capturedEpochs.push_back(captured);
  });
}

// During query execution: promote to N+1
restartEpoch();
transitionToIngest();
// ... ingest ...
transitionToSeal();
transitionToServe();

// Wait for all queries
for (auto& t : threads) t.join();

// Verify: all queries saw epoch N (not N+1)
EXPECT_EQ(capturedEpochs.size(), 10u);
for (auto epoch : capturedEpochs) {
  EXPECT_EQ(epoch, epochN);  // All bound to original
}
```

### Pattern 4: Manifest Reproducibility
```cpp
// Build 1: E1 with data set A
transitionToIngest();
ingressPipeline_->ingestRdfData(dataA, countA);
transitionToSeal();
EpochManifest m1 = createManifest(hashA1, hashA2, hashA3, hashA4);
manifestE1 = m1.getHash();
transitionToServe();

// Build 2: E2 with same data set A
restartEpoch();
transitionToIngest();
ingressPipeline_->ingestRdfData(dataA, countA);  // Identical!
transitionToSeal();
EpochManifest m2 = createManifest(hashA1, hashA2, hashA3, hashA4);  // Same hashes
manifestE2 = m2.getHash();
transitionToServe();

// Verify reproducibility
EXPECT_EQ(manifestE1, manifestE2);  // Same hash!

// Cache reuse works
cacheSystem_->cachingQueryResult(query, manifestE1, result1);
bool hit = cacheSystem_->tryGetCached(query, manifestE2, result2);
EXPECT_TRUE(hit);  // Found with E2's hash (same as E1)
```

---

## Troubleshooting

### Test Fails: "Write attempted outside INGEST epoch"
**Cause:** Not in INGEST state when ingesting
**Fix:** Call `transitionToIngest()` first

### Test Fails: "Query attempted outside SERVE epoch"
**Cause:** Not in SERVE state when querying
**Fix:** Transition through full cycle: INGEST → SEAL → SERVE

### Test Hangs
**Cause:** Deadlock in concurrent queries
**Fix:** Check for proper thread synchronization; use `join()` on threads

### Cache Hit Unexpected (Expected Miss)
**Cause:** Different manifest hash but same query
**Fix:** Verify manifests are actually different; check hash computation

### Manifest Hash Mismatch
**Cause:** Input data or hash function changed
**Fix:** Use deterministic hash function; verify data is identical

---

## Performance Expectations

| Test | Duration | Notes |
|------|----------|-------|
| FullContractWorkflow | ~10ms | Simple transitions |
| IngressPipelineIntegration | ~20ms | Two ingestion ops |
| DeterministicCaching | ~30ms | Three epochs |
| AtomicPromotionSafety | ~200ms | 10 threads × 50ms sleep |
| ManifestReproducibility | ~50ms | Two epochs |
| CacheInvalidationHookFiring | ~30ms | Hook execution |
| RapidPromotionStressTest | ~1-2s | 50 epochs × 20 threads |
| DisasterRecoveryRollback | ~40ms | Rollback sequence |

**Total Suite:** ~1-2 seconds

---

## Coverage Map

```
EPIC 1.1 Requirements
├── Contract System
│   ├── Manifest creation ..................... TEST 1, 5, 6
│   ├── Deterministic hashing ................. TEST 3, 5
│   └── Manifest reproducibility .............. TEST 5
│
├── Receipt (Cache Key)
│   ├── Manifest hash stability ............... TEST 1, 3, 5
│   ├── Cache hit/miss behavior ............... TEST 3, 5
│   └── Cache key uniqueness .................. TEST 3
│
├── Atomic Promotion
│   ├── Non-blocking transition ............... TEST 4, 7
│   ├── Query epoch binding ................... TEST 4, 7
│   └── No mixed-epoch results ................ TEST 4, 7
│
├── Ingress Integration
│   ├── Token enforcement ..................... TEST 2
│   ├── Audit trail ........................... TEST 2
│   └── Write barriers ........................ TEST 1, 2
│
├── Cache Invalidation
│   ├── Event firing .......................... TEST 6
│   ├── Handler execution ..................... TEST 6
│   └── Cache clearing ........................ TEST 3, 6
│
└── Robustness
    ├── Concurrent safety ..................... TEST 4, 7
    ├── Stress (1,000 ops) .................... TEST 7
    └── Disaster recovery ..................... TEST 8
```

---

## Key Assertions

### Determinism
```cpp
EXPECT_EQ(manifestHashA, manifestHashB);  // Same inputs = Same hash
```

### Atomicity
```cpp
EXPECT_EQ(capturedEpoch, epochN);  // Query sees original epoch
EXPECT_EQ(newQueryEpoch, epochNPlus1);  // New query sees new epoch
```

### Immutability
```cpp
EXPECT_FALSE(ingressPipeline_->tryAdHocWrite());  // Blocked
```

### Cache Behavior
```cpp
EXPECT_TRUE(cacheHit);  // Same hash = reuse
EXPECT_FALSE(cacheMiss);  // Different hash = recompute
```

---

## Integration Points

### With Epoch Manager
```cpp
globalEpochManager.acquire()->getState()          // Get current state
globalEpochManager.acquire()->getEpochId()        // Get epoch ID
globalEpochManager.acquire()->getCurrentEpochIdForQuery()  // Bind query
globalEpochManager.acquire()->checkAllowedToMutate()      // Verify INGEST
globalEpochManager.acquire()->getIngressWriteCount()      // Audit
```

### With Cache System
```cpp
cacheSystem_->tryGetCached(query, hash, result)   // Hit/miss
cacheSystem_->cachingQueryResult(query, hash, result)  // Store
cacheSystem_->invalidateOnPromotion(old, new)     // Invalidate
cacheSystem_->registerInvalidationHandler(fn)     // Hook
```

### With Ingress Pipeline
```cpp
ingressPipeline_->ingestRdfData(data, count)      // Ingest
ingressPipeline_->tryAdHocWrite()                 // Verify barrier
ingressPipeline_->getAuditTrail()                 // Audit
```

---

## File Locations

| File | Lines | Purpose |
|------|-------|---------|
| EpochContractIntegrationTest.cpp | 1,103 | Main test file |
| test/integration/CMakeLists.txt | 16 | Test registration |
| EPIC_1_1_INTEGRATION_TEST_SUMMARY.md | Comprehensive docs | This reference |
| EPIC_1_1_QUICK_REFERENCE.md | Quick guide | Quick lookup |

---

## Related Documentation

- **EPIC 1 Implementation:** `/home/user/qlever/CLAUDE.md`
- **Epoch Manager:** `/home/user/qlever/src/global/Epoch.h`
- **Epoch Metrics:** `/home/user/qlever/src/global/EpochMetrics.h`
- **Build Guide:** `/home/user/qlever/CLAUDE.md` (Development Workflow section)
- **Test Conventions:** `/home/user/qlever/CLAUDE.md` (Testing Framework section)

---

**Last Updated:** 2025-01-01
**Status:** Ready for Testing
