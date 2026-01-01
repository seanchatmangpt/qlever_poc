//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "parser/DatalogParser.h"
#include "parser/DatalogRule.h"
#include "parser/DatalogTokenizer.h"
#include "parser/RuleDatabase.h"
#include "util/ParseException.h"

using ::testing::HasSubstr;

// =============================================================================
// DatalogTokenizer Tests
// =============================================================================

TEST(DatalogTokenizer, BasicTokens) {
  DatalogTokenizer tokenizer("( ) , . :- ?-");

  EXPECT_EQ(tokenizer.next().type, DatalogTokenType::LPAREN);
  EXPECT_EQ(tokenizer.next().type, DatalogTokenType::RPAREN);
  EXPECT_EQ(tokenizer.next().type, DatalogTokenType::COMMA);
  EXPECT_EQ(tokenizer.next().type, DatalogTokenType::DOT);
  EXPECT_EQ(tokenizer.next().type, DatalogTokenType::IMPLIES);
  EXPECT_EQ(tokenizer.next().type, DatalogTokenType::QUERY);
  EXPECT_EQ(tokenizer.next().type, DatalogTokenType::END_OF_FILE);
}

TEST(DatalogTokenizer, Identifiers) {
  DatalogTokenizer tokenizer("ancestor parent foo_bar test123");

  auto token1 = tokenizer.next();
  EXPECT_EQ(token1.type, DatalogTokenType::IDENTIFIER);
  EXPECT_EQ(token1.value, "ancestor");

  auto token2 = tokenizer.next();
  EXPECT_EQ(token2.type, DatalogTokenType::IDENTIFIER);
  EXPECT_EQ(token2.value, "parent");

  auto token3 = tokenizer.next();
  EXPECT_EQ(token3.type, DatalogTokenType::IDENTIFIER);
  EXPECT_EQ(token3.value, "foo_bar");

  auto token4 = tokenizer.next();
  EXPECT_EQ(token4.type, DatalogTokenType::IDENTIFIER);
  EXPECT_EQ(token4.value, "test123");
}

TEST(DatalogTokenizer, Variables) {
  DatalogTokenizer tokenizer("?x ?Var ?test123 ?_underscore");

  auto token1 = tokenizer.next();
  EXPECT_EQ(token1.type, DatalogTokenType::VARIABLE);
  EXPECT_EQ(token1.value, "?x");

  auto token2 = tokenizer.next();
  EXPECT_EQ(token2.type, DatalogTokenType::VARIABLE);
  EXPECT_EQ(token2.value, "?Var");

  auto token3 = tokenizer.next();
  EXPECT_EQ(token3.type, DatalogTokenType::VARIABLE);
  EXPECT_EQ(token3.value, "?test123");

  auto token4 = tokenizer.next();
  EXPECT_EQ(token4.type, DatalogTokenType::VARIABLE);
  EXPECT_EQ(token4.value, "?_underscore");
}

TEST(DatalogTokenizer, IRIs) {
  DatalogTokenizer tokenizer("<http://example.org/parent> <urn:isbn:123>");

  auto token1 = tokenizer.next();
  EXPECT_EQ(token1.type, DatalogTokenType::IRI);
  EXPECT_EQ(token1.value, "<http://example.org/parent>");

  auto token2 = tokenizer.next();
  EXPECT_EQ(token2.type, DatalogTokenType::IRI);
  EXPECT_EQ(token2.value, "<urn:isbn:123>");
}

TEST(DatalogTokenizer, StringLiterals) {
  DatalogTokenizer tokenizer(R"("hello" "world with spaces" "with\"escape")");

  auto token1 = tokenizer.next();
  EXPECT_EQ(token1.type, DatalogTokenType::STRING_LITERAL);
  EXPECT_EQ(token1.value, "\"hello\"");

  auto token2 = tokenizer.next();
  EXPECT_EQ(token2.type, DatalogTokenType::STRING_LITERAL);
  EXPECT_EQ(token2.value, "\"world with spaces\"");

  auto token3 = tokenizer.next();
  EXPECT_EQ(token3.type, DatalogTokenType::STRING_LITERAL);
  EXPECT_EQ(token3.value, "\"with\\\"escape\"");
}

