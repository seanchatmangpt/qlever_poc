// Copyright 2025, University of Freiburg,
//                 Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: WorkloadManifest - Ordered collection of WorkloadRecords (EPIC 4 - Subsystem 1)
// Thread-safe, append-only manifest with atomic visibility guarantees.

#ifndef QLEVER_SRC_ENGINE_READPLANE_WORKLOADMANIFEST_H
#define QLEVER_SRC_ENGINE_READPLANE_WORKLOADMANIFEST_H

#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <vector>

#include "engine/readPlane/WorkloadRecord.h"
#include "util/CryptographicHashUtils.h"
#include "util/Synchronized.h"

namespace readPlane {

// ============================================================================
// WorkloadManifest - Thread-safe ordered collection of WorkloadRecords
// ============================================================================
//
// CONSTRAINTS from EPIC4_SHARED_INVARIANTS.md:
// - Once created, manifest is immutable to readers (write-through reads see partial)
// - manifest_digest must be reproducible on another machine
// - All records must share single epoch (no cross-epoch workloads)
// - Protected by RWLock (writers exclusive)
// - Readers see consistent prefixes
//
class WorkloadManifest {
 public:
  static constexpr const char* FORMAT_VERSION = "v1.0";

  // Statistics - informational only, not for correctness
  struct Statistics {
    uint64_t total_records = 0;
    uint64_t deterministic_count = 0;
    uint64_t nondeterministic_excluded = 0;
    std::map<std::string, uint64_t> execution_class_counts;

    // Serialize to JSON
    [[nodiscard]] std::string toJson() const {
      std::ostringstream ss;
      ss << "{\n";
      ss << R"(  "deterministic_count": )" << deterministic_count << ",\n";
      ss << R"(  "execution_class_counts": {)" << "\n";
      bool first = true;
      for (const auto& [cls, count] : execution_class_counts) {
        if (!first) ss << ",\n";
        ss << "    \"" << cls << "\": " << count;
        first = false;
      }
      ss << "\n  },\n";
      ss << R"(  "nondeterministic_excluded": )" << nondeterministic_excluded << ",\n";
      ss << R"(  "total_records": )" << total_records << "\n";
      ss << "}";
      return ss.str();
    }
  };

 private:
  // Internal state protected by Synchronized wrapper
  struct State {
    // Unique ID for this workload manifest (UUID-like)
    std::string id;

    // Format version for compatibility
    std::string format_version = FORMAT_VERSION;

    // Hostname for audit (not for correctness)
    std::string captured_on_hostname;

    // Epoch binding - all records MUST have same epoch
    ad_utility::EpochId epoch_id = 0;
    std::string epoch_manifest_sha256;

    // Ordered records (append-only)
    std::vector<WorkloadRecord> records;

    // Excluded records log (non-deterministic queries)
    std::vector<WorkloadExcludedRecord> excluded_records;

    // Cached digest (recomputed after each append)
    std::string manifest_digest;

    // Statistics
    Statistics stats;
  };

  // Thread-safe state with RW lock
  mutable ad_utility::Synchronized<State> state_;

  // Atomic pointer for lock-free reads of the entire manifest
  // Allows readers to get consistent snapshot without blocking writers
  std::atomic<std::shared_ptr<std::vector<WorkloadRecord>>> atomic_records_{
      std::make_shared<std::vector<WorkloadRecord>>()};

 public:
  // ============================================================================
  // Construction
  // ============================================================================

  WorkloadManifest() {
    state_.withWriteLock([](State& s) {
      s.id = generateManifestId();
      s.format_version = FORMAT_VERSION;
    });
  }

  // Create manifest bound to specific epoch
  explicit WorkloadManifest(ad_utility::EpochId epochId,
                            std::string epochManifestSha256,
                            std::string hostname = "")
      : WorkloadManifest() {
    state_.withWriteLock([&](State& s) {
      s.epoch_id = epochId;
      s.epoch_manifest_sha256 = std::move(epochManifestSha256);
      s.captured_on_hostname = std::move(hostname);
    });
  }

  // ============================================================================
  // Atomic Append
  // ============================================================================

