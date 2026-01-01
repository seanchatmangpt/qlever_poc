#ifndef PARSER_SHEX_H
#define PARSER_SHEX_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <limits>
#include <cmath>

#include "absl/container/flat_hash_set.h"
#include "absl/container/flat_hash_map.h"
#include "re2/re2.h"
#include "../rdfTypes/Iri.h"
#include "../rdfTypes/Literal.h"
#include "ShExErrorReporting.h"
#include "ShExNegation.h"
#include "ShExTripleExpression.h"

namespace shex {

// ============================================================================
// Core ShEx Data Types (Phase 2A: Advanced Value Constraints)
// ============================================================================

enum class ValueType { IRI, LITERAL, BNODE };

enum class Cardinality { EXACTLY_ONE, ZERO_OR_ONE, ZERO_OR_MORE, ONE_OR_MORE };

// ============================================================================
// Phase 2A: Advanced Value Constraint Types
// ============================================================================

// XSD Datatype enumeration for facet constraints
enum class XsdDatatype {
  INTEGER,
  DECIMAL,
  DOUBLE,
  FLOAT,
  STRING,
  BOOLEAN,
  DATE,
  DATETIME,
  TIME,
  UNKNOWN
};

// Numeric range constraints for XSD numeric types
struct NumericRangeConstraint {
  std::optional<double> minInclusive;
  std::optional<double> maxInclusive;
  std::optional<double> minExclusive;
  std::optional<double> maxExclusive;
  std::optional<int> totalDigits;      // Total number of digits
  std::optional<int> fractionDigits;   // Number of fractional digits

  NumericRangeConstraint() = default;
  bool validate(const std::string& value, XsdDatatype datatype) const;
};

// Pattern constraint using RE2 regex with caching
struct PatternConstraint {
  std::string pattern;
  mutable std::unique_ptr<RE2> compiledRegex;  // Lazy compilation with caching
  mutable bool compiled = false;

  PatternConstraint() = default;
  explicit PatternConstraint(const std::string& pat) : pattern(pat) {}

  // Move constructor for unique_ptr
  PatternConstraint(PatternConstraint&& other) noexcept
      : pattern(std::move(other.pattern)),
        compiledRegex(std::move(other.compiledRegex)),
        compiled(other.compiled) {}

  // Copy constructor (compiles new regex)
  PatternConstraint(const PatternConstraint& other)
      : pattern(other.pattern), compiled(false) {}

  // Move assignment
  PatternConstraint& operator=(PatternConstraint&& other) noexcept {
    if (this != &other) {
      pattern = std::move(other.pattern);
      compiledRegex = std::move(other.compiledRegex);
      compiled = other.compiled;
    }
    return *this;
  }

  // Copy assignment
  PatternConstraint& operator=(const PatternConstraint& other) {
    if (this != &other) {
      pattern = other.pattern;
      compiledRegex.reset();
      compiled = false;
    }
    return *this;
  }

  bool validate(const std::string& value) const;
  void ensureCompiled() const;
};

// Language tag constraint (BCP47)
struct LanguageTagConstraint {
  std::optional<std::string> languageTag;       // Exact language tag match (e.g., "en")
  std::optional<std::string> languagePattern;   // Pattern for language tag (e.g., "en-*")

  LanguageTagConstraint() = default;
  bool validate(const std::string& languageTag) const;

private:
  static bool isValidBCP47(const std::string& tag);
};

// Length constraint (UTF-8 aware)
struct LengthConstraint {
  std::optional<size_t> minLength;
  std::optional<size_t> maxLength;
  std::optional<size_t> exactLength;

  LengthConstraint() = default;
  bool validate(const std::string& value) const;

private:
  static size_t countUtf8Chars(const std::string& str);
};

// XSD Datatype facet constraint
struct DatatypeFacetConstraint {
  XsdDatatype datatype = XsdDatatype::UNKNOWN;

  DatatypeFacetConstraint() = default;
  explicit DatatypeFacetConstraint(XsdDatatype dt) : datatype(dt) {}

