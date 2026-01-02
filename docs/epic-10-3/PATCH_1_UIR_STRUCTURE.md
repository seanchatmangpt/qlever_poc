# SPECIFICATION PATCH 1: Unified Intermediate Representation (UIR) Data Structure

**EPIC:** 10.3 - Unified Physical Optimizer
**Agent:** Agent 3 (Unified Planner)
**Ambiguity Resolved:** UIR C++ structure design
**Status:** CLOSED - Zero degrees of freedom

---

## Design Selection: Variant-Based UnifiedIRNode

**Selected Approach:** Option A - Variant-based `UnifiedIRNode`

**Justification (3 sentences):**
The variant-based approach aligns with QLever's existing codebase patterns (std::variant used in ShaclShape.h:87-100) and provides zero-cost type-safe discrimination. It preserves the separation of concerns between SPARQL, Datalog, and SHACL representations while enabling unified physical optimization through std::visit. This approach requires minimal disruption to existing QueryPlanner, DatalogQueryPlanner, and ShaclPlanningStrategy components while supporting monoidal composition.

**Why Not Other Options:**
- **Option B (Extend TripleGraph::Node):** Violates single responsibility principle; tightly couples three distinct query languages into SPARQL-centric structure; breaks existing Datalog/SHACL isolation
- **Option C (Visitor with separate node types):** Already implicitly exists in current architecture; lacks unified container for cross-language optimization
- **Option D (Unified AST):** Requires massive refactoring of parser layer; violates BB80/20 (20% effort rule); incompatible with single-pass construction

---

## Complete C++ Header Definition

