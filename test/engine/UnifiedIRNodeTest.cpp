//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 10.3 - Agent 3 (Unified Physical Optimizer)

#include <gtest/gtest.h>

#include "engine/UnifiedIRGraph.h"
#include "engine/UnifiedIRNode.h"
#include "parser/TripleComponent.h"

using namespace qlever::unified;

// Test fixture for UnifiedIRNode tests
class UnifiedIRNodeTest : public ::testing::Test {
 protected:
  // Helper to create a simple SPARQL triple: ?x <p> ?y
  static QueryPlanner::TripleGraph::Node createSparqlNode(size_t id = 0) {
    SparqlTriple triple{
        TripleComponent{Variable{"?x"}},
        ad_utility::triple_component::Iri::fromIriref("<http://example.org/p>"),
        TripleComponent{Variable{"?y"}}};
    return QueryPlanner::TripleGraph::Node(id, std::move(triple));
  }

  // Helper to create a Datalog rule: ancestor(?x, ?y) :- parent(?x, ?y)
  static DatalogRule createDatalogRule() {
    SparqlTriple bodyPattern{TripleComponent{Variable{"?x"}},
                             ad_utility::triple_component::Iri::fromIriref(
                                 "<http://example.org/parent>"),
                             TripleComponent{Variable{"?y"}}};
    return DatalogRule("ancestor", {Variable{"?x"}, Variable{"?y"}},
                       {std::move(bodyPattern)});
  }

  // Helper to create a SHACL property shape
  static shacl::PropertyShape createPropertyShape() {
    shacl::PropertyShape shape("<http://example.org/name>");
    shacl::ShaclConstraint constraint(shacl::ConstraintType::MinCount);
    constraint.value = 1;
    shape.constraints.push_back(constraint);
    return shape;
  }
};

// ============================================================================
// SPARQL Node Tests
// ============================================================================

TEST_F(UnifiedIRNodeTest, CreateSparqlNode) {
  auto sparqlNode = createSparqlNode(0);
  UnifiedIRNode node(42, std::move(sparqlNode));

  EXPECT_EQ(node.id_, 42);
  EXPECT_TRUE(node.isSparql());
  EXPECT_FALSE(node.isDatalog());
  EXPECT_FALSE(node.isShacl());
  EXPECT_STREQ(node.getTypeName(), "SPARQL");
}

TEST_F(UnifiedIRNodeTest, SparqlNodeVariableExtraction) {
  auto sparqlNode = createSparqlNode(0);
  UnifiedIRNode node(1, std::move(sparqlNode));

  const auto& vars = node.getVariables();
  EXPECT_EQ(vars.size(), 2);
  EXPECT_TRUE(vars.contains(Variable{"?x"}));
  EXPECT_TRUE(vars.contains(Variable{"?y"}));
}

TEST_F(UnifiedIRNodeTest, SparqlNodeAccessor) {
  auto sparqlNode = createSparqlNode(0);
  UnifiedIRNode node(1, std::move(sparqlNode));

  EXPECT_NO_THROW({
    const auto& retrieved = node.asSparql();
    EXPECT_EQ(retrieved._variables.size(), 2);
  });
}

TEST_F(UnifiedIRNodeTest, SparqlNodeThrowsOnWrongAccessor) {
  auto sparqlNode = createSparqlNode(0);
  UnifiedIRNode node(1, std::move(sparqlNode));

  EXPECT_THROW(node.asDatalog(), std::bad_variant_access);
  EXPECT_THROW(node.asShacl(), std::bad_variant_access);
}

// ============================================================================
// Datalog Node Tests
// ============================================================================

TEST_F(UnifiedIRNodeTest, CreateDatalogNode) {
  auto datalogRule = createDatalogRule();
  UnifiedIRNode node(42, std::move(datalogRule));

  EXPECT_EQ(node.id_, 42);
  EXPECT_FALSE(node.isSparql());
  EXPECT_TRUE(node.isDatalog());
  EXPECT_FALSE(node.isShacl());
  EXPECT_STREQ(node.getTypeName(), "Datalog");
}

