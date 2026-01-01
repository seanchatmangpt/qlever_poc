//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#include "parser/DatalogRule.h"

#include <sstream>

#include "util/StringUtils.h"

// _____________________________________________________________________________
std::string DatalogRule::toString() const {
  std::ostringstream oss;

  // Write head: predicateName(var1, var2, ...)
  oss << headPredicate_ << "(";
  for (size_t i = 0; i < headVariables_.size(); ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << headVariables_[i].name();
  }
  oss << ")";

  // Write body if not empty
  if (!bodyPatterns_.empty() || !filters_.empty()) {
    oss << " :- ";

    // Write body patterns
    for (size_t i = 0; i < bodyPatterns_.size(); ++i) {
      if (i > 0) {
        oss << ", ";
      }
      oss << bodyPatterns_[i].asString();
    }

    // Write filters
    if (!filters_.empty()) {
      if (!bodyPatterns_.empty()) {
        oss << ", ";
      }
      for (size_t i = 0; i < filters_.size(); ++i) {
        if (i > 0) {
          oss << ", ";
        }
        oss << filters_[i].asString();
      }
    }
  }

  oss << ".";
  return oss.str();
}
