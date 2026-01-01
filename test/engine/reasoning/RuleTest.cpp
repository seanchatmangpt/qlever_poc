// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include <gtest/gtest.h>

#include "engine/reasoning/Rule.h"
#include "parser/SparqlTriple.h"

namespace reasoning {

class RuleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create test triples for rules
    // Example: parent(X, Z), ancestor(Z, Y) => ancestor(X, Y)
  }
};

TEST_F(RuleTest, RuleCreationAndAccess) {
  // Create body and head triples
  Variable x{"?x"};
  Variable y{"?y"};
  Variable z{"?z"};
  LiteralOrIri parent{":parent"};
  LiteralOrIri ancestor{":ancestor"};

  std::vector<SparqlTriple> body = {
      SparqlTriple{x, parent, z},
      SparqlTriple{z, ancestor, y},
  };

  std::vector<SparqlTriple> head = {
      SparqlTriple{x, ancestor, y},
  };

  Rule rule{body, head, 0};

  EXPECT_EQ(rule.getBody().size(), 2);
  EXPECT_EQ(rule.getHead().size(), 1);
  EXPECT_EQ(rule.getRuleId(), 0);
}

TEST_F(RuleTest, RecursiveRuleDetection) {
  Variable x{"?x"};
  Variable y{"?y"};
  Variable z{"?z"};
  LiteralOrIri ancestor{":ancestor"};
  LiteralOrIri parent{":parent"};

  // Recursive rule: ancestor(X, Z), ancestor(Z, Y) => ancestor(X, Y)
  std::vector<SparqlTriple> recursiveBody = {
      SparqlTriple{x, ancestor, z},
      SparqlTriple{z, ancestor, y},
  };

  std::vector<SparqlTriple> recursiveHead = {
      SparqlTriple{x, ancestor, y},
  };

  Rule recursiveRule{recursiveBody, recursiveHead, 1};
  EXPECT_TRUE(recursiveRule.isRecursive());

  // Non-recursive rule: parent(X, Z) => ancestor(X, Z)
  std::vector<SparqlTriple> nonRecursiveBody = {
      SparqlTriple{x, parent, z},
  };

  std::vector<SparqlTriple> nonRecursiveHead = {
      SparqlTriple{x, ancestor, z},
  };

  Rule nonRecursiveRule{nonRecursiveBody, nonRecursiveHead, 2};
  EXPECT_FALSE(nonRecursiveRule.isRecursive());
}

TEST_F(RuleTest, RuleDatabaseOperations) {
  RuleDatabase db;

  Variable x{"?x"};
  Variable y{"?y"};
  LiteralOrIri ancestor{":ancestor"};
  LiteralOrIri parent{":parent"};

  // Add some rules
  auto rule1 = std::make_shared<Rule>(
      std::vector<SparqlTriple>{SparqlTriple{x, parent, y}},
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 0);

  auto rule2 = std::make_shared<Rule>(
      std::vector<SparqlTriple>{
          SparqlTriple{x, ancestor, Variable{"?z"}},
          SparqlTriple{Variable{"?z"}, ancestor, y},
      },
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 1);

  db.addRule(rule1);
  db.addRule(rule2);

  EXPECT_EQ(db.size(), 2);
  EXPECT_EQ(db.getRules().size(), 2);

  // Get rules for ancestor predicate
  auto ancestorRules = db.getRulesForPredicate(ancestor);
  EXPECT_EQ(ancestorRules.size(), 2);

  // Get recursive rules
  auto recursiveRules = db.getRecursiveRules();
  EXPECT_EQ(recursiveRules.size(), 1);

  // Clear database
  db.clear();
  EXPECT_EQ(db.size(), 0);
}

TEST_F(RuleTest, RuleDatabasePredicateFiltering) {
  RuleDatabase db;

  Variable x{"?x"};
  Variable y{"?y"};
  LiteralOrIri ancestor{":ancestor"};
  LiteralOrIri sibling{":sibling"};
  LiteralOrIri parent{":parent"};

  // Add rule that derives ancestor
  auto ancestorRule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{SparqlTriple{x, parent, y}},
      std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}}, 0);

  // Add rule that derives sibling
  auto siblingRule = std::make_shared<Rule>(
      std::vector<SparqlTriple>{
          SparqlTriple{x, parent, Variable{"?p"}},
          SparqlTriple{Variable{"?p"}, parent, y},
      },
      std::vector<SparqlTriple>{SparqlTriple{x, sibling, y}}, 1);

  db.addRule(ancestorRule);
  db.addRule(siblingRule);

  // Query for ancestor rules
  auto ancestorRules = db.getRulesForPredicate(ancestor);
  EXPECT_EQ(ancestorRules.size(), 1);

  // Query for sibling rules
  auto siblingRules = db.getRulesForPredicate(sibling);
  EXPECT_EQ(siblingRules.size(), 1);

  // Query for non-existent predicate
  auto unknownRules = db.getRulesForPredicate(LiteralOrIri{":unknown"});
  EXPECT_EQ(unknownRules.size(), 0);
}

}  // namespace reasoning
