// PhaseLockVerifierTest.cpp
// EPIC 10.2 Agent 10: Phase Lock Verification Tests

#include "util/PhaseLockVerifier.h"

#include <gtest/gtest.h>

#include <fstream>
#include <filesystem>

namespace ad_utility {

class PhaseLockVerifierTest : public ::testing::Test {
 protected:
  std::filesystem::path testDir;
  std::filesystem::path manifestPath;
  std::filesystem::path lockPath;

  void SetUp() override {
    testDir = std::filesystem::temp_directory_path() / "phase_lock_test";
    std::filesystem::create_directories(testDir);
    manifestPath = testDir / "manifest.json";
    lockPath = testDir / ".phase.lock";
  }

  void TearDown() override {
    std::filesystem::remove_all(testDir);
  }

  void createValidManifest() {
    std::ofstream manifest(manifestPath);
    manifest << R"({
  "engine_version": "1.0.0",
  "build_timestamp": "2026-01-02T12:00:00Z",
  "hash_algorithm": "SHA256",
  "binary_digest": "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234",
  "logic_digest": "ef123456789abcdef123456789abcdef123456789abcdef123456789abcdef12",
  "dependency_receipt": {
    "libicu_uc": "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef",
    "simdjson": "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210"
  },
  "golden_corpus_digest": "9876543210abcdef9876543210abcdef9876543210abcdef9876543210abcdef"
})";
    manifest.close();
  }

  void createValidPhaseLock(const std::string& manifestHash,
                            const std::string& chainHash) {
    std::ofstream lock(lockPath);
    lock << "PHASE_LOCK_VERSION=1\n";
    lock << "LOCK_TIMESTAMP=2026-01-02T12:00:00Z\n";
    lock << "MANIFEST_HASH=" << manifestHash << "\n";
    lock << "CHAIN_HASH=" << chainHash << "\n";
    lock << "# This file is an immutable witness of deterministic build\n";
    lock << "# Modification indicates tampering\n";
    lock.close();
  }
};

TEST_F(PhaseLockVerifierTest, LoadManifestSuccess) {
  createValidManifest();

  auto manifest = PhaseLockVerifier::loadManifest(manifestPath);
  ASSERT_TRUE(manifest.has_value());
  EXPECT_EQ(manifest->engineVersion, "1.0.0");
  EXPECT_EQ(manifest->buildTimestamp, "2026-01-02T12:00:00Z");
  EXPECT_EQ(manifest->hashAlgorithm, "SHA256");
  EXPECT_EQ(manifest->binaryDigest,
            "abcd1234567890abcd1234567890abcd1234567890abcd1234567890abcd1234");
  EXPECT_EQ(manifest->dependencyReceipt.size(), 2);
  EXPECT_TRUE(manifest->dependencyReceipt.contains("libicu_uc"));
  EXPECT_TRUE(manifest->dependencyReceipt.contains("simdjson"));
}

TEST_F(PhaseLockVerifierTest, LoadManifestNotFound) {
  auto manifest = PhaseLockVerifier::loadManifest(manifestPath);
  EXPECT_FALSE(manifest.has_value());
}

TEST_F(PhaseLockVerifierTest, LoadPhaseLockSuccess) {
  createValidPhaseLock(
      "abc123", "def456");

  auto lock = PhaseLockVerifier::loadPhaseLock(lockPath);
  ASSERT_TRUE(lock.has_value());
  EXPECT_EQ(lock->version, 1);
  EXPECT_EQ(lock->lockTimestamp, "2026-01-02T12:00:00Z");
  EXPECT_EQ(lock->manifestHash, "abc123");
  EXPECT_EQ(lock->chainHash, "def456");
}

TEST_F(PhaseLockVerifierTest, VerifyIntegritySuccess) {
  createValidManifest();

  // Compute actual manifest hash
  std::string manifestHash = PhaseLockVerifier::computeFileSHA256(manifestPath);

  // Compute chain hash
  std::string lockTimestamp = "2026-01-02T12:00:00Z";
  std::string chainHash = PhaseLockVerifier::computeStringSHA256(
      manifestHash + lockTimestamp);

  createValidPhaseLock(manifestHash, chainHash);

  // Verify should pass
  EXPECT_TRUE(PhaseLockVerifier::verify(lockPath, manifestPath));
}

TEST_F(PhaseLockVerifierTest, DetectTamperedManifest) {
  createValidManifest();

  // Compute original manifest hash
  std::string originalHash = PhaseLockVerifier::computeFileSHA256(manifestPath);

  // Create valid phase lock
  std::string lockTimestamp = "2026-01-02T12:00:00Z";
  std::string chainHash = PhaseLockVerifier::computeStringSHA256(
      originalHash + lockTimestamp);
  createValidPhaseLock(originalHash, chainHash);

  // Tamper with manifest
  std::ofstream manifest(manifestPath, std::ios::app);
  manifest << "\n// TAMPERED\n";
  manifest.close();

  // Verify should FAIL
  EXPECT_FALSE(PhaseLockVerifier::verify(lockPath, manifestPath));
}

TEST_F(PhaseLockVerifierTest, DetectTamperedPhaseLock) {
  createValidManifest();

  std::string manifestHash = PhaseLockVerifier::computeFileSHA256(manifestPath);
  std::string lockTimestamp = "2026-01-02T12:00:00Z";
  std::string chainHash = PhaseLockVerifier::computeStringSHA256(
      manifestHash + lockTimestamp);

  createValidPhaseLock(manifestHash, chainHash);

  // Tamper with phase lock (change chain hash)
  std::ofstream lock(lockPath, std::ios::trunc);
  lock << "PHASE_LOCK_VERSION=1\n";
  lock << "LOCK_TIMESTAMP=" << lockTimestamp << "\n";
  lock << "MANIFEST_HASH=" << manifestHash << "\n";
  lock << "CHAIN_HASH=TAMPERED_HASH_123456\n";
  lock.close();

  // Verify should FAIL
  EXPECT_FALSE(PhaseLockVerifier::verify(lockPath, manifestPath));
}

TEST_F(PhaseLockVerifierTest, VerifyOrThrowSuccess) {
  createValidManifest();

  std::string manifestHash = PhaseLockVerifier::computeFileSHA256(manifestPath);
  std::string lockTimestamp = "2026-01-02T12:00:00Z";
  std::string chainHash = PhaseLockVerifier::computeStringSHA256(
      manifestHash + lockTimestamp);

  createValidPhaseLock(manifestHash, chainHash);

  // Should not throw
  EXPECT_NO_THROW(PhaseLockVerifier::verifyOrThrow(lockPath, manifestPath));
}

TEST_F(PhaseLockVerifierTest, VerifyOrThrowFailure) {
  createValidManifest();

  // Create invalid phase lock
  createValidPhaseLock("wrong_hash", "wrong_chain");

  // Should throw
  EXPECT_THROW(PhaseLockVerifier::verifyOrThrow(lockPath, manifestPath),
               std::runtime_error);
}

}  // namespace ad_utility
