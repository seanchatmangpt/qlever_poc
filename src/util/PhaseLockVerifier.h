// PhaseLockVerifier.h
// EPIC 10.2 Agent 10: Phase Lock Runtime Verification
// Provides C++ API for verifying build manifest integrity at runtime

#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>

namespace ad_utility {

struct BuildManifest {
  std::string engineVersion;
  std::string buildTimestamp;
  std::string hashAlgorithm;
  std::string binaryDigest;
  std::string logicDigest;
  std::unordered_map<std::string, std::string> dependencyReceipt;
  std::string goldenCorpusDigest;
};

struct PhaseLock {
  int version;
  std::string lockTimestamp;
  std::string manifestHash;
  std::string chainHash;
};

class PhaseLockVerifier {
 public:
  // Load manifest from JSON file
  static std::optional<BuildManifest> loadManifest(
      const std::filesystem::path& manifestPath);

  // Load phase lock from witness file
  static std::optional<PhaseLock> loadPhaseLock(
      const std::filesystem::path& lockPath);

  // Verify phase lock integrity
  // Returns true if verification passes, false otherwise
  static bool verify(const std::filesystem::path& lockPath,
                     const std::filesystem::path& manifestPath);

  // Verify and throw exception on failure
  static void verifyOrThrow(const std::filesystem::path& lockPath,
                            const std::filesystem::path& manifestPath);

  // Get build manifest info (for runtime introspection)
  static std::optional<BuildManifest> getBuildInfo(
      const std::filesystem::path& buildDir = ".");

 private:
  // Compute SHA256 hash of file
  static std::string computeFileSHA256(const std::filesystem::path& path);

  // Compute SHA256 hash of string
  static std::string computeStringSHA256(const std::string& data);

  // Parse manifest JSON (simplified parser for deterministic structure)
  static std::optional<BuildManifest> parseManifestJSON(
      const std::string& json);

  // Parse phase lock file
  static std::optional<PhaseLock> parsePhaseLock(const std::string& content);
};

}  // namespace ad_utility
