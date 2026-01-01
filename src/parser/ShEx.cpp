#include "ShEx.h"

#include <sstream>
#include <algorithm>
#include <cctype>

namespace shex {

// ============================================================================
// Helper Functions for Type Conversion
// ============================================================================

/**
 * Convert ValueType to NodeKind for triple expression validation
 */
static NodeKind valueTypeToNodeKind(ValueType vt) {
  switch (vt) {
    case ValueType::IRI: return NodeKind::IRI;
    case ValueType::LITERAL: return NodeKind::LITERAL;
    case ValueType::BNODE: return NodeKind::BNODE;
  }
  return NodeKind::LITERAL;  // Fallback
}

/**
 * Convert NodeKind to ValueType for backward compatibility
 */
static ValueType nodeKindToValueType(NodeKind nk) {
  switch (nk) {
    case NodeKind::IRI: return ValueType::IRI;
    case NodeKind::LITERAL: return ValueType::LITERAL;
    case NodeKind::BNODE: return ValueType::BNODE;
    case NodeKind::NONLITERAL: return ValueType::IRI;
  }
  return ValueType::LITERAL;  // Fallback
}

// ============================================================================
// ValueSetConstraint Implementation
// ============================================================================

bool ValueSetConstraint::validate(const std::string& value, ValueType type) const {
  bool result;

  // Phase 2C: Use constraint tree if available (for complex logical compositions)
  if (constraintTree) {
    std::string valueTypeStr;
    switch (type) {
      case ValueType::IRI: valueTypeStr = "IRI"; break;
      case ValueType::LITERAL: valueTypeStr = "LITERAL"; break;
      case ValueType::BNODE: valueTypeStr = "BNODE"; break;
    }
    result = ConstraintEvaluator::evaluate(constraintTree, value, valueTypeStr);
  } else {
    // Simple constraint validation (original logic)
    // If valueType is specified, check it first
    if (valueType.has_value() && valueType.value() != type) {
      result = false;
    } else if (!allowedIris.empty()) {
      // If specific IRIs are allowed, check against them
      result = allowedIris.contains(value);
    } else if (datatypeRestriction.has_value()) {
      // If datatype restriction is specified, validate
      // Simple check: if it's a literal, we could validate the datatype
      // For 80/20: just ensure it's not an IRI
      result = type == ValueType::LITERAL;
    } else {
      // Default: allow everything
      result = true;
    }

    // Phase 2A: Apply advanced value constraints
    if (result && type == ValueType::LITERAL) {
      // Check pattern constraint
      if (pattern.has_value() && !pattern.value().validate(value)) {
        result = false;
      }

      // Check length constraint
      if (result && length.has_value() && !length.value().validate(value)) {
        result = false;
      }

      // Check datatype facet constraint
      if (result && datatypeFacet.has_value() && !datatypeFacet.value().validate(value)) {
        result = false;
      }

      // Check numeric range constraint (if datatype facet is numeric)
      if (result && numericRange.has_value()) {
        XsdDatatype dtype = datatypeFacet.has_value() ? datatypeFacet.value().datatype : XsdDatatype::DECIMAL;
        if (!numericRange.value().validate(value, dtype)) {
          result = false;
        }
      }
    }
  }

  // Phase 2C: Apply negation operator
  if (negation == NegationOperator::NOT) {
    result = !result;
  }

  return result;
}

// Phase 2A: Validate with language tag support
bool ValueSetConstraint::validate(const std::string& value, ValueType type,
                                  const std::string& langTag) const {
  // First check basic validation
  bool result = validate(value, type);

  // If basic validation passed and we have a language tag constraint, check it
  if (result && type == ValueType::LITERAL && languageTag.has_value()) {
    result = languageTag.value().validate(langTag);
  }

  return result;
}

// ============================================================================
// PropertyShape Implementation
// ============================================================================

bool PropertyShape::validate(const std::string& value, ValueType type) const {
  bool result = valueConstraint.validate(value, type);

  // Phase 2C: Apply negation operator at property level
  if (negation == NegationOperator::NOT) {
    result = !result;
  }

  return result;
}

// ============================================================================
// Shape Implementation
// ============================================================================

Shape::ValidationResult Shape::validate(
    const std::map<std::string,
      std::vector<std::pair<std::string, ValueType>>>& nodeData) const {
  ValidationResult result{true, {}, {}, {}, {}, {}};

  // Phase 2E: Use triple expression if available
  if (tripleExpression) {
    // Convert nodeData to TripleContext
    TripleContext context;
    context.subject = id;  // Use shape ID as subject placeholder

    // Convert ValueType to NodeKind
    for (const auto& [pred, values] : nodeData) {
      for (const auto& [value, vtype] : values) {
        context.triples[pred].push_back({value, valueTypeToNodeKind(vtype)});
      }
    }

    // Validate using triple expression
    auto exprResult = tripleExpression->validate(context);

    result.isValid = exprResult.isValid;
    result.errors = exprResult.errors;

    // Convert matched/unmatched triples back to ValidationResult format
    result.matchedTriples = context.matchedTriples;

    auto remainingTriples = context.getAllRemainingTriples();
    result.unmatchedTriples = remainingTriples;

    // For closed shapes, check if there are unmatched triples
    if (closed && !remainingTriples.empty()) {
      for (const auto& [pred, values] : remainingTriples) {
        // Check if predicate is in EXTRA
        if (!extraPredicates_.contains(pred)) {
          result.isValid = false;
          result.unexpectedPredicates.insert(pred);
          result.errors.push_back("Unexpected property " + pred +
                                " in closed shape with triple expression");
        }
      }
    }

    return result;
  }

  // Legacy validation using PropertyShape vector
  // Build set of defined predicates for CLOSED shape checking
  absl::flat_hash_set<std::string> definedPredicates;
  for (const auto& prop : properties) {
    definedPredicates.insert(prop.predicate);
  }

  // Check each property in the shape
  for (const auto& prop : properties) {
    auto it = nodeData.find(prop.predicate);

    // Check cardinality
    size_t count = (it != nodeData.end()) ? it->second.size() : 0;

    switch (prop.cardinality) {
      case Cardinality::EXACTLY_ONE:
        if (count != 1) {
          result.isValid = false;
          result.failedPredicates.insert(prop.predicate);
          result.errors.push_back("Property " + prop.predicate +
                                " must appear exactly once (found " +
                                std::to_string(count) + ")");
        }
        break;
      case Cardinality::ZERO_OR_ONE:
        if (count > 1) {
          result.isValid = false;
          result.failedPredicates.insert(prop.predicate);
          result.errors.push_back("Property " + prop.predicate +
                                " must appear at most once (found " +
                                std::to_string(count) + ")");
        }
        break;
      case Cardinality::ZERO_OR_MORE:
        // Always valid
        break;
      case Cardinality::ONE_OR_MORE:
        if (count < 1) {
          result.isValid = false;
          result.failedPredicates.insert(prop.predicate);
          result.errors.push_back("Property " + prop.predicate +
                                " must appear at least once");
        }
        break;
    }

    // Validate each value against constraints
    if (it != nodeData.end()) {
      for (const auto& [value, type] : it->second) {
        if (!prop.validate(value, type)) {
          result.isValid = false;
          result.failedPredicates.insert(prop.predicate);
          result.errors.push_back("Property " + prop.predicate +
                                " value '" + value + "' does not match constraints");
        }
      }
    }
  }

  // Phase 2B: CLOSED shape validation with EXTRA and !EXTRA
  if (closed) {
    for (const auto& [predicate, values] : nodeData) {
      // Check if predicate is defined in the shape
      if (!definedPredicates.contains(predicate)) {
        // Check if predicate is in EXTRA (allowed)
        if (extraPredicates_.contains(predicate)) {
          // Check if it's also in !EXTRA (forbidden) - !EXTRA takes precedence
          if (forbiddenExtraPredicates_.contains(predicate)) {
            result.isValid = false;
            result.unexpectedPredicates.insert(predicate);
            result.errors.push_back("Property " + predicate +
                                  " is explicitly forbidden (!EXTRA) in closed shape");
          }
          // Otherwise it's allowed by EXTRA
        } else {
          // Check if it's in !EXTRA
          if (forbiddenExtraPredicates_.contains(predicate)) {
            result.isValid = false;
            result.unexpectedPredicates.insert(predicate);
            result.errors.push_back("Property " + predicate +
                                  " is explicitly forbidden (!EXTRA)");
          } else {
            // Not in EXTRA, not in !EXTRA, but shape is closed
            result.isValid = false;
            result.unexpectedPredicates.insert(predicate);
            result.errors.push_back("Property " + predicate +
                                  " is not allowed in closed shape");
          }
        }
      }
    }
  } else {
    // Shape is not closed, but !EXTRA predicates are still forbidden
    for (const auto& [predicate, values] : nodeData) {
      if (forbiddenExtraPredicates_.contains(predicate)) {
        result.isValid = false;
        result.unexpectedPredicates.insert(predicate);
        result.errors.push_back("Property " + predicate +
                              " is explicitly forbidden (!EXTRA)");
      }
    }
  }

  return result;
}

// ============================================================================
// ShExSchema Implementation
// ============================================================================

void ShExSchema::addShape(const Shape& shape) { shapes_[shape.id] = shape; }

const Shape* ShExSchema::getShape(const std::string& shapeId) const {
  auto it = shapes_.find(shapeId);
  if (it != shapes_.end()) {
    return &it->second;
  }
  return nullptr;
}

bool ShExSchema::hasShape(const std::string& shapeId) const {
  return shapes_.count(shapeId) > 0;
}

// Phase 2B: Inheritance resolution implementation
void ShExSchema::mergeShapes(Shape& child, const Shape& parent) {
  // Merge properties - parent properties are added first, then child overrides
  absl::flat_hash_set<std::string> childPredicates;
  for (const auto& prop : child.properties) {
    childPredicates.insert(prop.predicate);
  }

  // Add parent properties that are not overridden by child
  std::vector<PropertyShape> mergedProperties;
  for (const auto& parentProp : parent.properties) {
    if (!childPredicates.contains(parentProp.predicate)) {
      mergedProperties.push_back(parentProp);
    }
  }

  // Add child properties (these override parent)
  for (const auto& childProp : child.properties) {
    mergedProperties.push_back(childProp);
  }

  child.properties = std::move(mergedProperties);

  // Merge CLOSED: child overrides parent
  if (!child.closed && parent.closed) {
    child.closed = parent.closed;
  }

  // Merge EXTRA predicates: union of parent and child
  for (const auto& pred : parent.extraPredicates_) {
    child.extraPredicates_.insert(pred);
  }

  // Merge !EXTRA predicates: union of parent and child
  for (const auto& pred : parent.forbiddenExtraPredicates_) {
    child.forbiddenExtraPredicates_.insert(pred);
  }
}

ShExSchema::InheritanceResolutionResult ShExSchema::resolveInheritanceHelper(
    const std::string& shapeId,
    absl::flat_hash_set<std::string>& visitedInPath) const {
  InheritanceResolutionResult result;

  // Check if shape exists
  auto it = shapes_.find(shapeId);
  if (it == shapes_.end()) {
    result.errors.push_back("Shape '" + shapeId + "' not found");
    return result;
  }

  const Shape& shape = it->second;

  // Check for circular inheritance
  if (visitedInPath.contains(shapeId)) {
    result.errors.push_back("Circular inheritance detected involving shape '" +
                          shapeId + "'");
    return result;
  }

  // Add to visited path for cycle detection
  visitedInPath.insert(shapeId);
  result.visitedShapes.insert(shapeId);

  // If no inheritance, return the shape as-is
  if (!shape.extendsShapeId_.has_value()) {
    result.resolvedShape = shape;
    visitedInPath.erase(shapeId);
    return result;
  }

  // Recursively resolve parent
  auto parentResult = resolveInheritanceHelper(shape.extendsShapeId_.value(),
                                              visitedInPath);

  // Remove from visited path after recursion
  visitedInPath.erase(shapeId);

  if (!parentResult.resolvedShape.has_value()) {
    // Propagate errors from parent resolution
    result.errors = parentResult.errors;
    result.visitedShapes.insert(parentResult.visitedShapes.begin(),
                               parentResult.visitedShapes.end());
    return result;
  }

  // Merge parent into child
  Shape resolvedShape = shape;
  mergeShapes(resolvedShape, parentResult.resolvedShape.value());

  // Clear the extends relationship in resolved shape
  resolvedShape.extendsShapeId_.reset();

  result.resolvedShape = std::move(resolvedShape);
  result.visitedShapes.insert(parentResult.visitedShapes.begin(),
                             parentResult.visitedShapes.end());

  return result;
}

ShExSchema::InheritanceResolutionResult ShExSchema::resolveInheritance(
    const std::string& shapeId) const {
  absl::flat_hash_set<std::string> visitedInPath;
  return resolveInheritanceHelper(shapeId, visitedInPath);
}

bool ShExSchema::resolveAllInheritance() {
  bool allResolved = true;
  absl::flat_hash_map<std::string, Shape> resolvedShapes;

  for (const auto& [shapeId, shape] : shapes_) {
    auto result = resolveInheritance(shapeId);
    if (result.resolvedShape.has_value()) {
      resolvedShapes[shapeId] = result.resolvedShape.value();
    } else {
      allResolved = false;
      // Keep original shape if resolution failed
      resolvedShapes[shapeId] = shape;
    }
  }

  // Replace shapes with resolved versions
  shapes_ = std::move(resolvedShapes);
  return allResolved;
}

// ============================================================================
// ShExParser Implementation
// ============================================================================

void ShExParser::skipWhitespace(const std::string& input, size_t& pos) {
  while (pos < input.length() && std::isspace(input[pos])) {
    ++pos;
  }
}

std::string ShExParser::readWord(const std::string& input, size_t& pos) {
  skipWhitespace(input, pos);
  std::string word;
  while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_' ||
                                  input[pos] == ':' || input[pos] == '/' ||
                                  input[pos] == '#' || input[pos] == '-' ||
                                  input[pos] == '.')) {
    word += input[pos];
    ++pos;
  }
  return word;
}