TEST(DatalogTokenizer, CommentsSkipped) {
  DatalogTokenizer tokenizer(R"(
    ancestor # this is a comment
    // another comment
    parent /* block comment */ child
  )");

  EXPECT_EQ(tokenizer.next().value, "ancestor");
  EXPECT_EQ(tokenizer.next().value, "parent");
  EXPECT_EQ(tokenizer.next().value, "child");
}

TEST(DatalogTokenizer, WhitespaceSkipped) {
  DatalogTokenizer tokenizer("  \t\n  foo  \n\r  bar  ");

  EXPECT_EQ(tokenizer.next().value, "foo");
  EXPECT_EQ(tokenizer.next().value, "bar");
}

TEST(DatalogTokenizer, LineAndColumnTracking) {
  DatalogTokenizer tokenizer("foo\n  bar\nbaz");

  auto token1 = tokenizer.next();
  EXPECT_EQ(token1.line, 1);
  EXPECT_EQ(token1.column, 0);

  auto token2 = tokenizer.next();
  EXPECT_EQ(token2.line, 2);

  auto token3 = tokenizer.next();
  EXPECT_EQ(token3.line, 3);
}

TEST(DatalogTokenizer, PeekDoesNotAdvance) {
  DatalogTokenizer tokenizer("foo bar");

  auto peek1 = tokenizer.peek();
  EXPECT_EQ(peek1.value, "foo");

  auto peek2 = tokenizer.peek();
  EXPECT_EQ(peek2.value, "foo");  // Still "foo"

  auto next1 = tokenizer.next();
  EXPECT_EQ(next1.value, "foo");

  auto next2 = tokenizer.next();
  EXPECT_EQ(next2.value, "bar");
}

TEST(DatalogTokenizer, ExpectSuccess) {
  DatalogTokenizer tokenizer("ancestor ( )");

  auto token = tokenizer.expect(DatalogTokenType::IDENTIFIER);
  EXPECT_EQ(token.value, "ancestor");

  tokenizer.expect(DatalogTokenType::LPAREN);
  tokenizer.expect(DatalogTokenType::RPAREN);
}

TEST(DatalogTokenizer, ExpectFailure) {
  DatalogTokenizer tokenizer("ancestor");

  EXPECT_THROW(tokenizer.expect(DatalogTokenType::LPAREN), ParseException);
}

TEST(DatalogTokenizer, UnclosedIRI) {
  DatalogTokenizer tokenizer("<http://example.org");

  EXPECT_THROW(tokenizer.next(), ParseException);
}

TEST(DatalogTokenizer, UnclosedString) {
  DatalogTokenizer tokenizer("\"unclosed string");

  EXPECT_THROW(tokenizer.next(), ParseException);
}

TEST(DatalogTokenizer, EmptyVariable) {
  DatalogTokenizer tokenizer("?");

  EXPECT_THROW(tokenizer.next(), ParseException);
}

// =============================================================================
// DatalogParser Tests - Simple Rules
// =============================================================================

TEST(DatalogParser, SimpleFactRule) {
  auto rule = DatalogParser::parseDatalogRule(
      "ancestor(?x, ?y) :- parent(?x, ?y).");

  EXPECT_EQ(rule.getHeadPredicate(), "ancestor");
  EXPECT_EQ(rule.getHeadVariables().size(), 2);
  EXPECT_EQ(rule.getHeadVariables()[0].name(), "?x");
  EXPECT_EQ(rule.getHeadVariables()[1].name(), "?y");
  EXPECT_EQ(rule.getBodyPatterns().size(), 1);
  EXPECT_FALSE(rule.isRecursive());
}

