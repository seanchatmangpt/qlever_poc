//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 10.3 - Agent 3 (Unified Physical Optimizer)

#ifndef QLEVER_SRC_ENGINE_UNIFIED_IR_NODE_H
#define QLEVER_SRC_ENGINE_UNIFIED_IR_NODE_H

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "engine/QueryPlanner.h"
#include "engine/shacl/ShaclShape.h"
#include "parser/DatalogRule.h"
#include "parser/SparqlTriple.h"
#include "rdfTypes/Variable.h"
#include "util/HashSet.h"

namespace qlever::unified {

// Forward declaration
class UnifiedPhysicalOptimizer;

/// Discriminated union representing a single node in the Unified Intermediate
/// Representation (UIR). Each node represents a logical operation from one of
/// three supported query languages: SPARQL, Datalog, or SHACL.
///
/// Memory Layout:
/// - sizeof(UnifiedIRNode) = sizeof(std::variant) + sizeof(size_t) +
///   sizeof(HashSet<Variable>) + padding
/// - Typical size: ~64 bytes on x86_64 (variant: 40 bytes, id: 8 bytes,
///   vars: 16 bytes)
///
/// Invariants:
/// 1. Exactly one variant alternative is active at any time (enforced by
/// std::variant)
/// 2. id_ is unique within a single UIR graph (enforced by
/// UnifiedPhysicalOptimizer)
/// 3. variables_ contains all free variables in the node (derived from payload)
/// 4. Node is immutable after construction (const methods only, move semantics)
///
/// Thread Safety: UnifiedIRNode is thread-safe for concurrent reads after
/// construction. Mutation requires external synchronization.
struct UnifiedIRNode {
  /// Node payload discriminated by query language
  using Payload =
      std::variant<QueryPlanner::TripleGraph::Node,  // SPARQL triple pattern
                   DatalogRule,          // Datalog rule (head + body)
                   shacl::PropertyShape  // SHACL property constraint
                   >;

  /// Unique identifier within UIR graph (set by optimizer)
  size_t id_;

  /// Node payload (one of SPARQL/Datalog/SHACL)
  Payload payload_;

  /// Cached set of all variables referenced by this node
  /// (extracted from payload during construction)
  ad_utility::HashSet<Variable> variables_;

  /// Default constructor (creates invalid node with id 0)
  UnifiedIRNode()
      : id_(0), payload_(QueryPlanner::TripleGraph::Node{}), variables_{} {}

  /// Construct from SPARQL triple pattern
  /// @param id Unique node identifier
  /// @param node SPARQL triple graph node
  explicit UnifiedIRNode(size_t id, QueryPlanner::TripleGraph::Node node)
      : id_(id),
        payload_(std::move(node)),
        variables_(extractVariables(payload_)) {}

  /// Construct from Datalog rule
  /// @param id Unique node identifier
  /// @param rule Datalog rule (head + body patterns)
  explicit UnifiedIRNode(size_t id, DatalogRule rule)
      : id_(id),
        payload_(std::move(rule)),
        variables_(extractVariables(payload_)) {}

  /// Construct from SHACL property shape
  /// @param id Unique node identifier
  /// @param shape SHACL property shape constraint
  explicit UnifiedIRNode(size_t id, shacl::PropertyShape shape)
      : id_(id),
        payload_(std::move(shape)),
        variables_(extractVariables(payload_)) {}

  /// Get node type as string (for debugging/logging)
  [[nodiscard]] const char* getTypeName() const {
    return std::visit(
        [](const auto& payload) -> const char* {
          using T = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<T, QueryPlanner::TripleGraph::Node>) {
            return "SPARQL";
          } else if constexpr (std::is_same_v<T, DatalogRule>) {
            return "Datalog";
          } else if constexpr (std::is_same_v<T, shacl::PropertyShape>) {
            return "SHACL";
          }
          return "Unknown";
        },
        payload_);
  }

  /// Check if this node is a SPARQL triple pattern
  [[nodiscard]] bool isSparql() const {
    return std::holds_alternative<QueryPlanner::TripleGraph::Node>(payload_);
  }

  /// Check if this node is a Datalog rule
  [[nodiscard]] bool isDatalog() const {
    return std::holds_alternative<DatalogRule>(payload_);
  }

  /// Check if this node is a SHACL property shape
  [[nodiscard]] bool isShacl() const {
    return std::holds_alternative<shacl::PropertyShape>(payload_);
  }

  /// Get SPARQL node (throws if not SPARQL)
  [[nodiscard]] const QueryPlanner::TripleGraph::Node& asSparql() const {
    return std::get<QueryPlanner::TripleGraph::Node>(payload_);
  }

  /// Get Datalog rule (throws if not Datalog)
  [[nodiscard]] const DatalogRule& asDatalog() const {
    return std::get<DatalogRule>(payload_);
  }

  /// Get SHACL property shape (throws if not SHACL)
  [[nodiscard]] const shacl::PropertyShape& asShacl() const {
    return std::get<shacl::PropertyShape>(payload_);
  }

  /// Get all variables referenced by this node
  [[nodiscard]] const ad_utility::HashSet<Variable>& getVariables() const {
    return variables_;
  }

  /// Visit the payload with a generic visitor
  /// Enables pattern matching over node types for optimization passes
  template <typename Visitor>
  [[nodiscard]] decltype(auto) visit(Visitor&& visitor) const {
    return std::visit(std::forward<Visitor>(visitor), payload_);
  }

  /// Equality comparison (based on id only, assumes unique ids)
  friend bool operator==(const UnifiedIRNode& lhs, const UnifiedIRNode& rhs) {
    return lhs.id_ == rhs.id_;
  }

  friend bool operator!=(const UnifiedIRNode& lhs, const UnifiedIRNode& rhs) {
    return !(lhs == rhs);
  }

 private:
  /// Extract variables from payload (called during construction)
  static ad_utility::HashSet<Variable> extractVariables(const Payload& payload);
};

}  // namespace qlever::unified

#endif  // QLEVER_SRC_ENGINE_UNIFIED_IR_NODE_H
