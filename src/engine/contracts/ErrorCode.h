// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant

#ifndef QLEVER_SRC_ENGINE_CONTRACTS_ERRORCODE_H
#define QLEVER_SRC_ENGINE_CONTRACTS_ERRORCODE_H

#include <string_view>

namespace qlever::contracts {

// Comprehensive error taxonomy for validation services (SHACL, ShEx), rules
// processing, N3 reasoning, and guard conditions. Error codes are stable and
// immutable to ensure backward compatibility across service versions.
enum class ErrorCode {
  // Generic errors (0-99)
  UNKNOWN_ERROR = 0,
  INTERNAL_ERROR = 1,
  NOT_IMPLEMENTED = 2,
  INVALID_ARGUMENT = 3,
  RESOURCE_EXHAUSTED = 4,
  TIMEOUT = 5,

  // Parsing errors (100-199)
  PARSE_ERROR = 100,
  SYNTAX_ERROR = 101,
  INVALID_IRI = 102,
  INVALID_LITERAL = 103,
  INVALID_BLANK_NODE = 104,
  INVALID_PREFIX = 105,
  MALFORMED_SHAPE = 106,
  MALFORMED_RULE = 107,
  MALFORMED_FORMULA = 108,

  // Validation errors - SHACL (200-299)
  SHACL_VALIDATION_ERROR = 200,
  SHACL_NODE_CONSTRAINT_VIOLATION = 201,
  SHACL_PROPERTY_CONSTRAINT_VIOLATION = 202,
  SHACL_CLASS_CONSTRAINT_VIOLATION = 203,
  SHACL_DATATYPE_CONSTRAINT_VIOLATION = 204,
  SHACL_MIN_COUNT_VIOLATION = 205,
  SHACL_MAX_COUNT_VIOLATION = 206,
  SHACL_MIN_LENGTH_VIOLATION = 207,
  SHACL_MAX_LENGTH_VIOLATION = 208,
  SHACL_PATTERN_VIOLATION = 209,
  SHACL_MIN_INCLUSIVE_VIOLATION = 210,
  SHACL_MAX_INCLUSIVE_VIOLATION = 211,
  SHACL_MIN_EXCLUSIVE_VIOLATION = 212,
  SHACL_MAX_EXCLUSIVE_VIOLATION = 213,
  SHACL_UNIQUE_LANG_VIOLATION = 214,
  SHACL_LANGUAGE_IN_VIOLATION = 215,
  SHACL_CLOSED_SHAPE_VIOLATION = 216,
  SHACL_HAS_VALUE_VIOLATION = 217,
  SHACL_IN_VIOLATION = 218,
  SHACL_AND_VIOLATION = 219,
  SHACL_OR_VIOLATION = 220,
  SHACL_NOT_VIOLATION = 221,
  SHACL_XONE_VIOLATION = 222,
  SHACL_QUALIFIED_VALUE_SHAPE_VIOLATION = 223,
  SHACL_DISJOINT_VIOLATION = 224,
  SHACL_EQUALS_VIOLATION = 225,
  SHACL_LESS_THAN_VIOLATION = 226,
  SHACL_LESS_THAN_OR_EQUALS_VIOLATION = 227,

  // Validation errors - ShEx (300-399)
  SHEX_VALIDATION_ERROR = 300,
  SHEX_SHAPE_MISMATCH = 301,
  SHEX_CARDINALITY_VIOLATION = 302,
  SHEX_VALUE_CONSTRAINT_VIOLATION = 303,
  SHEX_NODE_CONSTRAINT_VIOLATION = 304,
  SHEX_REFERENCE_ERROR = 305,
  SHEX_DATATYPE_MISMATCH = 306,
  SHEX_FACET_VIOLATION = 307,
  SHEX_SEMANTIC_ACTION_FAILURE = 308,
  SHEX_EXTRA_PROPERTY = 309,
  SHEX_MISSING_PROPERTY = 310,

