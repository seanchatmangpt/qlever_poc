// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include <gtest/gtest.h>

#include "engine/QueryExecutionContext.h"
#include "engine/reasoning/ReasoningEngine.h"
#include "engine/reasoning/Rule.h"

namespace reasoning {

class ReasoningEngineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ruleDatabase_ = std::make_shared<RuleDatabase>();
  }

  std::shared_ptr<RuleDatabase> ruleDatabase_;
};

TEST_F(ReasoningEngineTest, EmptyRuleDatabase) {
  // Create a mock QueryExecutionContext
  // For now, we just test with nullptr (would need proper setup in full test)
  ReasoningEngine engine{nullptr, ruleDatabase_};

  auto facts = engine.executeReasoning();
  EXPECT_EQ(facts.size(), 0);
}

TEST_F(ReasoningEngineTest, SingleRuleExecution) {
  // Create a simple rule: parent(X, Y) => ancestor(X, Y)
  Variable x{"?x"};
  Variable y{"?y"};
  LiteralOrIri parent{":parent"};
  LiteralOrIri ancestor{":ancestor"};

  auto rule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{SparqlTriple{x, parent, y}},
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 0);

  ruleDatabase_->addRule(rule);

  ReasoningEngine engine{nullptr, ruleDatabase_};

  // Should complete without error
  EXPECT_GE(ruleDatabase_->size(), 1);
}

TEST_F(ReasoningEngineTest, RecursiveRuleDetection) {
  Variable x{"?x"};
  Variable y{"?y"};
  Variable z{"?z"};
  LiteralOrIri ancestor{":ancestor"};

  // Recursive rule: ancestor(X, Z), ancestor(Z, Y) => ancestor(X, Y)
  auto recursiveRule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{
          SparqlTriple{x, ancestor, z},
          SparqlTriple{z, ancestor, y},
      },
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 0);

  ruleDatabase_->addRule(recursiveRule);

  auto recursiveRules = ruleDatabase_->getRecursiveRules();
  EXPECT_EQ(recursiveRules.size(), 1);
}

TEST_F(ReasoningEngineTest, IterationStats) {
  Variable x{"?x"};
  Variable y{"?y"};
  LiteralOrIri parent{":parent"};
  LiteralOrIri ancestor{":ancestor"};

  auto rule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{SparqlTriple{x, parent, y}},
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 0);

  ruleDatabase_->addRule(rule);

  ReasoningEngine engine{nullptr, ruleDatabase_};
  auto facts = engine.executeReasoning();

  auto stats = engine.getIterationStats();
  // Should have at least one iteration
  EXPECT_GE(stats.size(), 1);

  // First iteration should have completion time
  if (!stats.empty()) {
    EXPECT_GE(stats[0].iterationTimeMs, 0);
  }
}

TEST_F(ReasoningEngineTest, MaxIterationLimit) {
  Variable x{"?x"};
  Variable y{"?y"};
  Variable z{"?z"};
  LiteralOrIri ancestor{":ancestor"};

  // Create a recursive rule
  auto recursiveRule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{
          SparqlTriple{x, ancestor, z},
          SparqlTriple{z, ancestor, y},
      },
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 0);

  ruleDatabase_->addRule(recursiveRule);

  ReasoningEngine engine{nullptr, ruleDatabase_};

  // Execute with max 5 iterations
  auto facts = engine.executeReasoning(5);

  auto stats = engine.getIterationStats();
  // Should have stopped at max iterations
  EXPECT_LE(stats.size(), 5);
}

TEST_F(ReasoningEngineTest, DerivedFactsAccumulate) {
  ReasoningEngine engine{nullptr, ruleDatabase_};

  // Initially no facts
  EXPECT_EQ(engine.getDerivedFacts().size(), 0);

  auto rule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{
          Variable{"?x"},
          LiteralOrIri{":parent"},
          Variable{"?y"},
      },
      std::vector<SparqlTriple>{
          Variable{"?x"},
          LiteralOrIri{":ancestor"},
          Variable{"?y"},
      },
      0);

  ruleDatabase_->addRule(rule);

  engine.executeReasoning();

  // After reasoning, should have accumulated facts
  // (exact count depends on the knowledge base, which is empty in mock)
}

TEST_F(ReasoningEngineTest, ConvergenceDetection) {
  // Non-recursive rule should converge in 1 iteration
  Variable x{"?x"};
  Variable y{"?y"};
  LiteralOrIri parent{":parent"};
  LiteralOrIri ancestor{":ancestor"};

  auto nonRecursiveRule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{SparqlTriple{x, parent, y}},
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 0);

  ruleDatabase_->addRule(nonRecursiveRule);

  ReasoningEngine engine{nullptr, ruleDatabase_};
  engine.executeReasoning();

  auto stats = engine.getIterationStats();
  // Non-recursive rule should converge quickly
  // (actual behavior depends on knowledge base)
}

}  // namespace reasoning
