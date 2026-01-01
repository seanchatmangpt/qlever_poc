//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (EPIC 5 - Datalog Guards Implementation)

#ifndef QLEVER_SRC_ENGINE_RULES_RULESSERVICE_H
#define QLEVER_SRC_ENGINE_RULES_RULESSERVICE_H

#include <memory>
#include <string>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/idTable/IdTable.h"
#include "engine/rules/RuleExecutionContext.h"
#include "engine/rules/RuleExecutionResult.h"
#include "engine/rules/RulesConfig.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "parser/TripleComponent.h"

namespace rules {

/// Input specification for Datalog rule execution
struct RulesInput {
  /// Query execution context (required)
  QueryExecutionContext* qec;

  /// Database containing all Datalog rules
  std::shared_ptr<RuleDatabase> ruleDatabase;

  /// The predicate name to evaluate
  std::string rulePredicate;

  /// Arguments bound to the rule head
  std::vector<TripleComponent> arguments;

  /// Execution limits/guards (uses defaults if not specified)
  RulesConfig config;

  /// Constructor
  RulesInput(QueryExecutionContext* execContext,
             std::shared_ptr<RuleDatabase> db, std::string predicate,
             std::vector<TripleComponent> args,
             RulesConfig limits = RulesConfig())
      : qec(execContext),
        ruleDatabase(std::move(db)),
        rulePredicate(std::move(predicate)),
        arguments(std::move(args)),
        config(limits) {}
};

/// Service for executing Datalog rules with mandatory guardrails.
///
/// This service wraps the existing Datalog execution engine
/// (FixpointComputation) and enforces all guards during execution.
///
/// Key features:
/// - Enforces all guards (iterations, facts, rule fires, runtime, memory)
/// - Returns standardized RuleExecutionResult with deterministic digest
/// - Thread-safe if underlying engine is thread-safe
/// - Fail-closed: On guard trigger, returns outcome=GUARDED (no partial
/// results)
///
/// Usage:
/// ```cpp
/// RulesService service;
/// RulesInput input{qec, ruleDb, "ancestor", args};
/// RuleExecutionResult result = service.execute(input);
/// if (result.outcome == ExecutionOutcome::OK) {
///   // Process result
/// }
/// ```
class RulesService {
 public:
  /// Execute Datalog rules with guards
  ///
  /// @param input Rule execution input (rules, config, context)
  /// @return Standardized result with outcome, metrics, and digest
  ///
  /// Behavior:
  /// - If any guard is triggered: outcome=GUARDED, digest=""
  /// - If execution succeeds: outcome=OK, digest=SHA256 of results
  /// - If execution errors: outcome=ERROR, error message set
  ///
  /// Guards are checked at the START of each iteration (before executing).
  /// This ensures we fail fast and don't waste resources.
  RuleExecutionResult execute(const RulesInput& input);

 private:
  /// Run the fixpoint iteration loop with guard monitoring
  ///
  /// @param input Rule execution input
  /// @param monitor Guard monitor for tracking metrics
  /// @return IdTable with derived facts (empty if guard triggered)
  ///
  /// This is the core execution loop. It wraps the existing
  /// FixpointComputation logic but adds guard checks at each iteration.
  IdTable runGuardedFixpoint(const RulesInput& input, GuardMonitor& monitor);

  /// Compute SHA-256 digest of derived facts for reproducibility
  ///
  /// @param facts The IdTable containing derived facts
  /// @param ruleFires Per-rule fire counts (sorted alphabetically)
  /// @return 64-character hex string (lowercase) or empty on error
  ///
  /// The digest is computed deterministically:
  /// 1. Sort facts lexicographically
  /// 2. Serialize to canonical string representation
  /// 3. Append sorted rule_fires
  /// 4. Compute SHA-256 hash
  std::string computeDigest(const IdTable& facts,
                            const std::map<std::string, uint64_t>& ruleFires);

  /// Merge and deduplicate two IdTables
  ///
  /// @param target Table to merge into (modified)
  /// @param source Table to merge from (not modified)
  /// @return Number of new rows added
  ///
  /// This is used during fixpoint iteration to combine results
  /// from multiple iterations while removing duplicates.
  size_t mergeAndDeduplicate(IdTable& target, const IdTable& source);
};

}  // namespace rules

#endif  // QLEVER_SRC_ENGINE_RULES_RULESSERVICE_H