  // Rules processing errors (400-499)
  RULE_EXECUTION_ERROR = 400,
  RULE_DEPENDENCY_CYCLE = 401,
  RULE_INVALID_HEAD = 402,
  RULE_INVALID_BODY = 403,
  RULE_UNBOUND_VARIABLE = 404,
  RULE_UNSAFE_NEGATION = 405,
  RULE_CONFLICTING_DERIVATION = 406,
  RULE_FIXPOINT_NOT_REACHED = 407,
  RULE_STRATIFICATION_ERROR = 408,
  RULE_BUILTIN_ERROR = 409,
  RULE_AGGREGATION_ERROR = 410,
  RULE_RECURSION_DEPTH_EXCEEDED = 411,

  // N3 reasoning errors (500-599)
  N3_REASONING_ERROR = 500,
  N3_FORMULA_ERROR = 501,
  N3_BUILTIN_ERROR = 502,
  N3_UNIFICATION_ERROR = 503,
  N3_GRAPH_LITERAL_ERROR = 504,
  N3_QUANTIFIER_ERROR = 505,
  N3_IMPLICATION_ERROR = 506,
  N3_LIST_ERROR = 507,
  N3_PATH_ERROR = 508,
  N3_CONTRADICTION = 509,
  N3_INCONSISTENT_FORMULA = 510,

  // Guard condition violations (600-699)
  GUARD_VIOLATION = 600,
  GUARD_PRECONDITION_FAILED = 601,
  GUARD_POSTCONDITION_FAILED = 602,
  GUARD_INVARIANT_VIOLATED = 603,
  GUARD_SIZE_LIMIT_EXCEEDED = 604,
  GUARD_DEPTH_LIMIT_EXCEEDED = 605,
  GUARD_COMPLEXITY_LIMIT_EXCEEDED = 606,
  GUARD_MEMORY_LIMIT_EXCEEDED = 607,
  GUARD_RECURSION_LIMIT_EXCEEDED = 608,
  GUARD_CIRCULAR_DEPENDENCY = 609,

