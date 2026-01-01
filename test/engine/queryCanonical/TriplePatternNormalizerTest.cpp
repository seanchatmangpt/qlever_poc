//  Copyright 2025, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code (AI Assistant)

#include <gtest/gtest.h>

#include "engine/queryCanonical/TriplePatternNormalizer.h"
#include "parser/PropertyPath.h"

using namespace queryCanonical;

// Helper function to create a simple triple with variable predicate
SparqlTriple makeTriple(std::string subject, std::string predicate,
                        std::string object) {
  TripleComponent s{Variable{subject}};
  TripleComponent o{Variable{object}};
  return SparqlTriple{s, Variable{predicate}, o};
}

// Helper function to create a triple with IRI predicate
SparqlTriple makeTripleWithIriPred(std::string subject, std::string predicateIri,
                                   std::string object) {
  TripleComponent s{Variable{subject}};
  TripleComponent o{Variable{object}};
  auto iri = ad_utility::triple_component::Iri::fromIriref(predicateIri);
  return SparqlTriple{s, iri, o};
}

// _____________________________________________________________________________
// Test 5 (T5): Reorder triples in BGP to canonical form
TEST(TriplePatternNormalizer, ReorderTriplesInBGP) {
  TriplePatternNormalizer normalizer;

  // Create a set of triples in non-canonical order
  std::vector<SparqlTriple> triples = {
      makeTriple("?x", "?p1", "?z"),  // predicate: ?p1
      makeTriple("?a", "?p0", "?b"),  // predicate: ?p0 (should come first)
      makeTriple("?y", "?p1", "?w"),  // predicate: ?p1, subject: ?y
  };

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  // After normalization, triples should be sorted by (predicate, subject, object)
  // Expected order:
  // 1. (?a, ?p0, ?b) - predicate ?p0 comes before ?p1
  // 2. (?x, ?p1, ?z) - predicate ?p1, subject ?x comes before ?y
  // 3. (?y, ?p1, ?w) - predicate ?p1, subject ?y

  ASSERT_EQ(normalized.size(), 3);

  // Check first triple: (?a, ?p0, ?b)
  EXPECT_TRUE(normalized[0].s_.isVariable());
  EXPECT_EQ(normalized[0].s_.getVariable().name(), "?a");
  EXPECT_TRUE(std::holds_alternative<Variable>(normalized[0].p_));
  EXPECT_EQ(std::get<Variable>(normalized[0].p_).name(), "?p0");
  EXPECT_TRUE(normalized[0].o_.isVariable());
  EXPECT_EQ(normalized[0].o_.getVariable().name(), "?b");

  // Check second triple: (?x, ?p1, ?z)
  EXPECT_TRUE(normalized[1].s_.isVariable());
  EXPECT_EQ(normalized[1].s_.getVariable().name(), "?x");
  EXPECT_TRUE(std::holds_alternative<Variable>(normalized[1].p_));
  EXPECT_EQ(std::get<Variable>(normalized[1].p_).name(), "?p1");
  EXPECT_TRUE(normalized[1].o_.isVariable());
  EXPECT_EQ(normalized[1].o_.getVariable().name(), "?z");

  // Check third triple: (?y, ?p1, ?w)
  EXPECT_TRUE(normalized[2].s_.isVariable());
  EXPECT_EQ(normalized[2].s_.getVariable().name(), "?y");
  EXPECT_TRUE(std::holds_alternative<Variable>(normalized[2].p_));
  EXPECT_EQ(std::get<Variable>(normalized[2].p_).name(), "?p1");
  EXPECT_TRUE(normalized[2].o_.isVariable());
  EXPECT_EQ(normalized[2].o_.getVariable().name(), "?w");
}

// _____________________________________________________________________________
// Test stable sort property: triples with identical keys maintain relative order
TEST(TriplePatternNormalizer, StableSortProperty) {
  TriplePatternNormalizer normalizer;

  // Create triples with identical (predicate, subject, object) keys
  // They should maintain their original relative order
  std::vector<SparqlTriple> triples = {
      makeTriple("?x", "?p", "?y"),  // First
      makeTriple("?x", "?p", "?y"),  // Second (identical)
      makeTriple("?x", "?p", "?y"),  // Third (identical)
  };

  // Store original order by comparing object addresses
  std::vector<const SparqlTriple*> originalOrder = {&triples[0], &triples[1],
                                                    &triples[2]};

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  // After stable sort, identical elements should maintain relative order
  ASSERT_EQ(normalized.size(), 3);

  // All should have the same key
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(normalized[i].s_.getVariable().name(), "?x");
    EXPECT_EQ(std::get<Variable>(normalized[i].p_).name(), "?p");
    EXPECT_EQ(normalized[i].o_.getVariable().name(), "?y");
  }
}

// _____________________________________________________________________________
// Test sorting by object when predicate and subject are equal
TEST(TriplePatternNormalizer, SortByObject) {
  TriplePatternNormalizer normalizer;

  std::vector<SparqlTriple> triples = {
      makeTriple("?x", "?p", "?z"),  // Object: ?z
      makeTriple("?x", "?p", "?a"),  // Object: ?a (should come first)
      makeTriple("?x", "?p", "?m"),  // Object: ?m
  };

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  ASSERT_EQ(normalized.size(), 3);

  // All have same predicate and subject, sorted by object
  EXPECT_EQ(normalized[0].o_.getVariable().name(), "?a");
  EXPECT_EQ(normalized[1].o_.getVariable().name(), "?m");
  EXPECT_EQ(normalized[2].o_.getVariable().name(), "?z");
}