std::optional<Cardinality> ShExParser::parseCardinality(const std::string& input) {
  if (input == "?" || input == "*") {
    return Cardinality::ZERO_OR_ONE;
  }
  if (input == "*") {
    return Cardinality::ZERO_OR_MORE;
  }
  if (input == "+") {
    return Cardinality::ONE_OR_MORE;
  }
  if (input.empty()) {
    return Cardinality::EXACTLY_ONE;
  }
  return std::nullopt;
}

std::optional<ValueSetConstraint> ShExParser::parseValueConstraint(
    const std::string& input) {
  // Simple constraint parsing
  // Format: IRI, LITERAL, BNODE, or specific IRI values
  ValueSetConstraint constraint;

  if (input == "IRI") {
    constraint.valueType = ValueType::IRI;
  } else if (input == "LITERAL") {
    constraint.valueType = ValueType::LITERAL;
  } else if (input == "BNODE") {
    constraint.valueType = ValueType::BNODE;
  } else if (input.substr(0, 8) == "DATATYPE") {
    // DATATYPE<type>
    constraint.datatypeRestriction = input;
  } else if (input[0] == '<' && input[input.length() - 1] == '>') {
    // Specific IRI
    constraint.allowedIris.insert(input.substr(1, input.length() - 2));
  } else {
    // Default to accepting anything
    return constraint;
  }

  return constraint;
}

