// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: EPIC 14.0 Agent 6 - Rule-Level Determinism Tests
//
// Purpose: Test rule-level determinism analysis for Datalog and N3
// This is a new capability not present in the original DeterminismClassifier

#include <gtest/gtest.h>

#include "engine/formalism/unified/UnifiedDeterminismClassifier.h"
#include "parser/DatalogRule.h"
#include "parser/sparqlExpressions/NaryExpression.h"
#include "parser/sparqlExpressions/NowDatetimeExpression.h"
#include "parser/sparqlExpressions/RandomExpression.h"

using namespace formalism::unified;

namespace {

// Helper to create a basic Datalog rule
DatalogRule createBasicRule(const std::string& headPredicate,
                            const std::vector<Variable>& headVars,
                            const std::vector<SparqlTriple>& bodyPatterns,
                            bool isRecursive = false) {
  return DatalogRule(headPredicate, headVars, bodyPatterns, {}, isRecursive);
}

// Helper to create a filter expression descriptor
std::string createFilterDescriptor(const std::string& funcName) {
  return funcName + "()";
}

}  // namespace

class RuleLevelDeterminismTest : public ::testing::Test {};

// ===========================================================================
// T1: Single Datalog Rule - Deterministic
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, SingleRuleDeterministic) {
  // Rule: sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y).
  Variable varX("?x");
  Variable varY("?y");
  Variable varP("?p");

  TripleComponent::Iri parentPred(
      ad_utility::triple_component::Iri::fromIriref("<parent>"));

  SparqlTriple triple1(varP, parentPred, varX);
  SparqlTriple triple2(varP, parentPred, varY);

  DatalogRule rule("sibling", {varX, varY}, {triple1, triple2}, {}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.isDeterministic());
  EXPECT_FALSE(features.hasDatalogRecursion);
  EXPECT_FALSE(features.hasNow);
  EXPECT_FALSE(features.hasRand);
  EXPECT_FALSE(features.hasDatalogNegation);
}

// ===========================================================================
// T2: Single Datalog Rule - Recursive (Deterministic)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, SingleRuleRecursiveDeterministic) {
  // Rule: reachable(?x, ?y) :- edge(?x, ?y).
  Variable varX("?x");
  Variable varY("?y");

  TripleComponent::Iri edgePred(
      ad_utility::triple_component::Iri::fromIriref("<edge>"));

  SparqlTriple triple(varX, edgePred, varY);

  DatalogRule rule("reachable", {varX, varY}, {triple}, {}, true);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.isDeterministic());  // Recursion alone is deterministic
  EXPECT_TRUE(features.hasDatalogRecursion);
}

// ===========================================================================
// T3: Single Datalog Rule - With Filter (Deterministic)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, SingleRuleWithDeterministicFilter) {
  // Rule: adult(?x) :- person(?x), age(?x, ?a), FILTER(?a > 18).
  Variable varX("?x");
  Variable varA("?a");

  TripleComponent::Iri personPred(
      ad_utility::triple_component::Iri::fromIriref("<person>"));
  TripleComponent::Iri agePred(
      ad_utility::triple_component::Iri::fromIriref("<age>"));

  SparqlTriple triple1(varX, personPred, varX);
  SparqlTriple triple2(varX, agePred, varA);

  // Create a deterministic filter (comparison)
  // Note: This is a simplified test; actual filter construction would be more
  // complex
  auto filterExpr = sparqlExpression::makeNaryExpression(
      sparqlExpression::naryExpression::AndExpression{});
  SparqlFilter filter(std::move(filterExpr));

  DatalogRule rule("adult", {varX}, {triple1, triple2}, {filter}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.isDeterministic());  // Comparison filter is
                                            // deterministic
}

// ===========================================================================
// T4: Single Datalog Rule - With NOW() (Non-Deterministic)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, SingleRuleWithNowFunction) {
  // Rule: recent(?x) :- event(?x), timestamp(?x, ?t), FILTER(?t > NOW()).
  Variable varX("?x");
  Variable varT("?t");

  TripleComponent::Iri eventPred(
      ad_utility::triple_component::Iri::fromIriref("<event>"));
  TripleComponent::Iri timestampPred(
      ad_utility::triple_component::Iri::fromIriref("<timestamp>"));

  SparqlTriple triple1(varX, eventPred, varX);
  SparqlTriple triple2(varX, timestampPred, varT);

  // Create filter with NOW()
  auto nowExpr = sparqlExpression::makeNowDatetimeExpression();
  SparqlFilter filter(std::move(nowExpr));

  DatalogRule rule("recent", {varX}, {triple1, triple2}, {filter}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_FALSE(
      features.isDeterministic());  // NOW() makes it non-deterministic
  EXPECT_TRUE(features.hasNow);
}

