// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#include "engine/queryCanonical/CanonicalSerializer.h"

#include <sstream>

namespace queryCanonical {

// ____________________________________________________________________________
std::string CanonicalSerializer::serialize(const ParsedQuery& query) const {
  std::ostringstream oss;
  
  // Serialize query header (SELECT/CONSTRUCT/ASK)
  if (query.hasSelectClause()) {
    oss << "SELECT ";
    const auto& selectClause = query.selectClause();
    if (selectClause.isAsterisk()) {
      oss << "* ";
    }
  } else if (query.hasConstructClause()) {
    oss << "CONSTRUCT ";
  } else if (query.hasAskClause()) {
    oss << "ASK ";
  }
  
  // Serialize graph pattern
  oss << "WHERE { ";
  oss << serializeGraphPattern(query._rootGraphPattern);
  oss << " }";
  
  // Serialize ORDER BY
  if (!query._orderBy.empty()) {
    oss << " ORDER BY";
    for (const auto& orderKey : query._orderBy) {
      oss << " " << orderKey.variable.name();
    }
  }
  
  // Serialize LIMIT/OFFSET
  if (query._limitOffset._limit.has_value()) {
    oss << " LIMIT " << query._limitOffset._limit.value();
  }
  if (query._limitOffset._offset != 0) {
    oss << " OFFSET " << query._limitOffset._offset;
  }
  
  std::string result = oss.str();
  serializationLength_ = result.length();
  return result;
}

// ____________________________________________________________________________
std::string CanonicalSerializer::serializeGraphPattern(
    const parsedQuery::GraphPattern& pattern) const {
  // Stub - in a full implementation, would recursively serialize
  // all triple patterns, FILTERs, OPTIONALs, UNIONs, etc.
  return "/* graph pattern */";
}

}  // namespace queryCanonical
