#include <gtest/gtest.h>

#include "engine/shacl/ComplexPropertyPaths.h"

using namespace shacl;

// Test SimplePath
TEST(ComplexPropertyPathsTest, SimplePath) {
  auto path = PropertyPath::simple("http://example.org/property");

  EXPECT_TRUE(path.isSimple());
  EXPECT_EQ(path.getType(), PathType::Simple);
  EXPECT_EQ(path.getSimpleIri(), "http://example.org/property");
  EXPECT_EQ(path.toString(), "http://example.org/property");
}

// Test InversePath
TEST(ComplexPropertyPathsTest, InversePath) {
  auto innerPath = PropertyPath::simple("http://example.org/property");
  auto inversePath = PropertyPath::inverse(innerPath);

  EXPECT_FALSE(inversePath.isSimple());
  EXPECT_EQ(inversePath.getType(), PathType::Inverse);
  EXPECT_EQ(inversePath.toString(), "^(http://example.org/property)");
}

// Test SequencePath
TEST(ComplexPropertyPathsTest, SequencePath) {
  std::vector<PropertyPath> paths = {
      PropertyPath::simple("http://example.org/p1"),
      PropertyPath::simple("http://example.org/p2"),
      PropertyPath::simple("http://example.org/p3")};

  auto seqPath = PropertyPath::sequence(std::move(paths));

  EXPECT_FALSE(seqPath.isSimple());
  EXPECT_EQ(seqPath.getType(), PathType::Sequence);
  EXPECT_EQ(seqPath.toString(),
            "http://example.org/p1/http://example.org/p2/http://example.org/p3");
}

// Test AlternativePath
TEST(ComplexPropertyPathsTest, AlternativePath) {
  std::vector<PropertyPath> paths = {
      PropertyPath::simple("http://example.org/p1"),
      PropertyPath::simple("http://example.org/p2")};

  auto altPath = PropertyPath::alternative(std::move(paths));

  EXPECT_FALSE(altPath.isSimple());
  EXPECT_EQ(altPath.getType(), PathType::Alternative);
  EXPECT_EQ(altPath.toString(),
            "(http://example.org/p1|http://example.org/p2)");
}

// Test ZeroOrMorePath
TEST(ComplexPropertyPathsTest, ZeroOrMorePath) {
  auto innerPath = PropertyPath::simple("http://example.org/property");
  auto zeroOrMore = PropertyPath::zeroOrMore(innerPath);

  EXPECT_FALSE(zeroOrMore.isSimple());
  EXPECT_EQ(zeroOrMore.getType(), PathType::ZeroOrMore);
  EXPECT_EQ(zeroOrMore.toString(), "(http://example.org/property)*");
}

// Test OneOrMorePath
TEST(ComplexPropertyPathsTest, OneOrMorePath) {
  auto innerPath = PropertyPath::simple("http://example.org/property");
  auto oneOrMore = PropertyPath::oneOrMore(innerPath);

  EXPECT_FALSE(oneOrMore.isSimple());
  EXPECT_EQ(oneOrMore.getType(), PathType::OneOrMore);
  EXPECT_EQ(oneOrMore.toString(), "(http://example.org/property)+");
}

// Test ZeroOrOnePath
TEST(ComplexPropertyPathsTest, ZeroOrOnePath) {
  auto innerPath = PropertyPath::simple("http://example.org/property");
  auto zeroOrOne = PropertyPath::zeroOrOne(innerPath);

  EXPECT_FALSE(zeroOrOne.isSimple());
  EXPECT_EQ(zeroOrOne.getType(), PathType::ZeroOrOne);
  EXPECT_EQ(zeroOrOne.toString(), "(http://example.org/property)?");
}

// Test WildcardPath
TEST(ComplexPropertyPathsTest, WildcardPath) {
  auto wildcard = PropertyPath::wildcard();

  EXPECT_FALSE(wildcard.isSimple());
  EXPECT_EQ(wildcard.getType(), PathType::Wildcard);
  EXPECT_EQ(wildcard.toString(), "*");
}

// Test nested paths
TEST(ComplexPropertyPathsTest, NestedPaths) {
  // Create: ^(p1/p2)
  auto p1 = PropertyPath::simple("http://example.org/p1");
  auto p2 = PropertyPath::simple("http://example.org/p2");
  std::vector<PropertyPath> seqPaths = {p1, p2};
  auto seq = PropertyPath::sequence(std::move(seqPaths));
  auto inversed = PropertyPath::inverse(seq);

  EXPECT_EQ(inversed.getType(), PathType::Inverse);
  EXPECT_EQ(inversed.toString(),
            "^(http://example.org/p1/http://example.org/p2)");
}

