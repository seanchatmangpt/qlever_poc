// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: EPIC 14.0 Agent 6 - Unified Determinism Classification Tests

#include <gtest/gtest.h>

#include "engine/formalism/unified/UnifiedDeterminismClassifier.h"
#include "engine/queryCanonical/QueryFingerprint.h"
#include "index/EncodedIriManager.h"
#include "parser/DatalogRule.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlParser.h"

using namespace formalism::unified;

namespace {

// Helper to parse a SPARQL query string
ParsedQuery parseQuery(const std::string& queryString) {
  static EncodedIriManager encodedIriManager;
  return SparqlParser::parseQuery(&encodedIriManager, queryString);
}

}  // namespace

class UnifiedDeterminismClassifierTest : public ::testing::Test {};

// ===========================================================================
// T1: SPARQL Determinism (Delegation to DeterminismClassifier)
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, SparqlDeterministicQuery) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeSparqlQuery(parsed);

  EXPECT_EQ(features.formalism, FormalismType::SPARQL);
  EXPECT_TRUE(features.isDeterministic());
  EXPECT_FALSE(features.hasNow);
  EXPECT_FALSE(features.hasRand);
  EXPECT_FALSE(features.hasService);
}

TEST_F(UnifiedDeterminismClassifierTest, SparqlQueryWithNow) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(NOW() AS ?time)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeSparqlQuery(parsed);

  EXPECT_EQ(features.formalism, FormalismType::SPARQL);
  EXPECT_FALSE(features.isDeterministic());
  EXPECT_TRUE(features.hasNow);
}

TEST_F(UnifiedDeterminismClassifierTest, SparqlQueryWithRand) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      FILTER(RAND() < 0.5)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeSparqlQuery(parsed);

  EXPECT_EQ(features.formalism, FormalismType::SPARQL);
  EXPECT_FALSE(features.isDeterministic());
  EXPECT_TRUE(features.hasRand);
}

TEST_F(UnifiedDeterminismClassifierTest, SparqlQueryWithService) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      SERVICE <http://example.org/sparql> {
        ?x ?q ?r
      }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeSparqlQuery(parsed);

  EXPECT_EQ(features.formalism, FormalismType::SPARQL);
  EXPECT_FALSE(features.isDeterministic());
  EXPECT_TRUE(features.hasService);
}

// ===========================================================================
// T2: Datalog Rule-Level Determinism
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, DatalogDeterministicRule) {
  // Rule: parent(?x, ?y) :- hasChild(?x, ?y).
  Variable varX("?x");
  Variable varY("?y");

  TripleComponent::Iri hasChildPred(
      ad_utility::triple_component::Iri::fromIriref("<hasChild>"));
  SparqlTriple triple(varX, hasChildPred, varY);

  DatalogRule rule("parent", {varX, varY}, {triple}, {}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.isDeterministic());
  EXPECT_FALSE(features.hasDatalogRecursion);
  EXPECT_FALSE(features.hasNow);
  EXPECT_FALSE(features.hasRand);
}

TEST_F(UnifiedDeterminismClassifierTest, DatalogRecursiveRule) {
  // Rule: ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
  Variable varX("?x");
  Variable varY("?y");
  Variable varZ("?z");

  TripleComponent::Iri parentPred(
      ad_utility::triple_component::Iri::fromIriref("<parent>"));
  TripleComponent::Iri ancestorPred(
      ad_utility::triple_component::Iri::fromIriref("<ancestor>"));

  SparqlTriple triple1(varX, parentPred, varY);
  SparqlTriple triple2(varY, ancestorPred, varZ);

  DatalogRule rule("ancestor", {varX, varZ}, {triple1, triple2}, {}, true);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.hasDatalogRecursion);
  EXPECT_TRUE(features.isDeterministic());  // Recursion alone is deterministic
}

// ===========================================================================
// T3: Datalog Program-Level Determinism
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, DatalogDeterministicProgram) {
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

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogProgram({rule1, rule2});

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.hasDatalogRecursion);
  EXPECT_TRUE(features.isDeterministic());
}

// ===========================================================================
// T4: N3 Document Analysis
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, N3DeterministicImplication) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .
    { ?x a :Person } => { ?x a :Agent } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Implications);
  EXPECT_FALSE(features.hasN3BuiltIns);
  EXPECT_TRUE(features.isDeterministic());
}