  // Index and data access errors (700-799)
  INDEX_ERROR = 700,
  INDEX_NOT_FOUND = 701,
  TRIPLE_NOT_FOUND = 702,
  VOCABULARY_ERROR = 703,
  ID_RESOLUTION_ERROR = 704,
  GRAPH_NOT_FOUND = 705,
  DUPLICATE_ENTITY = 706,
};

// Returns a stable, human-readable description of the error code.
// These descriptions are immutable and part of the contract API.
constexpr std::string_view errorCodeDescription(ErrorCode code) {
  switch (code) {
    // Generic errors
    case ErrorCode::UNKNOWN_ERROR:
      return "Unknown error";
    case ErrorCode::INTERNAL_ERROR:
      return "Internal error";
    case ErrorCode::NOT_IMPLEMENTED:
      return "Feature not implemented";
    case ErrorCode::INVALID_ARGUMENT:
      return "Invalid argument";
    case ErrorCode::RESOURCE_EXHAUSTED:
      return "Resource exhausted";
    case ErrorCode::TIMEOUT:
      return "Operation timeout";

    // Parsing errors
    case ErrorCode::PARSE_ERROR:
      return "Parse error";
    case ErrorCode::SYNTAX_ERROR:
      return "Syntax error";
    case ErrorCode::INVALID_IRI:
      return "Invalid IRI";
    case ErrorCode::INVALID_LITERAL:
      return "Invalid literal";
    case ErrorCode::INVALID_BLANK_NODE:
      return "Invalid blank node";
    case ErrorCode::INVALID_PREFIX:
      return "Invalid prefix";
    case ErrorCode::MALFORMED_SHAPE:
      return "Malformed shape definition";
    case ErrorCode::MALFORMED_RULE:
      return "Malformed rule definition";
    case ErrorCode::MALFORMED_FORMULA:
      return "Malformed formula";

    // SHACL validation errors
    case ErrorCode::SHACL_VALIDATION_ERROR:
      return "SHACL validation error";
    case ErrorCode::SHACL_NODE_CONSTRAINT_VIOLATION:
      return "SHACL node constraint violation";
    case ErrorCode::SHACL_PROPERTY_CONSTRAINT_VIOLATION:
      return "SHACL property constraint violation";
    case ErrorCode::SHACL_CLASS_CONSTRAINT_VIOLATION:
      return "SHACL class constraint violation";
    case ErrorCode::SHACL_DATATYPE_CONSTRAINT_VIOLATION:
      return "SHACL datatype constraint violation";
    case ErrorCode::SHACL_MIN_COUNT_VIOLATION:
      return "SHACL minimum count violation";
    case ErrorCode::SHACL_MAX_COUNT_VIOLATION:
      return "SHACL maximum count violation";
    case ErrorCode::SHACL_MIN_LENGTH_VIOLATION:
      return "SHACL minimum length violation";
    case ErrorCode::SHACL_MAX_LENGTH_VIOLATION:
      return "SHACL maximum length violation";
    case ErrorCode::SHACL_PATTERN_VIOLATION:
      return "SHACL pattern violation";
    case ErrorCode::SHACL_MIN_INCLUSIVE_VIOLATION:
      return "SHACL minimum inclusive violation";
    case ErrorCode::SHACL_MAX_INCLUSIVE_VIOLATION:
      return "SHACL maximum inclusive violation";
    case ErrorCode::SHACL_MIN_EXCLUSIVE_VIOLATION:
      return "SHACL minimum exclusive violation";
    case ErrorCode::SHACL_MAX_EXCLUSIVE_VIOLATION:
      return "SHACL maximum exclusive violation";
    case ErrorCode::SHACL_UNIQUE_LANG_VIOLATION:
      return "SHACL unique language violation";
    case ErrorCode::SHACL_LANGUAGE_IN_VIOLATION:
      return "SHACL language-in violation";
    case ErrorCode::SHACL_CLOSED_SHAPE_VIOLATION:
      return "SHACL closed shape violation";
    case ErrorCode::SHACL_HAS_VALUE_VIOLATION:
      return "SHACL has-value violation";
    case ErrorCode::SHACL_IN_VIOLATION:
      return "SHACL in-list violation";
    case ErrorCode::SHACL_AND_VIOLATION:
      return "SHACL AND constraint violation";
    case ErrorCode::SHACL_OR_VIOLATION:
      return "SHACL OR constraint violation";
    case ErrorCode::SHACL_NOT_VIOLATION:
      return "SHACL NOT constraint violation";
    case ErrorCode::SHACL_XONE_VIOLATION:
      return "SHACL XONE constraint violation";
    case ErrorCode::SHACL_QUALIFIED_VALUE_SHAPE_VIOLATION:
      return "SHACL qualified value shape violation";
    case ErrorCode::SHACL_DISJOINT_VIOLATION:
      return "SHACL disjoint violation";
    case ErrorCode::SHACL_EQUALS_VIOLATION:
      return "SHACL equals violation";
    case ErrorCode::SHACL_LESS_THAN_VIOLATION:
      return "SHACL less-than violation";
    case ErrorCode::SHACL_LESS_THAN_OR_EQUALS_VIOLATION:
      return "SHACL less-than-or-equals violation";

    // ShEx validation errors
    case ErrorCode::SHEX_VALIDATION_ERROR:
      return "ShEx validation error";
    case ErrorCode::SHEX_SHAPE_MISMATCH:
      return "ShEx shape mismatch";
    case ErrorCode::SHEX_CARDINALITY_VIOLATION:
      return "ShEx cardinality violation";
    case ErrorCode::SHEX_VALUE_CONSTRAINT_VIOLATION:
      return "ShEx value constraint violation";
    case ErrorCode::SHEX_NODE_CONSTRAINT_VIOLATION:
      return "ShEx node constraint violation";
    case ErrorCode::SHEX_REFERENCE_ERROR:
      return "ShEx reference error";
    case ErrorCode::SHEX_DATATYPE_MISMATCH:
      return "ShEx datatype mismatch";
    case ErrorCode::SHEX_FACET_VIOLATION:
      return "ShEx facet violation";
    case ErrorCode::SHEX_SEMANTIC_ACTION_FAILURE:
      return "ShEx semantic action failure";
    case ErrorCode::SHEX_EXTRA_PROPERTY:
      return "ShEx extra property";
    case ErrorCode::SHEX_MISSING_PROPERTY:
      return "ShEx missing property";

    // Rules processing errors
    case ErrorCode::RULE_EXECUTION_ERROR:
      return "Rule execution error";
    case ErrorCode::RULE_DEPENDENCY_CYCLE:
      return "Rule dependency cycle detected";
    case ErrorCode::RULE_INVALID_HEAD:
      return "Invalid rule head";
    case ErrorCode::RULE_INVALID_BODY:
      return "Invalid rule body";
    case ErrorCode::RULE_UNBOUND_VARIABLE:
      return "Unbound variable in rule";
    case ErrorCode::RULE_UNSAFE_NEGATION:
      return "Unsafe negation in rule";
    case ErrorCode::RULE_CONFLICTING_DERIVATION:
      return "Conflicting rule derivation";
    case ErrorCode::RULE_FIXPOINT_NOT_REACHED:
      return "Rule fixpoint not reached";
    case ErrorCode::RULE_STRATIFICATION_ERROR:
      return "Rule stratification error";
    case ErrorCode::RULE_BUILTIN_ERROR:
      return "Rule builtin error";
    case ErrorCode::RULE_AGGREGATION_ERROR:
      return "Rule aggregation error";
    case ErrorCode::RULE_RECURSION_DEPTH_EXCEEDED:
      return "Rule recursion depth exceeded";

    // N3 reasoning errors
    case ErrorCode::N3_REASONING_ERROR:
      return "N3 reasoning error";
    case ErrorCode::N3_FORMULA_ERROR:
      return "N3 formula error";
    case ErrorCode::N3_BUILTIN_ERROR:
      return "N3 builtin error";
    case ErrorCode::N3_UNIFICATION_ERROR:
      return "N3 unification error";
    case ErrorCode::N3_GRAPH_LITERAL_ERROR:
      return "N3 graph literal error";
    case ErrorCode::N3_QUANTIFIER_ERROR:
      return "N3 quantifier error";
    case ErrorCode::N3_IMPLICATION_ERROR:
      return "N3 implication error";
    case ErrorCode::N3_LIST_ERROR:
      return "N3 list error";
    case ErrorCode::N3_PATH_ERROR:
      return "N3 path error";
    case ErrorCode::N3_CONTRADICTION:
      return "N3 contradiction detected";
    case ErrorCode::N3_INCONSISTENT_FORMULA:
      return "N3 inconsistent formula";

    // Guard condition violations
    case ErrorCode::GUARD_VIOLATION:
      return "Guard condition violation";
    case ErrorCode::GUARD_PRECONDITION_FAILED:
      return "Guard precondition failed";
    case ErrorCode::GUARD_POSTCONDITION_FAILED:
      return "Guard postcondition failed";
    case ErrorCode::GUARD_INVARIANT_VIOLATED:
      return "Guard invariant violated";
    case ErrorCode::GUARD_SIZE_LIMIT_EXCEEDED:
      return "Guard size limit exceeded";
    case ErrorCode::GUARD_DEPTH_LIMIT_EXCEEDED:
      return "Guard depth limit exceeded";
    case ErrorCode::GUARD_COMPLEXITY_LIMIT_EXCEEDED:
      return "Guard complexity limit exceeded";
    case ErrorCode::GUARD_MEMORY_LIMIT_EXCEEDED:
      return "Guard memory limit exceeded";
    case ErrorCode::GUARD_RECURSION_LIMIT_EXCEEDED:
      return "Guard recursion limit exceeded";
    case ErrorCode::GUARD_CIRCULAR_DEPENDENCY:
      return "Guard circular dependency detected";

    // Index and data access errors
    case ErrorCode::INDEX_ERROR:
      return "Index error";
    case ErrorCode::INDEX_NOT_FOUND:
      return "Index not found";
    case ErrorCode::TRIPLE_NOT_FOUND:
      return "Triple not found";
    case ErrorCode::VOCABULARY_ERROR:
      return "Vocabulary error";
    case ErrorCode::ID_RESOLUTION_ERROR:
      return "ID resolution error";
    case ErrorCode::GRAPH_NOT_FOUND:
      return "Graph not found";
    case ErrorCode::DUPLICATE_ENTITY:
      return "Duplicate entity";

    default:
      return "Unknown error code";
  }
}

}  // namespace qlever::contracts

#endif  // QLEVER_SRC_ENGINE_CONTRACTS_ERRORCODE_H
