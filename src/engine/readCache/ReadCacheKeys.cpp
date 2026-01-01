// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Implementation file for read cache key types

#include "engine/readCache/ReadCacheKeys.h"

namespace readCache {

// All hash and equality implementations are header-only via AbslHashValue
// template specialization and QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL macro.
// This compilation unit exists for:
// 1. Verifying that the header compiles correctly
// 2. Future extensibility (e.g., debugging utilities, serialization)

}  // namespace readCache