TEST_F(UnifiedDeterminismClassifierTest, N3WithBuiltIns) {
  std::string n3Content = R"(
    @prefix math: <http://www.w3.org/2000/10/swap/math#> .
    { (?a ?b) math:sum ?c } => { ?a :sumsWith ?b :equals ?c } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3BuiltIns);
  EXPECT_FALSE(features.isDeterministic());  // Conservative
}

TEST_F(UnifiedDeterminismClassifierTest, N3WithQuantifiers) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .
    @forAll :x, :y .
    { :x :parent :y } => { :x :ancestor :y } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Quantifiers);
  EXPECT_TRUE(features.hasN3Implications);
  EXPECT_FALSE(features.hasN3BuiltIns);
  EXPECT_TRUE(features.isDeterministic());
}

TEST_F(UnifiedDeterminismClassifierTest, N3WithFormulae) {
  std::string n3Content = R"(
    @prefix : <http://example.org/> .
    :alice :believes { :bob :knows :carol } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Formulae);
  EXPECT_TRUE(features.isDeterministic());  // Formulae alone are deterministic
}

// ===========================================================================
// T5: SHACL Shapes Analysis
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, ShaclDeterministicShape) {
  // SHACL shape without temporal or dynamic constraints
  std::string shaclQuery = R"(
    SELECT ?s WHERE {
      ?s a sh:NodeShape ;
         sh:targetClass ex:Person ;
         sh:property [
           sh:path ex:name ;
           sh:minCount 1
         ]
    }
  )";

  ParsedQuery parsed = parseQuery(shaclQuery);
  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeShaclShapes(parsed);

  EXPECT_EQ(features.formalism, FormalismType::SHACL);
  EXPECT_TRUE(features.isDeterministic());
}

// ===========================================================================
// T6: Guard Configuration Creation
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, GuardConfigSparql) {
  UnifiedDeterminismClassifier classifier;
  auto guards = classifier.createGuardConfig(FormalismType::SPARQL);

  EXPECT_EQ(guards.max_input_size_bytes, 10 * 1024 * 1024);  // 10MB
  EXPECT_EQ(guards.max_nesting_depth, 50);
  EXPECT_EQ(guards.timeout_ms, 30000);  // 30 seconds
}

TEST_F(UnifiedDeterminismClassifierTest, GuardConfigN3) {
  UnifiedDeterminismClassifier classifier;
  auto guards = classifier.createGuardConfig(FormalismType::N3);

  EXPECT_EQ(guards.max_input_size_bytes, 50 * 1024 * 1024);  // 50MB
  EXPECT_EQ(guards.max_nesting_depth, 100);
  EXPECT_EQ(guards.timeout_ms, 60000);  // 60 seconds
}

TEST_F(UnifiedDeterminismClassifierTest, GuardConfigDatalog) {
  UnifiedDeterminismClassifier classifier;
  auto guards = classifier.createGuardConfig(FormalismType::DATALOG);

  EXPECT_EQ(guards.max_input_size_bytes, 20 * 1024 * 1024);  // 20MB
  EXPECT_EQ(guards.max_nesting_depth, 50);
  EXPECT_EQ(guards.timeout_ms, 45000);  // 45 seconds
}

TEST_F(UnifiedDeterminismClassifierTest, GuardConfigShacl) {
  UnifiedDeterminismClassifier classifier;
  auto guards = classifier.createGuardConfig(FormalismType::SHACL);

  EXPECT_EQ(guards.max_input_size_bytes, 25 * 1024 * 1024);  // 25MB
  EXPECT_EQ(guards.max_nesting_depth, 75);
  EXPECT_EQ(guards.timeout_ms, 40000);  // 40 seconds
}

// ===========================================================================
// T7: Cacheability Check
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, CacheabilityDeterministic) {
  UnifiedDeterminismClassifier classifier;

  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::SPARQL;
  // All flags false (default) → deterministic

  EXPECT_TRUE(features.isDeterministic());
  EXPECT_TRUE(classifier.isCacheable(features));
  EXPECT_TRUE(DeterminismContract::satisfiesCachingContract(features));
}

TEST_F(UnifiedDeterminismClassifierTest, CacheabilityNonDeterministic) {
  UnifiedDeterminismClassifier classifier;

  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::SPARQL;
  features.hasRand = true;

  EXPECT_FALSE(features.isDeterministic());
  EXPECT_FALSE(classifier.isCacheable(features));
  EXPECT_FALSE(DeterminismContract::satisfiesCachingContract(features));
}

