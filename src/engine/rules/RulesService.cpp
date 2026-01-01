//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (EPIC 5 - Datalog Guards Implementation)

#include "engine/rules/RulesService.h"

#include <absl/strings/str_join.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>

#include "engine/RuleExpansion.h"
#include "util/CryptographicHashUtils.h"
#include "util/Exception.h"
#include "util/Log.h"

namespace rules {

// _____________________________________________________________________________
RuleExecutionResult RulesService::execute(const RulesInput& input) {
  // Validate input
  AD_CONTRACT_CHECK(input.qec != nullptr, "QueryExecutionContext is null");
  AD_CONTRACT_CHECK(input.ruleDatabase != nullptr, "RuleDatabase is null");
  AD_CONTRACT_CHECK(!input.rulePredicate.empty(), "Rule predicate is empty");

  LOG(DEBUG) << "Starting Datalog execution with guards for predicate: "
             << input.rulePredicate << std::endl;

  // Create guard monitor
  GuardMonitor monitor(input.config);
  monitor.start();

  try {
    // Check if there are any rules for this predicate
    if (!input.ruleDatabase->hasRuleFor(input.rulePredicate)) {
      LOG(WARNING) << "No rules found for predicate: " << input.rulePredicate
                   << std::endl;
      monitor.stop();
      // Return OK with zero facts (empty result is valid)
      return RuleExecutionResult::ok(0, 0, {}, "", monitor.getRuntimeMs(), 0);
    }

    // Run the guarded fixpoint computation
    IdTable facts = runGuardedFixpoint(input, monitor);

    // Stop the timer
    monitor.stop();

    // Check if a guard was triggered
    auto snapshot = monitor.snapshot();
    if (snapshot.guard_triggered) {
      LOG(WARNING) << "Guard triggered during execution: "
                   << *snapshot.guard_triggered << std::endl;
      return RuleExecutionResult::guarded(
          *snapshot.guard_triggered, snapshot.iterations,
          snapshot.derived_facts, snapshot.rule_fires, snapshot.runtime_ms,
          snapshot.memory_peak_bytes);
    }

    // Compute digest for reproducibility
    std::string digest = computeDigest(facts, snapshot.rule_fires);

    LOG(DEBUG) << "Datalog execution completed successfully. Facts: "
               << facts.size() << ", Iterations: " << snapshot.iterations
               << ", Digest: " << digest << std::endl;

    // Return successful result
    return RuleExecutionResult::ok(facts.size(), snapshot.iterations,
                                   snapshot.rule_fires, digest,
                                   snapshot.runtime_ms,
                                   snapshot.memory_peak_bytes);

  } catch (const ad_utility::Exception& e) {
    monitor.stop();
    LOG(ERROR) << "Datalog execution failed: " << e.what() << std::endl;
    return RuleExecutionResult::error(e.what());
  } catch (const std::exception& e) {
    monitor.stop();
    LOG(ERROR) << "Datalog execution failed: " << e.what() << std::endl;
    return RuleExecutionResult::error(e.what());
  }
}

// _____________________________________________________________________________
IdTable RulesService::runGuardedFixpoint(const RulesInput& input,
                                         GuardMonitor& monitor) {
  // Initialize result table
  // Count variables in arguments to determine result width
  size_t resultWidth = 0;
  for (const auto& arg : input.arguments) {
    if (arg.isVariable()) {
      ++resultWidth;
    }
  }

  IdTable allResults(resultWidth, input.qec->getAllocator());

  LOG(DEBUG) << "Starting fixpoint iterations with result width: "
             << resultWidth << std::endl;

  // Fixpoint iteration loop
  while (true) {
    // **CRITICAL**: Check guards at the START of each iteration
    // This ensures we fail fast before doing expensive work
    if (monitor.shouldAbort()) {
      LOG(WARNING) << "Guard triggered at iteration "
                   << monitor.getIterations() << std::endl;
      // Return empty table (fail closed - no partial results)
      return IdTable(resultWidth, input.qec->getAllocator());
    }

    LOG(DEBUG) << "Starting iteration " << monitor.getIterations()
               << std::endl;

    // Create RuleExpansion for this iteration
    // NOTE: In a full semi-naive implementation, we would pass previous
    // iteration results as additional base facts. For now, we create
    // a fresh expansion each time.
    auto ruleExpansion = std::make_unique<RuleExpansion>(
        input.qec, input.ruleDatabase, input.rulePredicate, input.arguments);

    // Create execution tree
    auto ruleExpansionTree = ad_utility::makeExecutionTree<RuleExpansion>(
        input.qec, std::move(ruleExpansion));

    // Execute the rule expansion
    try {
      std::shared_ptr<const Result> iterationResult =
          ruleExpansionTree->getResult();
      const IdTable& newFacts = iterationResult->idTable();

      LOG(DEBUG) << "Iteration " << monitor.getIterations() << " produced "
                 << newFacts.size() << " facts" << std::endl;

      // Record rule fires for this iteration
      // For now, we record one fire per derived fact
      // In a full implementation, we would track individual rule applications
      if (newFacts.size() > 0) {
        monitor.recordRuleFire(input.rulePredicate);
      }

      // First iteration: initialize results
      if (monitor.getIterations() == 0) {
        if (newFacts.size() > 0) {
          allResults = newFacts.clone();
          monitor.addDerivedFacts(allResults.size());
        } else {
          // No base facts, we're done
          LOG(DEBUG) << "No base facts found, terminating" << std::endl;
          break;
        }
      } else {
        // Subsequent iterations: merge and deduplicate
        size_t sizeBefore = allResults.size();
        size_t newRowsAdded = mergeAndDeduplicate(allResults, newFacts);
        monitor.addDerivedFacts(newRowsAdded);

        LOG(DEBUG) << "Iteration " << monitor.getIterations() << ": Added "
                   << newRowsAdded << " new rows (total: " << allResults.size()
                   << ")" << std::endl;

        // Check if we've reached a fixpoint (no new facts)
        if (newRowsAdded == 0) {
          LOG(DEBUG) << "Fixpoint reached at iteration "
                     << monitor.getIterations() << std::endl;
          break;
        }
      }

      // Update memory usage estimate
      size_t estimatedMemory =
          allResults.size() * allResults.numColumns() * sizeof(Id);
      monitor.updateMemoryUsage(estimatedMemory);

      // Increment iteration counter
      monitor.incrementIteration();

    } catch (const ad_utility::AbortException& e) {
      // Query was cancelled, propagate up
      throw;
    } catch (const std::exception& e) {
      LOG(ERROR) << "Error during iteration " << monitor.getIterations()
                 << ": " << e.what() << std::endl;
      throw;
    }
  }

  LOG(DEBUG) << "Fixpoint computation completed. Total facts: "
             << allResults.size() << ", Iterations: " << monitor.getIterations()
             << std::endl;

  return allResults;
}

// _____________________________________________________________________________
std::string RulesService::computeDigest(
    const IdTable& facts, const std::map<std::string, uint64_t>& ruleFires) {
  if (facts.size() == 0) {
    return "";
  }

  try {
    // Build a canonical string representation for hashing
    std::ostringstream oss;

    // 1. Serialize facts in a deterministic order
    // For simplicity, we serialize each row as a sequence of IDs
    // In a production system, we would convert IDs to their string
    // representations and sort lexicographically
    std::vector<std::string> rowStrings;
    rowStrings.reserve(facts.size());

    for (const auto& row : facts) {
      std::ostringstream rowOss;
      for (size_t i = 0; i < row.size(); ++i) {
        if (i > 0) rowOss << ",";
        rowOss << row[i].getBits();  // Use raw ID bits
      }
      rowStrings.push_back(rowOss.str());
    }

    // Sort rows for deterministic ordering
    std::sort(rowStrings.begin(), rowStrings.end());

    // Append sorted rows
    for (const auto& rowStr : rowStrings) {
      oss << rowStr << "\n";
    }

    // 2. Append rule fires (already sorted alphabetically by std::map)
    oss << "RULE_FIRES:\n";
    for (const auto& [rule, count] : ruleFires) {
      oss << rule << ":" << count << "\n";
    }

    // 3. Compute SHA-256 hash
    std::string canonical = oss.str();
    ad_utility::HashSha256 hasher;
    auto hashBytes = hasher(canonical);

    // Convert to hex string (lowercase)
    std::string hexHash =
        absl::StrJoin(hashBytes, "", ad_utility::hexFormatter);

    // Convert to lowercase (hexFormatter may produce uppercase)
    std::transform(hexHash.begin(), hexHash.end(), hexHash.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return hexHash;

  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to compute digest: " << e.what() << std::endl;
    return "";
  }
}

// _____________________________________________________________________________
size_t RulesService::mergeAndDeduplicate(IdTable& target,
                                         const IdTable& source) {
  // Validate that tables have the same width
  AD_CONTRACT_CHECK(target.numColumns() == source.numColumns(),
                    "Cannot merge tables with different column counts");

  if (source.size() == 0) {
    return 0;
  }

  size_t originalSize = target.size();

  // Helper to create a hash of a row for deduplication
  auto rowToHash = [](const auto& row) {
    size_t hash = 0;
    for (const auto& id : row) {
      // Combine hashes using a simple algorithm
      hash ^= std::hash<Id>{}(id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  };

  // Build set of existing rows for fast lookup
  std::unordered_set<size_t> existingRows;
  existingRows.reserve(target.size());
  for (const auto& row : target) {
    existingRows.insert(rowToHash(row));
  }

  // Add new rows that don't exist yet
  std::vector<IdTable::row_type> newRows;
  for (const auto& row : source) {
    size_t hash = rowToHash(row);
    if (existingRows.find(hash) == existingRows.end()) {
      // New row, add it
      IdTable::row_type rowCopy(row.size());
      std::copy(row.begin(), row.end(), rowCopy.begin());
      newRows.push_back(std::move(rowCopy));
      existingRows.insert(hash);
    }
  }

  // Append new rows to target
  for (auto& row : newRows) {
    target.push_back(row);
  }

  size_t newRowsAdded = target.size() - originalSize;
  return newRowsAdded;
}

}  // namespace rules
