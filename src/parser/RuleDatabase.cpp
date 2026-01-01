//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#include "parser/RuleDatabase.h"

// _____________________________________________________________________________
void RuleDatabase::addRule(DatalogRule rule) {
  auto lock = rules_.wlock();
  std::string predicateName = rule.getHeadPredicate();
  (*lock)[predicateName].push_back(std::move(rule));
}

// _____________________________________________________________________________
std::vector<DatalogRule> RuleDatabase::getRulesByPredicate(
    const std::string& predicateName) const {
  auto lock = rules_.rlock();
  auto it = lock->find(predicateName);
  if (it != lock->end()) {
    return it->second;
  }
  return {};
}

// _____________________________________________________________________________
std::vector<DatalogRule> RuleDatabase::getAllRules() const {
  auto lock = rules_.rlock();
  std::vector<DatalogRule> allRules;
  for (const auto& [predicate, rules] : *lock) {
    allRules.insert(allRules.end(), rules.begin(), rules.end());
  }
  return allRules;
}

// _____________________________________________________________________________
bool RuleDatabase::hasRuleFor(const std::string& predicateName) const {
  auto lock = rules_.rlock();
  return lock->find(predicateName) != lock->end();
}

// _____________________________________________________________________________
size_t RuleDatabase::getRuleCount() const {
  auto lock = rules_.rlock();
  size_t count = 0;
  for (const auto& [predicate, rules] : *lock) {
    count += rules.size();
  }
  return count;
}

// _____________________________________________________________________________
size_t RuleDatabase::getPredicateCount() const {
  auto lock = rules_.rlock();
  return lock->size();
}

// _____________________________________________________________________________
void RuleDatabase::clear() {
  auto lock = rules_.wlock();
  lock->clear();
}

// _____________________________________________________________________________
std::vector<std::string> RuleDatabase::getPredicateNames() const {
  auto lock = rules_.rlock();
  std::vector<std::string> names;
  names.reserve(lock->size());
  for (const auto& [predicate, rules] : *lock) {
    names.push_back(predicate);
  }
  return names;
}