TEST(DatalogParser, RecursiveRule) {
  auto rule = DatalogParser::parseDatalogRule(
      "ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).");

  EXPECT_EQ(rule.getHeadPredicate(), "ancestor");
  EXPECT_EQ(rule.getHeadVariables().size(), 2);
  EXPECT_EQ(rule.getBodyPatterns().size(), 2);
  EXPECT_TRUE(rule.isRecursive());
}

TEST(DatalogParser, UnaryPredicate) {
  auto rule =
      DatalogParser::parseDatalogRule("person(?x) :- human(?x).");

  EXPECT_EQ(rule.getHeadPredicate(), "person");
  EXPECT_EQ(rule.getHeadVariables().size(), 1);
  EXPECT_EQ(rule.getBodyPatterns().size(), 1);
}

TEST(DatalogParser, MultipleBodyPatterns) {
  auto rule = DatalogParser::parseDatalogRule(
      "related(?x, ?y) :- parent(?x, ?z), parent(?z, ?y), "
      "sibling(?x, ?y).");

  EXPECT_EQ(rule.getHeadPredicate(), "related");
  EXPECT_EQ(rule.getBodyPatterns().size(), 3);
}

TEST(DatalogParser, RuleWithIRIs) {
  auto rule = DatalogParser::parseDatalogRule(
      "knows(?x, ?y) :- friend(?x, <http://example.org/alice>).");

  EXPECT_EQ(rule.getHeadPredicate(), "knows");
  EXPECT_EQ(rule.getBodyPatterns().size(), 1);
}

TEST(DatalogParser, RuleWithStringLiterals) {
  auto rule = DatalogParser::parseDatalogRule(
      R"(named(?x, ?name) :- person(?x, "Alice").)");

  EXPECT_EQ(rule.getHeadPredicate(), "named");
  EXPECT_EQ(rule.getBodyPatterns().size(), 1);
}

TEST(DatalogParser, RuleWithWhitespaceAndComments) {
  auto rule = DatalogParser::parseDatalogRule(R"(
    # This is a comment
    ancestor(?x, ?y) :- // Another comment
      parent(?x, ?y).   # End comment
  )");

  EXPECT_EQ(rule.getHeadPredicate(), "ancestor");
  EXPECT_EQ(rule.getBodyPatterns().size(), 1);
}

TEST(DatalogParser, RuleWithUnderscoreInPredicate) {
  auto rule = DatalogParser::parseDatalogRule(
      "has_parent(?x, ?y) :- parent(?x, ?y).");

  EXPECT_EQ(rule.getHeadPredicate(), "has_parent");
}

TEST(DatalogParser, ThreeArgumentPredicate) {
  auto rule = DatalogParser::parseDatalogRule(
      "triple(?s, ?p, ?o) :- edge(?s, ?p, ?o).");

  EXPECT_EQ(rule.getHeadPredicate(), "triple");
  EXPECT_EQ(rule.getHeadVariables().size(), 3);
}

// =============================================================================
// DatalogParser Tests - Error Cases
// =============================================================================

TEST(DatalogParser, MissingImplies) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("ancestor(?x, ?y) parent(?x, ?y)."),
               ParseException);
}

TEST(DatalogParser, MissingDot) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("ancestor(?x, ?y) :- parent(?x, ?y)"),
               ParseException);
}

TEST(DatalogParser, EmptyBody) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("ancestor(?x, ?y) :- ."),
               ParseException);
}

TEST(DatalogParser, InvalidPredicateName_StartsWithUppercase) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("Ancestor(?x, ?y) :- parent(?x, ?y)."),
               ParseException);
}

TEST(DatalogParser, MissingLeftParen) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("ancestor ?x, ?y) :- parent(?x, ?y)."),
               ParseException);
}

TEST(DatalogParser, MissingRightParen) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("ancestor(?x, ?y :- parent(?x, ?y)."),
               ParseException);
}

TEST(DatalogParser, MissingCommaInArgumentList) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("ancestor(?x ?y) :- parent(?x, ?y)."),
               ParseException);
}

TEST(DatalogParser, TrailingContent) {
  EXPECT_THROW(DatalogParser::parseDatalogRule(
                   "ancestor(?x, ?y) :- parent(?x, ?y). extra"),
               ParseException);
}