std::optional<PropertyShape> ShExParser::parseProperty(const std::string& input,
                                                       size_t& pos) {
  skipWhitespace(input, pos);

  // Read predicate
  std::string predicate = readWord(input, pos);
  if (predicate.empty()) {
    setError("Expected predicate");
    return std::nullopt;
  }

  PropertyShape prop(predicate);

  skipWhitespace(input, pos);

  // Read value constraint (optional)
  if (pos < input.length() && input[pos] != ';' && input[pos] != '}') {
    std::string constraint = readWord(input, pos);
    if (auto vc = parseValueConstraint(constraint)) {
      prop.valueConstraint = vc.value();
    }
  }

  skipWhitespace(input, pos);

  // Read cardinality (optional)
  if (pos < input.length() && (input[pos] == '?' || input[pos] == '*' ||
                               input[pos] == '+')) {
    std::string card(1, input[pos]);
    ++pos;
    if (auto c = parseCardinality(card)) {
      prop.cardinality = c.value();
    }
  }

  return prop;
}

std::optional<Shape> ShExParser::parseShape(const std::string& input, size_t& pos) {
  skipWhitespace(input, pos);

  // Read shape keyword
  std::string keyword = readWord(input, pos);
  if (keyword != "shape") {
    setError("Expected 'shape' keyword");
    return std::nullopt;
  }

  // Read shape ID
  std::string shapeId = readWord(input, pos);
  if (shapeId.empty()) {
    setError("Expected shape ID");
    return std::nullopt;
  }

  Shape shape(shapeId);

  skipWhitespace(input, pos);

  // Phase 2B: Check for EXTENDS keyword
  std::string maybeExtends = readWord(input, pos);
  if (maybeExtends == "EXTENDS") {
    skipWhitespace(input, pos);
    std::string parentShapeId = readWord(input, pos);
    if (parentShapeId.empty()) {
      setError("Expected parent shape ID after EXTENDS");
      return std::nullopt;
    }
    shape.setExtends(parentShapeId);
    skipWhitespace(input, pos);
  } else if (!maybeExtends.empty()) {
    // Not EXTENDS, rewind - this is a simple implementation
    // In production, would need better lookahead handling
    pos -= maybeExtends.length();
    skipWhitespace(input, pos);
  }

  // Expect opening brace
  if (pos >= input.length() || input[pos] != '{') {
    setError("Expected '{'");
    return std::nullopt;
  }
  ++pos;

  // Parse shape body (properties, CLOSED, EXTRA, !EXTRA)
  while (pos < input.length()) {
    skipWhitespace(input, pos);

    if (pos >= input.length()) break;

    if (input[pos] == '}') {
      ++pos;
      break;
    }

    // Phase 2B: Check for CLOSED keyword
    size_t savedPos = pos;
    std::string keyword = readWord(input, pos);

    if (keyword == "CLOSED") {
      shape.closed = true;
      skipWhitespace(input, pos);
      if (pos < input.length() && input[pos] == ';') {
        ++pos;
      }
      continue;
    }

    // Phase 2B: Check for EXTRA keyword
    if (keyword == "EXTRA") {
      skipWhitespace(input, pos);
      std::string predicate = readWord(input, pos);
      if (predicate.empty()) {
        setError("Expected predicate after EXTRA");
        return std::nullopt;
      }
      shape.addExtraPredicate(predicate);
      skipWhitespace(input, pos);
      if (pos < input.length() && input[pos] == ';') {
        ++pos;
      }
      continue;
    }

    // Phase 2B: Check for !EXTRA keyword (forbidden extra)
    if (pos > 0 && input[savedPos] == '!' && keyword == "EXTRA") {
      skipWhitespace(input, pos);
      std::string predicate = readWord(input, pos);
      if (predicate.empty()) {
        setError("Expected predicate after !EXTRA");
        return std::nullopt;
      }
      shape.addForbiddenExtraPredicate(predicate);
      skipWhitespace(input, pos);
      if (pos < input.length() && input[pos] == ';') {
        ++pos;
      }
      continue;
    }

    // Restore position and try to parse as property
    pos = savedPos;

    // Check for !EXTRA with better handling
    if (input[pos] == '!') {
      ++pos;
      std::string maybeExtra = readWord(input, pos);
      if (maybeExtra == "EXTRA") {
        skipWhitespace(input, pos);
        std::string predicate = readWord(input, pos);
        if (predicate.empty()) {
          setError("Expected predicate after !EXTRA");
          return std::nullopt;
        }
        shape.addForbiddenExtraPredicate(predicate);
        skipWhitespace(input, pos);
        if (pos < input.length() && input[pos] == ';') {
          ++pos;
        }
        continue;
      } else {
        // Not !EXTRA, rewind
        pos = savedPos;
      }
    }

    // Parse as property
    if (auto prop = parseProperty(input, pos)) {
      shape.addProperty(prop.value());
    } else {
      return std::nullopt;
    }

    skipWhitespace(input, pos);

    // Expect semicolon
    if (pos < input.length() && input[pos] == ';') {
      ++pos;
    }
  }

  return shape;
}

