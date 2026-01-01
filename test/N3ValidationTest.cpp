// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include <gtest/gtest.h>

#include <string>

#include "./util/GTestHelpers.h"
#include "./util/TripleComponentTestHelpers.h"
#include "gmock/gmock.h"
#include "parser/RdfParser.h"
#include "parser/Tokenizer.h"
#include "parser/TokenizerCtre.h"
#include "util/Exception.h"

using namespace std::literals;
using ::testing::ContainsRegex;
using ::testing::HasSubstr;

namespace {
auto encodedIriManager = []() -> const EncodedIriManager* {
  static EncodedIriManager encodedIriManager_;
  return &encodedIriManager_;
};

using N3Re2Parser = RdfStringParser<N3Parser<Tokenizer>>;
using N3CtreParser = RdfStringParser<N3Parser<TokenizerCtre>>;

// Helper to create N3 parser
auto n3Parser = []() { return N3Re2Parser{encodedIriManager()}; };

}  // namespace

// ===========================================================================
// 1. SYNTAX VALIDATION TESTS
// ===========================================================================

TEST(N3ValidationTest, InvalidTripleSyntax_MissingSubject) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://pred> <http://obj> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, InvalidTripleSyntax_MissingPredicate) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://subj> <http://obj> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, InvalidTripleSyntax_MissingObject) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://subj> <http://pred> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, MissingPeriodAtEndOfTriple) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://s> <http://p> <http://o>"),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, MalformedIRI_UnclosedAngleBracket) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://example.org/unclosed <http://p> <http://o> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, MalformedIRI_MissingOpeningBracket) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("http://example.org> <http://p> <http://o> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, InvalidBlankNodeSyntax_InvalidCharacters) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("_:invalid$char <http://p> <http://o> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, InvalidPrefixSyntax_MissingColon) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("@prefix ex <http://example.org/> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, InvalidPrefixSyntax_MissingIRI) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("@prefix ex: ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, InvalidBaseSyntax_MissingIRI) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("@base ."),
      HasSubstr("Parse error"));
}

// ===========================================================================
// 2. SEMANTIC VALIDATION TESTS
// ===========================================================================

TEST(N3ValidationTest, UndefinedPrefixUsed) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("undefined:term <http://p> <http://o> ."),
      HasSubstr("undefined"));
}

TEST(N3ValidationTest, UndefinedPrefixInPredicate) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://s> unknown:predicate <http://o> ."),
      HasSubstr("unknown"));
}

TEST(N3ValidationTest, UndefinedPrefixInObject) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://s> <http://p> bad:object ."),
      HasSubstr("bad"));
}

TEST(N3ValidationTest, InvalidDatatypeIRI_Malformed) {
  auto parser = n3Parser();
  parser.invalidLiteralsAreSkipped() = false;
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String(
          "<http://s> <http://p> \"123\"^^<http://invalid datatype> ."),
      HasSubstr("Parse error"));
}

// ===========================================================================
// 3. LITERAL VALIDATION TESTS
// ===========================================================================

TEST(N3ValidationTest, InvalidTypedLiteral_StringAsInteger) {
  auto parser = n3Parser();
  parser.invalidLiteralsAreSkipped() = false;
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String(
          "<http://s> <http://p> \"not_a_number\"^^<http://www.w3.org/2001/XMLSchema#integer> ."),
      HasSubstr("integer"));
}

TEST(N3ValidationTest, InvalidTypedLiteral_OutOfRangeInteger) {
  auto parser = n3Parser();
  parser.invalidLiteralsAreSkipped() = false;
  // Very large number that could overflow
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String(
          "<http://s> <http://p> \"99999999999999999999999999999999999999\"^^<http://www.w3.org/2001/XMLSchema#integer> ."),
      HasSubstr("overflow"));
}

TEST(N3ValidationTest, InvalidTypedLiteral_StringAsDouble) {
  auto parser = n3Parser();
  parser.invalidLiteralsAreSkipped() = false;
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String(
          "<http://s> <http://p> \"not_a_double\"^^<http://www.w3.org/2001/XMLSchema#double> ."),
      HasSubstr("double"));
}

TEST(N3ValidationTest, InvalidTypedLiteral_SkipWhenConfigured) {
  auto parser = n3Parser();
  parser.invalidLiteralsAreSkipped() = true;
  // Should not throw, triple should be skipped
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s1> <http://p1> <http://o1> .\n"
          "<http://s2> <http://p2> \"invalid\"^^<http://www.w3.org/2001/XMLSchema#integer> .\n"
          "<http://s3> <http://p3> <http://o3> ."));
  auto triples = parser.getTriples();
  // Should have 2 valid triples, invalid one skipped
  EXPECT_EQ(triples.size(), 2u);
}

