// EPIC 10.3: Chaos Invariance - Entropy Injection Harness
// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Agent 7 - Chaos Invariance
//
// Bit-flip injection harness for chaos testing: prove that silent corruption
// is impossible under adversarial bit-flip conditions.
//
// INVARIANT: ∀ bit_flip ∈ NonCriticalBuffers:
//              (CorrectResult ∨ DivergenceAbort) ∧ ¬SilentCorruption
//
// PROTECTED ZONES (NO bit-flips allowed):
// - Instruction Pointer (IP)
// - Stack memory
// - Global Invariant State (DivergenceAbort infrastructure, hash functions)
//
// INJECTABLE ZONES (bit-flips allowed):
// - IdTable column buffers (user-space data)
// - ResultCache buffers (cached results)
//
// DETECTION MECHANISM:
// - Hash verification at critical boundaries
// - DivergenceAbort on hash mismatch
// - Zero tolerance for silent corruption

#ifndef QLEVER_TEST_CHAOS_ENTROPY_INJECTION_HARNESS_H
#define QLEVER_TEST_CHAOS_ENTROPY_INJECTION_HARNESS_H

#include <array>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include "engine/idTable/IdTable.h"
#include "engine/ingress/DivergenceAbort.h"
#include "engine/ingress/ResultDigest.h"
#include "global/Id.h"

namespace qlever::chaos {

// ============================================================================
// PROTECTED ZONES: Memory regions that MUST NOT be corrupted
// ============================================================================
enum class ProtectedZone : uint8_t {
  INSTRUCTION_POINTER = 1,  // Code segment (read-only)
  STACK = 2,                // Stack memory (volatile state)
  GLOBAL_INVARIANT = 3,     // Hash functions, abort handlers
  HEAP_CONTROL = 4,         // Malloc metadata, allocator state
};

// ============================================================================
// INJECTABLE ZONES: Memory regions safe for bit-flip injection
// ============================================================================
enum class InjectableZone : uint8_t {
  IDTABLE_COLUMN_BUFFER = 1,  // IdTable column data
  RESULT_CACHE_BUFFER = 2,    // Cached query results
};

// ============================================================================
// BitFlipTarget: Describes where and how to inject a bit-flip
// ============================================================================
struct BitFlipTarget {
  // Target memory address (validated to be in injectable zone)
  void* address = nullptr;

  // Byte offset from address
  size_t byte_offset = 0;

  // Bit position within byte (0-7)
  uint8_t bit_position = 0;

  // Zone classification (for validation)
  InjectableZone zone = InjectableZone::IDTABLE_COLUMN_BUFFER;

  // Flip the bit at the target location
  // Returns true if flip succeeded, false if target is invalid
  bool flip() {
    if (address == nullptr) return false;
    if (bit_position > 7) return false;

    // Compute target byte address
    uint8_t* target_byte =
        reinterpret_cast<uint8_t*>(address) + byte_offset;

    // Flip the bit
    *target_byte ^= (1 << bit_position);

    return true;
  }
};

// ============================================================================
// EntropyInjectionHarness: Chaos testing harness for bit-flip injection
// ============================================================================
class EntropyInjectionHarness {
 private:
  std::mt19937_64 rng_;  // Random number generator for chaos scenarios

 public:
  // Constructor with deterministic seed for reproducibility
  explicit EntropyInjectionHarness(uint64_t seed = 0xCAFEBABEDEADBEEF)
      : rng_(seed) {}

  // ========================================================================
  // PHASE 1: Target Selection
  // ========================================================================

  // Generate random bit-flip target within IdTable column buffer
  // Constraints:
  // - Target must be within allocated column memory
  // - Target must NOT overlap with control structures
  // - Returns nullopt if IdTable is empty or invalid
  std::optional<BitFlipTarget> selectRandomTarget(IdTable& table) {
    if (table.empty() || table.numColumns() == 0) {
      return std::nullopt;
    }

    // Select random column
    size_t col_idx = rng_() % table.numColumns();

    // Select random row
    size_t row_idx = rng_() % table.numRows();

    // Select random byte within Id (sizeof(Id) = 8 bytes for uint64_t)
    size_t byte_offset = rng_() % sizeof(Id);

    // Select random bit within byte (0-7)
    uint8_t bit_pos = rng_() % 8;

    // Get address of target Id
    Id* target_id_ptr = &table(row_idx, col_idx);

    BitFlipTarget target;
    target.address = reinterpret_cast<void*>(target_id_ptr);
    target.byte_offset = byte_offset;
    target.bit_position = bit_pos;
    target.zone = InjectableZone::IDTABLE_COLUMN_BUFFER;

    return target;
  }

  // Generate multiple random targets (for multi-bit-flip scenarios)
  std::vector<BitFlipTarget> selectMultipleTargets(IdTable& table,
                                                   size_t count) {
    std::vector<BitFlipTarget> targets;
    targets.reserve(count);

    for (size_t i = 0; i < count; ++i) {
      auto target = selectRandomTarget(table);
      if (target.has_value()) {
        targets.push_back(target.value());
      }
    }

    return targets;
  }

  // ========================================================================
  // PHASE 2: Hash Computation (Pre-Corruption)
  // ========================================================================

