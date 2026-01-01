# EpochManifest - Usage Guide and Integration Examples

## Overview

`EpochManifest` provides a complete snapshot of the epoch state at the point when data becomes immutable (sealed). This snapshot enables:

1. **Deterministic Cache Keys** - Reproducible, content-addressed caching
2. **ETag Generation** - HTTP cache validation using manifest hash
3. **Reproducibility Validation** - Verify identical computation with identical input

## Core Concepts

### What Gets Hashed?

The manifest captures SHA-256 hashes of:

- **Asserted Triples**: The explicit RDF data loaded into the epoch
- **Derived Triples**: Computed triples from ruleset inference
- **Ruleset**: The Notation3 rules (N3) applied during this epoch
- **Shapes**: SHACL constraint definitions
- **Config**: Configuration parameters affecting query execution

### Deterministic Manifest Hash

All fields are concatenated in a deterministic order:
```
epochId=<id>,assertedTriplesHash=<h1>,...,sealTimestampMs=<ts>
```

Then SHA-256 is applied to this entire string, producing a **64-character hex string**.

This manifest hash serves as:
- **Cache Key** for query results
- **ETag** for HTTP responses
- **Reproducibility Token** for validation

## Usage Examples

### Example 1: Building a Manifest (Builder Pattern)

```cpp
#include "ad_utility/EpochManifest.h"

using namespace ad_utility;

// Create manifest when epoch transitions to SERVE
EpochManifest manifest = EpochManifestBuilder(currentEpochId)
    .withAssertedTriples("abc123def456...")        // SHA-256 of triples bundle
    .withDerivedTriples("def789ghi012...")         // SHA-256 of derived triples
    .withRuleset("sha256hash_of_ruleset_n3")      // Hash of N3 rules
    .withShapes("sha256hash_of_shapes_ttl")       // Hash of SHACL shapes
    .withConfig("sha256hash_of_config_yaml")      // Hash of configuration
    .withBuildToolVersions("ANTLR=4.13.12,CMake=3.27.0,Boost=1.83.0")
    .build();
```

### Example 2: Generating Cache Keys

```cpp
#include "ad_utility/EpochManifest.h"

// In query cache implementation
EpochManifest manifest = /* ... */;

// Generate cache key from manifest hash
std::string cacheKey = absl::StrCat(
    "query_cache:",
    manifest.getManifestHash(),  // 64-char SHA-256 hex string
    ":",
    queryHashFingerprint         // Hash of normalized SPARQL query
);

// Store in cache
queryCache[cacheKey] = queryResult;
```

### Example 3: Using Manifest Hash as ETag

```cpp
#include "ad_utility/EpochManifest.h"

// In HTTP response handler
EpochManifest manifest = /* ... */;

// Set ETag header
response.setHeader("ETag", absl::StrCat("\"", manifest.getManifestHash(), "\""));

// Client can validate with If-None-Match header
if (request.getHeader("If-None-Match") == absl::StrCat("\"", manifest.getManifestHash(), "\"")) {
    return HTTP_304_NOT_MODIFIED;
}
```

### Example 4: Reproducibility Validation

```cpp
#include "ad_utility/EpochManifest.h"

EpochManifest manifest1 = /* ... original manifest ... */;
EpochManifest manifest2 = /* ... recomputed manifest ... */;

// Verify the computation was reproducible
if (!manifest1.matches(manifest2)) {
    LOG(WARNING) << "Reproducibility check failed!";
    LOG(WARNING) << "Expected: " << manifest1.toString();
    LOG(WARNING) << "Got:      " << manifest2.toString();
    throw std::runtime_error("Epoch state not reproducible");
}

// Or use the manifest hash for quick comparison
if (manifest1.getManifestHash() != manifest2.getManifestHash()) {
    // Manifests differ
}
```

### Example 5: Validation Before Use

```cpp
#include "ad_utility/EpochManifest.h"

EpochManifest manifest = /* ... loaded from storage ... */;

// Validate before using
if (!manifest.isValid()) {
    throw std::runtime_error("Invalid manifest: missing required fields");
}

// Safe to use now
std::string cacheKeyPrefix = manifest.getManifestHash();
```

### Example 6: Logging and Debugging