// ===========================================================================
// T5: Single Datalog Rule - With RAND() (Non-Deterministic)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, SingleRuleWithRandFunction) {
  // Rule: sample(?x) :- data(?x), FILTER(RAND() < 0.1).
  Variable varX("?x");

  TripleComponent::Iri dataPred(
      ad_utility::triple_component::Iri::fromIriref("<data>"));

  SparqlTriple triple(varX, dataPred, varX);

  // Create filter with RAND()
  auto randExpr = sparqlExpression::makeRandomExpression();
  SparqlFilter filter(std::move(randExpr));

  DatalogRule rule("sample", {varX}, {triple}, {filter}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_FALSE(
      features.isDeterministic());  // RAND() makes it non-deterministic
  EXPECT_TRUE(features.hasRand);
}

// ===========================================================================
// T6: Datalog Program - Multiple Rules (All Deterministic)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, ProgramAllRulesDeterministic) {
  Variable varX("?x");
  Variable varY("?y");
  Variable varZ("?z");

  TripleComponent::Iri parentPred(
      ad_utility::triple_component::Iri::fromIriref("<parent>"));
  TripleComponent::Iri ancestorPred(
      ad_utility::triple_component::Iri::fromIriref("<ancestor>"));

  // Rule 1: ancestor(?x, ?y) :- parent(?x, ?y).
  SparqlTriple triple1(varX, parentPred, varY);
  DatalogRule rule1("ancestor", {varX, varY}, {triple1}, {}, false);

  // Rule 2: ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
  SparqlTriple triple2a(varX, parentPred, varY);
  SparqlTriple triple2b(varY, ancestorPred, varZ);
  DatalogRule rule2("ancestor", {varX, varZ}, {triple2a, triple2b}, {}, true);

  // Rule 3: sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y).
  Variable varP("?p");
  SparqlTriple triple3a(varP, parentPred, varX);
  SparqlTriple triple3b(varP, parentPred, varY);
  DatalogRule rule3("sibling", {varX, varY}, {triple3a, triple3b}, {}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogProgram({rule1, rule2, rule3});

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.isDeterministic());
  EXPECT_TRUE(features.hasDatalogRecursion);  // rule2 is recursive
}

// ===========================================================================
// T7: Datalog Program - One Non-Deterministic Rule
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, ProgramOneNonDeterministicRule) {
  Variable varX("?x");
  Variable varY("?y");

  TripleComponent::Iri parentPred(
      ad_utility::triple_component::Iri::fromIriref("<parent>"));

  // Rule 1: ancestor(?x, ?y) :- parent(?x, ?y).
  SparqlTriple triple1(varX, parentPred, varY);
  DatalogRule rule1("ancestor", {varX, varY}, {triple1}, {}, false);

  // Rule 2: sample(?x) :- data(?x), FILTER(RAND() < 0.5).
  TripleComponent::Iri dataPred(
      ad_utility::triple_component::Iri::fromIriref("<data>"));
  SparqlTriple triple2(varX, dataPred, varX);
  auto randExpr = sparqlExpression::makeRandomExpression();
  SparqlFilter filter(std::move(randExpr));
  DatalogRule rule2("sample", {varX}, {triple2}, {filter}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogProgram({rule1, rule2});

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_FALSE(
      features.isDeterministic());  // One non-deterministic rule → whole
                                    // program non-deterministic
  EXPECT_TRUE(features.hasRand);
}

// ===========================================================================
// T8: N3 Document - Deterministic Implication
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, N3DeterministicImplication) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .
    @prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .

    # Simple deterministic implication
    { ?x a :Person } => { ?x a :Agent } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Implications);
  EXPECT_FALSE(features.hasN3BuiltIns);
  EXPECT_TRUE(features.isDeterministic());
}

// ===========================================================================
// T9: N3 Document - Multiple Deterministic Rules
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, N3MultipleRulesDeterministic) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .

    # Rule 1: Person implies Agent
    { ?x a :Person } => { ?x a :Agent } .

    # Rule 2: Employee implies Person
    { ?x a :Employee } => { ?x a :Person } .

    # Rule 3: Transitive property
    { ?x :parent ?y . ?y :parent ?z } => { ?x :grandparent ?z } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Implications);
  EXPECT_FALSE(features.hasN3BuiltIns);
  EXPECT_TRUE(features.isDeterministic());
}

// ===========================================================================
// T10: N3 Document - Non-Deterministic Built-in (math:)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, N3WithMathBuiltIn) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .
    @prefix math: <http://www.w3.org/2000/10/swap/math#> .

    # Rule with math built-in
    { ?x :value ?a . ?y :value ?b . (?a ?b) math:sum ?c }
      => { ?x :sumsWith ?y :equals ?c } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3BuiltIns);
  EXPECT_FALSE(features.isDeterministic());  // Conservative: built-ins are
                                             // non-deterministic
}