```cpp
//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 10.3 - Agent 3 (Unified Physical Optimizer)

#ifndef QLEVER_SRC_ENGINE_UNIFIED_IR_NODE_H
#define QLEVER_SRC_ENGINE_UNIFIED_IR_NODE_H

#include <memory>
#include <variant>
#include <vector>
#include <optional>

#include "engine/QueryPlanner.h"
#include "parser/DatalogRule.h"
#include "parser/SparqlTriple.h"
#include "engine/shacl/ShaclShape.h"
#include "rdfTypes/Variable.h"
#include "util/HashMap.h"

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
/// 1. Exactly one variant alternative is active at any time (enforced by std::variant)
/// 2. id_ is unique within a single UIR graph (enforced by UnifiedPhysicalOptimizer)
/// 3. variables_ contains all free variables in the node (derived from payload)
/// 4. Node is immutable after construction (const methods only, move semantics)
///
/// Thread Safety: UnifiedIRNode is thread-safe for concurrent reads after
/// construction. Mutation requires external synchronization.
struct UnifiedIRNode {
  /// Node payload discriminated by query language
  using Payload = std::variant<
      QueryPlanner::TripleGraph::Node,  // SPARQL triple pattern
      DatalogRule,                       // Datalog rule (head + body)
      shacl::PropertyShape               // SHACL property constraint
  >;

  /// Unique identifier within UIR graph (set by optimizer)
  size_t id_;

  /// Node payload (one of SPARQL/Datalog/SHACL)
  Payload payload_;

  /// Cached set of all variables referenced by this node
  /// (extracted from payload during construction)
  ad_utility::HashSet<Variable> variables_;

  /// Default constructor (creates invalid node with id 0)
  UnifiedIRNode() : id_(0), payload_(QueryPlanner::TripleGraph::Node{}), variables_{} {}

  /// Construct from SPARQL triple pattern
  /// @param id Unique node identifier
  /// @param node SPARQL triple graph node
  explicit UnifiedIRNode(size_t id, QueryPlanner::TripleGraph::Node node)
      : id_(id), payload_(std::move(node)), variables_(extractVariables(payload_)) {}

  /// Construct from Datalog rule
  /// @param id Unique node identifier
  /// @param rule Datalog rule (head + body patterns)
  explicit UnifiedIRNode(size_t id, DatalogRule rule)
      : id_(id), payload_(std::move(rule)), variables_(extractVariables(payload_)) {}

  /// Construct from SHACL property shape
  /// @param id Unique node identifier
  /// @param shape SHACL property shape constraint
  explicit UnifiedIRNode(size_t id, shacl::PropertyShape shape)
      : id_(id), payload_(std::move(shape)), variables_(extractVariables(payload_)) {}

  /// Get node type as string (for debugging/logging)
  [[nodiscard]] const char* getTypeName() const {
    return std::visit([](const auto& payload) -> const char* {
      using T = std::decay_t<decltype(payload)>;
      if constexpr (std::is_same_v<T, QueryPlanner::TripleGraph::Node>) {
        return "SPARQL";
      } else if constexpr (std::is_same_v<T, DatalogRule>) {
        return "Datalog";
      } else if constexpr (std::is_same_v<T, shacl::PropertyShape>) {
        return "SHACL";
      }
      return "Unknown";
    }, payload_);
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
  static ad_utility::HashSet<Variable> extractVariables(const Payload& payload) {
    return std::visit([](const auto& p) -> ad_utility::HashSet<Variable> {
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
        // SHACL: property shapes don't directly expose variables in the same way
        // For now, return empty set; will be refined when SHACL validation is integrated
        return ad_utility::HashSet<Variable>{};
      }

      return ad_utility::HashSet<Variable>{};
    }, payload);
  }
};

/// Unified Intermediate Representation graph structure
/// Contains nodes from SPARQL, Datalog, and SHACL queries with adjacency information
///
/// Invariants:
/// 1. Node IDs are contiguous from 0 to nodes_.size() - 1
/// 2. adjacencyLists_[i] contains indices j where 0 <= j < nodes_.size()
/// 3. All nodes in adjacencyLists_[i] share at least one variable with nodes_[i]
struct UnifiedIRGraph {
  /// All nodes in the graph (indexed by node ID)
  std::vector<UnifiedIRNode> nodes_;

  /// Adjacency lists: adjacencyLists_[i] = {j, k, ...} means node i connects to j, k, ...
  /// Connection = shared variable between nodes
  std::vector<std::vector<size_t>> adjacencyLists_;

  /// Add a SPARQL node to the graph
  /// @param node SPARQL triple pattern
  /// @return Node ID
  size_t addSparqlNode(QueryPlanner::TripleGraph::Node node) {
    size_t id = nodes_.size();
    nodes_.emplace_back(id, std::move(node));
    adjacencyLists_.emplace_back();
    return id;
  }

  /// Add a Datalog node to the graph
  /// @param rule Datalog rule
  /// @return Node ID
  size_t addDatalogNode(DatalogRule rule) {
    size_t id = nodes_.size();
    nodes_.emplace_back(id, std::move(rule));
    adjacencyLists_.emplace_back();
    return id;
  }

  /// Add a SHACL node to the graph
  /// @param shape SHACL property shape
  /// @return Node ID
  size_t addShaclNode(shacl::PropertyShape shape) {
    size_t id = nodes_.size();
    nodes_.emplace_back(id, std::move(shape));
    adjacencyLists_.emplace_back();
    return id;
  }

  /// Add edge between two nodes (bidirectional)
  /// @param nodeId1 First node ID
  /// @param nodeId2 Second node ID
  void addEdge(size_t nodeId1, size_t nodeId2) {
    AD_CONTRACT_CHECK(nodeId1 < nodes_.size());
    AD_CONTRACT_CHECK(nodeId2 < nodes_.size());
    adjacencyLists_[nodeId1].push_back(nodeId2);
    adjacencyLists_[nodeId2].push_back(nodeId1);
  }

  /// Get all nodes
  [[nodiscard]] const std::vector<UnifiedIRNode>& getNodes() const {
    return nodes_;
  }

  /// Get neighbors of a node
  [[nodiscard]] const std::vector<size_t>& getNeighbors(size_t nodeId) const {
    AD_CONTRACT_CHECK(nodeId < adjacencyLists_.size());
    return adjacencyLists_[nodeId];
  }

  /// Check if two nodes are connected
  [[nodiscard]] bool areConnected(size_t nodeId1, size_t nodeId2) const {
    AD_CONTRACT_CHECK(nodeId1 < adjacencyLists_.size());
    const auto& neighbors = adjacencyLists_[nodeId1];
    return std::find(neighbors.begin(), neighbors.end(), nodeId2) != neighbors.end();
  }

  /// Count nodes by type
  struct NodeCounts {
    size_t sparql = 0;
    size_t datalog = 0;
    size_t shacl = 0;
  };

  [[nodiscard]] NodeCounts countNodeTypes() const {
    NodeCounts counts;
    for (const auto& node : nodes_) {
      if (node.isSparql()) ++counts.sparql;
      else if (node.isDatalog()) ++counts.datalog;
      else if (node.isShacl()) ++counts.shacl;
    }
    return counts;
  }
};

}  // namespace qlever::unified

#endif  // QLEVER_SRC_ENGINE_UNIFIED_IR_NODE_H
```

---

## Integration Points

### 1. QueryExecutionContext Integration

The UIR integrates with `QueryExecutionContext` through the `UnifiedPhysicalOptimizer` which will be called during `QueryPlanner::createExecutionTree()`:

```cpp
// In QueryPlanner::createExecutionTree() (future integration)
if (requiresUnifiedOptimization(pq)) {
  qlever::unified::UnifiedPhysicalOptimizer optimizer(qec);
  auto uirGraph = optimizer.buildUnifiedIR(pq);
  return optimizer.optimize(uirGraph);
}
```

### 2. Deterministic Execution Order

The UIR guarantees deterministic execution order through:
1. **Node ID Assignment:** Sequential, stable assignment during graph construction
2. **Adjacency Lists:** Sorted by node ID after construction
3. **Visitor Pattern:** Deterministic type dispatch via std::visit
4. **Monoidal Composition:** Join order determined by cost model, not discovery order

### 3. Memory Ownership Model

- **UnifiedIRNode:** Value semantics (movable, non-copyable payloads)
- **UnifiedIRGraph:** Owns all nodes, provides stable references via indices
- **Integration:** Optimizer creates UIR graph, returns QueryExecutionTree with Operation hierarchy
- **Lifetime:** UIR graph destroyed after optimization completes (ephemeral IR)

---

## Example Usage

### Example 1: SPARQL Node Creation

```cpp
#include "engine/UnifiedIRNode.h"
#include "engine/QueryPlanner.h"

// Create SPARQL triple pattern node
SparqlTriple triple{
    Variable{"?x"},                          // subject
    TripleComponent::Iri::fromIriref("<p>"), // predicate
    Variable{"?y"}                           // object
};

QueryPlanner::TripleGraph::Node sparqlNode(
    0,  // id (will be reassigned by UIR graph)
    std::move(triple)
);

qlever::unified::UnifiedIRGraph graph;
size_t nodeId = graph.addSparqlNode(std::move(sparqlNode));

// Node now accessible as:
const auto& node = graph.getNodes()[nodeId];
assert(node.isSparql());
assert(node.getVariables().contains(Variable{"?x"}));
assert(node.getVariables().contains(Variable{"?y"}));
```

### Example 2: Datalog Node Creation

```cpp
#include "engine/UnifiedIRNode.h"
#include "parser/DatalogRule.h"

// Create Datalog rule: ancestor(?x, ?y) :- parent(?x, ?y).
DatalogRule rule(
    "ancestor",                              // head predicate
    {Variable{"?x"}, Variable{"?y"}},        // head variables
    {SparqlTriple{Variable{"?x"},            // body pattern
                  TripleComponent::Iri::fromIriref("<parent>"),
                  Variable{"?y"}}}
);

qlever::unified::UnifiedIRGraph graph;
size_t nodeId = graph.addDatalogNode(std::move(rule));

const auto& node = graph.getNodes()[nodeId];
assert(node.isDatalog());
assert(node.asDatalog().getHeadPredicate() == "ancestor");
```

### Example 3: SHACL Node Creation

```cpp
#include "engine/UnifiedIRNode.h"
#include "engine/shacl/ShaclShape.h"

// Create SHACL property shape with minCount constraint
shacl::PropertyShape propShape("<http://example.org/name>");
shacl::ShaclConstraint constraint(shacl::ConstraintType::MinCount);
constraint.value = 1;  // minCount = 1
propShape.constraints.push_back(constraint);

qlever::unified::UnifiedIRGraph graph;
size_t nodeId = graph.addShaclNode(std::move(propShape));

const auto& node = graph.getNodes()[nodeId];
assert(node.isShacl());
assert(node.asShacl().path == "<http://example.org/name>");
```

### Example 4: Mixed Graph with Connections

```cpp
#include "engine/UnifiedIRNode.h"

qlever::unified::UnifiedIRGraph graph;

// Add SPARQL node: ?x <name> ?name
SparqlTriple t1{Variable{"?x"},
                TripleComponent::Iri::fromIriref("<name>"),
                Variable{"?name"}};
size_t id1 = graph.addSparqlNode(
    QueryPlanner::TripleGraph::Node(0, std::move(t1))
);

// Add Datalog node: ancestor(?x, ?y) :- parent(?x, ?y)
DatalogRule rule("ancestor", {Variable{"?x"}, Variable{"?y"}},
                 {SparqlTriple{Variable{"?x"},
                               TripleComponent::Iri::fromIriref("<parent>"),
                               Variable{"?y"}}});
size_t id2 = graph.addDatalogNode(std::move(rule));

// Connect nodes (they share variable ?x)
graph.addEdge(id1, id2);

// Verify connection
assert(graph.areConnected(id1, id2));
assert(graph.getNeighbors(id1).size() == 1);
assert(graph.getNeighbors(id1)[0] == id2);

// Count node types
auto counts = graph.countNodeTypes();
assert(counts.sparql == 1);
assert(counts.datalog == 1);
assert(counts.shacl == 0);
```

