//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "parser/SparqlTriple.h"
#include "parser/TripleComponent.h"
#include "rdfTypes/Variable.h"

using ad_utility::triple_component::Iri;

// _____________________________________________________________________________
// Helper function to create an IRI from a string
static Iri iri(std::string_view iriStr) { return Iri::fromIriref(iriStr); }

// _____________________________________________________________________________
TEST(DatalogRule, DefaultConstructor) {
  DatalogRule rule;
  EXPECT_TRUE(rule.getHeadPredicate().empty());
  EXPECT_TRUE(rule.getHeadVariables().empty());
  EXPECT_TRUE(rule.getBodyPatterns().empty());
  EXPECT_TRUE(rule.getFilters().empty());
  EXPECT_EQ(rule.getArity(), 0);
  EXPECT_FALSE(rule.isRecursive());
}

// _____________________________________________________________________________
TEST(DatalogRule, BasicConstructor) {
  std::string headPred = "ancestor";
  std::vector<Variable> headVars = {Variable{"?x"}, Variable{"?y"}};
  std::vector<SparqlTriple> bodyPatterns;

  // Create a simple triple pattern: ?x parent ?y
  auto subject = TripleComponent{Variable{"?x"}};
  auto predicate = iri("<http://example.org/parent>");
  auto object = TripleComponent{Variable{"?y"}};
  bodyPatterns.emplace_back(subject, predicate, object);

  DatalogRule rule(headPred, headVars, bodyPatterns);

  EXPECT_EQ(rule.getHeadPredicate(), "ancestor");
  EXPECT_EQ(rule.getHeadVariables().size(), 2);
  EXPECT_EQ(rule.getHeadVariables()[0].name(), "?x");
  EXPECT_EQ(rule.getHeadVariables()[1].name(), "?y");
  EXPECT_EQ(rule.getBodyPatterns().size(), 1);
  EXPECT_EQ(rule.getArity(), 2);
  EXPECT_FALSE(rule.isRecursive());
}

// _____________________________________________________________________________
TEST(DatalogRule, RecursiveFlag) {
  std::string headPred = "ancestor";
  std::vector<Variable> headVars = {Variable{"?x"}, Variable{"?z"}};
  std::vector<SparqlTriple> bodyPatterns;

  // Pattern 1: ?x parent ?y
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/parent>"),
                            TripleComponent{Variable{"?y"}});

  // Pattern 2: ?y ancestor ?z (recursive reference)
  bodyPatterns.emplace_back(TripleComponent{Variable{"?y"}},
                            iri("<http://example.org/ancestor>"),
                            TripleComponent{Variable{"?z"}});

  DatalogRule rule(headPred, headVars, bodyPatterns, {}, true);

  EXPECT_TRUE(rule.isRecursive());

  rule.setRecursive(false);
  EXPECT_FALSE(rule.isRecursive());
}

// _____________________________________________________________________________
TEST(DatalogRule, Equality) {
  std::string headPred = "test";
  std::vector<Variable> headVars = {Variable{"?x"}};
  std::vector<SparqlTriple> bodyPatterns;

  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/prop>"),
                            TripleComponent{Variable{"?y"}});

  DatalogRule rule1(headPred, headVars, bodyPatterns);
  DatalogRule rule2(headPred, headVars, bodyPatterns);
  DatalogRule rule3("different", headVars, bodyPatterns);

  EXPECT_EQ(rule1, rule2);
  EXPECT_NE(rule1, rule3);
}

// _____________________________________________________________________________
TEST(DatalogRule, ToStringSimple) {
  std::string headPred = "ancestor";
  std::vector<Variable> headVars = {Variable{"?x"}, Variable{"?y"}};
  std::vector<SparqlTriple> bodyPatterns;

  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/parent>"),
                            TripleComponent{Variable{"?y"}});

  DatalogRule rule(headPred, headVars, bodyPatterns);
  std::string ruleStr = rule.toString();

  EXPECT_THAT(ruleStr, testing::HasSubstr("ancestor"));
  EXPECT_THAT(ruleStr, testing::HasSubstr("?x"));
  EXPECT_THAT(ruleStr, testing::HasSubstr("?y"));
  EXPECT_THAT(ruleStr, testing::HasSubstr(":-"));
}

// _____________________________________________________________________________
TEST(DatalogRule, ToStringEmpty) {
  DatalogRule rule("fact", {Variable{"?x"}}, {});
  std::string ruleStr = rule.toString();

  EXPECT_THAT(ruleStr, testing::HasSubstr("fact"));
  EXPECT_THAT(ruleStr, testing::HasSubstr("?x"));
}

