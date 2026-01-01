#include <gtest/gtest.h>
#include "parser/ShEx.h"

using namespace shex;

// ============================================================================
// Phase 2A: NumericRangeConstraint Tests
// ============================================================================

TEST(NumericRangeConstraintTest, MinInclusiveConstraint) {
  NumericRangeConstraint constraint;
  constraint.minInclusive = 10.0;

  EXPECT_TRUE(constraint.validate("10", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("15", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("9", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("9.9", XsdDatatype::DECIMAL));
}

TEST(NumericRangeConstraintTest, MaxInclusiveConstraint) {
  NumericRangeConstraint constraint;
  constraint.maxInclusive = 100.0;

  EXPECT_TRUE(constraint.validate("100", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("50", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("101", XsdDatatype::INTEGER));
}

TEST(NumericRangeConstraintTest, MinExclusiveConstraint) {
  NumericRangeConstraint constraint;
  constraint.minExclusive = 0.0;

  EXPECT_TRUE(constraint.validate("0.1", XsdDatatype::DECIMAL));
  EXPECT_TRUE(constraint.validate("1", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("0", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("-1", XsdDatatype::INTEGER));
}

TEST(NumericRangeConstraintTest, MaxExclusiveConstraint) {
  NumericRangeConstraint constraint;
  constraint.maxExclusive = 100.0;

  EXPECT_TRUE(constraint.validate("99", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("99.9", XsdDatatype::DECIMAL));
  EXPECT_FALSE(constraint.validate("100", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("100.1", XsdDatatype::DECIMAL));
}

TEST(NumericRangeConstraintTest, RangeConstraint) {
  NumericRangeConstraint constraint;
  constraint.minInclusive = 0.0;
  constraint.maxInclusive = 100.0;

  EXPECT_TRUE(constraint.validate("0", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("50", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("100", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("-1", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("101", XsdDatatype::INTEGER));
}

TEST(NumericRangeConstraintTest, TotalDigitsConstraint) {
  NumericRangeConstraint constraint;
  constraint.totalDigits = 3;

  EXPECT_TRUE(constraint.validate("123", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("12.3", XsdDatatype::DECIMAL));
  EXPECT_TRUE(constraint.validate("1", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("1234", XsdDatatype::INTEGER));
}

TEST(NumericRangeConstraintTest, FractionDigitsConstraint) {
  NumericRangeConstraint constraint;
  constraint.fractionDigits = 2;

  EXPECT_TRUE(constraint.validate("123.45", XsdDatatype::DECIMAL));
  EXPECT_TRUE(constraint.validate("1.2", XsdDatatype::DECIMAL));
  EXPECT_TRUE(constraint.validate("100", XsdDatatype::INTEGER));  // 0 fraction digits
  EXPECT_FALSE(constraint.validate("1.234", XsdDatatype::DECIMAL));
}

TEST(NumericRangeConstraintTest, IntegerTypeEnforcement) {
  NumericRangeConstraint constraint;
  constraint.minInclusive = 0.0;

  EXPECT_TRUE(constraint.validate("5", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("5.5", XsdDatatype::INTEGER));  // Not an integer
}

TEST(NumericRangeConstraintTest, SpecialValues) {
  NumericRangeConstraint constraint;

  // INF should be valid for DOUBLE
  EXPECT_TRUE(constraint.validate("INF", XsdDatatype::DOUBLE));
  EXPECT_TRUE(constraint.validate("-INF", XsdDatatype::DOUBLE));
  EXPECT_TRUE(constraint.validate("NaN", XsdDatatype::DOUBLE));

  // But should fail max constraint
  constraint.maxInclusive = 1000.0;
  EXPECT_FALSE(constraint.validate("INF", XsdDatatype::DOUBLE));
}

TEST(NumericRangeConstraintTest, NegativeNumbers) {
  NumericRangeConstraint constraint;
  constraint.minInclusive = -100.0;
  constraint.maxInclusive = -10.0;

  EXPECT_TRUE(constraint.validate("-50", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("-100", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("-10", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("-101", XsdDatatype::INTEGER));
  EXPECT_FALSE(constraint.validate("-9", XsdDatatype::INTEGER));
}

TEST(NumericRangeConstraintTest, ZeroValue) {
  NumericRangeConstraint constraint;
  constraint.minInclusive = 0.0;

  EXPECT_TRUE(constraint.validate("0", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("0.0", XsdDatatype::DECIMAL));
  EXPECT_FALSE(constraint.validate("-0.1", XsdDatatype::DECIMAL));
}

// ============================================================================
// Phase 2A: PatternConstraint Tests
// ============================================================================

TEST(PatternConstraintTest, BasicPattern) {
  PatternConstraint constraint("[0-9]+");

  EXPECT_TRUE(constraint.validate("123"));
  EXPECT_TRUE(constraint.validate("0"));
  EXPECT_FALSE(constraint.validate("abc"));
  EXPECT_FALSE(constraint.validate("12a"));
}

TEST(PatternConstraintTest, EmailPattern) {
  PatternConstraint constraint(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");

  EXPECT_TRUE(constraint.validate("test@example.com"));
  EXPECT_TRUE(constraint.validate("user.name+tag@domain.co.uk"));
  EXPECT_FALSE(constraint.validate("invalid@"));
  EXPECT_FALSE(constraint.validate("@example.com"));
  EXPECT_FALSE(constraint.validate("notanemail"));
}

TEST(PatternConstraintTest, PhoneNumberPattern) {
  PatternConstraint constraint(R"(\d{3}-\d{3}-\d{4})");

  EXPECT_TRUE(constraint.validate("123-456-7890"));
  EXPECT_FALSE(constraint.validate("1234567890"));
  EXPECT_FALSE(constraint.validate("123-45-6789"));
}

TEST(PatternConstraintTest, RegexCaching) {
  PatternConstraint constraint("[a-z]+");

  // First validation triggers compilation
  EXPECT_TRUE(constraint.validate("abc"));

  // Second validation should use cached regex
  EXPECT_TRUE(constraint.validate("xyz"));
  EXPECT_FALSE(constraint.validate("ABC"));

  // Verify regex is compiled
  EXPECT_TRUE(constraint.compiled);
  EXPECT_NE(constraint.compiledRegex, nullptr);
}

TEST(PatternConstraintTest, ComplexPattern) {
  PatternConstraint constraint(R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)");

  EXPECT_TRUE(constraint.validate("http://example.com"));
  EXPECT_TRUE(constraint.validate("https://www.example.com/path?query=value"));
  EXPECT_TRUE(constraint.validate("ftp://ftp.example.com"));
  EXPECT_FALSE(constraint.validate("not a url"));
  EXPECT_FALSE(constraint.validate("example.com"));
}

TEST(PatternConstraintTest, EmptyPattern) {
  PatternConstraint constraint("");

  // Empty pattern matches empty string
  EXPECT_TRUE(constraint.validate(""));
}

TEST(PatternConstraintTest, SpecialCharacters) {
  PatternConstraint constraint(R"([\w\-\.]+)");

  EXPECT_TRUE(constraint.validate("test-name.value"));
  EXPECT_TRUE(constraint.validate("123_abc"));
  EXPECT_FALSE(constraint.validate("test@name"));
}

TEST(PatternConstraintTest, UnicodePattern) {
  PatternConstraint constraint(".*");

  EXPECT_TRUE(constraint.validate("Hello"));
  EXPECT_TRUE(constraint.validate("你好"));  // Chinese
  EXPECT_TRUE(constraint.validate("مرحبا"));  // Arabic
  EXPECT_TRUE(constraint.validate("🎉"));    // Emoji
}

TEST(PatternConstraintTest, InvalidRegex) {
  PatternConstraint constraint("[invalid(regex");

  // Invalid regex should fail validation
  EXPECT_FALSE(constraint.validate("any string"));
}

// ============================================================================
// Phase 2A: LanguageTagConstraint Tests
// ============================================================================

TEST(LanguageTagConstraintTest, ExactMatch) {
  LanguageTagConstraint constraint;
  constraint.languageTag = "en";

  EXPECT_TRUE(constraint.validate("en"));
  EXPECT_FALSE(constraint.validate("en-US"));
  EXPECT_FALSE(constraint.validate("fr"));
}

TEST(LanguageTagConstraintTest, PatternMatchWildcard) {
  LanguageTagConstraint constraint;
  constraint.languagePattern = "en-*";

  EXPECT_TRUE(constraint.validate("en-US"));
  EXPECT_TRUE(constraint.validate("en-GB"));
  EXPECT_TRUE(constraint.validate("en-AU"));
  EXPECT_FALSE(constraint.validate("fr-FR"));
}

TEST(LanguageTagConstraintTest, ValidBCP47Tags) {
  LanguageTagConstraint constraint;

  EXPECT_TRUE(constraint.validate("en"));
  EXPECT_TRUE(constraint.validate("en-US"));
  EXPECT_TRUE(constraint.validate("zh-Hans"));
  EXPECT_TRUE(constraint.validate("zh-Hans-CN"));
  EXPECT_TRUE(constraint.validate("sr-Latn-RS"));
}

TEST(LanguageTagConstraintTest, InvalidBCP47Tags) {
  LanguageTagConstraint constraint;

  EXPECT_FALSE(constraint.validate(""));
  EXPECT_FALSE(constraint.validate("e"));      // Too short
  EXPECT_FALSE(constraint.validate("EN"));     // Primary language must be lowercase
  EXPECT_FALSE(constraint.validate("en_US"));  // Must use hyphen, not underscore
}

TEST(LanguageTagConstraintTest, ComplexTags) {
  LanguageTagConstraint constraint;

  EXPECT_TRUE(constraint.validate("sr-Latn"));      // Serbian in Latin script
  EXPECT_TRUE(constraint.validate("zh-Hans-CN"));   // Chinese, Simplified, China
  EXPECT_TRUE(constraint.validate("hy-Latn-IT-arevela"));  // With variant
}

TEST(LanguageTagConstraintTest, RegionCodes) {
  LanguageTagConstraint constraint;
  constraint.languagePattern = "en-*";

  EXPECT_TRUE(constraint.validate("en-US"));   // US
  EXPECT_TRUE(constraint.validate("en-GB"));   // Great Britain
  EXPECT_TRUE(constraint.validate("en-029"));  // Caribbean (3-digit region)
}

TEST(LanguageTagConstraintTest, NoConstraint) {
  LanguageTagConstraint constraint;

  // No specific constraint, just validate BCP47
  EXPECT_TRUE(constraint.validate("en"));
  EXPECT_TRUE(constraint.validate("fr-FR"));
  EXPECT_FALSE(constraint.validate("invalid"));
}

// ============================================================================
// Phase 2A: LengthConstraint Tests
// ============================================================================

TEST(LengthConstraintTest, MinLengthConstraint) {
  LengthConstraint constraint;
  constraint.minLength = 3;

  EXPECT_TRUE(constraint.validate("abc"));
  EXPECT_TRUE(constraint.validate("abcd"));
  EXPECT_FALSE(constraint.validate("ab"));
  EXPECT_FALSE(constraint.validate(""));
}

TEST(LengthConstraintTest, MaxLengthConstraint) {
  LengthConstraint constraint;
  constraint.maxLength = 10;

  EXPECT_TRUE(constraint.validate("short"));
  EXPECT_TRUE(constraint.validate("1234567890"));
  EXPECT_FALSE(constraint.validate("this is too long"));
}

TEST(LengthConstraintTest, ExactLengthConstraint) {
  LengthConstraint constraint;
  constraint.exactLength = 5;

  EXPECT_TRUE(constraint.validate("hello"));
  EXPECT_FALSE(constraint.validate("hi"));
  EXPECT_FALSE(constraint.validate("toolong"));
}

TEST(LengthConstraintTest, RangeConstraint) {
  LengthConstraint constraint;
  constraint.minLength = 3;
  constraint.maxLength = 10;

  EXPECT_TRUE(constraint.validate("abc"));
  EXPECT_TRUE(constraint.validate("1234567890"));
  EXPECT_FALSE(constraint.validate("ab"));
  EXPECT_FALSE(constraint.validate("this is way too long"));
}

TEST(LengthConstraintTest, UTF8Characters) {
  LengthConstraint constraint;
  constraint.exactLength = 3;

  // ASCII characters
  EXPECT_TRUE(constraint.validate("abc"));

  // UTF-8 characters (each is one character, not multiple bytes)
  EXPECT_TRUE(constraint.validate("你好世"));    // 3 Chinese characters
  EXPECT_TRUE(constraint.validate("مرحبا"));      // 3 Arabic characters (note: may vary based on normalization)

  // Emojis (each counts as one character)
  EXPECT_TRUE(constraint.validate("🎉🎊🎈"));    // 3 emojis
}

TEST(LengthConstraintTest, MixedUTF8) {
  LengthConstraint constraint;
  constraint.minLength = 5;
  constraint.maxLength = 10;

  EXPECT_TRUE(constraint.validate("Hello"));
  EXPECT_TRUE(constraint.validate("你好World"));  // Mixed Chinese and English
  EXPECT_TRUE(constraint.validate("Café☕"));      // Accented chars and emoji
}

TEST(LengthConstraintTest, EmptyString) {
  LengthConstraint constraint;
  constraint.minLength = 0;

  EXPECT_TRUE(constraint.validate(""));

  constraint.minLength = 1;
  EXPECT_FALSE(constraint.validate(""));
}

TEST(LengthConstraintTest, ZeroLength) {
  LengthConstraint constraint;
  constraint.exactLength = 0;

  EXPECT_TRUE(constraint.validate(""));
  EXPECT_FALSE(constraint.validate("a"));
}

// ============================================================================
// Phase 2A: DatatypeFacetConstraint Tests
// ============================================================================

TEST(DatatypeFacetConstraintTest, IntegerValidation) {
  DatatypeFacetConstraint constraint(XsdDatatype::INTEGER);

  EXPECT_TRUE(constraint.validate("0"));
  EXPECT_TRUE(constraint.validate("123"));
  EXPECT_TRUE(constraint.validate("-456"));
  EXPECT_FALSE(constraint.validate("12.34"));
  EXPECT_FALSE(constraint.validate("abc"));
}

TEST(DatatypeFacetConstraintTest, DecimalValidation) {
  DatatypeFacetConstraint constraint(XsdDatatype::DECIMAL);

  EXPECT_TRUE(constraint.validate("0"));
  EXPECT_TRUE(constraint.validate("123.456"));
  EXPECT_TRUE(constraint.validate("-78.9"));
  EXPECT_TRUE(constraint.validate("100"));
  EXPECT_FALSE(constraint.validate("abc"));
}

TEST(DatatypeFacetConstraintTest, DoubleValidation) {
  DatatypeFacetConstraint constraint(XsdDatatype::DOUBLE);

  EXPECT_TRUE(constraint.validate("123.456"));
  EXPECT_TRUE(constraint.validate("1.23e10"));
  EXPECT_TRUE(constraint.validate("INF"));
  EXPECT_TRUE(constraint.validate("-INF"));
  EXPECT_TRUE(constraint.validate("NaN"));
  EXPECT_FALSE(constraint.validate("not a number"));
}

TEST(DatatypeFacetConstraintTest, BooleanValidation) {
  DatatypeFacetConstraint constraint(XsdDatatype::BOOLEAN);

  EXPECT_TRUE(constraint.validate("true"));
  EXPECT_TRUE(constraint.validate("false"));
  EXPECT_TRUE(constraint.validate("1"));
  EXPECT_TRUE(constraint.validate("0"));
  EXPECT_FALSE(constraint.validate("yes"));
  EXPECT_FALSE(constraint.validate("no"));
  EXPECT_FALSE(constraint.validate("TRUE"));
}

TEST(DatatypeFacetConstraintTest, DateValidation) {
  DatatypeFacetConstraint constraint(XsdDatatype::DATE);

  EXPECT_TRUE(constraint.validate("2024-01-15"));
  EXPECT_TRUE(constraint.validate("2000-12-31"));
  EXPECT_TRUE(constraint.validate("1999-02-28"));
  EXPECT_FALSE(constraint.validate("2024-13-01"));  // Invalid month
  EXPECT_FALSE(constraint.validate("2024-01-32"));  // Invalid day
  EXPECT_FALSE(constraint.validate("24-01-15"));    // Wrong format
  EXPECT_FALSE(constraint.validate("2024/01/15"));  // Wrong separator
}

TEST(DatatypeFacetConstraintTest, DateLeapYear) {
  DatatypeFacetConstraint constraint(XsdDatatype::DATE);

  EXPECT_TRUE(constraint.validate("2024-02-29"));   // 2024 is leap year
  EXPECT_FALSE(constraint.validate("2023-02-29"));  // 2023 is not leap year
  EXPECT_TRUE(constraint.validate("2000-02-29"));   // 2000 is leap year
  EXPECT_FALSE(constraint.validate("1900-02-29"));  // 1900 is not leap year
}

TEST(DatatypeFacetConstraintTest, DateTimeValidation) {
  DatatypeFacetConstraint constraint(XsdDatatype::DATETIME);

  EXPECT_TRUE(constraint.validate("2024-01-15T10:30:00"));
  EXPECT_TRUE(constraint.validate("2024-12-31T23:59:59"));
  EXPECT_TRUE(constraint.validate("2024-01-15T10:30:00Z"));       // With timezone
  EXPECT_TRUE(constraint.validate("2024-01-15T10:30:00+05:30"));  // With offset
  EXPECT_FALSE(constraint.validate("2024-01-15 10:30:00"));       // Missing 'T'
  EXPECT_FALSE(constraint.validate("2024-01-15T25:00:00"));       // Invalid hour
  EXPECT_FALSE(constraint.validate("2024-01-15T10:60:00"));       // Invalid minute
}

TEST(DatatypeFacetConstraintTest, StringValidation) {
  DatatypeFacetConstraint constraint(XsdDatatype::STRING);

  // All strings are valid
  EXPECT_TRUE(constraint.validate(""));
  EXPECT_TRUE(constraint.validate("hello"));
  EXPECT_TRUE(constraint.validate("12345"));
  EXPECT_TRUE(constraint.validate("你好"));
  EXPECT_TRUE(constraint.validate("with spaces"));
}

TEST(DatatypeFacetConstraintTest, UnknownDatatype) {
  DatatypeFacetConstraint constraint(XsdDatatype::UNKNOWN);

  // Unknown datatype allows anything
  EXPECT_TRUE(constraint.validate("anything"));
  EXPECT_TRUE(constraint.validate("123"));
}

// ============================================================================
// Phase 2A: Integration Tests
// ============================================================================

TEST(Phase2AIntegrationTest, NumericRangeWithDatatypeFacet) {
  ValueSetConstraint constraint;
  constraint.datatypeFacet = DatatypeFacetConstraint(XsdDatatype::INTEGER);
  constraint.numericRange = NumericRangeConstraint();
  constraint.numericRange->minInclusive = 1;
  constraint.numericRange->maxInclusive = 100;

  EXPECT_TRUE(constraint.validate("50", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("1", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("100", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("0", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("101", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("50.5", ValueType::LITERAL));  // Not integer
}

TEST(Phase2AIntegrationTest, PatternWithLength) {
  ValueSetConstraint constraint;
  constraint.pattern = PatternConstraint("[a-z]+");
  constraint.length = LengthConstraint();
  constraint.length->minLength = 3;
  constraint.length->maxLength = 10;

  EXPECT_TRUE(constraint.validate("abc", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("abcdefghij", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("ab", ValueType::LITERAL));        // Too short
  EXPECT_FALSE(constraint.validate("abcdefghijk", ValueType::LITERAL));  // Too long
  EXPECT_FALSE(constraint.validate("ABC", ValueType::LITERAL));       // Doesn't match pattern
}

TEST(Phase2AIntegrationTest, AllConstraintsTogether) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::LITERAL;
  constraint.datatypeFacet = DatatypeFacetConstraint(XsdDatatype::DECIMAL);
  constraint.numericRange = NumericRangeConstraint();
  constraint.numericRange->minInclusive = 0.0;
  constraint.numericRange->maxInclusive = 999.99;
  constraint.numericRange->fractionDigits = 2;
  constraint.pattern = PatternConstraint(R"(\d+\.\d{2})");  // Must have exactly 2 decimal places

  EXPECT_TRUE(constraint.validate("123.45", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("0.00", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("999.99", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("123", ValueType::LITERAL));       // No decimal point
  EXPECT_FALSE(constraint.validate("123.4", ValueType::LITERAL));     // Only 1 decimal place
  EXPECT_FALSE(constraint.validate("1000.00", ValueType::LITERAL));   // Exceeds max
  EXPECT_FALSE(constraint.validate("-1.00", ValueType::LITERAL));     // Below min
}

TEST(Phase2AIntegrationTest, LanguageTagWithPattern) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::LITERAL;
  constraint.languageTag = LanguageTagConstraint();
  constraint.languageTag->languagePattern = "en-*";
  constraint.pattern = PatternConstraint("[A-Z][a-z]+");  // Capitalized word

  EXPECT_TRUE(constraint.validate("Hello", ValueType::LITERAL, "en-US"));
  EXPECT_TRUE(constraint.validate("World", ValueType::LITERAL, "en-GB"));
  EXPECT_FALSE(constraint.validate("Hello", ValueType::LITERAL, "fr-FR"));  // Wrong language
  EXPECT_FALSE(constraint.validate("hello", ValueType::LITERAL, "en-US"));  // Doesn't match pattern
}

// ============================================================================
// Phase 2A: Edge Case Tests
// ============================================================================

TEST(Phase2AEdgeCaseTest, VeryLargeNumbers) {
  NumericRangeConstraint constraint;
  constraint.minInclusive = -1e100;
  constraint.maxInclusive = 1e100;

  EXPECT_TRUE(constraint.validate("999999999999999999", XsdDatatype::INTEGER));
  EXPECT_TRUE(constraint.validate("-999999999999999999", XsdDatatype::INTEGER));
}

TEST(Phase2AEdgeCaseTest, VeryLongStrings) {
  LengthConstraint constraint;
  constraint.maxLength = 10000;

  std::string longStr(5000, 'a');
  EXPECT_TRUE(constraint.validate(longStr));

  std::string tooLongStr(10001, 'a');
  EXPECT_FALSE(constraint.validate(tooLongStr));
}

TEST(Phase2AEdgeCaseTest, EmptyPatterns) {
  PatternConstraint constraint("");
  EXPECT_TRUE(constraint.validate(""));
}

TEST(Phase2AEdgeCaseTest, ComplexUTF8) {
  LengthConstraint constraint;
  constraint.exactLength = 5;

  // Combining characters and emojis
  EXPECT_TRUE(constraint.validate("café🎉"));  // 5 characters (e with acute, and emoji)
}

TEST(Phase2AEdgeCaseTest, ScientificNotation) {
  DatatypeFacetConstraint constraint(XsdDatatype::DOUBLE);

  EXPECT_TRUE(constraint.validate("1e10"));
  EXPECT_TRUE(constraint.validate("1.23e-5"));
  EXPECT_TRUE(constraint.validate("-4.56E+7"));
}

TEST(Phase2AEdgeCaseTest, DateBoundaries) {
  DatatypeFacetConstraint constraint(XsdDatatype::DATE);

  EXPECT_TRUE(constraint.validate("0001-01-01"));   // Minimum date
  EXPECT_TRUE(constraint.validate("9999-12-31"));   // Maximum date
  EXPECT_FALSE(constraint.validate("0000-01-01"));  // Invalid year (though technically could be valid in some systems)
}

// ============================================================================
// Phase 2A: Performance/Caching Tests
// ============================================================================

TEST(Phase2APerformanceTest, RegexCachingEfficiency) {
  PatternConstraint constraint("[a-z0-9]+");

  // Measure cache hits
  int iterations = 100;
  for (int i = 0; i < iterations; i++) {
    EXPECT_TRUE(constraint.validate("test123"));
  }

  // Regex should be compiled only once
  EXPECT_TRUE(constraint.compiled);
  EXPECT_NE(constraint.compiledRegex, nullptr);
}

TEST(Phase2APerformanceTest, UTF8CountingEfficiency) {
  LengthConstraint constraint;
  constraint.minLength = 1;
  constraint.maxLength = 1000;

  std::string testStr;
  for (int i = 0; i < 500; i++) {
    testStr += "你";  // Chinese character (3 bytes each)
  }

  EXPECT_TRUE(constraint.validate(testStr));
}
