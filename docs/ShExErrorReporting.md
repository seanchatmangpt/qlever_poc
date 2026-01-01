# W3C ShEx Comprehensive Error Reporting System

## Overview

This document describes the comprehensive error reporting system for W3C ShEx (Shape Expressions) validation in QLever. The system provides detailed, context-rich error messages to help users understand and fix validation failures.

## Architecture

### Core Components

1. **DetailedValidationError** - Rich error structure with full context
2. **ShapeConformanceMap** - Tracks which nodes fail which shapes
3. **EnhancedValidationReport** - Complete validation report with statistics
4. **ErrorBuilder** - Fluent API for constructing errors
5. **Multiple Output Formats** - Human-readable, JSON, and XML

## Features

### 1. Error Severity Levels

```cpp
enum class ErrorSeverity {
  ERROR,    // Validation failure, shape does not conform
  WARNING,  // Potential issue but not a hard failure
  INFO      // Informational message about validation
};
```

**Use Cases:**
- **ERROR**: Hard validation failures (cardinality violations, type mismatches, missing required properties)
- **WARNING**: Soft issues (datatype coercions, deprecated patterns)
- **INFO**: Informational messages (validation progress, shape matches)

### 2. Error Type Classification

```cpp
enum class ErrorType {
  CARDINALITY_VIOLATION,      // Wrong number of property occurrences
  TYPE_MISMATCH,               // Value type doesn't match constraint
  VALUE_NOT_ALLOWED,           // Value not in allowed set
  DATATYPE_MISMATCH,           // Literal datatype doesn't match
  SHAPE_NOT_FOUND,             // Referenced shape doesn't exist
  MISSING_REQUIRED_PROPERTY,   // Required property absent
  EXTRA_PROPERTY,              // Property not allowed in closed shape
  PARSER_SYNTAX_ERROR,         // Parsing failed
  PARSER_SEMANTIC_ERROR,       // Parsing succeeded but semantically invalid
  CONSTRAINT_VIOLATION         // Generic constraint violation
};
```

### 3. Triple Context

Every validation error can include complete triple information:

```cpp
struct TripleContext {
  std::string subject;    // Node being validated
  std::string predicate;  // Property causing the error
  std::string object;     // Value that failed validation
  std::string objectType; // "IRI", "LITERAL", or "BNODE"
};
```

**Example:**
```
<ex:person1> <ex:email> "alice@example.org"
```

### 4. Source Location (Parser Errors)

Parser errors include precise line/column information:

```cpp
struct SourceLocation {
  int line;
  int column;
  std::optional<int> endLine;
  std::optional<int> endColumn;
};
```

**Example:**
```
line 15, column 42 to line 15, column 43
```

### 5. Expected vs Actual Values

Errors include both what was expected and what was found:

```cpp
// For type errors
expectedValue: "LITERAL"
actualValue: "IRI"

// For cardinality errors
expectedCount: 1
actualCount: 3
```

### 6. Suggestions and Hints

Each error can include actionable suggestions:

```cpp
suggestion: "Remove 2 occurrences of ex:email property, keeping only one"
suggestion: "Change the value to a literal string (e.g., \"John\") instead of an IRI"
suggestion: "Add a triple with ex:name property: ex:person3 ex:name \"Name\"^^xsd:string"
```

### 7. Shape Conformance Map

Tracks validation status for every node-shape pair:

```cpp
class ShapeConformanceMap {
  // Map: NodeIRI -> ShapeID -> ConformanceEntry

  // Query conformance status
  bool nodeConformsToShape(const std::string& nodeId, const std::string& shapeId);

  // Get all shapes a node fails
  std::vector<std::string> getFailedShapes(const std::string& nodeId);

  // Add error for specific node-shape pair
  void addError(const std::string& nodeId, const std::string& shapeId,
                const DetailedValidationError& error);
};
```

## Usage Examples

### Building an Error

```cpp
using namespace shex;

auto error = ErrorBuilder()
    .setSeverity(ErrorSeverity::ERROR)
    .setErrorType(ErrorType::CARDINALITY_VIOLATION)
    .setMessage("Property occurs 3 times but shape requires exactly 1 occurrence")
    .setShapeId("ex:PersonShape")
    .setPropertyId("ex:email")
    .setNodeId("ex:person1")
    .setExpectedCount(1)
    .setActualCount(3)
    .setTripleContext(TripleContext("ex:person1", "ex:email",
                                    "alice@example.org", "LITERAL"))
    .setSuggestion("Remove 2 occurrences of ex:email property, keeping only one")
    .build();
```