std::optional<ShExSchema> ShExParser::parse(const std::string& input) {
  ShExSchema schema;
  size_t pos = 0;

  while (pos < input.length()) {
    skipWhitespace(input, pos);

    if (pos >= input.length()) break;

    if (auto shape = parseShape(input, pos)) {
      schema.addShape(shape.value());
    } else {
      return std::nullopt;
    }
  }

  return schema;
}

// ============================================================================
// ShExValidator Implementation
// ============================================================================

ShExValidator::ValidationReport ShExValidator::validateNode(
    const std::string& nodeIri, const std::string& targetShapeId,
    const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>&
        data) {
  ValidationReport report{true, {}, {}};

  const Shape* shape = schema_.getShape(targetShapeId);
  if (!shape) {
    report.schemaErrors.push_back("Shape '" + targetShapeId + "' not found");
    report.conforms = false;
    return report;
  }

  auto result = shape->validate(data);
  report.conforms = result.isValid;
  report.nodeErrors[nodeIri] = result.errors;

  return report;
}

ShExValidator::ValidationReport ShExValidator::validateDataset(
    const std::map<std::string,
      std::map<std::string,
        std::vector<std::pair<std::string, ValueType>>>>& dataset,
    const std::map<std::string, std::string>& nodeToShapeMapping) {
  ValidationReport report{true, {}, {}};

  // Phase 3B Performance Optimization Note:
  // ========================================
  // This is the baseline sequential implementation for compatibility.
  //
  // For production workloads with large datasets (>10K nodes), use the
  // optimized validators from src/shex/:
  //
  // 1. PARALLEL VALIDATION (Dataset-Level Parallelization):
  //    #include "shex/ParallelValidator.h"
  //    shex::ParallelValidator::Config config;
  //    config.numWorkers = 8;
  //    config.enableCaching = true;
  //    config.enableWorkStealing = true;
  //    shex::ParallelValidator validator(schema_, config);
  //    auto results = validator.validateDatasetParallel(dataset, nodeToShapeMapping);
  //    // Expected speedup: 7-8x on 8 cores
  //
  // 2. STREAMING VALIDATION (Memory-Efficient Batching):
  //    #include "shex/StreamingValidator.h"
  //    shex::StreamingValidator::Config config;
  //    config.batchSize = 1000;
  //    config.validatorParallelism = 8;
  //    shex::StreamingValidator validator(schema_, config);
  //    auto results = validator.validateDataset(dataset.begin(), dataset.end(), nodeToShapeMapping);
  //    // Memory footprint: O(batchSize) instead of O(dataset size)
  //
  // 3. CACHING (Repeated Validations):
  //    #include "shex/ValidationCache.h"
  //    auto cache = std::make_shared<shex::NodeShapeValidationCache>(
  //        shex::createHighPerformanceCache());
  //    shex::BatchValidator validator(schema_, cache);
  //    // First run: cold cache
  //    // Subsequent runs: >95% cache hit rate
  //
  // See benchmark/ShExValidationBenchmark.cpp for performance comparisons.

  for (const auto& [nodeIri, properties] : dataset) {
    auto shapeIt = nodeToShapeMapping.find(nodeIri);
    if (shapeIt == nodeToShapeMapping.end()) {
      report.schemaErrors.push_back("No shape mapping for node: " + nodeIri);
      continue;
    }

    auto nodeReport = validateNode(nodeIri, shapeIt->second, properties);
    if (!nodeReport.conforms) {
      report.conforms = false;
      report.nodeErrors.insert(nodeReport.nodeErrors.begin(),
                              nodeReport.nodeErrors.end());
    }
  }

  return report;
}