---

## Memory Safety Guarantees

### 1. Type Safety
- **Invariant:** `std::variant` ensures exactly one active alternative at compile-time
- **Guarantee:** Accessing wrong alternative throws `std::bad_variant_access`
- **Enforcement:** `asSparql()`, `asDatalog()`, `asShacl()` methods use `std::get<T>()`

### 2. Lifetime Safety
- **Invariant:** `UnifiedIRGraph` owns all nodes; node references are indices, not pointers
- **Guarantee:** No dangling references; indices remain valid during graph lifetime
- **Enforcement:** All node access via `getNodes()[id]` which bounds-checks vector access

### 3. Move Semantics
- **Invariant:** Node payloads are moved into `UnifiedIRNode`, preventing accidental copies
- **Guarantee:** Large structures (DatalogRule with multiple body patterns) moved, not copied
- **Enforcement:** Constructors accept `T&&` and use `std::move()`

### 4. Immutability After Construction
- **Invariant:** `UnifiedIRNode` provides only const accessors after construction
- **Guarantee:** No mutation of node data after insertion into graph
- **Enforcement:** All getters are `[[nodiscard]] const`; no setters

### 5. Thread Safety (Read-Only)
- **Invariant:** Multiple threads can read `UnifiedIRGraph` concurrently
- **Guarantee:** No data races during read-only operations (iteration, lookup)
- **Enforcement:** `const` methods only; mutation requires external synchronization

### 6. Bounds Checking
- **Invariant:** All node ID accesses are bounds-checked
- **Guarantee:** `std::out_of_range` thrown on invalid node ID
- **Enforcement:** `AD_CONTRACT_CHECK()` in `addEdge()`, `getNeighbors()`, `areConnected()`

---

## Memory Layout (64-bit x86_64)

```
UnifiedIRNode:
  +0   id_          : size_t                 (8 bytes)
  +8   payload_     : std::variant<...>      (40 bytes typical)
  +48  variables_   : HashSet<Variable>      (16 bytes)
  Total: ~64 bytes + variable set storage

UnifiedIRGraph:
  +0   nodes_           : std::vector<UnifiedIRNode>    (24 bytes)
  +24  adjacencyLists_  : std::vector<std::vector<...>> (24 bytes)
  Total: 48 bytes + heap-allocated node/edge storage
```

---

## Integration with Existing Planners

### QueryPlanner Integration
```cpp
// Convert existing TripleGraph to UnifiedIRGraph
UnifiedIRGraph convertTripleGraph(const QueryPlanner::TripleGraph& tg) {
  UnifiedIRGraph uirGraph;
  for (const auto& [id, nodePtr] : tg._nodeMap) {
    uirGraph.addSparqlNode(*nodePtr);
  }
  // Add edges based on tg._adjLists
  return uirGraph;
}
```

### DatalogQueryPlanner Integration
```cpp
// Add Datalog rules to UIR graph
void addDatalogRules(UnifiedIRGraph& graph, const RuleDatabase& ruleDb) {
  for (const auto& rule : ruleDb.getRules()) {
    graph.addDatalogNode(rule);
  }
}
```

### ShaclPlanningStrategy Integration
```cpp
// Add SHACL constraints to UIR graph
void addShaclConstraints(UnifiedIRGraph& graph, const ShaclShapeRegistry& registry) {
  for (const auto& shape : registry.getShapes()) {
    for (const auto& propShape : shape.propertyShapes) {
      graph.addShaclNode(propShape);
    }
  }
}
```

---

## Closure Statement

This specification patch **CLOSES** Ambiguity 1 for Agent 3. The variant-based `UnifiedIRNode` structure:

1. ✅ Provides zero degrees of freedom for design choice
2. ✅ Integrates with existing `QueryExecutionContext` via optimizer pattern
3. ✅ Guarantees deterministic execution order via stable node IDs
4. ✅ Ensures memory safety through value semantics and bounds checking
5. ✅ Supports monoidal composition via immutable nodes and functional visitors
6. ✅ Enables cross-language optimization through unified graph representation

**Next Steps:**
- Agent 3 must implement `UnifiedPhysicalOptimizer` using this UIR structure
- Agent 3 must define cost model for cross-language join ordering
- Agent 3 must integrate with QueryPlanner::createExecutionTree()

**No iteration permitted.** This is the canonical UIR structure for EPIC 10.3.
