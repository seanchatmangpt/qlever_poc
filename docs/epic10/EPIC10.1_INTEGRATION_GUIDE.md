# EPIC 10.1: Workload Replay & Divergence Abort - Integration Guide

**Agent**: Agent 8 - Workload Replay & Fail-Closed Divergence Abort
**Date**: 2026-01-02
**Spec Lock**: EPIC 10.1 Sections 3.6, 3.7, 6.2, 6.4

## Overview

This guide demonstrates how to integrate the **DivergenceAbortHandler** with **WorkloadReplayEngine** to achieve fail-closed divergence detection with structured error reporting.

## Architecture

```
WorkloadReplayEngine
    ↓
  replayAll() / replaySingle()
    ↓
  ExecutionDigest comparison
    ↓
  Divergence detected? → DivergenceAbortHandler
    ↓
  Classify divergence → DivergenceErrorCode
    ↓
  Create StructuredDivergenceError
    ↓
  Verify fail-closed semantics
    ↓
  Throw DivergenceAbortException
    ↓
  Halt replay (no further queries)
```

## Integration Example

### Basic Usage

```cpp
#include "engine/readPlane/WorkloadReplayEngine.h"
#include "engine/readPlane/DivergenceAbortHandler.h"

using namespace readPlane;

// 1. Load workload manifest
auto manifest = WorkloadManifest::loadFromFile("workload-manifest.jsonld");

// 2. Create replay engine with fail-closed configuration
ReplayConfiguration config;
config.abort_on_divergence = true;  // Fail-closed behavior
config.collect_trace_events = true; // For debugging
config.hostname = "replay-node-01";

WorkloadReplayEngine engine(&manifest, config);

// 3. Set execution context
engine.setQueryExecutionContext(qec);
engine.setServer(server);
engine.setIndex(index);
engine.setCache(cache);

// 4. Set replay epoch
EpochKey replay_epoch{42, "sha256:epoch_manifest..."};
engine.setReplayEpoch(replay_epoch);

// 5. Execute replay with divergence abort handling
try {
  ReplayRun run = engine.replayAll();

  // Check overall status
  if (run.overall_status == ReplayStatus::SUCCESS) {
    std::cout << "Replay succeeded: " << run.stats.successful_replays
              << " queries replayed" << std::endl;
  }

} catch (const DivergenceAbortException& ex) {
  // Divergence detected - fail-closed abort triggered
  const auto& error = ex.getError();

  // Log structured error
  std::cerr << "Divergence detected: " << error.getSummary() << std::endl;
  std::cerr << "Error code: " << divergenceErrorCodeToString(error.error_code)
            << " (" << static_cast<uint32_t>(error.error_code) << ")" << std::endl;
  std::cerr << "Sequence ID: " << error.workload_record_sequence_id << std::endl;

  // Verify fail-closed semantics
  if (!error.isValidFailClosed()) {
    std::cerr << "FAIL-CLOSED VIOLATION: Partial results emitted or abort deferred!"
              << std::endl;
  }

  // Write error artifact to disk
  std::ofstream artifact_file("divergence-artifact.jsonld");
  artifact_file << error.toJsonLD().dump(2);
  artifact_file.close();

  // Exit with deterministic error code
  return DivergenceAbortHandler::errorCodeToExitCode(error.error_code);
}
```

### Advanced: Custom Divergence Handling

```cpp
// Set custom callbacks for divergence events
ReplayCallbacks callbacks;

callbacks.onDivergenceDetected = [](const DivergenceArtifact& artifact) {
  // Custom logic on divergence detection
  std::cerr << "Divergence at sequence ID: "
            << artifact.workload_record_sequence_id << std::endl;

  // Classify divergence using DivergenceAbortHandler
  ExecutionDigest expected, actual;
  expected.digest_hash = artifact.expected_digest;
  actual.digest_hash = artifact.actual_digest;

  EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);
  DivergenceErrorCode code = DivergenceAbortHandler::classifyDivergence(diff);

  std::cerr << "Classification: " << divergenceErrorCodeToString(code) << std::endl;
};

callbacks.onRecordCompleted = [](const ReplayResult& result) {
  // Verify fail-closed semantics after each record
  try {
    DivergenceAbortHandler::verifyFailClosedSemantics(result);
  } catch (const DivergenceAbortException& ex) {
    std::cerr << "Fail-closed violation detected: " << ex.what() << std::endl;
    throw;  // Re-throw to halt replay
  }
};

engine.setCallbacks(callbacks);
```

### Manual Divergence Classification

