//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 10.3 - Agent 3 (Unified Physical Optimizer)

#ifndef QLEVER_SRC_ENGINE_UNIFIED_IR_GRAPH_H
#define QLEVER_SRC_ENGINE_UNIFIED_IR_GRAPH_H

#include <algorithm>
#include <vector>

#include "engine/UnifiedIRNode.h"
#include "util/Exception.h"

namespace qlever::unified {

/// Unified Intermediate Representation graph structure
/// Contains nodes from SPARQL, Datalog, and SHACL queries with adjacency
/// information
///
/// Invariants:
/// 1. Node IDs are contiguous from 0 to nodes_.size() - 1
/// 2. adjacencyLists_[i] contains indices j where 0 <= j < nodes_.size()
/// 3. All nodes in adjacencyLists_[i] share at least one variable with
/// nodes_[i]
struct UnifiedIRGraph {
  /// All nodes in the graph (indexed by node ID)
  std::vector<UnifiedIRNode> nodes_;

  /// Adjacency lists: adjacencyLists_[i] = {j, k, ...} means node i connects to
  /// j, k, ... Connection = shared variable between nodes
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
    return std::find(neighbors.begin(), neighbors.end(), nodeId2) !=
           neighbors.end();
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
      if (node.isSparql())
        ++counts.sparql;
      else if (node.isDatalog())
        ++counts.datalog;
      else if (node.isShacl())
        ++counts.shacl;
    }
    return counts;
  }
};

}  // namespace qlever::unified

#endif  // QLEVER_SRC_ENGINE_UNIFIED_IR_GRAPH_H