  bool validate(const std::string& value) const;

private:
  static bool validateInteger(const std::string& value);
  static bool validateDecimal(const std::string& value);
  static bool validateDouble(const std::string& value);
  static bool validateBoolean(const std::string& value);
  static bool validateDate(const std::string& value);
  static bool validateDateTime(const std::string& value);
};

struct ValueSetConstraint {
  std::optional<ValueType> valueType;
  absl::flat_hash_set<std::string> allowedIris;
  std::optional<std::string> datatypeRestriction;

  // Phase 2A: Advanced value constraints
  std::optional<NumericRangeConstraint> numericRange;
  std::optional<PatternConstraint> pattern;
  std::optional<LanguageTagConstraint> languageTag;
  std::optional<LengthConstraint> length;
  std::optional<DatatypeFacetConstraint> datatypeFacet;

  // Phase 2C: Negation support
  NegationOperator negation = NegationOperator::NONE;
  std::shared_ptr<ConstraintNode> constraintTree;  // For complex logical compositions

  ValueSetConstraint() = default;
  bool validate(const std::string& value, ValueType type) const;

  // Phase 2A: Validate with language tag support
  bool validate(const std::string& value, ValueType type,
                const std::string& langTag) const;
};

struct PropertyShape {
  std::string predicate;
  ValueSetConstraint valueConstraint;
  Cardinality cardinality = Cardinality::EXACTLY_ONE;
  bool inverse = false;
  std::optional<std::string> nodeKind;

  // Phase 2C: Negation support for property shapes
  NegationOperator negation = NegationOperator::NONE;
  std::optional<std::string> shapeReference;  // For shape reference negation (!@<ShapeName>)

  PropertyShape(const std::string& pred) : predicate(pred) {}
  bool validate(const std::string& value, ValueType type) const;
};

struct Shape {
  std::string id;
  std::vector<PropertyShape> properties;
  bool closed = false;

  // Phase 2E: Triple Expression support
  std::unique_ptr<TripleExpression> tripleExpression;

  // Phase 2B: EXTRA, !EXTRA, and EXTENDS support
  absl::flat_hash_set<std::string> extraPredicates_;
  absl::flat_hash_set<std::string> forbiddenExtraPredicates_;
  std::optional<std::string> extendsShapeId_;

  Shape(const std::string& shapeId) : id(shapeId) {}
  void addProperty(const PropertyShape& prop) { properties.push_back(prop); }

  // Phase 2E: Triple Expression methods
  void setTripleExpression(std::unique_ptr<TripleExpression> expr) {
    tripleExpression = std::move(expr);
  }

  bool hasTripleExpression() const {
    return tripleExpression != nullptr;
  }

  // Phase 2B: Methods for EXTRA, !EXTRA, EXTENDS
  void addExtraPredicate(const std::string& predicate) {
    extraPredicates_.insert(predicate);
  }

  void addForbiddenExtraPredicate(const std::string& predicate) {
    forbiddenExtraPredicates_.insert(predicate);
  }

  void setExtends(const std::string& shapeId) {
    extendsShapeId_ = shapeId;
  }

  const std::optional<std::string>& getExtends() const {
    return extendsShapeId_;
  }

  const absl::flat_hash_set<std::string>& getExtraPredicates() const {
    return extraPredicates_;
  }

  const absl::flat_hash_set<std::string>& getForbiddenExtraPredicates() const {
    return forbiddenExtraPredicates_;
  }

  struct ValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    absl::flat_hash_set<std::string> failedPredicates;  // Track which predicates failed
    absl::flat_hash_set<std::string> unexpectedPredicates;  // Track unexpected predicates in closed shapes

    // Phase 2E: Track matched/unmatched triples for triple expressions
    absl::flat_hash_map<std::string, std::vector<std::pair<std::string, NodeKind>>>
        matchedTriples;
    absl::flat_hash_map<std::string, std::vector<std::pair<std::string, NodeKind>>>
        unmatchedTriples;
  };

  ValidationResult validate(const std::map<std::string,
                            std::vector<std::pair<std::string, ValueType>>>& nodeData) const;
};

// ============================================================================
// ShEx Schema (Collection of Shapes)
// ============================================================================

class ShExSchema {
 public:
  void addShape(const Shape& shape);
  const Shape* getShape(const std::string& shapeId) const;
  bool hasShape(const std::string& shapeId) const;
  const absl::flat_hash_map<std::string, Shape>& getShapes() const {
    return shapes_;
  }