  // Append a WorkloadRecord atomically
  // CONSTRAINT: Record must have same epoch as manifest
  // Returns: true if appended, false if epoch mismatch
  bool append(WorkloadRecord record) {
    return state_.withWriteLock([&](State& s) -> bool {
      // Epoch validation - fail-closed on mismatch
      if (s.epoch_id != 0 && s.epoch_id != record.epoch_key.epoch_id) {
        // Log error but don't abort query
        return false;
      }
      if (!s.epoch_manifest_sha256.empty() &&
          s.epoch_manifest_sha256 != record.epoch_key.epoch_manifest_sha256) {
        return false;
      }

      // Bind epoch if not already set
      if (s.epoch_id == 0) {
        s.epoch_id = record.epoch_key.epoch_id;
        s.epoch_manifest_sha256 = record.epoch_key.epoch_manifest_sha256;
      }

      // Compute fingerprint if not set
      if (record.fingerprint_sha256.empty()) {
        record.fingerprint_sha256 = record.computeFingerprint();
      }

      // Append record
      s.records.push_back(std::move(record));

      // Update statistics
      s.stats.total_records = s.records.size();
      s.stats.deterministic_count = s.records.size();
      std::string ec_str =
          executionClassToString(s.records.back().execution_class);
      s.stats.execution_class_counts[ec_str]++;

      // Recompute digest
      s.manifest_digest = computeDigestInternal(s);

      // Update atomic pointer for lock-free reads
      auto newRecords =
          std::make_shared<std::vector<WorkloadRecord>>(s.records);
      atomic_records_.store(newRecords, std::memory_order_release);

      return true;
    });
  }

  // Append an excluded record (non-deterministic query)
  void appendExcluded(WorkloadExcludedRecord record) {
    state_.withWriteLock([&](State& s) {
      s.excluded_records.push_back(std::move(record));
      s.stats.nondeterministic_excluded = s.excluded_records.size();
    });
  }

  // ============================================================================
  // Lock-Free Reads
  // ============================================================================

  // Get snapshot of records (lock-free, consistent prefix guaranteed)
  [[nodiscard]] std::shared_ptr<std::vector<WorkloadRecord>> getRecordsSnapshot()
      const {
    return atomic_records_.load(std::memory_order_acquire);
  }

  // Get record count (lock-free)
  [[nodiscard]] size_t size() const {
    return atomic_records_.load(std::memory_order_acquire)->size();
  }

  // ============================================================================
  // Locked Reads (for full manifest access)
  // ============================================================================

  // Get manifest ID
  [[nodiscard]] std::string getId() const {
    return state_.withReadLock([](const State& s) { return s.id; });
  }

  // Get epoch ID
  [[nodiscard]] ad_utility::EpochId getEpochId() const {
    return state_.withReadLock([](const State& s) { return s.epoch_id; });
  }

  // Get epoch manifest hash
  [[nodiscard]] std::string getEpochManifestSha256() const {
    return state_.withReadLock(
        [](const State& s) { return s.epoch_manifest_sha256; });
  }

  // Get current digest (cached, updated after each append)
  [[nodiscard]] std::string getDigest() const {
    return state_.withReadLock(
        [](const State& s) { return s.manifest_digest; });
  }

  // Get statistics snapshot
  [[nodiscard]] Statistics getStatistics() const {
    return state_.withReadLock([](const State& s) { return s.stats; });
  }

  // Get excluded records snapshot
  [[nodiscard]] std::vector<WorkloadExcludedRecord> getExcludedRecords() const {
    return state_.withReadLock(
        [](const State& s) { return s.excluded_records; });
  }

  // ============================================================================
  // Digest Computation
  // ============================================================================

  // Compute SHA256 digest of manifest (deterministic)
  // Formula: SHA256(all records serialized || epoch_id || epoch_manifest_sha256)
  [[nodiscard]] std::string computeDigest() const {
    return state_.withReadLock(
        [](const State& s) { return computeDigestInternal(s); });
  }

  // ============================================================================
  // JSON-LD Serialization
  // ============================================================================

