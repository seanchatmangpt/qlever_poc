// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include <gtest/gtest.h>

#include "engine/queryCanonical/IriNormalizer.h"
#include "rdfTypes/Iri.h"
#include "rdfTypes/Literal.h"

using namespace queryCanonical;
using ad_utility::triple_component::Iri;
using ad_utility::triple_component::Literal;

// _____________________________________________________________________________
TEST(IriNormalizerTest, NormalizeIriWithoutPrefix) {
  IriNormalizer normalizer;

  // Create an IRI from a full IRI string
  auto iri1 = Iri::fromIriref("<http://example.org/Person>");

  // Normalize it
  auto normalized = normalizer.normalizeIri(iri1);

  // Should return the IRI without angle brackets
  EXPECT_EQ(normalized, "http://example.org/Person");
}

// _____________________________________________________________________________
TEST(IriNormalizerTest, SameIriWithAndWithoutPrefixProducesSameForm) {
  // Create prefix map
  IriNormalizer::PrefixMap prefixMap;
  prefixMap["ex"] = "<http://example.org/>";

  IriNormalizer normalizer(prefixMap);

  // Create IRI from full form
  auto iri1 = Iri::fromIriref("<http://example.org/Person>");

  // Create the same IRI using prefix expansion
  auto normalized1 = normalizer.normalizeIri(iri1);
  auto normalized2 = normalizer.normalizePrefix("ex", "Person");

  // Both should produce the same canonical form
  EXPECT_EQ(normalized1, normalized2);
  EXPECT_EQ(normalized1, "http://example.org/Person");
}

// _____________________________________________________________________________
TEST(IriNormalizerTest, DifferentPrefixDeclarationsForSameIri) {
  // Create two different prefix maps with different prefix names
  // but pointing to the same IRI
  IriNormalizer::PrefixMap prefixMap1;
  prefixMap1["ex"] = "<http://example.org/>";

  IriNormalizer::PrefixMap prefixMap2;
  prefixMap2["example"] = "<http://example.org/>";

  IriNormalizer normalizer1(prefixMap1);
  IriNormalizer normalizer2(prefixMap2);

  // Expand using different prefixes
  auto normalized1 = normalizer1.normalizePrefix("ex", "Person");
  auto normalized2 = normalizer2.normalizePrefix("example", "Person");

  // Should produce identical results
  EXPECT_EQ(normalized1, normalized2);
  EXPECT_EQ(normalized1, "http://example.org/Person");
}

// _____________________________________________________________________________
TEST(IriNormalizerTest, NormalizePrefixThrowsOnUnknownPrefix) {
  IriNormalizer normalizer;

  // Attempting to normalize with an unknown prefix should throw
  EXPECT_THROW(normalizer.normalizePrefix("unknown", "localName"),
               std::runtime_error);
}

// _____________________________________________________________________________
TEST(IriNormalizerTest, NormalizePlainLiteral) {
  IriNormalizer normalizer;

  // Create a plain literal
  auto literal = Literal::literalWithoutQuotes("Hello World");

  auto normalized = normalizer.normalizeLiteral(literal);

  // Should be quoted
  EXPECT_EQ(normalized, "\"Hello World\"");
}

// _____________________________________________________________________________
TEST(IriNormalizerTest, NormalizeLiteralWithLanguageTag) {
  IriNormalizer normalizer;

  // Create a literal with language tag
  auto literal = Literal::literalWithoutQuotes("Hallo Welt", "de");

  auto normalized = normalizer.normalizeLiteral(literal);

  // Should include language tag
  EXPECT_EQ(normalized, "\"Hallo Welt\"@de");
}

// _____________________________________________________________________________
TEST(IriNormalizerTest, NormalizeLiteralWithDatatype) {
  IriNormalizer normalizer;

  // Create a literal with datatype
  auto datatypeIri = Iri::fromIriref("<http://www.w3.org/2001/XMLSchema#int>");
  auto literal = Literal::literalWithoutQuotes("42", datatypeIri);

  auto normalized = normalizer.normalizeLiteral(literal);

  // Should include datatype IRI
  EXPECT_EQ(normalized, "\"42\"^^<http://www.w3.org/2001/XMLSchema#int>");
}

// _____________________________________________________________________________
TEST(IriNormalizerTest, DeterministicNormalization) {
  IriNormalizer::PrefixMap prefixMap;
  prefixMap["ex"] = "<http://example.org/>";
  IriNormalizer normalizer(prefixMap);

  // Normalize the same IRI multiple times
  auto iri = Iri::fromIriref("<http://example.org/Test>");

  auto normalized1 = normalizer.normalizeIri(iri);
  auto normalized2 = normalizer.normalizeIri(iri);
  auto normalized3 = normalizer.normalizeIri(iri);

  // All should be identical
  EXPECT_EQ(normalized1, normalized2);
  EXPECT_EQ(normalized2, normalized3);
}