  // Phase 2B: Inheritance resolution
  // Resolves inheritance for a shape, returning a fully resolved shape
  // Returns nullopt if circular inheritance is detected
  struct InheritanceResolutionResult {
    std::optional<Shape> resolvedShape;
    std::vector<std::string> errors;
    absl::flat_hash_set<std::string> visitedShapes;  // For circular detection
  };

  InheritanceResolutionResult resolveInheritance(const std::string& shapeId) const;

  // Resolve all shapes in the schema (materializes inheritance)
  bool resolveAllInheritance();

 private:
  absl::flat_hash_map<std::string, Shape> shapes_;

  // Helper for recursive inheritance resolution with cycle detection
  InheritanceResolutionResult resolveInheritanceHelper(
      const std::string& shapeId,
      absl::flat_hash_set<std::string>& visitedInPath) const;

  // Merge parent shape into child shape
  static void mergeShapes(Shape& child, const Shape& parent);
};

// ============================================================================
// ShEx Parser (80/20: Parse basic ShEx syntax)
// ============================================================================

class ShExParser {
 public:
  ShExParser() = default;

  // Parse ShEx from string format
  // Simple format: shape shape_id { prop pred cardinality constraint; ... }
  std::optional<ShExSchema> parse(const std::string& input);

  std::string getLastError() const { return lastError_; }

 private:
  std::string lastError_;

  std::optional<Shape> parseShape(const std::string& input, size_t& pos);
  std::optional<PropertyShape> parseProperty(const std::string& input, size_t& pos);
  std::optional<Cardinality> parseCardinality(const std::string& input);
  std::optional<ValueSetConstraint> parseValueConstraint(const std::string& input);

  void skipWhitespace(const std::string& input, size_t& pos);
  std::string readWord(const std::string& input, size_t& pos);
  void setError(const std::string& msg) { lastError_ = msg; }
};

// ============================================================================
// Validator for RDF Conformance to ShEx
// ============================================================================

class ShExValidator {
 public:
  explicit ShExValidator(const ShExSchema& schema) : schema_(schema) {}

  // Legacy simple validation report (deprecated, for backward compatibility)
  struct ValidationReport {
    bool conforms;
    std::map<std::string, std::vector<std::string>> nodeErrors;
    std::vector<std::string> schemaErrors;
  };

  // Validate a node with enhanced error reporting
  EnhancedValidationReport validateNodeEnhanced(
      const std::string& nodeIri,
      const std::string& targetShapeId,
      const std::map<std::string,
        std::vector<std::pair<std::string, ValueType>>>& data);

  // Validate all nodes in a dataset with enhanced reporting
  EnhancedValidationReport validateDatasetEnhanced(
      const std::map<std::string,
        std::map<std::string, std::vector<std::pair<std::string, ValueType>>>>& dataset,
      const std::map<std::string, std::string>& nodeToShapeMapping);

  // Legacy validation methods (backward compatibility)
  ValidationReport validateNode(const std::string& nodeIri,
                               const std::string& targetShapeId,
                               const std::map<std::string,
                                 std::vector<std::pair<std::string, ValueType>>>& data);

  ValidationReport validateDataset(
      const std::map<std::string,
        std::map<std::string, std::vector<std::pair<std::string, ValueType>>>>& dataset,
      const std::map<std::string, std::string>& nodeToShapeMapping);

 private:
  const ShExSchema& schema_;

  // Helper method to create detailed errors
  DetailedValidationError createCardinalityError(
      const std::string& nodeId,
      const std::string& shapeId,
      const PropertyShape& prop,
      int actualCount);

  DetailedValidationError createTypeMismatchError(
      const std::string& nodeId,
      const std::string& shapeId,
      const PropertyShape& prop,
      const std::string& value,
      ValueType actualType);

  DetailedValidationError createShapeNotFoundError(
      const std::string& shapeId);

  DetailedValidationError createValueNotAllowedError(
      const std::string& nodeId,
      const std::string& shapeId,
      const PropertyShape& prop,
      const std::string& value);

  DetailedValidationError createExtraPropertyError(
      const std::string& nodeId,
      const std::string& shapeId,
      const std::string& predicate);
};

}  // namespace shex

#endif  // PARSER_SHEX_H