// ============================================================================
// Enhanced Validation with Detailed Error Reporting
// ============================================================================

DetailedValidationError ShExValidator::createCardinalityError(
    const std::string& nodeId,
    const std::string& shapeId,
    const PropertyShape& prop,
    int actualCount) {
  ErrorBuilder builder;

  int expectedMin = 0, expectedMax = -1;
  std::string cardinalityStr;

  switch (prop.cardinality) {
    case Cardinality::EXACTLY_ONE:
      expectedMin = expectedMax = 1;
      cardinalityStr = "exactly 1";
      break;
    case Cardinality::ZERO_OR_ONE:
      expectedMin = 0;
      expectedMax = 1;
      cardinalityStr = "0 or 1";
      break;
    case Cardinality::ZERO_OR_MORE:
      expectedMin = 0;
      expectedMax = -1;
      cardinalityStr = "0 or more";
      break;
    case Cardinality::ONE_OR_MORE:
      expectedMin = 1;
      expectedMax = -1;
      cardinalityStr = "1 or more";
      break;
  }

  std::ostringstream msg;
  msg << "Cardinality violation for property " << prop.predicate;
  msg << ": expected " << cardinalityStr << " occurrence(s), found " << actualCount;

  std::ostringstream suggestion;
  if (actualCount < expectedMin) {
    suggestion << "Add " << (expectedMin - actualCount)
               << " more value(s) for property " << prop.predicate;
  } else if (expectedMax != -1 && actualCount > expectedMax) {
    suggestion << "Remove " << (actualCount - expectedMax)
               << " value(s) from property " << prop.predicate;
  }

  return builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage(msg.str())
      .setSuggestion(suggestion.str())
      .setShapeId(shapeId)
      .setPropertyId(prop.predicate)
      .setNodeId(nodeId)
      .setExpectedCount(expectedMin)
      .setActualCount(actualCount)
      .build();
}

