// PhaseLockVerifier.cpp
// EPIC 10.2 Agent 10: Phase Lock Runtime Verification Implementation

#include "util/PhaseLockVerifier.h"

#include <openssl/sha.h>

#include <algorithm>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "util/Exception.h"
#include "util/Log.h"

namespace ad_utility {

// Compute SHA256 hash of file
std::string PhaseLockVerifier::computeFileSHA256(
    const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot open file for hashing: " +
                             path.string());
  }

  SHA256_CTX sha256;
  SHA256_Init(&sha256);

  constexpr size_t BUFFER_SIZE = 8192;
  char buffer[BUFFER_SIZE];

  while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
    SHA256_Update(&sha256, buffer, file.gcount());
  }

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_Final(hash, &sha256);

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char byte : hash) {
    oss << std::setw(2) << static_cast<int>(byte);
  }

  return oss.str();
}

// Compute SHA256 hash of string
std::string PhaseLockVerifier::computeStringSHA256(const std::string& data) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         hash);

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char byte : hash) {
    oss << std::setw(2) << static_cast<int>(byte);
  }

  return oss.str();
}

// Parse manifest JSON (simplified regex-based parser for deterministic
// structure)
std::optional<BuildManifest> PhaseLockVerifier::parseManifestJSON(
    const std::string& json) {
  BuildManifest manifest;

  // Extract engine_version
  std::regex engineVersionRegex(R"("engine_version"\s*:\s*"([^"]+)")");
  std::smatch match;
  if (std::regex_search(json, match, engineVersionRegex)) {
    manifest.engineVersion = match[1];
  } else {
    return std::nullopt;
  }

  // Extract build_timestamp
  std::regex buildTimestampRegex(R"("build_timestamp"\s*:\s*"([^"]+)")");
  if (std::regex_search(json, match, buildTimestampRegex)) {
    manifest.buildTimestamp = match[1];
  }

  // Extract hash_algorithm
  std::regex hashAlgoRegex(R"("hash_algorithm"\s*:\s*"([^"]+)")");
  if (std::regex_search(json, match, hashAlgoRegex)) {
    manifest.hashAlgorithm = match[1];
  }

  // Extract binary_digest
  std::regex binaryDigestRegex(R"("binary_digest"\s*:\s*"([^"]+)")");
  if (std::regex_search(json, match, binaryDigestRegex)) {
    manifest.binaryDigest = match[1];
  }

  // Extract logic_digest
  std::regex logicDigestRegex(R"("logic_digest"\s*:\s*"([^"]+)")");
  if (std::regex_search(json, match, logicDigestRegex)) {
    manifest.logicDigest = match[1];
  }

  // Extract golden_corpus_digest
  std::regex goldenDigestRegex(R"("golden_corpus_digest"\s*:\s*"([^"]+)")");
  if (std::regex_search(json, match, goldenDigestRegex)) {
    manifest.goldenCorpusDigest = match[1];
  }

  // Extract dependency_receipt (simplified - extract key-value pairs)
  std::regex depReceiptRegex(
      R"("dependency_receipt"\s*:\s*\{([^}]+)\})");
  if (std::regex_search(json, match, depReceiptRegex)) {
    std::string depContent = match[1];
    std::regex depPairRegex(R"("([^"]+)"\s*:\s*"([^"]+)")");
    auto begin = std::sregex_iterator(depContent.begin(), depContent.end(),
                                      depPairRegex);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
      manifest.dependencyReceipt[(*it)[1]] = (*it)[2];
    }
  }

  return manifest;
}

// Parse phase lock file
std::optional<PhaseLock> PhaseLockVerifier::parsePhaseLock(
    const std::string& content) {
  PhaseLock lock;
  lock.version = 1;  // Default

  std::istringstream iss(content);
  std::string line;

  while (std::getline(iss, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '#') {
      continue;
    }

    size_t pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    if (key == "PHASE_LOCK_VERSION") {
      lock.version = std::stoi(value);
    } else if (key == "LOCK_TIMESTAMP") {
      lock.lockTimestamp = value;
    } else if (key == "MANIFEST_HASH") {
      lock.manifestHash = value;
    } else if (key == "CHAIN_HASH") {
      lock.chainHash = value;
    }
  }

  if (lock.manifestHash.empty() || lock.chainHash.empty()) {
    return std::nullopt;
  }

  return lock;
}

// Load manifest from JSON file
std::optional<BuildManifest> PhaseLockVerifier::loadManifest(
    const std::filesystem::path& manifestPath) {
  if (!std::filesystem::exists(manifestPath)) {
    LOG(ERROR) << "Manifest not found: " << manifestPath << std::endl;
    return std::nullopt;
  }

  std::ifstream file(manifestPath);
  if (!file) {
    LOG(ERROR) << "Cannot open manifest: " << manifestPath << std::endl;
    return std::nullopt;
  }

  std::string json((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());

  return parseManifestJSON(json);
}

// Load phase lock from witness file
std::optional<PhaseLock> PhaseLockVerifier::loadPhaseLock(
    const std::filesystem::path& lockPath) {
  if (!std::filesystem::exists(lockPath)) {
    LOG(ERROR) << "Phase lock not found: " << lockPath << std::endl;
    return std::nullopt;
  }

  std::ifstream file(lockPath);
  if (!file) {
    LOG(ERROR) << "Cannot open phase lock: " << lockPath << std::endl;
    return std::nullopt;
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  return parsePhaseLock(content);
}

// Verify phase lock integrity
bool PhaseLockVerifier::verify(const std::filesystem::path& lockPath,
                                const std::filesystem::path& manifestPath) {
  // Load phase lock
  auto lockOpt = loadPhaseLock(lockPath);
  if (!lockOpt) {
    LOG(ERROR) << "Failed to load phase lock" << std::endl;
    return false;
  }
  const auto& lock = *lockOpt;

  // Load manifest
  auto manifestOpt = loadManifest(manifestPath);
  if (!manifestOpt) {
    LOG(ERROR) << "Failed to load manifest" << std::endl;
    return false;
  }

  // Recompute manifest hash
  std::string currentManifestHash = computeFileSHA256(manifestPath);

  // Verify manifest hash matches
  if (currentManifestHash != lock.manifestHash) {
    LOG(ERROR) << "INTEGRITY FAILURE: manifest.json has been modified!"
               << std::endl;
    LOG(ERROR) << "  Expected: " << lock.manifestHash << std::endl;
    LOG(ERROR) << "  Got:      " << currentManifestHash << std::endl;
    return false;
  }

  // Verify chain hash matches
  std::string recomputedChain =
      computeStringSHA256(lock.manifestHash + lock.lockTimestamp);

  if (recomputedChain != lock.chainHash) {
    LOG(ERROR) << "INTEGRITY FAILURE: .phase.lock hash chain broken!"
               << std::endl;
    LOG(ERROR) << "  Expected: " << lock.chainHash << std::endl;
    LOG(ERROR) << "  Got:      " << recomputedChain << std::endl;
    return false;
  }

  LOG(INFO) << "Phase Lock: Verification PASSED" << std::endl;
  LOG(INFO) << "  Engine Version: " << manifestOpt->engineVersion << std::endl;
  LOG(INFO) << "  Build Timestamp: " << manifestOpt->buildTimestamp
            << std::endl;
  LOG(INFO) << "  Binary Digest: " << manifestOpt->binaryDigest << std::endl;

  return true;
}

// Verify and throw exception on failure
void PhaseLockVerifier::verifyOrThrow(
    const std::filesystem::path& lockPath,
    const std::filesystem::path& manifestPath) {
  if (!verify(lockPath, manifestPath)) {
    throw std::runtime_error("Phase lock verification failed");
  }
}

// Get build manifest info (for runtime introspection)
std::optional<BuildManifest> PhaseLockVerifier::getBuildInfo(
    const std::filesystem::path& buildDir) {
  auto manifestPath = buildDir / "manifest.json";
  return loadManifest(manifestPath);
}

}  // namespace ad_utility