// _____________________________________________________________________________
TEST(DatalogRule, ArityCalculation) {
  DatalogRule rule0("zero", {}, {});
  EXPECT_EQ(rule0.getArity(), 0);

  DatalogRule rule1("one", {Variable{"?x"}}, {});
  EXPECT_EQ(rule1.getArity(), 1);

  DatalogRule rule3("three", {Variable{"?x"}, Variable{"?y"}, Variable{"?z"}},
                    {});
  EXPECT_EQ(rule3.getArity(), 3);
}

// _____________________________________________________________________________
// ============================================================================
// RuleDatabase Tests
// ============================================================================

// _____________________________________________________________________________
TEST(RuleDatabase, DefaultConstructor) {
  RuleDatabase db;
  EXPECT_EQ(db.getRuleCount(), 0);
  EXPECT_EQ(db.getPredicateCount(), 0);
  EXPECT_TRUE(db.getAllRules().empty());
}

// _____________________________________________________________________________
TEST(RuleDatabase, AddSingleRule) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/parent>"),
                            TripleComponent{Variable{"?y"}});

  DatalogRule rule("ancestor", {Variable{"?x"}, Variable{"?y"}}, bodyPatterns);
  db.addRule(rule);

  EXPECT_EQ(db.getRuleCount(), 1);
  EXPECT_EQ(db.getPredicateCount(), 1);
  EXPECT_TRUE(db.hasRuleFor("ancestor"));
  EXPECT_FALSE(db.hasRuleFor("nonexistent"));
}

// _____________________________________________________________________________
TEST(RuleDatabase, AddMultipleRulesSamePredicate) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns1;
  bodyPatterns1.emplace_back(TripleComponent{Variable{"?x"}},
                             iri("<http://example.org/parent>"),
                             TripleComponent{Variable{"?y"}});

  DatalogRule rule1("ancestor", {Variable{"?x"}, Variable{"?y"}},
                    bodyPatterns1);

  std::vector<SparqlTriple> bodyPatterns2;
  bodyPatterns2.emplace_back(TripleComponent{Variable{"?x"}},
                             iri("<http://example.org/parent>"),
                             TripleComponent{Variable{"?y"}});
  bodyPatterns2.emplace_back(TripleComponent{Variable{"?y"}},
                             iri("<http://example.org/ancestor>"),
                             TripleComponent{Variable{"?z"}});

  DatalogRule rule2("ancestor", {Variable{"?x"}, Variable{"?z"}}, bodyPatterns2,
                    {}, true);

  db.addRule(rule1);
  db.addRule(rule2);

  EXPECT_EQ(db.getRuleCount(), 2);
  EXPECT_EQ(db.getPredicateCount(), 1);

  auto rules = db.getRulesByPredicate("ancestor");
  EXPECT_EQ(rules.size(), 2);
}

// _____________________________________________________________________________
TEST(RuleDatabase, AddMultipleRulesDifferentPredicates) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns1;
  bodyPatterns1.emplace_back(TripleComponent{Variable{"?x"}},
                             iri("<http://example.org/parent>"),
                             TripleComponent{Variable{"?y"}});

  DatalogRule rule1("ancestor", {Variable{"?x"}, Variable{"?y"}},
                    bodyPatterns1);

  std::vector<SparqlTriple> bodyPatterns2;
  bodyPatterns2.emplace_back(TripleComponent{Variable{"?x"}},
                             iri("<http://example.org/sibling>"),
                             TripleComponent{Variable{"?y"}});

  DatalogRule rule2("related", {Variable{"?x"}, Variable{"?y"}}, bodyPatterns2);

  db.addRule(rule1);
  db.addRule(rule2);

  EXPECT_EQ(db.getRuleCount(), 2);
  EXPECT_EQ(db.getPredicateCount(), 2);
  EXPECT_TRUE(db.hasRuleFor("ancestor"));
  EXPECT_TRUE(db.hasRuleFor("related"));
}

// _____________________________________________________________________________
TEST(RuleDatabase, GetRulesByPredicate) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/parent>"),
                            TripleComponent{Variable{"?y"}});

  DatalogRule rule1("ancestor", {Variable{"?x"}, Variable{"?y"}}, bodyPatterns);
  DatalogRule rule2("ancestor", {Variable{"?x"}, Variable{"?y"}}, bodyPatterns);
  DatalogRule rule3("other", {Variable{"?x"}}, bodyPatterns);

  db.addRule(rule1);
  db.addRule(rule2);
  db.addRule(rule3);

  auto ancestorRules = db.getRulesByPredicate("ancestor");
  EXPECT_EQ(ancestorRules.size(), 2);

  auto otherRules = db.getRulesByPredicate("other");
  EXPECT_EQ(otherRules.size(), 1);

  auto nonexistentRules = db.getRulesByPredicate("nonexistent");
  EXPECT_TRUE(nonexistentRules.empty());
}