DetailedValidationError ShExValidator::createTypeMismatchError(
    const std::string& nodeId,
    const std::string& shapeId,
    const PropertyShape& prop,
    const std::string& value,
    ValueType actualType) {
  ErrorBuilder builder;

  std::string expectedTypeStr, actualTypeStr;

  if (prop.valueConstraint.valueType.has_value()) {
    switch (prop.valueConstraint.valueType.value()) {
      case ValueType::IRI: expectedTypeStr = "IRI"; break;
      case ValueType::LITERAL: expectedTypeStr = "LITERAL"; break;
      case ValueType::BNODE: expectedTypeStr = "BNODE"; break;
    }
  }

  switch (actualType) {
    case ValueType::IRI: actualTypeStr = "IRI"; break;
    case ValueType::LITERAL: actualTypeStr = "LITERAL"; break;
    case ValueType::BNODE: actualTypeStr = "BNODE"; break;
  }

  std::ostringstream msg;
  msg << "Type mismatch for property " << prop.predicate
      << ": expected " << expectedTypeStr
      << ", but got " << actualTypeStr;

  std::ostringstream suggestion;
  if (expectedTypeStr == "IRI") {
    suggestion << "Use an IRI (e.g., <http://example.org/...>) instead of a literal";
  } else if (expectedTypeStr == "LITERAL") {
    suggestion << "Use a literal value (e.g., \"value\") instead of an IRI";
  }

  return builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage(msg.str())
      .setSuggestion(suggestion.str())
      .setShapeId(shapeId)
      .setPropertyId(prop.predicate)
      .setNodeId(nodeId)
      .setExpectedValue(expectedTypeStr)
      .setActualValue(actualTypeStr)
      .setTripleContext(TripleContext(nodeId, prop.predicate, value, actualTypeStr))
      .build();
}