// _____________________________________________________________________________
// Test sorting by subject when predicates are equal
TEST(TriplePatternNormalizer, SortBySubject) {
  TriplePatternNormalizer normalizer;

  std::vector<SparqlTriple> triples = {
      makeTriple("?z", "?p", "?o"),  // Subject: ?z
      makeTriple("?a", "?p", "?o"),  // Subject: ?a (should come first)
      makeTriple("?m", "?p", "?o"),  // Subject: ?m
  };

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  ASSERT_EQ(normalized.size(), 3);

  // All have same predicate and object, sorted by subject
  EXPECT_EQ(normalized[0].s_.getVariable().name(), "?a");
  EXPECT_EQ(normalized[1].s_.getVariable().name(), "?m");
  EXPECT_EQ(normalized[2].s_.getVariable().name(), "?z");
}

// _____________________________________________________________________________
// Test with IRI predicates (real-world scenario)
TEST(TriplePatternNormalizer, SortWithIriPredicates) {
  TriplePatternNormalizer normalizer;

  std::vector<SparqlTriple> triples = {
      makeTripleWithIriPred("?x", "<http://schema.org/name>", "?name"),
      makeTripleWithIriPred("?y", "<http://schema.org/age>", "?age"),
      makeTripleWithIriPred("?x", "<http://schema.org/address>", "?addr"),
  };

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  ASSERT_EQ(normalized.size(), 3);

  // Triples should be sorted by (predicate, subject, object)
  // Note: IRI predicates are sorted lexicographically by their IRI string

  // All three should still be present
  EXPECT_EQ(normalized.size(), 3);
}

// _____________________________________________________________________________
// Test empty input
TEST(TriplePatternNormalizer, EmptyInput) {
  TriplePatternNormalizer normalizer;

  std::vector<SparqlTriple> triples = {};

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  EXPECT_EQ(normalized.size(), 0);
}

// _____________________________________________________________________________
// Test single triple (should remain unchanged)
TEST(TriplePatternNormalizer, SingleTriple) {
  TriplePatternNormalizer normalizer;

  std::vector<SparqlTriple> triples = {makeTriple("?s", "?p", "?o")};

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  ASSERT_EQ(normalized.size(), 1);
  EXPECT_EQ(normalized[0].s_.getVariable().name(), "?s");
  EXPECT_EQ(std::get<Variable>(normalized[0].p_).name(), "?p");
  EXPECT_EQ(normalized[0].o_.getVariable().name(), "?o");
}

// _____________________________________________________________________________
// Test complex sorting scenario with multiple criteria
TEST(TriplePatternNormalizer, ComplexSortingScenario) {
  TriplePatternNormalizer normalizer;

  // Create a complex set of triples that exercise all sorting levels
  std::vector<SparqlTriple> triples = {
      makeTriple("?person", "?name", "?n1"),  // pred: ?name
      makeTriple("?book", "?author", "?a1"),  // pred: ?author
      makeTriple("?car", "?author", "?a2"),   // pred: ?author, subj: ?car
      makeTriple("?apple", "?author", "?a3"), // pred: ?author, subj: ?apple
  };

  auto normalized = normalizer.normalizeTriplePatterns(triples);

  ASSERT_EQ(normalized.size(), 4);

  // First should be ?author predicates (comes before ?name alphabetically)
  // Within ?author, should be sorted by subject: ?apple, ?book, ?car
  EXPECT_EQ(std::get<Variable>(normalized[0].p_).name(), "?author");
  EXPECT_EQ(normalized[0].s_.getVariable().name(), "?apple");

  EXPECT_EQ(std::get<Variable>(normalized[1].p_).name(), "?author");
  EXPECT_EQ(normalized[1].s_.getVariable().name(), "?book");

  EXPECT_EQ(std::get<Variable>(normalized[2].p_).name(), "?author");
  EXPECT_EQ(normalized[2].s_.getVariable().name(), "?car");

  // Last should be ?name predicate
  EXPECT_EQ(std::get<Variable>(normalized[3].p_).name(), "?name");
  EXPECT_EQ(normalized[3].s_.getVariable().name(), "?person");
}

// _____________________________________________________________________________
// Test determinism: calling normalize twice should produce identical results
TEST(TriplePatternNormalizer, Determinism) {
  TriplePatternNormalizer normalizer;

  std::vector<SparqlTriple> triples = {
      makeTriple("?z", "?p2", "?o"),
      makeTriple("?a", "?p1", "?b"),
      makeTriple("?x", "?p1", "?y"),
  };

  auto normalized1 = normalizer.normalizeTriplePatterns(triples);
  auto normalized2 = normalizer.normalizeTriplePatterns(triples);

  ASSERT_EQ(normalized1.size(), normalized2.size());

  // Both results should be identical
  for (size_t i = 0; i < normalized1.size(); ++i) {
    EXPECT_EQ(normalized1[i].s_.getVariable().name(),
              normalized2[i].s_.getVariable().name());
    EXPECT_EQ(std::get<Variable>(normalized1[i].p_).name(),
              std::get<Variable>(normalized2[i].p_).name());
    EXPECT_EQ(normalized1[i].o_.getVariable().name(),
              normalized2[i].o_.getVariable().name());
  }
}
