//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#ifndef PARSER_DATALOG_TOKENIZER_H
#define PARSER_DATALOG_TOKENIZER_H

#include <string>
#include <string_view>
#include <vector>

#include "util/Exception.h"

/// Token types for Datalog syntax
enum class DatalogTokenType {
  // Identifiers and values
  IDENTIFIER,      // Predicate names (lowercase identifiers)
  VARIABLE,        // ?X, ?Var (SPARQL-style variables)
  IRI,             // <http://example.org/resource>
  STRING_LITERAL,  // "string" or "string"^^<type>

  // Operators and delimiters
  LPAREN,    // (
  RPAREN,    // )
  COMMA,     // ,
  DOT,       // .
  IMPLIES,   // :-
  QUERY,     // ?-
  DATATYPE,  // ^^

  // Special
  END_OF_FILE,
  INVALID
};

/// Represents a single token in Datalog source
struct DatalogToken {
  DatalogTokenType type;
  std::string value;
  size_t line;
  size_t column;

  DatalogToken(DatalogTokenType t, std::string v, size_t l, size_t c)
      : type(t), value(std::move(v)), line(l), column(c) {}

  /// Get human-readable token type name
  [[nodiscard]] std::string typeName() const;
};

/// Tokenizer for Datalog syntax
/// Converts Datalog text into a stream of tokens for parsing
///
/// Example usage:
///   DatalogTokenizer tokenizer("ancestor(?x, ?y) :- parent(?x, ?y).");
///   while (tokenizer.hasNext()) {
///     auto token = tokenizer.next();
///     // Process token...
///   }
class DatalogTokenizer {
 public:
  /// Construct tokenizer from Datalog source text
  explicit DatalogTokenizer(std::string_view source);

  /// Check if more tokens are available
  [[nodiscard]] bool hasNext() const;

  /// Get the next token (advances position)
  DatalogToken next();

  /// Peek at the next token without consuming it
  [[nodiscard]] DatalogToken peek() const;

  /// Expect a specific token type and consume it
  /// Throws ParseException if the next token doesn't match
  DatalogToken expect(DatalogTokenType expectedType);

  /// Get current line number (1-based)
  [[nodiscard]] size_t currentLine() const { return line_; }

  /// Get current column (0-based)
  [[nodiscard]] size_t currentColumn() const { return column_; }

 private:
  /// Skip whitespace and comments
  void skipWhitespaceAndComments();

  /// Check if character is whitespace
  static bool isWhitespace(char c);

  /// Check if character can start an identifier
  static bool isIdentifierStart(char c);

  /// Check if character can be part of an identifier
  static bool isIdentifierPart(char c);

  /// Tokenize an identifier (predicate name)
  DatalogToken tokenizeIdentifier();

  /// Tokenize a variable (?X, ?Var)
  DatalogToken tokenizeVariable();

  /// Tokenize an IRI (<http://...>)
  DatalogToken tokenizeIri();

  /// Tokenize a string literal ("string" or "string"^^<type>)
  DatalogToken tokenizeStringLiteral();

  /// Peek at the current character without consuming
  [[nodiscard]] char peekChar() const;

  /// Peek at character at offset from current position
  [[nodiscard]] char peekChar(size_t offset) const;

  /// Get current character and advance position
  char consumeChar();

  /// Check if we're at end of input
  [[nodiscard]] bool atEnd() const;

  /// Source text being tokenized
  std::string source_;

  /// Current position in source
  size_t position_ = 0;

  /// Current line number (1-based)
  size_t line_ = 1;

  /// Current column (0-based)
  size_t column_ = 0;
};

#endif  // PARSER_DATALOG_TOKENIZER_H