DetailedValidationError ShExValidator::createShapeNotFoundError(
    const std::string& shapeId) {
  ErrorBuilder builder;

  return builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::SHAPE_NOT_FOUND)
      .setMessage("Shape '" + shapeId + "' not found in schema")
      .setSuggestion("Check that the shape is defined in the schema and the name is correct")
      .setShapeId(shapeId)
      .build();
}

DetailedValidationError ShExValidator::createValueNotAllowedError(
    const std::string& nodeId,
    const std::string& shapeId,
    const PropertyShape& prop,
    const std::string& value) {
  ErrorBuilder builder;

  std::ostringstream msg;
  msg << "Value '" << value << "' not allowed for property " << prop.predicate;

  std::ostringstream suggestion;
  if (!prop.valueConstraint.allowedIris.empty()) {
    suggestion << "Use one of the allowed values: ";
    bool first = true;
    for (const auto& allowed : prop.valueConstraint.allowedIris) {
      if (!first) suggestion << ", ";
      suggestion << "<" << allowed << ">";
      first = false;
      if (suggestion.str().length() > 100) {
        suggestion << "...";
        break;
      }
    }
  }

  return builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::VALUE_NOT_ALLOWED)
      .setMessage(msg.str())
      .setSuggestion(suggestion.str())
      .setShapeId(shapeId)
      .setPropertyId(prop.predicate)
      .setNodeId(nodeId)
      .setActualValue(value)
      .build();
}

DetailedValidationError ShExValidator::createExtraPropertyError(
    const std::string& nodeId,
    const std::string& shapeId,
    const std::string& predicate) {
  ErrorBuilder builder;

  return builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::EXTRA_PROPERTY)
      .setMessage("Extra property '" + predicate + "' not allowed in closed shape")
      .setSuggestion("Remove this property or add it to the shape definition, "
                    "or make the shape non-closed")
      .setShapeId(shapeId)
      .setPropertyId(predicate)
      .setNodeId(nodeId)
      .build();
}

