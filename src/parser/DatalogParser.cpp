//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#include "parser/DatalogParser.h"

#include <absl/strings/str_cat.h>

#include <cctype>
#include <sstream>

#include "parser/PropertyPath.h"
#include "parser/TripleComponent.h"
#include "util/ParseException.h"

// =============================================================================
// DatalogToken Implementation
// =============================================================================

std::string DatalogToken::typeName() const {
  switch (type) {
    case DatalogTokenType::IDENTIFIER:
      return "IDENTIFIER";
    case DatalogTokenType::VARIABLE:
      return "VARIABLE";
    case DatalogTokenType::IRI:
      return "IRI";
    case DatalogTokenType::STRING_LITERAL:
      return "STRING_LITERAL";
    case DatalogTokenType::LPAREN:
      return "LPAREN";
    case DatalogTokenType::RPAREN:
      return "RPAREN";
    case DatalogTokenType::COMMA:
      return "COMMA";
    case DatalogTokenType::DOT:
      return "DOT";
    case DatalogTokenType::IMPLIES:
      return "IMPLIES";
    case DatalogTokenType::QUERY:
      return "QUERY";
    case DatalogTokenType::DATATYPE:
      return "DATATYPE";
    case DatalogTokenType::END_OF_FILE:
      return "END_OF_FILE";
    case DatalogTokenType::INVALID:
      return "INVALID";
    default:
      return "UNKNOWN";
  }
}

// =============================================================================
// DatalogTokenizer Implementation
// =============================================================================

DatalogTokenizer::DatalogTokenizer(std::string_view source) : source_(source) {
  skipWhitespaceAndComments();
}

bool DatalogTokenizer::hasNext() const {
  return !atEnd() && peekChar() != '\0';
}

DatalogToken DatalogTokenizer::next() {
  skipWhitespaceAndComments();

  if (atEnd()) {
    return DatalogToken(DatalogTokenType::END_OF_FILE, "", line_, column_);
  }

  char current = peekChar();
  size_t tokenLine = line_;
  size_t tokenColumn = column_;

  // Check for two-character operators first
  if (current == ':' && peekChar(1) == '-') {
    consumeChar();  // consume ':'
    consumeChar();  // consume '-'
    return DatalogToken(DatalogTokenType::IMPLIES, ":-", tokenLine,
                        tokenColumn);
  }

  if (current == '?' && peekChar(1) == '-') {
    consumeChar();  // consume '?'
    consumeChar();  // consume '-'
    return DatalogToken(DatalogTokenType::QUERY, "?-", tokenLine, tokenColumn);
  }

  if (current == '^' && peekChar(1) == '^') {
    consumeChar();  // consume '^'
    consumeChar();  // consume '^'
    return DatalogToken(DatalogTokenType::DATATYPE, "^^", tokenLine,
                        tokenColumn);
  }

  // Single character tokens
  switch (current) {
    case '(':
      consumeChar();
      return DatalogToken(DatalogTokenType::LPAREN, "(", tokenLine,
                          tokenColumn);
    case ')':
      consumeChar();
      return DatalogToken(DatalogTokenType::RPAREN, ")", tokenLine,
                          tokenColumn);
    case ',':
      consumeChar();
      return DatalogToken(DatalogTokenType::COMMA, ",", tokenLine, tokenColumn);
    case '.':
      consumeChar();
      return DatalogToken(DatalogTokenType::DOT, ".", tokenLine, tokenColumn);
  }

  // Variables: ?X, ?Var, ?var
  if (current == '?' && isIdentifierStart(peekChar(1))) {
    return tokenizeVariable();
  }

  // IRIs: <http://...>
  if (current == '<') {
    return tokenizeIri();
  }

  // String literals: "text"
  if (current == '"') {
    return tokenizeStringLiteral();
  }

  // Identifiers (predicate names): ancestor, parent
  if (isIdentifierStart(current)) {
    return tokenizeIdentifier();
  }

  // Invalid character
  consumeChar();
  throw ParseException(absl::StrCat("Unexpected character '",
                                    std::string(1, current), "' at line ",
                                    tokenLine, ", column ", tokenColumn));
}