TEST(DatalogParser, NoArguments) {
  EXPECT_THROW(DatalogParser::parseDatalogRule("ancestor() :- parent(?x, ?y)."),
               ParseException);
}

// =============================================================================
// DatalogParser Tests - Complete Programs
// =============================================================================

TEST(DatalogParser, SimpleProgramWithOneRule) {
  auto program = DatalogParser::parseDatalogProgram(
      "ancestor(?x, ?y) :- parent(?x, ?y).");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 1);
  EXPECT_EQ(program.queries.size(), 0);
  EXPECT_TRUE(program.ruleDatabase.hasRuleFor("ancestor"));
}

TEST(DatalogParser, ProgramWithMultipleRules) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    ancestor(?x, ?y) :- parent(?x, ?y).
    ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
    sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y).
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 3);
  EXPECT_EQ(program.ruleDatabase.getPredicateCount(), 2);
  EXPECT_TRUE(program.ruleDatabase.hasRuleFor("ancestor"));
  EXPECT_TRUE(program.ruleDatabase.hasRuleFor("sibling"));

  auto ancestorRules = program.ruleDatabase.getRulesByPredicate("ancestor");
  EXPECT_EQ(ancestorRules.size(), 2);
}

TEST(DatalogParser, ProgramWithQuery) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    ancestor(?x, ?y) :- parent(?x, ?y).
    ?- ancestor(?alice, ?bob).
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 1);
  EXPECT_EQ(program.queries.size(), 1);

  const auto& query = program.queries[0];
  EXPECT_EQ(query.queryPredicate, "ancestor");
  EXPECT_EQ(query.queryVars.size(), 2);
  EXPECT_EQ(query.queryVars[0].name(), "?alice");
  EXPECT_EQ(query.queryVars[1].name(), "?bob");
}

TEST(DatalogParser, ProgramWithMultipleQueries) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    ancestor(?x, ?y) :- parent(?x, ?y).
    ?- ancestor(?alice, ?bob).
    ?- ancestor(?charlie, ?dave).
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 1);
  EXPECT_EQ(program.queries.size(), 2);
}

TEST(DatalogParser, ComplexProgram) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    # Ancestry rules
    ancestor(?x, ?y) :- parent(?x, ?y).
    ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

    # Sibling rules
    sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y).

    # Cousin rules
    cousin(?x, ?y) :- parent(?px, ?x), parent(?py, ?y), sibling(?px, ?py).

    # Queries
    ?- ancestor(?alice, ?bob).
    ?- cousin(?x, ?y).
  )");

  EXPECT_EQ(program.ruleDatabase.getPredicateCount(), 3);
  EXPECT_GE(program.ruleDatabase.getRuleCount(), 4);
  EXPECT_EQ(program.queries.size(), 2);
}

TEST(DatalogParser, EmptyProgram) {
  auto program = DatalogParser::parseDatalogProgram("");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 0);
  EXPECT_EQ(program.queries.size(), 0);
}

TEST(DatalogParser, ProgramWithOnlyComments) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    # This is a comment
    // Another comment
    /* Block comment */
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 0);
  EXPECT_EQ(program.queries.size(), 0);
}

// =============================================================================
// DatalogParser Tests - Arity Consistency
// =============================================================================

TEST(DatalogParser, ConsistentArity) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    ancestor(?x, ?y) :- parent(?x, ?y).
    ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 2);
  auto rules = program.ruleDatabase.getRulesByPredicate("ancestor");
  EXPECT_EQ(rules.size(), 2);
  EXPECT_EQ(rules[0].getArity(), 2);
  EXPECT_EQ(rules[1].getArity(), 2);
}

TEST(DatalogParser, InconsistentArity) {
  EXPECT_THROW(DatalogParser::parseDatalogProgram(R"(
    ancestor(?x, ?y) :- parent(?x, ?y).
    ancestor(?x) :- person(?x).
  )"),
               ParseException);
}