  // Compute deterministic hash of IdTable column
  // Uses simple XOR-fold of all Ids for speed (not cryptographic)
  // NOTE: For production, would use ResultDigest SHA256
  uint64_t hashIdTableColumn(const IdTable& table, size_t col_idx) const {
    if (col_idx >= table.numColumns()) {
      return 0;
    }

    uint64_t hash = 0xDEADBEEFCAFEBABE;  // Initial seed

    auto column = table.getColumn(col_idx);
    for (size_t row = 0; row < table.numRows(); ++row) {
      Id id = column[row];
      uint64_t id_bits = id.getBits();

      // XOR-fold with rotation (simple but deterministic)
      hash ^= id_bits;
      hash = (hash << 7) | (hash >> (64 - 7));  // Rotate left by 7
    }

    return hash;
  }

  // Compute hash of entire IdTable
  uint64_t hashIdTable(const IdTable& table) const {
    uint64_t combined_hash = 0x0123456789ABCDEF;

    for (size_t col = 0; col < table.numColumns(); ++col) {
      uint64_t col_hash = hashIdTableColumn(table, col);
      combined_hash ^= col_hash;
      combined_hash = (combined_hash << 11) | (combined_hash >> (64 - 11));
    }

    return combined_hash;
  }

  // ========================================================================
  // PHASE 3: Bit-Flip Injection
  // ========================================================================

  // Inject single bit-flip at target location
  // Returns true if injection succeeded
  bool injectBitFlip(BitFlipTarget& target) { return target.flip(); }

  // Inject multiple bit-flips (multi-bit corruption scenario)
  size_t injectMultipleBitFlips(std::vector<BitFlipTarget>& targets) {
    size_t success_count = 0;
    for (auto& target : targets) {
      if (target.flip()) {
        ++success_count;
      }
    }
    return success_count;
  }

  // ========================================================================
  // PHASE 4: Corruption Detection
  // ========================================================================

  // Verify that hash changed after bit-flip (proves corruption occurred)
  // Returns true if corruption detected (hash mismatch)
  bool detectCorruption(const IdTable& table, size_t col_idx,
                        uint64_t original_hash) const {
    uint64_t new_hash = hashIdTableColumn(table, col_idx);
    return new_hash != original_hash;
  }

  // Verify corruption at table level
  bool detectTableCorruption(const IdTable& table,
                             uint64_t original_hash) const {
    uint64_t new_hash = hashIdTable(table);
    return new_hash != original_hash;
  }

  // ========================================================================
  // PHASE 5: DivergenceAbort Verification
  // ========================================================================

  // Trigger DivergenceAbort if corruption detected
  // This is the fail-closed mechanism: detect → abort
  void abortOnCorruption(uint64_t expected_hash, uint64_t actual_hash,
                         const char* context) const {
    if (expected_hash != actual_hash) {
      DIVERGENCE_ABORT_HASH_MISMATCH(expected_hash, actual_hash, context);
    }
  }

  // ========================================================================
  // UTILITY: Chaos Scenario Generators
  // ========================================================================

  // Create minimal IdTable for testing (2 columns, 10 rows)
  static IdTable createMinimalIdTable() {
    IdTable table(2);  // 2 columns
    for (size_t i = 0; i < 10; ++i) {
      table.push_back({Id::makeFromInt(i), Id::makeFromInt(i * 2)});
    }
    return table;
  }

  // Create large IdTable for stress testing (10 columns, 1000 rows)
  static IdTable createLargeIdTable() {
    IdTable table(10);  // 10 columns
    for (size_t i = 0; i < 1000; ++i) {
      std::array<Id, 10> row;
      for (size_t col = 0; col < 10; ++col) {
        row[col] = Id::makeFromInt(i * 10 + col);
      }
      table.push_back(row);
    }
    return table;
  }

  // Create IdTable with known pattern for verification
  static IdTable createPatternIdTable(uint64_t pattern) {
    IdTable table(4);  // 4 columns
    for (size_t i = 0; i < 100; ++i) {
      table.push_back({Id::makeFromInt(pattern), Id::makeFromInt(pattern + 1),
                       Id::makeFromInt(pattern + 2),
                       Id::makeFromInt(pattern + 3)});
    }
    return table;
  }
};

// ============================================================================
// CHAOS TEST RESULT: Outcome of a single chaos scenario
// ============================================================================
enum class ChaosTestOutcome : uint8_t {
  CORRECT_RESULT = 1,      // Computation proceeded, result is correct
  DIVERGENCE_ABORT = 2,    // DivergenceAbort triggered (expected)
  SILENT_CORRUPTION = 3,   // CATASTROPHIC: Incorrect result without abort
  INJECTION_FAILED = 4,    // Bit-flip injection failed (invalid target)
  DETECTION_FAILED = 5,    // Corruption occurred but not detected
};

struct ChaosTestResult {
  ChaosTestOutcome outcome = ChaosTestOutcome::INJECTION_FAILED;
  uint64_t original_hash = 0;
  uint64_t corrupted_hash = 0;
  size_t bit_flips_injected = 0;
  bool corruption_detected = false;

  // Check if result is acceptable (correct or abort, NOT silent corruption)
  [[nodiscard]] bool isAcceptable() const {
    return outcome == ChaosTestOutcome::CORRECT_RESULT ||
           outcome == ChaosTestOutcome::DIVERGENCE_ABORT;
  }

  // Check if result is EPIC failure (silent corruption)
  [[nodiscard]] bool isSilentCorruption() const {
    return outcome == ChaosTestOutcome::SILENT_CORRUPTION;
  }
};

}  // namespace qlever::chaos

#endif  // QLEVER_TEST_CHAOS_ENTROPY_INJECTION_HARNESS_H