EnhancedValidationReport ShExValidator::validateNodeEnhanced(
    const std::string& nodeIri,
    const std::string& targetShapeId,
    const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>& data) {
  EnhancedValidationReport report;

  const Shape* shape = schema_.getShape(targetShapeId);
  if (!shape) {
    auto error = createShapeNotFoundError(targetShapeId);
    report.addError(error);
    report.conformanceMap.addEntry(nodeIri, targetShapeId,
                                   ConformanceStatus::DOES_NOT_CONFORM);
    return report;
  }

  // Track which predicates we've seen
  absl::flat_hash_set<std::string> seenPredicates;

  // Check each property in the shape
  for (const auto& prop : shape->properties) {
    auto it = data.find(prop.predicate);
    seenPredicates.insert(prop.predicate);

    // Check cardinality
    size_t count = (it != data.end()) ? it->second.size() : 0;

    bool cardinalityValid = true;
    switch (prop.cardinality) {
      case Cardinality::EXACTLY_ONE:
        if (count != 1) {
          auto error = createCardinalityError(nodeIri, targetShapeId, prop, count);
          report.addError(error);
          report.conformanceMap.addError(nodeIri, targetShapeId, error);
          cardinalityValid = false;
        }
        break;
      case Cardinality::ZERO_OR_ONE:
        if (count > 1) {
          auto error = createCardinalityError(nodeIri, targetShapeId, prop, count);
          report.addError(error);
          report.conformanceMap.addError(nodeIri, targetShapeId, error);
          cardinalityValid = false;
        }
        break;
      case Cardinality::ZERO_OR_MORE:
        // Always valid
        break;
      case Cardinality::ONE_OR_MORE:
        if (count < 1) {
          auto error = createCardinalityError(nodeIri, targetShapeId, prop, count);
          report.addError(error);
          report.conformanceMap.addError(nodeIri, targetShapeId, error);
          cardinalityValid = false;
        }
        break;
    }

    // Validate each value against constraints
    if (it != data.end()) {
      for (const auto& [value, type] : it->second) {
        // Type constraint
        if (prop.valueConstraint.valueType.has_value() &&
            prop.valueConstraint.valueType.value() != type) {
          auto error = createTypeMismatchError(nodeIri, targetShapeId, prop, value, type);
          report.addError(error);
          report.conformanceMap.addError(nodeIri, targetShapeId, error);
        }

        // Value set constraint
        if (!prop.valueConstraint.allowedIris.empty() &&
            !prop.valueConstraint.allowedIris.contains(value)) {
          auto error = createValueNotAllowedError(nodeIri, targetShapeId, prop, value);
          report.addError(error);
          report.conformanceMap.addError(nodeIri, targetShapeId, error);
        }
      }
    }
  }

  // Check for extra properties in closed shapes
  if (shape->closed) {
    for (const auto& [predicate, values] : data) {
      if (!seenPredicates.contains(predicate) &&
          !shape->extraPredicates_.contains(predicate)) {
        auto error = createExtraPropertyError(nodeIri, targetShapeId, predicate);
        report.addError(error);
        report.conformanceMap.addError(nodeIri, targetShapeId, error);
      }
    }
  }

  // If no errors, mark as conforming
  if (report.errors.empty()) {
    report.conformanceMap.addEntry(nodeIri, targetShapeId, ConformanceStatus::CONFORMS);
  }

  report.computeStatistics();
  return report;
}

EnhancedValidationReport ShExValidator::validateDatasetEnhanced(
    const std::map<std::string,
      std::map<std::string, std::vector<std::pair<std::string, ValueType>>>>& dataset,
    const std::map<std::string, std::string>& nodeToShapeMapping) {
  EnhancedValidationReport report;

  for (const auto& [nodeIri, properties] : dataset) {
    auto shapeIt = nodeToShapeMapping.find(nodeIri);
    if (shapeIt == nodeToShapeMapping.end()) {
      ErrorBuilder builder;
      auto error = builder
          .setSeverity(ErrorSeverity::ERROR)
          .setErrorType(ErrorType::SHAPE_NOT_FOUND)
          .setMessage("No shape mapping for node: " + nodeIri)
          .setSuggestion("Add a shape mapping for this node")
          .setNodeId(nodeIri)
          .build();
      report.addError(error);
      continue;
    }

    auto nodeReport = validateNodeEnhanced(nodeIri, shapeIt->second, properties);

    // Merge errors
    for (const auto& error : nodeReport.errors) {
      report.addError(error);
    }

    // Merge conformance maps
    const auto& nodeConformanceMap = nodeReport.conformanceMap.getMap();
    for (const auto& [nId, shapeMap] : nodeConformanceMap) {
      for (const auto& [sId, entry] : shapeMap) {
        report.conformanceMap.addEntry(nId, sId, entry.status);
        for (const auto& error : entry.errors) {
          report.conformanceMap.addError(nId, sId, error);
        }
      }
    }
  }

  report.computeStatistics();
  return report;
}

// ============================================================================
// 80/20 Performance Optimizations Implementation
// ============================================================================

void ShExValidator::buildPredicateIndex() {
  // Build index mapping predicates to shapes that use them
  std::vector<std::pair<std::string, std::vector<std::string>>> shapes;

  for (const auto& [shapeId, shape] : schema_.getShapes()) {
    std::vector<std::string> predicates;
    for (const auto& prop : shape.properties) {
      predicates.push_back(prop.predicate);
    }
    shapes.push_back({shapeId, predicates});
  }

  predicateIndex_.buildIndex(shapes);
}

}  // namespace shex