DatalogToken DatalogTokenizer::peek() const {
  // Create a copy of the tokenizer to peek without modifying state
  DatalogTokenizer copy = *this;
  return copy.next();
}

DatalogToken DatalogTokenizer::expect(DatalogTokenType expectedType) {
  DatalogToken token = next();
  if (token.type != expectedType) {
    throw ParseException(absl::StrCat("Expected ", token.typeName(),
                                      " but got '", token.value, "' at line ",
                                      token.line, ", column ", token.column));
  }
  return token;
}

void DatalogTokenizer::skipWhitespaceAndComments() {
  while (!atEnd()) {
    char c = peekChar();

    // Skip whitespace
    if (isWhitespace(c)) {
      consumeChar();
      continue;
    }

    // Skip line comments: # comment or // comment
    if (c == '#' || (c == '/' && peekChar(1) == '/')) {
      // Skip until end of line
      while (!atEnd() && peekChar() != '\n') {
        consumeChar();
      }
      if (!atEnd() && peekChar() == '\n') {
        consumeChar();
      }
      continue;
    }

    // Skip block comments: /* comment */
    if (c == '/' && peekChar(1) == '*') {
      consumeChar();  // consume '/'
      consumeChar();  // consume '*'
      while (!atEnd() && !(peekChar() == '*' && peekChar(1) == '/')) {
        consumeChar();
      }
      if (!atEnd()) {
        consumeChar();  // consume '*'
        consumeChar();  // consume '/'
      }
      continue;
    }

    // No more whitespace or comments
    break;
  }
}

