# Datalog Resource Guards - Quick Reference

**EPIC 10.2 - Agent 6 Implementation**

## Overview

Datalog/N3 rule execution is protected by resource guards that prevent:
- Fact explosion (runaway recursive rules)
- Infinite loops (non-terminating computation)
- OOM conditions (excessive memory usage)
- Cross-epoch cache contamination (stale results)

## Resource Limits (Default Values)

| Guard | Default | Purpose |
|-------|---------|---------|
| **MaxFactCount** | 1,000,000 facts | Prevent fact explosion |
| **MaxRuleTime** | 30,000 ms (30s) | Prevent infinite loops |
| **MaxMemory** | 1 GB | Prevent OOM conditions |

## Epoch Isolation

### Cache Key Structure

All Datalog/N3 operations include epoch information in cache keys:

```
FIXPOINT_COMPUTATION ancestor(?x,?y) max_iter=1000 epoch=5 manifest=a1b2c3d4...
RULE_EXPANSION parent(?x,?y) epoch=5 manifest=a1b2c3d4...
```

### Epoch ID Computation

Epoch ID is derived from `EpochManifest.getManifestHash()`:

```
SHA-256(
  epochId=5,
  assertedTriplesHash=abc123...,
  derivedTriplesHash=def456...,
  rulesetHash=ghi789...,
  shapesHash=jkl012...,
  configHash=mno345...,
  buildToolVersions=ANTLR=4.13.12,CMake=3.27.0,
  sealTimestampMs=1735797964000
)
```

Result: 64-character hex string uniquely identifying data version.

## Usage

### Automatic (Default Behavior)

Resource guards are **automatically applied** to all `FixpointComputation` operations:

```cpp
auto fixpoint = std::make_unique<FixpointComputation>(
    qec, ruleDatabase, "ancestor", arguments);
// Resource guards initialized from QueryExecutionContext
// Epoch ID extracted automatically
```

### Customizing Resource Limits

To customize resource limits (advanced use cases):

```cpp
auto fixpoint = std::make_unique<FixpointComputation>(
    qec, ruleDatabase, "ancestor", arguments, maxIterations);

// Access resource guards (before execution)
// Note: This is internal API, not exposed publicly
// Limits are set via runtime parameters or configuration
```

**Recommended approach**: Use runtime parameters or configuration files to adjust limits globally.

## Exception Handling

All resource guard violations throw `datalog::ResourceGuardViolation`:

```cpp
try {
  auto result = fixpoint->getResult();
} catch (const datalog::ResourceGuardViolation& e) {
  // Handle resource violation
  LOG(ERROR) << "Resource guard violation: " << e.what();

  // Possible causes:
  // 1. Fact count exceeded (reduce rule complexity)
  // 2. Time limit exceeded (optimize rule or increase limit)
  // 3. Memory limit exceeded (reduce data size)
}
```

## Monitoring Resource Usage

Resource usage is logged automatically:

```
[DEBUG] Starting fixpoint iterations with resource guards:
[DEBUG]   - Max iterations: 1000
[DEBUG]   - Max facts: 1000000
[DEBUG]   - Max time: 30000ms
[DEBUG]   - Max memory: 1000000000 bytes
[DEBUG]   - Epoch ID: 5

[INFO] Fixpoint computation completed with resource usage:
[INFO]   - Total facts: 45678 / 1000000
[INFO]   - Elapsed time: 1234ms / 30000ms
[INFO]   - Memory usage: 123456789 / 1000000000 bytes
```

## Cross-Epoch Isolation Guarantee

**Guarantee**: Cache entries from epoch N **cannot** be retrieved by queries in epoch N+1.

**Mechanism**:
1. Each epoch has unique manifest hash
2. Cache keys include epoch ID and manifest hash
3. Different epochs → different cache keys → separate cache entries
4. No manual cache invalidation required

**Example**:

```
Epoch 5:  Cache key = "FIXPOINT ... epoch=5 manifest=abc123..."
Epoch 6:  Cache key = "FIXPOINT ... epoch=6 manifest=def456..."
          ^
          Different keys → No cache hit from Epoch 5
```

## Testing

Test coverage in `test/engine/datalog/DatalogEpochIsolationTest.cpp`:

- ✓ Fact count guard enforcement
- ✓ Time guard enforcement
- ✓ Memory guard enforcement
- ✓ Resource guard validation
- ✓ Epoch cache isolation

## Troubleshooting

### Problem: "Fact count limit exceeded"

**Cause**: Recursive rule generating too many facts.

**Solutions**:
1. Add filters to reduce fact generation
2. Rewrite rule to be more selective
3. Increase `maxFactCount` (if appropriate)

### Problem: "Rule execution time limit exceeded"

**Cause**: Non-terminating or slow rule computation.

**Solutions**:
1. Add termination conditions to rule
2. Optimize rule body (reduce joins, use indexes)
3. Increase `maxRuleTime` (if appropriate)
4. Split rule into multiple smaller rules

### Problem: "Memory limit exceeded"

**Cause**: Rule generating large intermediate results.

**Solutions**:
1. Reduce base data size
2. Add selectivity to rule body
3. Use streaming evaluation (if available)
4. Increase `maxMemoryBytes` (if appropriate)

### Problem: Cache not hitting after index update

**Expected behavior**: This is correct! Cache keys include epoch ID.

After SPARQL Update:
- Epoch ID increments
- New manifest hash computed
- New cache keys generated
- Old cache entries ignored

This ensures queries always see fresh data.

## Integration with Existing Code

### QueryExecutionContext Integration

```cpp
// Epoch ID extracted from context
ad_utility::EpochId epochId = qec->getCurrentEpochId();

// Manifest hash for deterministic cache keying
std::string manifestHash = qec->getEpochDeterministicKey();
```

### EpochManifest Integration

```cpp
// Get manifest from global epoch manager
auto manifest = ad_utility::globalEpochManager
    .wlock()->getCurrentEpochManifest();

if (manifest.has_value()) {
  std::string hash = manifest->getManifestHash();
  // Use hash in cache key
}
```

## Performance Considerations

| Operation | Overhead | Impact |
|-----------|----------|--------|
| Cache key generation | ~1-2% | Negligible |
| Resource tracking | ~5-10% | Low |
| Memory overhead | ~100 bytes per operation | Negligible |

**Recommendation**: Resource guards have minimal performance impact and provide critical safety guarantees. Leave enabled in production.

## See Also

- [EPIC10.2_AGENT6_DATALOG_N3_GUARDRAILS.receipt](../../EPIC10.2_AGENT6_DATALOG_N3_GUARDRAILS.receipt) - Delivery receipt with full implementation details
- [src/engine/datalog/DatalogResourceGuards.h](../../src/engine/datalog/DatalogResourceGuards.h) - Resource guard definitions
- [src/global/EpochManifest.h](../../src/global/EpochManifest.h) - Epoch manifest structure
- [test/engine/datalog/DatalogEpochIsolationTest.cpp](../../test/engine/datalog/DatalogEpochIsolationTest.cpp) - Test coverage

---

**Last Updated**: 2026-01-02 04:46:04 UTC
**Agent**: 6 (Datalog/N3 Guardrails)
**Status**: SEALED