```cpp
#include "ad_utility/EpochManifest.h"

EpochManifest manifest = /* ... */;

// Pretty-print manifest for logging
LOG(INFO) << "Epoch " << manifest.epochId_
          << " sealed with manifest:\n"
          << manifest.toString();

// Output example:
// {
//   "epochId": 42,
//   "assertedTriplesHash": "a1b2c3d4e5f6...",
//   "derivedTriplesHash": "b2c3d4e5f6a7...",
//   "rulesetHash": "c3d4e5f6a7b8...",
//   "shapesHash": "d4e5f6a7b8c9...",
//   "configHash": "e5f6a7b8c9da...",
//   "buildToolVersions": "ANTLR=4.13.12,CMake=3.27.0",
//   "sealTimestampMs": 1704067200000,
//   "manifestHash": "f6a7b8c9dae0..."
// }
```

## Integration Patterns

### Pattern 1: Query Result Caching

```cpp
class QueryResultCache {
 private:
  std::unordered_map<std::string, CachedResult> cache_;
  ad_utility::EpochManifest currentManifest_;

 public:
  std::optional<IdTable> get(const std::string& normalizedQuery) {
    // Build cache key from manifest hash + query hash
    std::string cacheKey = absl::StrCat(
        currentManifest_.getManifestHash(),
        ":",
        hashQuery(normalizedQuery)
    );

    auto it = cache_.find(cacheKey);
    if (it != cache_.end()) {
      return it->second.result;
    }
    return std::nullopt;
  }

  void put(const std::string& normalizedQuery, const IdTable& result) {
    std::string cacheKey = absl::StrCat(
        currentManifest_.getManifestHash(),
        ":",
        hashQuery(normalizedQuery)
    );

    cache_[cacheKey] = {result, std::chrono::system_clock::now()};
  }
};
```

### Pattern 2: HTTP ETag Support

```cpp
class SparqlEndpoint {
 private:
  ad_utility::EpochManifest manifest_;

 public:
  HttpResponse executeQuery(const HttpRequest& request) {
    // Check If-None-Match header
    std::string clientETag = request.getHeader("If-None-Match");
    std::string serverETag = absl::StrCat("\"", manifest_.getManifestHash(), "\"");

    if (clientETag == serverETag) {
      return HttpResponse::NotModified();
    }

    // Execute query normally
    auto result = executeQueryLogic(request);

    // Add ETag to response
    result.setHeader("ETag", serverETag);
    result.setHeader("Cache-Control", "public, max-age=3600");

    return result;
  }
};
```

### Pattern 3: Materialized View Keying

```cpp
class MaterializedViewCache {
 private:
  std::unordered_map<std::string, MaterializedView> views_;

 public:
  std::optional<MaterializedView> getMaterializedView(
      const std::string& viewName,
      const ad_utility::EpochManifest& manifest) {

    // Key includes manifest hash to ensure view is from this epoch
    std::string cacheKey = absl::StrCat(
        viewName,
        ":",
        manifest.getManifestHash()
    );

    auto it = views_.find(cacheKey);
    if (it != views_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  void putMaterializedView(
      const std::string& viewName,
      const ad_utility::EpochManifest& manifest,
      const MaterializedView& view) {

    std::string cacheKey = absl::StrCat(
        viewName,
        ":",
        manifest.getManifestHash()
    );

    views_[cacheKey] = view;
  }
};
```

### Pattern 4: Snapshot Validation

```cpp
class EpochSnapshot {
 private:
  ad_utility::EpochManifest manifest_;
  std::vector<unsigned char> computedState_;

 public:
  bool verifySnapshot() {
    // Recompute manifest from current state
    ad_utility::EpochManifest recomputed = recomputeManifest();

    // Verify reproducibility
    if (!manifest_.matches(recomputed)) {
      LOG(ERROR) << "Snapshot verification failed!";
      LOG(ERROR) << "Original:   " << manifest_.toString();
      LOG(ERROR) << "Recomputed: " << recomputed.toString();
      return false;
    }

    return true;
  }

  std::string getSnapshotId() const {
    return manifest_.getManifestHash();
  }
};
```

## Design Rationale

### Why Hash Everything?

1. **Deterministic** - Same input always produces same hash
2. **Compact** - 64-char string vs. multi-gigabyte data
3. **Tamper-Proof** - Any data change produces different hash
4. **Fast Comparison** - O(1) comparison of manifest hashes