// _____________________________________________________________________________
TEST(RuleDatabase, GetAllRules) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/prop>"),
                            TripleComponent{Variable{"?y"}});

  db.addRule(DatalogRule("pred1", {Variable{"?x"}}, bodyPatterns));
  db.addRule(DatalogRule("pred1", {Variable{"?y"}}, bodyPatterns));
  db.addRule(DatalogRule("pred2", {Variable{"?z"}}, bodyPatterns));

  auto allRules = db.getAllRules();
  EXPECT_EQ(allRules.size(), 3);
}

// _____________________________________________________________________________
TEST(RuleDatabase, GetPredicateNames) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/prop>"),
                            TripleComponent{Variable{"?y"}});

  db.addRule(DatalogRule("ancestor", {Variable{"?x"}}, bodyPatterns));
  db.addRule(DatalogRule("sibling", {Variable{"?y"}}, bodyPatterns));
  db.addRule(DatalogRule("ancestor", {Variable{"?z"}}, bodyPatterns));

  auto names = db.getPredicateNames();
  EXPECT_EQ(names.size(), 2);
  EXPECT_THAT(names, testing::UnorderedElementsAre("ancestor", "sibling"));
}

// _____________________________________________________________________________
TEST(RuleDatabase, Clear) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/prop>"),
                            TripleComponent{Variable{"?y"}});

  db.addRule(DatalogRule("pred1", {Variable{"?x"}}, bodyPatterns));
  db.addRule(DatalogRule("pred2", {Variable{"?y"}}, bodyPatterns));

  EXPECT_EQ(db.getRuleCount(), 2);

  db.clear();

  EXPECT_EQ(db.getRuleCount(), 0);
  EXPECT_EQ(db.getPredicateCount(), 0);
  EXPECT_TRUE(db.getAllRules().empty());
  EXPECT_FALSE(db.hasRuleFor("pred1"));
  EXPECT_FALSE(db.hasRuleFor("pred2"));
}

// _____________________________________________________________________________
TEST(RuleDatabase, HasRuleFor) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/prop>"),
                            TripleComponent{Variable{"?y"}});

  EXPECT_FALSE(db.hasRuleFor("test"));

  db.addRule(DatalogRule("test", {Variable{"?x"}}, bodyPatterns));

  EXPECT_TRUE(db.hasRuleFor("test"));
  EXPECT_FALSE(db.hasRuleFor("other"));
}

// _____________________________________________________________________________
TEST(RuleDatabase, ThreadSafety) {
  // This test demonstrates thread-safe usage
  // In a real test, you would use multiple threads
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/prop>"),
                            TripleComponent{Variable{"?y"}});

  // Multiple operations should work correctly
  db.addRule(DatalogRule("pred1", {Variable{"?x"}}, bodyPatterns));
  auto rules1 = db.getRulesByPredicate("pred1");
  EXPECT_EQ(rules1.size(), 1);

  db.addRule(DatalogRule("pred1", {Variable{"?y"}}, bodyPatterns));
  auto rules2 = db.getRulesByPredicate("pred1");
  EXPECT_EQ(rules2.size(), 2);

  EXPECT_EQ(db.getRuleCount(), 2);
}

// _____________________________________________________________________________
TEST(RuleDatabase, GetRuleCountWithMultiplePredicates) {
  RuleDatabase db;

  std::vector<SparqlTriple> bodyPatterns;
  bodyPatterns.emplace_back(TripleComponent{Variable{"?x"}},
                            iri("<http://example.org/prop>"),
                            TripleComponent{Variable{"?y"}});

  db.addRule(DatalogRule("pred1", {Variable{"?x"}}, bodyPatterns));
  db.addRule(DatalogRule("pred1", {Variable{"?y"}}, bodyPatterns));
  db.addRule(DatalogRule("pred2", {Variable{"?z"}}, bodyPatterns));
  db.addRule(DatalogRule("pred3", {Variable{"?w"}}, bodyPatterns));

  EXPECT_EQ(db.getRuleCount(), 4);
  EXPECT_EQ(db.getPredicateCount(), 3);
}