// =============================================================================
// DatalogParser Tests - Recursion Detection
// =============================================================================

TEST(DatalogParser, NonRecursiveRule) {
  auto rule = DatalogParser::parseDatalogRule(
      "sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y).");

  EXPECT_FALSE(rule.isRecursive());
}

TEST(DatalogParser, DirectRecursion) {
  auto rule = DatalogParser::parseDatalogRule(
      "ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).");

  EXPECT_TRUE(rule.isRecursive());
}

TEST(DatalogParser, RecursionInFirstPosition) {
  auto rule = DatalogParser::parseDatalogRule(
      "path(?x, ?z) :- path(?x, ?y), edge(?y, ?z).");

  EXPECT_TRUE(rule.isRecursive());
}

// =============================================================================
// DatalogParser Tests - Real-world Examples
// =============================================================================

TEST(DatalogParser, TransitiveClosureExample) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    # Base case: direct parent is ancestor
    ancestor(?x, ?y) :- parent(?x, ?y).

    # Recursive case: transitive closure
    ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

    # Query for all ancestors
    ?- ancestor(?person, ?ancestor).
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 2);
  EXPECT_EQ(program.queries.size(), 1);

  auto rules = program.ruleDatabase.getRulesByPredicate("ancestor");
  EXPECT_EQ(rules.size(), 2);
  EXPECT_FALSE(rules[0].isRecursive());
  EXPECT_TRUE(rules[1].isRecursive());
}

TEST(DatalogParser, SameGenerationExample) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    # Same generation base case
    same_gen(?x, ?x) :- person(?x).

    # Recursive case
    same_gen(?x, ?y) :- parent(?px, ?x), parent(?py, ?y), same_gen(?px, ?py).

    ?- same_gen(?alice, ?bob).
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 2);
  EXPECT_EQ(program.queries.size(), 1);
}

TEST(DatalogParser, GraphReachabilityExample) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    # Reachability in a graph
    reachable(?x, ?y) :- edge(?x, ?y).
    reachable(?x, ?z) :- edge(?x, ?y), reachable(?y, ?z).

    ?- reachable(?start, ?end).
  )");

  EXPECT_EQ(program.ruleDatabase.getRuleCount(), 2);
  auto rules = program.ruleDatabase.getRulesByPredicate("reachable");
  EXPECT_EQ(rules.size(), 2);
  EXPECT_TRUE(rules[1].isRecursive());
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(DatalogParser, EndToEndParseAndRetrieve) {
  auto program = DatalogParser::parseDatalogProgram(R"(
    parent(?x, ?y) :- has_child(?x, ?y).
    ancestor(?x, ?y) :- parent(?x, ?y).
    ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
  )");

  // Verify we can retrieve rules
  EXPECT_TRUE(program.ruleDatabase.hasRuleFor("parent"));
  EXPECT_TRUE(program.ruleDatabase.hasRuleFor("ancestor"));

  auto parentRules = program.ruleDatabase.getRulesByPredicate("parent");
  EXPECT_EQ(parentRules.size(), 1);
  EXPECT_EQ(parentRules[0].getArity(), 2);

  auto ancestorRules = program.ruleDatabase.getRulesByPredicate("ancestor");
  EXPECT_EQ(ancestorRules.size(), 2);
  EXPECT_FALSE(ancestorRules[0].isRecursive());
  EXPECT_TRUE(ancestorRules[1].isRecursive());
}

TEST(DatalogParser, RoundTripToString) {
  std::string originalRule = "ancestor(?x, ?y) :- parent(?x, ?y).";
  auto rule = DatalogParser::parseDatalogRule(originalRule);

  // Convert back to string
  std::string ruleStr = rule.toString();

  // Should contain key components
  EXPECT_THAT(ruleStr, HasSubstr("ancestor"));
  EXPECT_THAT(ruleStr, HasSubstr("?x"));
  EXPECT_THAT(ruleStr, HasSubstr("?y"));
  EXPECT_THAT(ruleStr, HasSubstr(":-"));
}
