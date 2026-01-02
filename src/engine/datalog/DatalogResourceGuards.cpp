//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Agent 6 - EPIC 10.2 Datalog/N3 Guardrails

#include "engine/datalog/DatalogResourceGuards.h"

#include "engine/QueryExecutionContext.h"

namespace datalog {

// Static factory method to create guards from QueryExecutionContext
DatalogResourceGuards DatalogResourceGuards::fromContext(
    const QueryExecutionContext* qec) {
  DatalogResourceGuards guards;

  // Extract epoch ID from context
  guards.epochId = qec->getCurrentEpochId();

  // Extract manifest hash from context
  guards.manifestHash = qec->getEpochDeterministicKey();

  // Use default resource limits (can be customized via setters)
  // maxFactCount: 1,000,000 facts
  // maxRuleTime: 30 seconds
  // maxMemoryBytes: 1 GB

  return guards;
}

}  // namespace datalog