### Why Multiple Hashes?

Separating hashes for different components enables:

- **Partial Caching** - Reuse results if only ruleset changed
- **Debugging** - Identify which component changed
- **Incremental Updates** - Update only changed components
- **Dependency Tracking** - Understand what affects what

### Why Timestamp?

The `sealTimestampMs_` field:

- Enables time-based queries on cache data
- Tracks when snapshots were created
- Helps with archive/purge decisions
- Provides reproducibility metadata

## Performance Characteristics

### Hash Computation
- **Asserted/Derived Triples**: O(data size)
- **Ruleset/Shapes/Config**: O(file size)
- **Manifest Hash**: O(1) after concatenation (small string)

### Cache Key Usage
- **Lookup**: O(1) hash table access
- **Collision**: SHA-256 has 2^256 possible outputs
- **False Positive**: Astronomically unlikely (< 10^-77)

### ETag Comparison
- **Header Format**: `"<64-char-hash>"`
- **Comparison**: O(64) string comparison
- **Bandwidth**: 64 bytes vs. kilobytes for full response

## Thread Safety Considerations

The `EpochManifest` struct itself is thread-safe because:
- All fields are immutable after sealing
- No mutable state in the struct
- `getManifestHash()` is const and deterministic

When used in caches:
- Use `Synchronized<EpochManifest>` for shared access
- Or assume epoch doesn't change after SERVE state
- Document synchronization boundaries

## Testing the Manifest

```cpp
#include <gtest/gtest.h>
#include "ad_utility/EpochManifest.h"

class EpochManifestTest : public ::testing::Test {};

TEST_F(EpochManifestTest, BuilderFluentInterface) {
  ad_utility::EpochManifest manifest =
      ad_utility::EpochManifestBuilder(42)
      .withAssertedTriples("hash1")
      .withDerivedTriples("hash2")
      .withRuleset("hash3")
      .withShapes("hash4")
      .withConfig("hash5")
      .withBuildToolVersions("v1.0")
      .build();

  EXPECT_EQ(manifest.epochId_, 42);
  EXPECT_TRUE(manifest.isValid());
}

TEST_F(EpochManifestTest, ManifestHashDeterministic) {
  auto manifest1 = ad_utility::EpochManifestBuilder(1)
      .withAssertedTriples("abc")
      .withDerivedTriples("def")
      .withRuleset("ghi")
      .withShapes("jkl")
      .withConfig("mno")
      .withBuildToolVersions("v1.0")
      .withSealTimestamp(1000)
      .build();

  auto manifest2 = ad_utility::EpochManifestBuilder(1)
      .withAssertedTriples("abc")
      .withDerivedTriples("def")
      .withRuleset("ghi")
      .withShapes("jkl")
      .withConfig("mno")
      .withBuildToolVersions("v1.0")
      .withSealTimestamp(1000)
      .build();

  EXPECT_EQ(manifest1.getManifestHash(), manifest2.getManifestHash());
}

TEST_F(EpochManifestTest, MatchesDetectsDifferences) {
  auto manifest1 = ad_utility::EpochManifestBuilder(1)
      .withAssertedTriples("abc")
      .withDerivedTriples("def")
      .withRuleset("ghi")
      .withShapes("jkl")
      .withConfig("mno")
      .withBuildToolVersions("v1.0")
      .build();

  auto manifest2 = ad_utility::EpochManifestBuilder(1)
      .withAssertedTriples("ABC")  // Different hash
      .withDerivedTriples("def")
      .withRuleset("ghi")
      .withShapes("jkl")
      .withConfig("mno")
      .withBuildToolVersions("v1.0")
      .build();

  EXPECT_FALSE(manifest1.matches(manifest2));
}
```

## Summary

The `EpochManifest` provides a robust foundation for:

1. **Reproducibility** - Verify identical computation with identical input
2. **Caching** - Deterministic, content-addressed cache keys
3. **HTTP** - ETag support for efficient caching
4. **Debugging** - Detailed manifest information for troubleshooting
5. **Validation** - Ensure epoch state integrity

By using manifest hashes as cache keys and ETags, QLever can provide efficient, reproducible query execution across epochs.