TEST_F(UnifiedIRNodeTest, DatalogNodeVariableExtraction) {
  auto datalogRule = createDatalogRule();
  UnifiedIRNode node(1, std::move(datalogRule));

  const auto& vars = node.getVariables();
  EXPECT_EQ(vars.size(), 2);
  EXPECT_TRUE(vars.contains(Variable{"?x"}));
  EXPECT_TRUE(vars.contains(Variable{"?y"}));
}

TEST_F(UnifiedIRNodeTest, DatalogNodeAccessor) {
  auto datalogRule = createDatalogRule();
  UnifiedIRNode node(1, std::move(datalogRule));

  EXPECT_NO_THROW({
    const auto& retrieved = node.asDatalog();
    EXPECT_EQ(retrieved.getHeadPredicate(), "ancestor");
    EXPECT_EQ(retrieved.getArity(), 2);
  });
}

TEST_F(UnifiedIRNodeTest, DatalogNodeThrowsOnWrongAccessor) {
  auto datalogRule = createDatalogRule();
  UnifiedIRNode node(1, std::move(datalogRule));

  EXPECT_THROW(node.asSparql(), std::bad_variant_access);
  EXPECT_THROW(node.asShacl(), std::bad_variant_access);
}

// ============================================================================
// SHACL Node Tests
// ============================================================================

TEST_F(UnifiedIRNodeTest, CreateShaclNode) {
  auto propShape = createPropertyShape();
  UnifiedIRNode node(42, std::move(propShape));

  EXPECT_EQ(node.id_, 42);
  EXPECT_FALSE(node.isSparql());
  EXPECT_FALSE(node.isDatalog());
  EXPECT_TRUE(node.isShacl());
  EXPECT_STREQ(node.getTypeName(), "SHACL");
}

TEST_F(UnifiedIRNodeTest, ShaclNodeVariableExtraction) {
  auto propShape = createPropertyShape();
  UnifiedIRNode node(1, std::move(propShape));

  // SHACL nodes currently return empty variable set
  // (as per specification line 203-204)
  const auto& vars = node.getVariables();
  EXPECT_EQ(vars.size(), 0);
}

TEST_F(UnifiedIRNodeTest, ShaclNodeAccessor) {
  auto propShape = createPropertyShape();
  UnifiedIRNode node(1, std::move(propShape));

  EXPECT_NO_THROW({
    const auto& retrieved = node.asShacl();
    EXPECT_EQ(retrieved.path, "<http://example.org/name>");
    EXPECT_EQ(retrieved.constraints.size(), 1);
  });
}

TEST_F(UnifiedIRNodeTest, ShaclNodeThrowsOnWrongAccessor) {
  auto propShape = createPropertyShape();
  UnifiedIRNode node(1, std::move(propShape));

  EXPECT_THROW(node.asSparql(), std::bad_variant_access);
  EXPECT_THROW(node.asDatalog(), std::bad_variant_access);
}

// ============================================================================
// UnifiedIRGraph Tests
// ============================================================================

TEST_F(UnifiedIRNodeTest, GraphAddSparqlNode) {
  UnifiedIRGraph graph;
  auto sparqlNode = createSparqlNode(0);

  size_t id = graph.addSparqlNode(std::move(sparqlNode));

  EXPECT_EQ(id, 0);
  EXPECT_EQ(graph.getNodes().size(), 1);
  EXPECT_TRUE(graph.getNodes()[0].isSparql());
}

TEST_F(UnifiedIRNodeTest, GraphAddDatalogNode) {
  UnifiedIRGraph graph;
  auto datalogRule = createDatalogRule();

  size_t id = graph.addDatalogNode(std::move(datalogRule));

  EXPECT_EQ(id, 0);
  EXPECT_EQ(graph.getNodes().size(), 1);
  EXPECT_TRUE(graph.getNodes()[0].isDatalog());
}

