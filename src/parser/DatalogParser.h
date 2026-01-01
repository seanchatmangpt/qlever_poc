//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#ifndef PARSER_DATALOG_PARSER_H
#define PARSER_DATALOG_PARSER_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "parser/DatalogRule.h"
#include "parser/DatalogTokenizer.h"
#include "parser/RuleDatabase.h"

/// Parser for Datalog rule definitions and queries
///
/// Supports Datalog syntax with SPARQL-style variables:
///   ancestor(?x, ?y) :- parent(?x, ?y).
///   ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
///
/// Also supports queries:
///   ancestor(?x, ?y) :- parent(?x, ?y).
///   ?- ancestor(?alice, ?bob).
///
/// Features:
/// - SPARQL-style variables (?x, ?Var)
/// - Predicate names (lowercase identifiers)
/// - IRIs: <http://example.org/resource>
/// - String literals: "text" or "text"^^<datatype>
/// - Comments: # comment or // comment
/// - Multiple rules in one program
///
/// Example usage:
///   auto rule = DatalogParser::parseDatalogRule(
///       "ancestor(?x, ?y) :- parent(?x, ?y).");
///
///   auto [db, queryInfo] = DatalogParser::parseDatalogProgram(R"(
///       ancestor(?x, ?y) :- parent(?x, ?y).
///       ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
///       ?- ancestor(?alice, ?bob).
///   )");
class DatalogParser {
 public:
  /// Parse a single Datalog rule from text
  /// Syntax: head(Vars) :- body_pattern1, body_pattern2, ...
  ///
  /// @param ruleText The Datalog rule text to parse
  /// @return Parsed DatalogRule object
  /// @throws ParseException if syntax is invalid
  static DatalogRule parseDatalogRule(std::string_view ruleText);

  /// Information about a query in a Datalog program
  struct QueryInfo {
    std::string queryPredicate;        // Predicate being queried
    std::vector<Variable> queryVars;   // Variables in the query
    size_t queryLine;                  // Line number where query appears

    QueryInfo(std::string predicate, std::vector<Variable> vars, size_t line)
        : queryPredicate(std::move(predicate)),
          queryVars(std::move(vars)),
          queryLine(line) {}
  };

  /// Result of parsing a complete Datalog program
  struct ParsedProgram {
    RuleDatabase ruleDatabase;              // All parsed rules
    std::vector<QueryInfo> queries;         // All queries in the program
    std::vector<std::string> warnings;      // Any parse warnings
  };

  /// Parse a complete Datalog program (multiple rules + optional queries)
  /// Syntax:
  ///   rule1(Vars) :- body1.
  ///   rule2(Vars) :- body2.
  ///   ?- query(Vars).
  ///
  /// @param programText The Datalog program text to parse
  /// @return ParsedProgram containing rules, queries, and warnings
  /// @throws ParseException if syntax is invalid
  static ParsedProgram parseDatalogProgram(std::string_view programText);

 private:
  /// Private constructor - parser is stateless and uses static methods
  DatalogParser() = delete;

  /// Parse rule head: predicate(var1, var2, ...)
  /// Returns predicate name and list of variables
  static std::pair<std::string, std::vector<Variable>> parseHead(
      DatalogTokenizer& tokenizer);

  /// Parse rule body: pattern1, pattern2, ...
  /// Returns list of triple patterns
  static std::vector<SparqlTriple> parseBody(DatalogTokenizer& tokenizer);

  /// Parse a single atom/pattern in the body
  /// Converts Datalog atom to SparqlTriple
  static SparqlTriple parseAtom(DatalogTokenizer& tokenizer);

  /// Parse argument list: (arg1, arg2, ...)
  /// Returns list of TripleComponents
  static std::vector<TripleComponent> parseArgumentList(
      DatalogTokenizer& tokenizer);

  /// Parse a single term (variable, IRI, or literal)
  static TripleComponent parseTerm(DatalogTokenizer& tokenizer);

  /// Validate that a predicate name is valid (lowercase identifier)
  static void validatePredicateName(const std::string& name, size_t line,
                                    size_t column);

  /// Check if two rules with same head have consistent arity
  static void checkArityConsistency(const std::string& predicate,
                                    size_t expectedArity, size_t actualArity,
                                    size_t line);

  /// Detect if a rule is recursive (references its own head predicate)
  static bool isRecursiveRule(const std::string& headPredicate,
                              const std::vector<SparqlTriple>& bodyPatterns);

  /// Convert parser errors to ParseException with location info
  [[noreturn]] static void throwParseError(const std::string& message,
                                           size_t line, size_t column);
};

#endif  // PARSER_DATALOG_PARSER_H