TEST(N3ValidationTest, LanguageTagWithDatatype_InvalidCombination) {
  auto parser = n3Parser();
  // Language tags and datatypes cannot be combined
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String(
          "<http://s> <http://p> \"text\"@en^^<http://www.w3.org/2001/XMLSchema#string> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, LargeLiteralHandling) {
  auto parser = n3Parser();
  // Create a very large literal (10KB)
  std::string largeLiteral(10000, 'x');
  std::string n3 = "<http://s> <http://p> \"" + largeLiteral + "\" .";
  // Should handle without error
  ASSERT_NO_THROW(parser.parseUtf8String(n3));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, SpecialCharacterEscaping_NewlineInLiteral) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(R"(<http://s> <http://p> "line1\nline2" .)"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, SpecialCharacterEscaping_TabInLiteral) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(R"(<http://s> <http://p> "col1\tcol2" .)"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, SpecialCharacterEscaping_QuoteInLiteral) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(R"(<http://s> <http://p> "He said \"hello\"" .)"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, SpecialCharacterEscaping_BackslashInLiteral) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(R"(<http://s> <http://p> "path\\to\\file" .)"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, MultilineLiteralString) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(R"(<http://s> <http://p> """Multi
line
literal""" .)"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

// ===========================================================================
// 4. IRI VALIDATION TESTS
// ===========================================================================

TEST(N3ValidationTest, InvalidIRISyntax_SpaceInIRI) {
  auto parser = n3Parser();
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://example.org/has space> <http://p> <http://o> ."),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, RelativeIRIWithoutBase) {
  auto parser = n3Parser();
  // Relative IRI should work with implicit base
  ASSERT_NO_THROW(
      parser.parseUtf8String("<relative> <http://p> <http://o> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, RelativeIRIWithBase) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "@base <http://example.org/> .\n"
          "<relative> <http://p> <http://o> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, InvalidIRIEscapeSequence) {
  auto parser = n3Parser();
  // Invalid percent encoding (not followed by two hex digits)
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String("<http://example.org/%ZZ> <http://p> <http://o> ."),
      HasSubstr("Parse error"));
}

// ===========================================================================
// 5. BLANK NODE VALIDATION TESTS
// ===========================================================================

TEST(N3ValidationTest, BlankNodeScope_SameFileReuse) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "_:b1 <http://p1> <http://o1> .\n"
          "_:b1 <http://p2> <http://o2> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 2u);
  // Both triples should reference the same blank node
  EXPECT_EQ(triples[0].subject_, triples[1].subject_);
}

TEST(N3ValidationTest, BlankNodeScope_DifferentNodes) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "_:b1 <http://p> <http://o1> .\n"
          "_:b2 <http://p> <http://o2> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 2u);
  // Different blank node IDs should be different
  EXPECT_NE(triples[0].subject_, triples[1].subject_);
}

TEST(N3ValidationTest, PropertyListExpansion) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "[ <http://p1> <http://o1> ; <http://p2> <http://o2> ] ."));
  auto triples = parser.getTriples();
  // Should generate 2 triples with same blank node subject
  EXPECT_EQ(triples.size(), 2u);
  EXPECT_EQ(triples[0].subject_, triples[1].subject_);
  EXPECT_TRUE(triples[0].subject_.isBlankNode());
}

TEST(N3ValidationTest, PropertyListExpansion_Nested) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "[ <http://p1> [ <http://p2> <http://o2> ] ] ."));
  auto triples = parser.getTriples();
  // Should generate 2 triples with nested blank nodes
  EXPECT_EQ(triples.size(), 2u);
  EXPECT_TRUE(triples[0].subject_.isBlankNode());
  EXPECT_TRUE(triples[0].object_.isBlankNode());
}

TEST(N3ValidationTest, AnonymousBlankNode) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("[] <http://p> <http://o> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
  EXPECT_TRUE(triples[0].subject_.isBlankNode());
}

// ===========================================================================
// 6. COLLECTION & LIST VALIDATION TESTS
// ===========================================================================

TEST(N3ValidationTest, EmptyCollection) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> <http://p> () ."));
  auto triples = parser.getTriples();
  // Empty collection is rdf:nil
  EXPECT_GE(triples.size(), 1u);
}

TEST(N3ValidationTest, SingleElementCollection) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> <http://p> (<http://item>) ."));
  auto triples = parser.getTriples();
  // Should generate triples for the list structure
  EXPECT_GE(triples.size(), 1u);
}

TEST(N3ValidationTest, MultiElementCollection) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> (<http://item1> <http://item2> <http://item3>) ."));
  auto triples = parser.getTriples();
  // Should generate multiple triples for list structure
  EXPECT_GE(triples.size(), 1u);
}

TEST(N3ValidationTest, NestedCollections) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> ((<http://a> <http://b>) (<http://c> <http://d>)) ."));
  auto triples = parser.getTriples();
  // Nested collections should be expanded properly
  EXPECT_GE(triples.size(), 1u);
}

TEST(N3ValidationTest, CollectionWithLiterals) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          R"(<http://s> <http://p> ("string" 123 true) .)"));
  auto triples = parser.getTriples();
  EXPECT_GE(triples.size(), 1u);
}

TEST(N3ValidationTest, CollectionWithBlankNodes) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> (_:b1 _:b2) ."));
  auto triples = parser.getTriples();
  EXPECT_GE(triples.size(), 1u);
}

// ===========================================================================
// 7. FILE ENCODING TESTS
// ===========================================================================

TEST(N3ValidationTest, UTF8Handling_BasicUnicode) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> \"Café\" ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, UTF8Handling_ChineseCharacters) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> \"你好世界\" ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, UTF8Handling_Emoji) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> \"Hello 👋 World\" ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, UTF8Handling_MixedScripts) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> \"Latin Кириллица العربية 中文\" ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

// ===========================================================================
// 8. ERROR RECOVERY TESTS
// ===========================================================================

TEST(N3ValidationTest, ErrorRecovery_SkipInvalidLiterals) {
  auto parser = n3Parser();
  parser.invalidLiteralsAreSkipped() = true;

  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s1> <http://p> <http://o1> .\n"
          "<http://s2> <http://p> \"bad\"^^<http://www.w3.org/2001/XMLSchema#integer> .\n"
          "<http://s3> <http://p> <http://o3> ."));

  auto triples = parser.getTriples();
  // Should have 2 valid triples, bad one skipped
  EXPECT_EQ(triples.size(), 2u);
}

TEST(N3ValidationTest, ErrorRecovery_MultipleInvalidLiterals) {
  auto parser = n3Parser();
  parser.invalidLiteralsAreSkipped() = true;

  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s1> <http://p> <http://o1> .\n"
          "<http://s2> <http://p> \"bad1\"^^<http://www.w3.org/2001/XMLSchema#integer> .\n"
          "<http://s3> <http://p> <http://o3> .\n"
          "<http://s4> <http://p> \"bad2\"^^<http://www.w3.org/2001/XMLSchema#double> .\n"
          "<http://s5> <http://p> <http://o5> ."));

  auto triples = parser.getTriples();
  // Should have 3 valid triples
  EXPECT_EQ(triples.size(), 3u);
}

TEST(N3ValidationTest, ErrorLocation_ReportsPosition) {
  auto parser = n3Parser();
  try {
    parser.parseUtf8String(
        "<http://s1> <http://p> <http://o1> .\n"
        "<http://s2> <http://p> <http://o2> .\n"
        "invalid triple syntax\n");
    FAIL() << "Expected ParseException";
  } catch (const ParseException& e) {
    std::string error = e.what();
    EXPECT_THAT(error, HasSubstr("byte position"));
  }
}

// ===========================================================================
// 9. EDGE CASES TESTS
// ===========================================================================

TEST(N3ValidationTest, EdgeCase_VeryLongIRI) {
  auto parser = n3Parser();
  // Create IRI with 1000 characters
  std::string longPath(1000, 'x');
  std::string n3 = "<http://example.org/" + longPath + "> <http://p> <http://o> .";
  ASSERT_NO_THROW(parser.parseUtf8String(n3));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_VeryLongLiteral) {
  auto parser = n3Parser();
  // Create literal with 50KB
  std::string longLiteral(50000, 'x');
  std::string n3 = "<http://s> <http://p> \"" + longLiteral + "\" .";
  ASSERT_NO_THROW(parser.parseUtf8String(n3));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_DeeplyNestedBlankNodes) {
  auto parser = n3Parser();
  // Nested blank node property lists
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "[ <http://p1> [ <http://p2> [ <http://p3> <http://o> ] ] ] ."));
  auto triples = parser.getTriples();
  // Should create multiple triples with nested blank nodes
  EXPECT_GE(triples.size(), 3u);
}

TEST(N3ValidationTest, EdgeCase_DeeplyNestedCollections) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> (((((<http://deep>))))) ."));
  auto triples = parser.getTriples();
  EXPECT_GE(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_ManyPrefixes) {
  auto parser = n3Parser();
  std::string n3;

  // Define 100 prefixes
  for (int i = 0; i < 100; i++) {
    n3 += "@prefix p" + std::to_string(i) + ": <http://example.org/" +
          std::to_string(i) + "/> .\n";
  }

  // Use some of them
  n3 += "p0:s p50:p p99:o .";

  ASSERT_NO_THROW(parser.parseUtf8String(n3));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_EmptyFile) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(parser.parseUtf8String(""));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 0u);
}

TEST(N3ValidationTest, EdgeCase_OnlyWhitespace) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(parser.parseUtf8String("   \n\t  \n  "));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 0u);
}

TEST(N3ValidationTest, EdgeCase_OnlyComments) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(parser.parseUtf8String(
      "# This is a comment\n"
      "# Another comment\n"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 0u);
}

TEST(N3ValidationTest, EdgeCase_MixedCommentsAndTriples) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(parser.parseUtf8String(
      "# Comment 1\n"
      "<http://s1> <http://p> <http://o1> . # inline comment\n"
      "# Comment 2\n"
      "<http://s2> <http://p> <http://o2> .\n"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 2u);
}

TEST(N3ValidationTest, EdgeCase_NumberLiterals_Zero) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> <http://p> 0 ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_NumberLiterals_NegativeZero) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> <http://p> -0 ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_NumberLiterals_ScientificNotation) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> <http://p> 1.23e10 ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_NumberLiterals_NegativeScientific) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> <http://p> -4.56e-7 ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_BooleanLiterals) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s1> <http://p> true .\n"
          "<http://s2> <http://p> false ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 2u);
}

TEST(N3ValidationTest, EdgeCase_EmptyStringLiteral) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(R"(<http://s> <http://p> "" .)"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_EmptyLanguageTag) {
  auto parser = n3Parser();
  // Language tag cannot be empty
  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String(R"(<http://s> <http://p> "text"@ .)"),
      HasSubstr("Parse error"));
}

TEST(N3ValidationTest, EdgeCase_LongLanguageTag) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          R"(<http://s> <http://p> "text"@en-US-x-twain .)"));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_PredicateA_Shorthand) {
  auto parser = n3Parser();
  // 'a' is shorthand for rdf:type
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> a <http://Type> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, EdgeCase_MultipleObjectsWithSemicolon) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p1> <http://o1> ;\n"
          "           <http://p2> <http://o2> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 2u);
  // Same subject
  EXPECT_EQ(triples[0].subject_, triples[1].subject_);
}

TEST(N3ValidationTest, EdgeCase_MultipleObjectsWithComma) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> <http://o1>, <http://o2> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 2u);
  // Same subject and predicate
  EXPECT_EQ(triples[0].subject_, triples[1].subject_);
  EXPECT_EQ(triples[0].predicate_, triples[1].predicate_);
}

// ===========================================================================
// 10. INTEGER OVERFLOW BEHAVIOR TESTS
// ===========================================================================

TEST(N3ValidationTest, IntegerOverflow_DefaultBehavior) {
  auto parser = n3Parser();
  parser.integerOverflowBehavior() =
      TurtleParserIntegerOverflowBehavior::Error;

  AD_EXPECT_THROW_WITH_MESSAGE(
      parser.parseUtf8String(
          "<http://s> <http://p> 99999999999999999999999999999999999999 ."),
      HasSubstr("overflow"));
}

TEST(N3ValidationTest, IntegerOverflow_ConvertToDouble) {
  auto parser = n3Parser();
  parser.integerOverflowBehavior() =
      TurtleParserIntegerOverflowBehavior::OverflowingToDouble;

  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s> <http://p> 99999999999999999999999999999999999999 ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
  // Should be converted to double
  EXPECT_TRUE(triples[0].object_.isDouble());
}

TEST(N3ValidationTest, IntegerOverflow_AllToDouble) {
  auto parser = n3Parser();
  parser.integerOverflowBehavior() =
      TurtleParserIntegerOverflowBehavior::AllToDouble;

  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "<http://s1> <http://p> 123 .\n"
          "<http://s2> <http://p> 99999999999999999999999999999999999999 ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 2u);
  // Both should be doubles
  EXPECT_TRUE(triples[0].object_.isDouble());
  EXPECT_TRUE(triples[1].object_.isDouble());
}

// ===========================================================================
// 11. COMPATIBILITY TESTS (N3 as superset of Turtle)
// ===========================================================================

TEST(N3ValidationTest, Compatibility_TurtleBasicTriple) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String("<http://s> <http://p> <http://o> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, Compatibility_TurtlePrefixes) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "@prefix ex: <http://example.org/> .\n"
          "ex:s ex:p ex:o ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, Compatibility_SPARQLStyleBase) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "BASE <http://example.org/>\n"
          "<s> <p> <o> ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}

TEST(N3ValidationTest, Compatibility_SPARQLStylePrefix) {
  auto parser = n3Parser();
  ASSERT_NO_THROW(
      parser.parseUtf8String(
          "PREFIX ex: <http://example.org/>\n"
          "ex:s ex:p ex:o ."));
  auto triples = parser.getTriples();
  EXPECT_EQ(triples.size(), 1u);
}