  // Serialize to deterministic JSON-LD format
  // Fields ordered alphabetically, portable across machines
  [[nodiscard]] std::string toJsonLD() const {
    return state_.withReadLock([](const State& s) {
      std::ostringstream ss;
      ss << "{\n";
      ss << R"(  "@context": "https://qlever.cs.uni-freiburg.de/workload/v1",)"
         << "\n";
      ss << R"(  "@type": "WorkloadManifest",)" << "\n";
      ss << R"(  "captured_on_hostname": ")" << s.captured_on_hostname << "\",\n";
      ss << R"(  "epoch_id": )" << s.epoch_id << ",\n";
      ss << R"(  "epoch_manifest_sha256": ")" << s.epoch_manifest_sha256 << "\",\n";
      ss << R"(  "format_version": ")" << s.format_version << "\",\n";
      ss << R"(  "id": ")" << s.id << "\",\n";
      ss << R"(  "manifest_digest": ")" << s.manifest_digest << "\",\n";

      // Records array
      ss << R"(  "records": [)" << "\n";
      for (size_t i = 0; i < s.records.size(); ++i) {
        // Indent each record's JSON-LD
        std::string recordJson = s.records[i].toJsonLD();
        // Add indentation to each line
        std::istringstream iss(recordJson);
        std::string line;
        bool firstLine = true;
        while (std::getline(iss, line)) {
          if (!firstLine) ss << "\n";
          ss << "    " << line;
          firstLine = false;
        }
        if (i < s.records.size() - 1) {
          ss << ",";
        }
        ss << "\n";
      }
      ss << "  ],\n";

      // Statistics
      ss << R"(  "statistics": )" << s.stats.toJson() << "\n";
      ss << "}";
      return ss.str();
    });
  }

  // ============================================================================
  // Validation
  // ============================================================================

  // Validate manifest integrity
  [[nodiscard]] bool isValid() const {
    return state_.withReadLock([](const State& s) {
      if (s.id.empty() || s.epoch_id == 0 || s.epoch_manifest_sha256.empty()) {
        return false;
      }

      // Verify all records have same epoch
      for (const auto& record : s.records) {
        if (record.epoch_key.epoch_id != s.epoch_id ||
            record.epoch_key.epoch_manifest_sha256 != s.epoch_manifest_sha256) {
          return false;
        }
      }

      // Verify digest
      std::string computed = computeDigestInternal(s);
      return computed == s.manifest_digest;
    });
  }

 private:
  // Generate unique manifest ID (UUID-like)
  static std::string generateManifestId() {
    // Simple timestamp + counter based ID (deterministic for testing)
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter.fetch_add(1, std::memory_order_relaxed);
    return absl::StrCat("wm-", id);
  }

  // Internal digest computation (requires lock held)
  static std::string computeDigestInternal(const State& s) {
    std::ostringstream ss;

    // Concatenate all record fingerprints
    for (const auto& record : s.records) {
      ss << record.fingerprint_sha256;
    }

    // Append epoch binding
    ss << s.epoch_id;
    ss << s.epoch_manifest_sha256;

    // Compute SHA256
    std::string data = ss.str();
    auto hash = ad_utility::HashSha256{}(data);
    return absl::StrJoin(hash, "", ad_utility::hexFormatter);
  }
};

// ============================================================================
// WorkloadManifestFactory - Creates epoch-bound manifests
// ============================================================================

class WorkloadManifestFactory {
 public:
  // Create new manifest for current epoch
  static std::unique_ptr<WorkloadManifest> createForCurrentEpoch() {
    // Get current epoch from global manager
    auto epochId = ad_utility::globalEpochManager.withReadLock(
        [](const auto& mgr) { return mgr.getEpochId(); });

    auto manifest =
        ad_utility::globalEpochManager.withReadLock([&](const auto& mgr) {
          auto opt = mgr.getCurrentEpochManifest();
          if (opt) {
            return std::make_unique<WorkloadManifest>(
                epochId, opt->getManifestHash());
          }
          // Fallback if no manifest available
          return std::make_unique<WorkloadManifest>(epochId, "");
        });

    return manifest;
  }
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_WORKLOADMANIFEST_H
