// EPIC 10.3 AGENT 9: THE INSTRUCTION MASK
// Scalar Vector Math Backend (100% Portable Baseline)
//
// Copyright 2026, QLever Architecture Neutrality Team
//
// CRITICAL: This file compiles WITHOUT hardware-specific flags.
// Provides 100% portable fallback for all vector operations.
// All SIMD backends must produce bit-identical results to these implementations.

#include "util/qleverest_vmath_abstraction.hpp"

#include <algorithm>
#include <numeric>

namespace qlever::vmath {

// =============================================================================
// SCALAR IMPLEMENTATIONS (Architecture-Neutral Baseline)
// =============================================================================

void fillRepeated(std::span<int64_t> dest, int64_t value,
                  Backend backend) noexcept {
  if (backend != Backend::Scalar) {
    // TODO: Dispatch to SIMD backends when implemented
    // For now, fallback to scalar
  }

  // Scalar baseline: standard library fill
  std::fill(dest.begin(), dest.end(), value);
}

void vectorCopy(std::span<const int64_t> src, std::span<int64_t> dest,
                Backend backend) noexcept {
  if (backend != Backend::Scalar) {
    // TODO: Dispatch to SIMD backends when implemented
    // For now, fallback to scalar
  }

  // Scalar baseline: standard library copy
  // Truncate if destination is smaller
  size_t copyCount = std::min(src.size(), dest.size());
  std::copy_n(src.begin(), copyCount, dest.begin());
}

int64_t vectorSum(std::span<const int64_t> src, Backend backend) noexcept {
  if (backend != Backend::Scalar) {
    // TODO: Dispatch to SIMD backends when implemented
    // For now, fallback to scalar
  }

  // Scalar baseline: standard library accumulate
  // DETERMINISM: Left-to-right evaluation order (guaranteed by accumulate)
  return std::accumulate(src.begin(), src.end(), int64_t(0));
}

int64_t vectorMin(std::span<const int64_t> src, Backend backend) noexcept {
  if (backend != Backend::Scalar) {
    // TODO: Dispatch to SIMD backends when implemented
    // For now, fallback to scalar
  }

  if (src.empty()) {
    return INT64_MAX;  // Sentinel for empty input
  }

  // Scalar baseline: standard library min_element
  return *std::min_element(src.begin(), src.end());
}

int64_t vectorMax(std::span<const int64_t> src, Backend backend) noexcept {
  if (backend != Backend::Scalar) {
    // TODO: Dispatch to SIMD backends when implemented
    // For now, fallback to scalar
  }

  if (src.empty()) {
    return INT64_MIN;  // Sentinel for empty input
  }

  // Scalar baseline: standard library max_element
  return *std::max_element(src.begin(), src.end());
}

void vectorCompare(std::span<const int64_t> src, int64_t compareValue,
                   CompareOp op, std::span<bool> result,
                   Backend backend) noexcept {
  if (backend != Backend::Scalar) {
    // TODO: Dispatch to SIMD backends when implemented
    // For now, fallback to scalar
  }

  // Scalar baseline: element-wise comparison
  size_t count = std::min(src.size(), result.size());

  for (size_t i = 0; i < count; ++i) {
    switch (op) {
      case CompareOp::LessThan:
        result[i] = src[i] < compareValue;
        break;
      case CompareOp::LessEqual:
        result[i] = src[i] <= compareValue;
        break;
      case CompareOp::Equal:
        result[i] = src[i] == compareValue;
        break;
      case CompareOp::NotEqual:
        result[i] = src[i] != compareValue;
        break;
      case CompareOp::GreaterEqual:
        result[i] = src[i] >= compareValue;
        break;
      case CompareOp::GreaterThan:
        result[i] = src[i] > compareValue;
        break;
    }
  }
}

}  // namespace qlever::vmath