```cpp
// Given two ExecutionDigests
ExecutionDigest expected = /* from capture */;
ExecutionDigest actual = /* from replay */;

// Compute envelope diff
EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

if (!diff.is_identical) {
  // Classify the divergence
  DivergenceErrorCode error_code =
      DivergenceAbortHandler::classifyDivergence(diff);

  std::cout << "Divergence classification: "
            << divergenceErrorCodeToString(error_code) << std::endl;

  // Examine specific component differences
  if (diff.hasComponentDiff("plan_hash")) {
    auto plan_diff = diff.getComponentDiff("plan_hash").value();
    std::cout << "Plan hash changed:" << std::endl;
    std::cout << "  Before: " << plan_diff.before_hash << std::endl;
    std::cout << "  After:  " << plan_diff.after_hash << std::endl;
  }

  // Create structured error
  auto error = DivergenceAbortHandler::createError(
      expected, actual,
      workload_record_sequence_id,
      replay_run_id);

  // Abort with fail-closed semantics
  DivergenceAbortHandler::abortFailClosed(error);
}
```

## Error Handling Patterns

### Pattern 1: Immediate Abort (Default)

```cpp
// Default behavior: abort on first divergence
ReplayConfiguration config;
config.abort_on_divergence = true;  // DEFAULT

try {
  ReplayRun run = engine.replayAll();
} catch (const DivergenceAbortException& ex) {
  // Handle divergence
  logDivergenceArtifact(ex.getError());
  return ex.getError().error_code;
}
```

### Pattern 2: Collect All Divergences (For Analysis Only)

```cpp
// WARNING: This violates fail-closed semantics for EPIC 10.1
// Only use for offline analysis, NOT for production replay
ReplayConfiguration config;
config.abort_on_divergence = false;  // COLLECT MODE

ReplayRun run = engine.replayAll();

// Analyze all divergences
for (const auto& result : run.results) {
  if (result.execution_status == ReplayStatus::DIVERGENCE) {
    // Process divergence artifact
    if (result.divergence_artifact.has_value()) {
      analyzeDivergence(*result.divergence_artifact);
    }
  }
}
```

### Pattern 3: Structured Error Reporting

```cpp
try {
  ReplayRun run = engine.replayAll();
} catch (const DivergenceAbortException& ex) {
  const auto& error = ex.getError();

  // Create machine-readable error report
  nlohmann::ordered_json error_report;
  error_report["@type"] = "DivergenceReport";
  error_report["timestamp"] = getCurrentTimestamp();
  error_report["replay_run_id"] = error.replay_run_id;
  error_report["error"] = error.toJsonLD();

  // Include envelope diff for detailed analysis
  if (error.envelope_diff.has_value()) {
    error_report["envelope_diff"] = error.envelope_diff->toJsonLD();
  }

  // Write to structured log
  writeStructuredLog(error_report);

  // Alert monitoring system
  sendAlert(error.error_code, error.getSummary());

  // Return deterministic exit code
  return DivergenceAbortHandler::errorCodeToExitCode(ex.getErrorCode());
}
```

## Integration with CI/CD

### CI Pipeline Integration

```bash
#!/bin/bash
# ci-replay-test.sh

# Run workload replay with fail-closed divergence detection
./qlever-replay \
  --manifest workload-baseline.jsonld \
  --epoch-id ${CI_BUILD_EPOCH} \
  --abort-on-divergence true \
  --output replay-results.jsonld

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
  echo "✓ Replay succeeded - determinism validated"
  exit 0
elif [ $EXIT_CODE -eq 11 ]; then
  echo "✗ ENVELOPE_PLAN_HASH_MISMATCH - plan divergence detected"
  cat divergence-artifact.jsonld | jq '.envelope_diff'
  exit 1
elif [ $EXIT_CODE -eq 12 ]; then
  echo "✗ ENVELOPE_RESOURCE_SIG_MISMATCH - resource envelope divergence"
  exit 1
else
  echo "✗ Replay failed with exit code $EXIT_CODE"
  exit 1
fi
```

### Baseline Comparison

```cpp
// Load baseline replay results
ReplayRun baseline = ReplayRun::loadFromFile("baseline-replay.jsonld");
ReplayRun current = engine.replayAll();

// Compare each query's digest
for (size_t i = 0; i < baseline.results.size(); ++i) {
  const auto& baseline_result = baseline.results[i];
  const auto& current_result = current.results[i];

  // Compute envelope diff
  EnvelopeDiff diff = EnvelopeDiff::compute(
      baseline_result.execution_digest,
      current_result.execution_digest);

  if (!diff.is_identical) {
    // Regression detected
    auto error = DivergenceAbortHandler::createError(
        baseline_result.execution_digest,
        current_result.execution_digest,
        baseline_result.workload_record_id,
        current.run_id);

    std::cerr << "Regression at query " << i << ": "
              << divergenceErrorCodeToString(error.error_code) << std::endl;

    // Fail CI build
    return 1;
  }
}
```

## Testing Integration

### Unit Test Example