### Creating a Validation Report

```cpp
EnhancedValidationReport report;

// Add errors
report.addError(cardinalityError);
report.addError(typeError);
report.addError(missingPropertyError);

// Update conformance map
report.conformanceMap.addEntry("ex:person1", "ex:PersonShape",
                               ConformanceStatus::DOES_NOT_CONFORM);
report.conformanceMap.addError("ex:person1", "ex:PersonShape", cardinalityError);

// Compute statistics
report.computeStatistics();

// Check results
std::cout << "Conforms: " << report.conforms << std::endl;
std::cout << "Total Errors: " << report.totalErrors << std::endl;
std::cout << "Total Warnings: " << report.totalWarnings << std::endl;
```

### Output Formats

#### Human-Readable Format

```cpp
std::cout << report.toHumanReadable() << std::endl;
```

Output:
```
=== ShEx Validation Report ===
Overall Status: DOES NOT CONFORM
Errors: 5, Warnings: 1, Info: 0

Detailed Errors:
================

[ERROR] CARDINALITY_VIOLATION: Property occurs 3 times but shape requires exactly 1 occurrence
  Node: ex:person1
  Shape: ex:PersonShape
  Property: ex:email
  Triple: <ex:person1> <ex:email> "alice@example.org"
  Expected count: 1
  Actual count: 3
  Suggestion: Remove 2 occurrences of ex:email property, keeping only one
```

#### JSON Format

```cpp
std::cout << report.toJson().dump(2) << std::endl;
```

Output:
```json
{
  "conforms": false,
  "statistics": {
    "totalErrors": 5,
    "totalWarnings": 1,
    "totalInfoMessages": 0
  },
  "errors": [
    {
      "severity": "ERROR",
      "errorType": "CARDINALITY_VIOLATION",
      "message": "Property occurs 3 times but shape requires exactly 1 occurrence",
      "shapeId": "ex:PersonShape",
      "nodeId": "ex:person1",
      "propertyId": "ex:email",
      "tripleContext": {
        "subject": "ex:person1",
        "predicate": "ex:email",
        "object": "alice@example.org",
        "objectType": "LITERAL"
      },
      "expectedCount": 1,
      "actualCount": 3,
      "suggestion": "Remove 2 occurrences of ex:email property, keeping only one"
    }
  ],
  "conformanceMap": { ... }
}
```

#### XML Format

```cpp
std::cout << report.toXml() << std::endl;
```

Output:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<validationReport>
  <conforms>false</conforms>
  <statistics>
    <totalErrors>5</totalErrors>
    <totalWarnings>1</totalWarnings>
    <totalInfoMessages>0</totalInfoMessages>
  </statistics>
  <errors>
    <error>
      <severity>ERROR</severity>
      <errorType>CARDINALITY_VIOLATION</errorType>
      <message>Property occurs 3 times but shape requires exactly 1 occurrence</message>
      <shapeId>ex:PersonShape</shapeId>
      <nodeId>ex:person1</nodeId>
      <propertyId>ex:email</propertyId>
      <expectedCount>1</expectedCount>
      <actualCount>3</actualCount>
      <suggestion>Remove 2 occurrences of ex:email property, keeping only one</suggestion>
    </error>
  </errors>
</validationReport>
```

## Error Scenarios

### Scenario 1: Cardinality Violation

**Problem:** Property appears 3 times but shape requires exactly 1

**Error Message:**
```
[ERROR] CARDINALITY_VIOLATION: Property occurs 3 times but shape requires exactly 1 occurrence
  Node: ex:person1
  Shape: ex:PersonShape
  Property: ex:email
  Triple: <ex:person1> <ex:email> "alice@example.org"
  Expected count: 1
  Actual count: 3
  Suggestion: Remove 2 occurrences of ex:email property, keeping only one
```

### Scenario 2: Type Mismatch

**Problem:** Value is IRI but shape requires LITERAL

**Error Message:**
```
[ERROR] TYPE_MISMATCH: Property value type is IRI but shape requires LITERAL
  Node: ex:person2
  Shape: ex:PersonShape
  Property: ex:name
  Triple: <ex:person2> <ex:name> <http://example.org/names/John>
  Expected: LITERAL
  Actual: IRI
  Suggestion: Change the value to a literal string (e.g., "John") instead of an IRI