// Test path equality
TEST(ComplexPropertyPathsTest, PathEquality) {
  auto path1 = PropertyPath::simple("http://example.org/property");
  auto path2 = PropertyPath::simple("http://example.org/property");
  auto path3 = PropertyPath::simple("http://example.org/other");

  EXPECT_EQ(path1, path2);
  EXPECT_NE(path1, path3);
}

// Test path parsing - simple path
TEST(ComplexPropertyPathsTest, ParseSimplePath) {
  auto path = PropertyPath::parse("http://example.org/property");

  EXPECT_TRUE(path.isSimple());
  EXPECT_EQ(path.getSimpleIri(), "http://example.org/property");
}

// Test path parsing - inverse path
TEST(ComplexPropertyPathsTest, ParseInversePath) {
  auto path = PropertyPath::parse("^http://example.org/property");

  EXPECT_EQ(path.getType(), PathType::Inverse);
}

// Test path parsing - sequence path
TEST(ComplexPropertyPathsTest, ParseSequencePath) {
  auto path = PropertyPath::parse("http://example.org/p1/http://example.org/p2");

  EXPECT_EQ(path.getType(), PathType::Sequence);
}

// Test path parsing - alternative path
TEST(ComplexPropertyPathsTest, ParseAlternativePath) {
  auto path = PropertyPath::parse("http://example.org/p1|http://example.org/p2");

  EXPECT_EQ(path.getType(), PathType::Alternative);
}

// Test path parsing - zero or more
TEST(ComplexPropertyPathsTest, ParseZeroOrMore) {
  auto path = PropertyPath::parse("http://example.org/property*");

  EXPECT_EQ(path.getType(), PathType::ZeroOrMore);
}

// Test path parsing - one or more
TEST(ComplexPropertyPathsTest, ParseOneOrMore) {
  auto path = PropertyPath::parse("http://example.org/property+");

  EXPECT_EQ(path.getType(), PathType::OneOrMore);
}

// Test path parsing - zero or one
TEST(ComplexPropertyPathsTest, ParseZeroOrOne) {
  auto path = PropertyPath::parse("http://example.org/property?");

  EXPECT_EQ(path.getType(), PathType::ZeroOrOne);
}

// Test path parsing - wildcard
TEST(ComplexPropertyPathsTest, ParseWildcard) {
  auto path = PropertyPath::parse("*");

  EXPECT_EQ(path.getType(), PathType::Wildcard);
}

// Test helper functions
TEST(ComplexPropertyPathsTest, IsTransitivePath) {
  auto simple = PropertyPath::simple("http://example.org/p");
  auto zeroOrMore = PropertyPath::zeroOrMore(simple);
  auto oneOrMore = PropertyPath::oneOrMore(simple);
  auto zeroOrOne = PropertyPath::zeroOrOne(simple);

  EXPECT_FALSE(isTransitivePath(simple));
  EXPECT_TRUE(isTransitivePath(zeroOrMore));
  EXPECT_TRUE(isTransitivePath(oneOrMore));
  EXPECT_TRUE(isTransitivePath(zeroOrOne));
}

// Test helper functions
TEST(ComplexPropertyPathsTest, RequiresRecursion) {
  auto simple = PropertyPath::simple("http://example.org/p");
  auto inverse = PropertyPath::inverse(simple);
  auto zeroOrMore = PropertyPath::zeroOrMore(simple);

  EXPECT_FALSE(requiresRecursion(simple));
  EXPECT_FALSE(requiresRecursion(inverse));
  EXPECT_TRUE(requiresRecursion(zeroOrMore));
}

// Test simplifyPath
TEST(ComplexPropertyPathsTest, SimplifyPath) {
  // Single-element sequence should simplify to simple path
  std::vector<PropertyPath> paths = {
      PropertyPath::simple("http://example.org/p")};
  auto seqPath = PropertyPath::sequence(std::move(paths));

  auto simplified = simplifyPath(seqPath);
  EXPECT_TRUE(simplified.isSimple());
}

// Test complex nested path
TEST(ComplexPropertyPathsTest, ComplexNestedPath) {
  // Create: (p1|p2)/p3*
  auto p1 = PropertyPath::simple("http://example.org/p1");
  auto p2 = PropertyPath::simple("http://example.org/p2");
  auto p3 = PropertyPath::simple("http://example.org/p3");

  std::vector<PropertyPath> altPaths = {p1, p2};
  auto alternative = PropertyPath::alternative(std::move(altPaths));
  auto zeroOrMore = PropertyPath::zeroOrMore(p3);

  std::vector<PropertyPath> seqPaths = {alternative, zeroOrMore};
  auto sequence = PropertyPath::sequence(std::move(seqPaths));

  EXPECT_EQ(sequence.getType(), PathType::Sequence);
  EXPECT_FALSE(sequence.isSimple());
}
