// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#ifndef QLEVER_SRC_ENGINE_REASONING_N3PARSER_H
#define QLEVER_SRC_ENGINE_REASONING_N3PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "engine/reasoning/Rule.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlTriple.h"

namespace reasoning {

/// Parser for N3 syntax and Datalog-style rules.
/// Extends SPARQL to support:
/// - N3 rule syntax: { body } => { head }
/// - N3 quoting: { triple }
/// - Datalog implications: :predicate :implies :otherPredicate
class N3Parser {
 public:
  N3Parser() = default;

  /// Parse N3 rules from a SPARQL query string.
  /// Handles constructs like:
  /// { ?x :parent ?y } => { ?x :ancestor ?y }
  [[nodiscard]] std::vector<std::shared_ptr<Rule>> parseRulesFromSparql(
      const std::string& sparqlQuery);

  /// Parse a single N3 implication rule from text.
  /// Format: { ?x :p ?y } => { ?x :q ?y }
  [[nodiscard]] std::shared_ptr<Rule> parseImplicationRule(
      const std::string& ruleText);

  /// Extract the body (WHERE clause) from an N3 rule.
  /// Input: "{ ?x :p ?y . ?y :q ?z } => { ?x :r ?z }"
  /// Returns: vector of SparqlTriple representing the body
  [[nodiscard]] std::vector<SparqlTriple> extractRuleBody(
      const std::string& ruleText);

  /// Extract the head (CONSTRUCT clause) from an N3 rule.
  /// Input: "{ ?x :p ?y . ?y :q ?z } => { ?x :r ?z }"
  /// Returns: vector of SparqlTriple representing the head
  [[nodiscard]] std::vector<SparqlTriple> extractRuleHead(
      const std::string& ruleText);

  /// Convert N3 rule syntax to SPARQL INSERT/CONSTRUCT.
  /// { body } => { head } becomes INSERT { head } WHERE { body }
  [[nodiscard]] std::string n3ToSparql(const std::string& n3Rule);

  /// Check if a SPARQL query contains N3 rule syntax.
  [[nodiscard]] static bool isN3Rule(const std::string& sparqlQuery);

  /// Check if a SPARQL query contains N3 quoting (curly braces).
  [[nodiscard]] static bool containsN3Quotes(const std::string& sparqlQuery);

  /// Extract quoted graph patterns (content between { }).
  [[nodiscard]] std::vector<std::string> extractQuotedPatterns(
      const std::string& text);

 private:
  static constexpr const char* IMPLICATION_OPERATORS[] = {"=>", ":implies",
                                                          "log:implies"};

  /// Check if text contains any implication operator.
  [[nodiscard]] static bool containsImplication(const std::string& text);

  /// Find the position of the implication operator.
  [[nodiscard]] static size_t findImplicationOperator(
      const std::string& text);

  /// Extract content between curly braces at a given position.
  /// Returns the content and the position after the closing brace.
  [[nodiscard]] std::pair<std::string, size_t> extractBracedContent(
      const std::string& text, size_t startPos);

  /// Parse a Turtle/SPARQL triple pattern from text.
  [[nodiscard]] SparqlTriple parseTriple(const std::string& tripleText);

  /// Split triple text into subject, predicate, object.
  [[nodiscard]] std::vector<std::string> splitTriple(
      const std::string& tripleText);
};

}  // namespace reasoning

#endif  // QLEVER_SRC_ENGINE_REASONING_N3PARSER_H
