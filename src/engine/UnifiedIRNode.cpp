//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 10.3 - Agent 3 (Unified Physical Optimizer)

#include "engine/UnifiedIRNode.h"

namespace qlever::unified {

// Extract variables from payload (called during construction)
ad_utility::HashSet<Variable> UnifiedIRNode::extractVariables(
    const Payload& payload) {
  return std::visit(
      [](const auto& p) -> ad_utility::HashSet<Variable> {
        using T = std::decay_t<decltype(p)>;

        if constexpr (std::is_same_v<T, QueryPlanner::TripleGraph::Node>) {
          // SPARQL: return pre-computed variables from TripleGraph::Node
          return p._variables;

        } else if constexpr (std::is_same_v<T, DatalogRule>) {
          // Datalog: extract from head variables + body patterns
          ad_utility::HashSet<Variable> vars;
          vars.insert(p.getHeadVariables().begin(), p.getHeadVariables().end());
          for (const auto& bodyPattern : p.getBodyPatterns()) {
            if (bodyPattern.s_.isVariable()) {
              vars.insert(bodyPattern.s_.getVariable());
            }
            if (auto predVar = bodyPattern.getPredicateVariable()) {
              vars.insert(*predVar);
            }
            if (bodyPattern.o_.isVariable()) {
              vars.insert(bodyPattern.o_.getVariable());
            }
          }
          return vars;

        } else if constexpr (std::is_same_v<T, shacl::PropertyShape>) {
          // SHACL: property shapes don't directly expose variables in the same
          // way For now, return empty set; will be refined when SHACL
          // validation is integrated
          return ad_utility::HashSet<Variable>{};
        }

        return ad_utility::HashSet<Variable>{};
      },
      payload);
}

}  // namespace qlever::unified