```

### Scenario 3: Value Not in Allowed Set

**Problem:** Value is not in the allowed enumeration

**Error Message:**
```
[ERROR] VALUE_NOT_ALLOWED: Property value is not in the allowed set of IRIs
  Node: ex:employee42
  Shape: ex:EmployeeShape
  Property: ex:department
  Triple: <ex:employee42> <ex:department> <http://example.org/HumanResources>
  Expected: One of: [ex:Sales, ex:Engineering, ex:Marketing]
  Actual: ex:HumanResources
  Suggestion: Use one of the allowed department IRIs: ex:Sales, ex:Engineering, or ex:Marketing
```

### Scenario 4: Missing Required Property

**Problem:** Required property is absent

**Error Message:**
```
[ERROR] MISSING_REQUIRED_PROPERTY: Required property is missing (cardinality requires at least 1 occurrence)
  Node: ex:person3
  Shape: ex:PersonShape
  Property: ex:name
  Expected count: 1
  Actual count: 0
  Suggestion: Add a triple with ex:name property: ex:person3 ex:name "Name"^^xsd:string
```

### Scenario 5: Parser Syntax Error

**Problem:** Invalid ShEx syntax

**Error Message:**
```
[ERROR] PARSER_SYNTAX_ERROR: Expected '}' to close shape definition but found ';'
  Shape: ex:PersonShape
  Location: line 15, column 42 to line 15, column 43
  Suggestion: Add a closing brace '}' at the end of the shape definition
```

### Scenario 6: Datatype Mismatch (Warning)

**Problem:** Datatype is string but integer was expected

**Error Message:**
```
[WARNING] DATATYPE_MISMATCH: Literal value has datatype xsd:string but xsd:integer was expected
  Node: ex:person4
  Shape: ex:PersonShape
  Property: ex:age
  Triple: <ex:person4> <ex:age> "thirty"^^xsd:string
  Expected: xsd:integer
  Actual: xsd:string
  Suggestion: Change the value to a numeric literal: "30"^^xsd:integer
```

## Integration with Existing ShEx Implementation

The enhanced error reporting system integrates seamlessly with the existing ShEx validator:

```cpp
// Existing basic validation
Shape::ValidationResult Shape::validate(
    const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>& nodeData) const;

// Enhanced validation (returns detailed errors)
EnhancedValidationReport Shape::validateEnhanced(
    const std::string& nodeId,
    const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>& nodeData) const;
```

### Migration Path

1. **Phase 1**: Basic errors still work (backward compatible)
2. **Phase 2**: Add enhanced error reporting as opt-in
3. **Phase 3**: Deprecate basic error reporting
4. **Phase 4**: Remove basic error reporting

## Implementation Files

- **Header**: `/home/user/qlever/src/parser/ShExErrorReporting.h`
- **Examples**: `/home/user/qlever/src/parser/ShExErrorReportingExamples.cpp`
- **Documentation**: `/home/user/qlever/docs/ShExErrorReporting.md`
- **Sample Output**: `/home/user/qlever/shex_error_reporting_output.json`

## Testing

Test cases should cover:

1. Error construction with ErrorBuilder
2. Validation report creation and statistics
3. Conformance map queries
4. Output format generation (human, JSON, XML)
5. Integration with existing ShEx validator
6. All error scenarios (6 types)

## Performance Considerations

- **Memory**: Detailed errors require more memory (acceptable for validation use case)
- **Construction**: ErrorBuilder uses move semantics for efficiency
- **Output**: JSON/XML generation is lazy (only when requested)
- **Caching**: Conformance map uses flat_hash_map for O(1) lookups

## Future Enhancements

1. **Error Aggregation**: Group related errors
2. **Error Recovery**: Suggest automatic fixes
3. **Localization**: Multi-language error messages
4. **Interactive Mode**: Step-through validation with breakpoints
5. **Visual Output**: HTML reports with syntax highlighting
6. **Machine Learning**: Learn common error patterns and improve suggestions

## References

- **W3C ShEx Specification**: https://shex.io/shex-semantics/
- **ShEx Primer**: https://shex.io/shex-primer/
- **QLever ShEx Implementation**: `/home/user/qlever/src/parser/ShEx.h`
- **QLever ShEx Documentation**: `/home/user/qlever/docs/ShEx.md`

## License

Apache 2.0 (same as QLever project)

## Contact

For questions about the error reporting system, consult the main QLever documentation or the ShEx implementation team.