// ===========================================================================
// T11: N3 Document - Non-Deterministic Built-in (time:)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, N3WithTimeBuiltIn) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .
    @prefix time: <http://www.w3.org/2000/10/swap/time#> .

    # Rule with time built-in (non-deterministic)
    { ?x time:now ?t } => { ?x :currentTime ?t } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3BuiltIns);
  EXPECT_FALSE(features.isDeterministic());
}

// ===========================================================================
// T12: N3 Document - Formulae (Deterministic)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, N3WithFormulae) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .

    # Belief context using formulae
    :alice :believes { :bob :knows :carol } .
    :bob :believes { :carol :knows :alice } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Formulae);
  EXPECT_FALSE(features.hasN3BuiltIns);
  EXPECT_TRUE(features.isDeterministic());  // Formulae alone are deterministic
}

// ===========================================================================
// T13: N3 Document - Quantifiers (Deterministic)
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, N3WithQuantifiers) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .

    # Universal quantification
    @forAll :x, :y .
    { :x :parent :y } => { :x :ancestor :y } .

    # Existential quantification
    @forSome :z .
    { :alice :knows :z } => { :alice :hasFriend :z } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Quantifiers);
  EXPECT_TRUE(features.hasN3Implications);
  EXPECT_FALSE(features.hasN3BuiltIns);
  EXPECT_TRUE(features.isDeterministic());
}

// ===========================================================================
// T14: Fail-Closed Caching - Rule-Level
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, FailClosedCachingRule) {
  // Deterministic rule
  Variable varX("?x");
  Variable varY("?y");
  TripleComponent::Iri parentPred(
      ad_utility::triple_component::Iri::fromIriref("<parent>"));
  SparqlTriple triple(varX, parentPred, varY);
  DatalogRule detRule("parent", {varX, varY}, {triple}, {}, false);

  UnifiedDeterminismClassifier classifier;
  auto detFeatures = classifier.analyzeDatalogRule(detRule);

  EXPECT_TRUE(classifier.isCacheable(detFeatures));
  EXPECT_TRUE(DeterminismContract::satisfiesCachingContract(detFeatures));

  // Non-deterministic rule with RAND()
  auto randExpr = sparqlExpression::makeRandomExpression();
  SparqlFilter filter(std::move(randExpr));
  DatalogRule nonDetRule("sample", {varX}, {triple}, {filter}, false);

  auto nonDetFeatures = classifier.analyzeDatalogRule(nonDetRule);

  EXPECT_FALSE(classifier.isCacheable(nonDetFeatures));
  EXPECT_FALSE(DeterminismContract::satisfiesCachingContract(nonDetFeatures));
}

// ===========================================================================
// T15: Fail-Closed Caching - Program-Level
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, FailClosedCachingProgram) {
  Variable varX("?x");
  Variable varY("?y");

  TripleComponent::Iri parentPred(
      ad_utility::triple_component::Iri::fromIriref("<parent>"));

  // All deterministic rules
  SparqlTriple triple1(varX, parentPred, varY);
  DatalogRule rule1("parent", {varX, varY}, {triple1}, {}, false);

  SparqlTriple triple2(varX, parentPred, varY);
  DatalogRule rule2("sibling", {varX, varY}, {triple2}, {}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogProgram({rule1, rule2});

  EXPECT_TRUE(classifier.isCacheable(features));

  // Add one non-deterministic rule
  auto randExpr = sparqlExpression::makeRandomExpression();
  SparqlFilter filter(std::move(randExpr));
  DatalogRule rule3("sample", {varX}, {triple1}, {filter}, false);

  auto featuresWithNonDet = classifier.analyzeDatalogProgram({rule1, rule2, rule3});

  EXPECT_FALSE(classifier.isCacheable(featuresWithNonDet));
  EXPECT_FALSE(
      DeterminismContract::satisfiesCachingContract(featuresWithNonDet));
}

// ===========================================================================
// T16: Contract Violation Report - Rule-Level
// ===========================================================================

TEST_F(RuleLevelDeterminismTest, ContractViolationReportRule) {
  Variable varX("?x");
  TripleComponent::Iri dataPred(
      ad_utility::triple_component::Iri::fromIriref("<data>"));
  SparqlTriple triple(varX, dataPred, varX);

  auto nowExpr = sparqlExpression::makeNowDatetimeExpression();
  SparqlFilter filter(std::move(nowExpr));
  DatalogRule rule("recent", {varX}, {triple}, {filter}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  std::string report = DeterminismContract::generateViolationReport(features);

  EXPECT_TRUE(report.find("DETERMINISM CONTRACT VIOLATION") !=
              std::string::npos);
  EXPECT_TRUE(report.find("DATALOG") != std::string::npos);
  EXPECT_TRUE(report.find("NOW() function") != std::string::npos);
  EXPECT_TRUE(report.find("REJECTED") != std::string::npos);
  EXPECT_TRUE(report.find("fail-closed") != std::string::npos);
}