TEST_F(UnifiedIRNodeTest, GraphAddShaclNode) {
  UnifiedIRGraph graph;
  auto propShape = createPropertyShape();

  size_t id = graph.addShaclNode(std::move(propShape));

  EXPECT_EQ(id, 0);
  EXPECT_EQ(graph.getNodes().size(), 1);
  EXPECT_TRUE(graph.getNodes()[0].isShacl());
}

TEST_F(UnifiedIRNodeTest, GraphMixedNodes) {
  UnifiedIRGraph graph;

  size_t id1 = graph.addSparqlNode(createSparqlNode(0));
  size_t id2 = graph.addDatalogNode(createDatalogRule());
  size_t id3 = graph.addShaclNode(createPropertyShape());

  EXPECT_EQ(id1, 0);
  EXPECT_EQ(id2, 1);
  EXPECT_EQ(id3, 2);
  EXPECT_EQ(graph.getNodes().size(), 3);

  auto counts = graph.countNodeTypes();
  EXPECT_EQ(counts.sparql, 1);
  EXPECT_EQ(counts.datalog, 1);
  EXPECT_EQ(counts.shacl, 1);
}

TEST_F(UnifiedIRNodeTest, GraphAddEdge) {
  UnifiedIRGraph graph;

  size_t id1 = graph.addSparqlNode(createSparqlNode(0));
  size_t id2 = graph.addDatalogNode(createDatalogRule());

  graph.addEdge(id1, id2);

  EXPECT_TRUE(graph.areConnected(id1, id2));
  EXPECT_TRUE(graph.areConnected(id2, id1));  // Bidirectional

  const auto& neighbors1 = graph.getNeighbors(id1);
  const auto& neighbors2 = graph.getNeighbors(id2);

  EXPECT_EQ(neighbors1.size(), 1);
  EXPECT_EQ(neighbors2.size(), 1);
  EXPECT_EQ(neighbors1[0], id2);
  EXPECT_EQ(neighbors2[0], id1);
}

TEST_F(UnifiedIRNodeTest, GraphNodeEquality) {
  auto sparqlNode1 = createSparqlNode(0);
  auto sparqlNode2 = createSparqlNode(0);

  UnifiedIRNode node1(42, std::move(sparqlNode1));
  UnifiedIRNode node2(42, std::move(sparqlNode2));
  UnifiedIRNode node3(99, createSparqlNode(0));

  EXPECT_EQ(node1, node2);  // Same ID
  EXPECT_NE(node1, node3);  // Different ID
}

TEST_F(UnifiedIRNodeTest, GraphVisitorPattern) {
  auto sparqlNode = createSparqlNode(0);
  UnifiedIRNode node(1, std::move(sparqlNode));

  // Test visitor pattern
  std::string result = node.visit([](const auto& payload) -> std::string {
    using T = std::decay_t<decltype(payload)>;
    if constexpr (std::is_same_v<T, QueryPlanner::TripleGraph::Node>) {
      return "SPARQL";
    } else if constexpr (std::is_same_v<T, DatalogRule>) {
      return "Datalog";
    } else if constexpr (std::is_same_v<T, shacl::PropertyShape>) {
      return "SHACL";
    }
    return "Unknown";
  });

  EXPECT_EQ(result, "SPARQL");
}

// ============================================================================
// Memory Layout Validation
// ============================================================================

TEST_F(UnifiedIRNodeTest, MemoryLayoutValidation) {
  // Verify memory layout matches specification (lines 488-501)
  // sizeof(UnifiedIRNode) should be reasonable (< 256 bytes)
  EXPECT_LT(sizeof(UnifiedIRNode), 256);

  // Verify node has expected members
  UnifiedIRNode node;
  EXPECT_EQ(node.id_, 0);
  EXPECT_EQ(node.getVariables().size(), 0);
}