```cpp
#include <gtest/gtest.h>
#include "engine/readPlane/DivergenceAbortHandler.h"

TEST(DivergenceAbortHandlerTest, FailClosedSemantics) {
  // Create test digests
  ExecutionDigest expected = createTestDigest("expected");
  ExecutionDigest actual = createPerturbedDigest("perturbed");

  // Create structured error
  auto error = DivergenceAbortHandler::createError(
      expected, actual, 42, 1001);

  // Verify fail-closed semantics
  EXPECT_FALSE(error.partial_results_emitted);
  EXPECT_TRUE(error.abort_was_immediate);
  EXPECT_TRUE(error.isValidFailClosed());

  // Verify machine-readable error code
  EXPECT_NE(error.error_code, DivergenceErrorCode::SUCCESS);

  // Verify JSON-LD serialization
  auto json = error.toJsonLD();
  EXPECT_EQ(json["@type"], "StructuredDivergenceError");
  EXPECT_TRUE(json["error_code"].is_number());
}
```

### Integration Test Example

```cpp
TEST(WorkloadReplayIntegrationTest, DivergenceAbortTriggered) {
  // Setup: Create manifest with expected digests
  WorkloadManifest manifest = createTestManifest();

  // Inject a perturbation to trigger divergence
  ReplayConfiguration config;
  config.abort_on_divergence = true;

  WorkloadReplayEngine engine(&manifest, config);
  setupTestContext(engine);

  // Execute: Replay should abort on divergence
  EXPECT_THROW({
    try {
      ReplayRun run = engine.replayAll();
    } catch (const DivergenceAbortException& ex) {
      // Verify exception structure
      EXPECT_EQ(ex.getErrorCode(),
                DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH);
      EXPECT_TRUE(ex.isValidFailClosed());
      throw;  // Re-throw for EXPECT_THROW
    }
  }, DivergenceAbortException);
}
```

## Monitoring & Alerting

### Structured Log Output

```cpp
// Configure structured logging
void logDivergenceError(const StructuredDivergenceError& error) {
  nlohmann::ordered_json log_entry;
  log_entry["@timestamp"] = getCurrentISO8601Timestamp();
  log_entry["@level"] = "ERROR";
  log_entry["@type"] = "DivergenceAbort";
  log_entry["error"] = error.toJsonLD();

  // Write to structured log (JSON Lines format)
  std::ofstream log_file("divergence.jsonl", std::ios::app);
  log_file << log_entry.dump() << std::endl;
  log_file.close();
}
```

### Prometheus Metrics

```cpp
// Expose error code metrics
void recordDivergenceMetric(DivergenceErrorCode code) {
  std::string metric_name = "qlever_replay_divergence_total";
  std::string labels = absl::StrCat(
      "error_code=\"", static_cast<uint32_t>(code), "\",",
      "error_code_name=\"", divergenceErrorCodeToString(code), "\"");

  // Increment Prometheus counter
  prometheus_counter_inc(metric_name, labels);
}
```

## Best Practices

1. **Always Enable Fail-Closed in Production**
   ```cpp
   config.abort_on_divergence = true;  // REQUIRED for EPIC 10.1
   ```

2. **Verify Fail-Closed Semantics**
   ```cpp
   ASSERT_TRUE(error.isValidFailClosed());
   ```

3. **Use Machine-Readable Error Codes**
   ```cpp
   // Good: Structured error code
   DivergenceErrorCode code = error.error_code;

   // Bad: String parsing
   // std::string msg = "Divergence detected...";
   ```

4. **Write Structured Artifacts**
   ```cpp
   // Good: JSON-LD artifact
   std::ofstream file("divergence.jsonld");
   file << error.toJsonLD().dump(2);

   // Bad: Plain text log
   // std::cerr << error.getSummary();
   ```

5. **Test Deliberate Perturbations**
   ```cpp
   // Create perturbation test
   ExecutionDigest perturbed = original;
   perturbed.plan_hash = "different_hash";

   EXPECT_THROW(
       DivergenceAbortHandler::handleDivergence(original, perturbed, 1, 1),
       DivergenceAbortException);
   ```

## Troubleshooting

### Common Issues

#### Issue: Divergence not detected
**Solution**: Verify `abort_on_divergence = true` in configuration

#### Issue: Partial results emitted before abort
**Solution**: Check `error.partial_results_emitted == false`

#### Issue: Deferred abort
**Solution**: Verify `error.abort_was_immediate == true`

#### Issue: Prose error messages instead of codes
**Solution**: Use `error.error_code` (enum), not `error.getSummary()` (prose)

## Summary

This integration guide demonstrates:

- ✅ **Fail-Closed Semantics**: Immediate abort, no partial results
- ✅ **Structured Error Codes**: Machine-readable `DivergenceErrorCode` enum
- ✅ **Deterministic Exit Codes**: Consistent process termination
- ✅ **JSON-LD Artifacts**: Structured, parsable error reporting
- ✅ **Envelope Identity Verification**: Digest-based comparison
- ✅ **CI/CD Integration**: Automated regression detection

**Status**: EPIC 10.1 Agent 8 integration complete.