bool DatalogTokenizer::isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool DatalogTokenizer::isIdentifierStart(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool DatalogTokenizer::isIdentifierPart(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

DatalogToken DatalogTokenizer::tokenizeIdentifier() {
  size_t tokenLine = line_;
  size_t tokenColumn = column_;
  std::string value;

  while (!atEnd() && isIdentifierPart(peekChar())) {
    value += consumeChar();
  }

  return DatalogToken(DatalogTokenType::IDENTIFIER, value, tokenLine,
                      tokenColumn);
}

DatalogToken DatalogTokenizer::tokenizeVariable() {
  size_t tokenLine = line_;
  size_t tokenColumn = column_;
  std::string value;

  // Consume '?'
  value += consumeChar();

  // Consume identifier part
  while (!atEnd() && isIdentifierPart(peekChar())) {
    value += consumeChar();
  }

  if (value.length() == 1) {
    throw ParseException(absl::StrCat("Invalid variable '?' at line ",
                                      tokenLine, ", column ", tokenColumn,
                                      ": variable name cannot be empty"));
  }

  return DatalogToken(DatalogTokenType::VARIABLE, value, tokenLine,
                      tokenColumn);
}

DatalogToken DatalogTokenizer::tokenizeIri() {
  size_t tokenLine = line_;
  size_t tokenColumn = column_;
  std::string value;

  // Consume '<'
  value += consumeChar();

  // Consume until '>'
  while (!atEnd() && peekChar() != '>') {
    char c = peekChar();
    if (c == '\n' || c == '\r') {
      throw ParseException(absl::StrCat("Unclosed IRI at line ", tokenLine,
                                        ", column ", tokenColumn,
                                        ": newline in IRI"));
    }
    value += consumeChar();
  }

  if (atEnd()) {
    throw ParseException(absl::StrCat("Unclosed IRI at line ", tokenLine,
                                      ", column ", tokenColumn,
                                      ": missing closing '>'"));
  }

  // Consume '>'
  value += consumeChar();

  return DatalogToken(DatalogTokenType::IRI, value, tokenLine, tokenColumn);
}

DatalogToken DatalogTokenizer::tokenizeStringLiteral() {
  size_t tokenLine = line_;
  size_t tokenColumn = column_;
  std::string value;

  // Consume opening '"'
  value += consumeChar();

  // Consume until closing '"'
  while (!atEnd() && peekChar() != '"') {
    char c = peekChar();

    // Handle escape sequences
    if (c == '\\') {
      value += consumeChar();  // consume '\'
      if (!atEnd()) {
        value += consumeChar();  // consume escaped character
      }
      continue;
    }

    if (c == '\n' || c == '\r') {
      throw ParseException(absl::StrCat("Unclosed string literal at line ",
                                        tokenLine, ", column ", tokenColumn,
                                        ": newline in string"));
    }

    value += consumeChar();
  }

  if (atEnd()) {
    throw ParseException(absl::StrCat("Unclosed string literal at line ",
                                      tokenLine, ", column ", tokenColumn,
                                      ": missing closing '\"'"));
  }

  // Consume closing '"'
  value += consumeChar();

  return DatalogToken(DatalogTokenType::STRING_LITERAL, value, tokenLine,
                      tokenColumn);
}

char DatalogTokenizer::peekChar() const { return peekChar(0); }

char DatalogTokenizer::peekChar(size_t offset) const {
  if (position_ + offset >= source_.length()) {
    return '\0';
  }
  return source_[position_ + offset];
}

char DatalogTokenizer::consumeChar() {
  if (atEnd()) {
    return '\0';
  }

  char c = source_[position_++];

  if (c == '\n') {
    line_++;
    column_ = 0;
  } else {
    column_++;
  }

  return c;
}

bool DatalogTokenizer::atEnd() const { return position_ >= source_.length(); }

// =============================================================================
// DatalogParser Implementation
// =============================================================================

DatalogRule DatalogParser::parseDatalogRule(std::string_view ruleText) {
  DatalogTokenizer tokenizer(ruleText);

  // Parse head: predicate(var1, var2, ...)
  auto [headPredicate, headVars] = parseHead(tokenizer);

  // Expect ':-' (implies)
  tokenizer.expect(DatalogTokenType::IMPLIES);

  // Parse body: pattern1, pattern2, ...
  auto bodyPatterns = parseBody(tokenizer);

  // Expect '.' (end of rule)
  tokenizer.expect(DatalogTokenType::DOT);

  // Check for trailing content
  if (tokenizer.hasNext() &&
      tokenizer.peek().type != DatalogTokenType::END_OF_FILE) {
    auto extra = tokenizer.peek();
    throw ParseException(absl::StrCat("Unexpected content after rule: '",
                                      extra.value, "' at line ", extra.line,
                                      ", column ", extra.column));
  }

  // Detect if rule is recursive
  bool isRecursive = isRecursiveRule(headPredicate, bodyPatterns);

  return DatalogRule(std::move(headPredicate), std::move(headVars),
                     std::move(bodyPatterns), {}, isRecursive);
}

DatalogParser::ParsedProgram DatalogParser::parseDatalogProgram(
    std::string_view programText) {
  ParsedProgram result;
  DatalogTokenizer tokenizer(programText);

  while (tokenizer.hasNext() &&
         tokenizer.peek().type != DatalogTokenType::END_OF_FILE) {
    // Check for query: ?- predicate(...)
    if (tokenizer.peek().type == DatalogTokenType::QUERY) {
      tokenizer.next();  // consume '?-'

      // Parse query predicate and arguments
      auto [queryPredicate, queryVars] = parseHead(tokenizer);
      tokenizer.expect(DatalogTokenType::DOT);

      result.queries.emplace_back(std::move(queryPredicate),
                                  std::move(queryVars),
                                  tokenizer.currentLine());
      continue;
    }

    // Parse rule
    auto [headPredicate, headVars] = parseHead(tokenizer);
    size_t ruleArity = headVars.size();
    size_t ruleLine = tokenizer.currentLine();

    tokenizer.expect(DatalogTokenType::IMPLIES);
    auto bodyPatterns = parseBody(tokenizer);
    tokenizer.expect(DatalogTokenType::DOT);

    // Check arity consistency with existing rules
    auto existingRules = result.ruleDatabase.getRulesByPredicate(headPredicate);
    if (!existingRules.empty()) {
      size_t expectedArity = existingRules[0].getArity();
      if (ruleArity != expectedArity) {
        checkArityConsistency(headPredicate, expectedArity, ruleArity,
                              ruleLine);
      }
    }

    // Create and add rule
    bool isRecursive = isRecursiveRule(headPredicate, bodyPatterns);
    DatalogRule rule(std::move(headPredicate), std::move(headVars),
                     std::move(bodyPatterns), {}, isRecursive);
    result.ruleDatabase.addRule(std::move(rule));
  }

  return result;
}

std::pair<std::string, std::vector<Variable>> DatalogParser::parseHead(
    DatalogTokenizer& tokenizer) {
  // Parse predicate name
  DatalogToken predicateToken = tokenizer.expect(DatalogTokenType::IDENTIFIER);
  std::string predicate = predicateToken.value;
  validatePredicateName(predicate, predicateToken.line, predicateToken.column);

  // Parse argument list: (var1, var2, ...)
  tokenizer.expect(DatalogTokenType::LPAREN);

  std::vector<Variable> variables;
  bool first = true;

  while (tokenizer.peek().type != DatalogTokenType::RPAREN) {
    if (!first) {
      tokenizer.expect(DatalogTokenType::COMMA);
    }
    first = false;

    // Expect variable
    DatalogToken varToken = tokenizer.expect(DatalogTokenType::VARIABLE);
    variables.emplace_back(varToken.value);
  }

  tokenizer.expect(DatalogTokenType::RPAREN);

  return {predicate, variables};
}

std::vector<SparqlTriple> DatalogParser::parseBody(
    DatalogTokenizer& tokenizer) {
  std::vector<SparqlTriple> patterns;
  bool first = true;

  // Parse comma-separated list of atoms
  while (tokenizer.peek().type != DatalogTokenType::DOT) {
    if (!first) {
      tokenizer.expect(DatalogTokenType::COMMA);
    }
    first = false;

    patterns.push_back(parseAtom(tokenizer));
  }

  if (patterns.empty()) {
    throw ParseException(absl::StrCat("Empty rule body at line ",
                                      tokenizer.currentLine(), ", column ",
                                      tokenizer.currentColumn()));
  }

  return patterns;
}

SparqlTriple DatalogParser::parseAtom(DatalogTokenizer& tokenizer) {
  // Parse: predicate(arg1, arg2, arg3)
  // Convert to triple: arg1 predicate arg2
  // For arity > 2, we need special handling

  DatalogToken predicateToken = tokenizer.expect(DatalogTokenType::IDENTIFIER);
  std::string predicate = predicateToken.value;

  auto args = parseArgumentList(tokenizer);

  // Convert to IRI for predicate
  std::string predicateIri =
      "<http://qlever.datalog.predicate/" + predicate + ">";
  auto predicateIriComponent = TripleComponent::Iri::fromIriref(predicateIri);

  if (args.size() == 2) {
    // Binary predicate: subject predicate object
    return SparqlTriple(args[0], predicateIriComponent, args[1]);
  } else if (args.size() == 1) {
    // Unary predicate: subject rdf:type predicate
    auto rdfType = TripleComponent::Iri::fromIriref(
        "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>");
    return SparqlTriple(args[0], rdfType,
                        TripleComponent(predicateIriComponent));
  } else if (args.size() == 0) {
    throw ParseException(
        absl::StrCat("Predicate '", predicate, "' has no arguments at line ",
                     predicateToken.line, ", column ", predicateToken.column));
  } else {
    // N-ary predicate (n > 2): use reification approach
    // For now, we'll use the first two arguments as subject/object
    // and encode the arity in the predicate IRI
    std::string naryPredicateIri = "<http://qlever.datalog.predicate/" +
                                   predicate + "/arity" +
                                   std::to_string(args.size()) + ">";
    auto naryIri = TripleComponent::Iri::fromIriref(naryPredicateIri);
    return SparqlTriple(args[0], naryIri, args[1]);
  }
}

std::vector<TripleComponent> DatalogParser::parseArgumentList(
    DatalogTokenizer& tokenizer) {
  tokenizer.expect(DatalogTokenType::LPAREN);

  std::vector<TripleComponent> args;
  bool first = true;

  while (tokenizer.peek().type != DatalogTokenType::RPAREN) {
    if (!first) {
      tokenizer.expect(DatalogTokenType::COMMA);
    }
    first = false;

    args.push_back(parseTerm(tokenizer));
  }

  tokenizer.expect(DatalogTokenType::RPAREN);
  return args;
}

TripleComponent DatalogParser::parseTerm(DatalogTokenizer& tokenizer) {
  DatalogToken token = tokenizer.peek();

  switch (token.type) {
    case DatalogTokenType::VARIABLE: {
      tokenizer.next();  // consume
      return TripleComponent(Variable(token.value));
    }

    case DatalogTokenType::IRI: {
      tokenizer.next();  // consume
      // Remove < and > brackets
      std::string iriValue = token.value.substr(1, token.value.length() - 2);
      return TripleComponent(
          TripleComponent::Iri::fromIriref("<" + iriValue + ">"));
    }

    case DatalogTokenType::STRING_LITERAL: {
      tokenizer.next();  // consume
      // Check for datatype annotation: "text"^^<type>
      if (tokenizer.peek().type == DatalogTokenType::DATATYPE) {
        tokenizer.next();  // consume '^^'
        DatalogToken datatypeToken = tokenizer.expect(DatalogTokenType::IRI);
        // Create literal with datatype
        // For now, just use the string value
        return TripleComponent(token.value);
      }
      // Plain string literal
      return TripleComponent(token.value);
    }

    default:
      throw ParseException(absl::StrCat(
          "Expected term (variable, IRI, or "
          "literal) but got '",
          token.value, "' at line ", token.line, ", column ", token.column));
  }
}

void DatalogParser::validatePredicateName(const std::string& name, size_t line,
                                          size_t column) {
  if (name.empty()) {
    throwParseError("Predicate name cannot be empty", line, column);
  }

  // Check that name starts with lowercase letter or underscore
  if (!std::islower(static_cast<unsigned char>(name[0])) && name[0] != '_') {
    throwParseError(
        absl::StrCat("Predicate name '", name,
                     "' must start with lowercase letter or underscore"),
        line, column);
  }

  // Check that rest of name contains only alphanumeric or underscore
  for (char c : name) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
      throwParseError(absl::StrCat("Invalid character '", std::string(1, c),
                                   "' in predicate name '", name, "'"),
                      line, column);
    }
  }
}

void DatalogParser::checkArityConsistency(const std::string& predicate,
                                          size_t expectedArity,
                                          size_t actualArity, size_t line) {
  if (expectedArity != actualArity) {
    throw ParseException(absl::StrCat(
        "Arity mismatch for predicate '", predicate, "' at line ", line,
        ": expected ", expectedArity, " arguments but got ", actualArity));
  }
}

bool DatalogParser::isRecursiveRule(
    const std::string& headPredicate,
    const std::vector<SparqlTriple>& bodyPatterns) {
  // Check if any body pattern references the head predicate
  // This is a simple check - we look for the predicate IRI in the patterns
  std::string predicateIri =
      "<http://qlever.datalog.predicate/" + headPredicate + ">";

  for (const auto& pattern : bodyPatterns) {
    // Check if predicate matches
    auto simplePred = pattern.getSimplePredicate();
    if (simplePred.has_value() &&
        simplePred.value().find(headPredicate) != std::string_view::npos) {
      return true;
    }
  }

  return false;
}

void DatalogParser::throwParseError(const std::string& message, size_t line,
                                    size_t column) {
  throw ParseException(
      absl::StrCat(message, " at line ", line, ", column ", column));
}