TEST_F(UnifiedDeterminismClassifierTest, CacheabilityMultipleFlags) {
  UnifiedDeterminismClassifier classifier;

  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::DATALOG;
  features.hasDatalogRecursion = true;  // Deterministic
  features.hasRand = false;
  features.hasNow = false;

  EXPECT_TRUE(features.isDeterministic());
  EXPECT_TRUE(classifier.isCacheable(features));
}

// ===========================================================================
// T8: Non-Deterministic Reasons
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, NonDeterministicReasons) {
  UnifiedDeterminismFeatures features;
  features.hasNow = true;
  features.hasRand = true;
  features.hasService = true;

  auto reasons = features.getNonDeterministicReasons();

  EXPECT_EQ(reasons.size(), 3);
  EXPECT_TRUE(std::find(reasons.begin(), reasons.end(), "NOW() function") !=
              reasons.end());
  EXPECT_TRUE(std::find(reasons.begin(), reasons.end(), "RAND() function") !=
              reasons.end());
  EXPECT_TRUE(std::find(reasons.begin(), reasons.end(), "SERVICE clause") !=
              reasons.end());
}

TEST_F(UnifiedDeterminismClassifierTest, DeterministicNoReasons) {
  UnifiedDeterminismFeatures features;
  // All flags false (deterministic)

  auto reasons = features.getNonDeterministicReasons();

  EXPECT_EQ(reasons.size(), 0);
}

// ===========================================================================
// T9: DeterminismContract Violation Report
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, ContractViolationReport) {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::SPARQL;
  features.hasNow = true;

  std::string report = DeterminismContract::generateViolationReport(features);

  EXPECT_TRUE(report.find("DETERMINISM CONTRACT VIOLATION") !=
              std::string::npos);
  EXPECT_TRUE(report.find("SPARQL") != std::string::npos);
  EXPECT_TRUE(report.find("NOW() function") != std::string::npos);
  EXPECT_TRUE(report.find("REJECTED") != std::string::npos);
}

TEST_F(UnifiedDeterminismClassifierTest, ContractNoViolation) {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::SPARQL;

  std::string report = DeterminismContract::generateViolationReport(features);

  EXPECT_TRUE(report.find("No contract violations") != std::string::npos);
  EXPECT_TRUE(report.find("cacheable") != std::string::npos);
}

// ===========================================================================
// T10: Formalism Type Conversion
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, FormalismTypeToString) {
  EXPECT_EQ(toString(FormalismType::SPARQL), "SPARQL");
  EXPECT_EQ(toString(FormalismType::SHACL), "SHACL");
  EXPECT_EQ(toString(FormalismType::N3), "N3");
  EXPECT_EQ(toString(FormalismType::DATALOG), "DATALOG");
}

// ===========================================================================
// T11: UnifiedDeterminismFeatures toString
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, FeaturesToString) {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::SPARQL;
  features.hasNow = true;
  features.hasRand = true;

  std::string str = features.toString();

  EXPECT_TRUE(str.find("SPARQL") != std::string::npos);
  EXPECT_TRUE(str.find("NON_DETERMINISTIC") != std::string::npos);
  EXPECT_TRUE(str.find("hasNow: true") != std::string::npos);
  EXPECT_TRUE(str.find("hasRand: true") != std::string::npos);
}

// ===========================================================================
// T12: Conversion to SPARQL DeterminismFeatures
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest,
       ConversionToSparqlDeterminismFeatures) {
  UnifiedDeterminismFeatures unified;
  unified.hasNow = true;
  unified.hasRand = false;
  unified.hasUuid = true;
  unified.hasBnode = false;
  unified.hasService = true;

  auto sparql = unified.toSparqlDeterminismFeatures();

  EXPECT_TRUE(sparql.hasNow);
  EXPECT_FALSE(sparql.hasRand);
  EXPECT_TRUE(sparql.hasUuid);
  EXPECT_FALSE(sparql.hasBnode);
  EXPECT_TRUE(sparql.hasService);
  EXPECT_FALSE(sparql.isDeterministic());
}

// ===========================================================================
// T13: Classification String
// ===========================================================================

TEST_F(UnifiedDeterminismClassifierTest, ClassificationStringDeterministic) {
  UnifiedDeterminismFeatures features;
  EXPECT_EQ(features.getClassification(), "DETERMINISTIC");
}

TEST_F(UnifiedDeterminismClassifierTest,
       ClassificationStringNonDeterministic) {
  UnifiedDeterminismFeatures features;
  features.hasRand = true;
  EXPECT_EQ(features.getClassification(), "NON_DETERMINISTIC");
}
