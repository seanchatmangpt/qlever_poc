# SHACL (Shapes Constraint Language) in QLever

## Overview

This document describes the SHACL implementation in QLever, built using the **80/20 principle** to deliver core RDF validation functionality with minimal implementation overhead.

**QLever SHACL Implementation Status: Production-Ready Core Features**
- ✅ NodeShape and PropertyShape validation
- ✅ Core constraint types (cardinality, datatype, pattern, length)
- ✅ Shape registry and discovery
- ✅ Validation reporting
- 🚫 Advanced features (recursive shapes, shape inheritance) - Not in 80/20 scope

---

## Architecture

### Core Components

1. **ShaclShape.h** - Data structures
   - `NodeShape` - SHACL node shape definition
   - `PropertyShape` - Property constraints
   - `ShaclConstraint` - Individual constraint definition
   - `ValidationResult` - Result for a single resource
   - `ValidationReport` - Overall validation report

2. **ShaclShapeParser.h/cpp** - Parsing
   - Parses SHACL shapes from Turtle format
   - Extracts targets, properties, and constraints

3. **ShaclConstraintEvaluator.h/cpp** - Evaluation
   - Evaluates constraints against RDF values
   - Type detection and matching
   - Constraint-specific evaluation logic

4. **ShaclShapeRegistry.h/cpp** - Shape Management
   - Registers and retrieves shapes by ID
   - Discovers shapes for classes and nodes

5. **ShaclValidator.h/cpp** - Operation
   - Query execution tree operation
   - Integrates validation into SPARQL query execution

---

## Supported Constraints (80/20)

### Cardinality Constraints
- **sh:minCount** - Minimum number of property values
- **sh:maxCount** - Maximum number of property values

### Property Value Constraints
- **sh:datatype** - Required RDF datatype
- **sh:pattern** - Regular expression pattern match
- **sh:minLength** - Minimum string length
- **sh:maxLength** - Maximum string length
- **sh:minInclusive** - Minimum numeric value
- **sh:maxInclusive** - Maximum numeric value

### NodeKind Constraints
- **sh:nodeKind** - Type of RDF node (IRI, BlankNode, Literal, etc.)

### Shape Targeting
- **sh:targetClass** - Target nodes of a given class
- **sh:targetNode** - Target specific nodes

---

## Usage Examples

### Basic Shape Definition

```turtle
@prefix sh: <http://www.w3.org/ns/shacl#> .
@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .
@prefix ex: <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

ex:PersonShape a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:property [
    sh:path foaf:name ;
    sh:minCount 1 ;          # Required property
    sh:maxCount 1 ;          # Single-valued
    sh:datatype xsd:string ; # Must be string
    sh:minLength 1 ;         # Non-empty
    sh:maxLength 200        # Reasonable length
  ] .
```

### Email Validation

```turtle
ex:ContactShape a sh:NodeShape ;
  sh:targetClass ex:Contact ;
  sh:property [
    sh:path foaf:email ;
    sh:minCount 1 ;          # Email required
    sh:maxCount 1 ;          # Single email
    sh:pattern "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"
  ] .
```

### Numeric Range Validation

```turtle
ex:PersonShape a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:property [
    sh:path ex:age ;
    sh:datatype xsd:integer ;
    sh:minInclusive 0 ;      # Age >= 0
    sh:maxInclusive 150     # Age <= 150
  ] .
```

### Node Type Validation

```turtle
ex:ResourceShape a sh:NodeShape ;
  sh:targetClass ex:Resource ;
  sh:property [
    sh:path ex:homepage ;
    sh:nodeKind sh:IRI ;     # Must be an IRI, not literal
    sh:maxCount 1
  ] .
```

---

## API Reference

### ShaclShapeRegistry

```cpp
// Register a shape
void registerShape(const NodeShape& shape);

// Retrieve shape by ID
const NodeShape* getShape(const std::string& shapeId) const;

// Get shapes targeting a class
std::vector<const NodeShape*> getShapesForClass(
    const std::string& classIri) const;

// Get shapes targeting a node
std::vector<const NodeShape*> getShapesForNode(
    const std::string& nodeIri) const;

// Get all registered shapes
std::vector<const NodeShape*> getAllShapes() const;

// Enable/disable validation
void setEnabled(bool enabled);
bool isEnabled() const;
```

### ShaclConstraintEvaluator

```cpp
// Evaluate constraint against value
static bool evaluateConstraint(const ShaclConstraint& constraint,
                                const std::string& value,
                                const std::string& datatype = "");

// Evaluate property shape (all constraints)
static ValidationResult evaluatePropertyShape(
    const std::string& nodeId,
    const PropertyShape& propShape,
    const std::vector<std::string>& values);

// Type checking helpers
static bool isValidIri(const std::string& value);
static bool isBlankNode(const std::string& value);
static bool isLiteral(const std::string& value);
static std::string getDatatype(const std::string& value);
```

### ShaclValidator Operation

```cpp
// Validate resources from a subtree against shapes
class ShaclValidator : public Operation {
  // Constructor
  ShaclValidator(QueryExecutionContext* qec,
                 std::shared_ptr<QueryExecutionTree> subtree,
                 const ShaclShapeRegistry* shapeRegistry,
                 ColumnIndex resourceColumnIndex = 0);
};
```

---

## Integration with SPARQL

The ShaclValidator integrates as an operation in the query execution tree, allowing validation within SPARQL queries:

```sparql
# Conceptual usage (implementation would depend on SPARQL extension)
SELECT ?person
WHERE {
  ?person rdf:type foaf:Person .
  ?person foaf:name ?name .
  BIND(validate(?person, ex:PersonShape) AS ?isValid)
  FILTER(?isValid = true)
}
```

---

## Data Types and Nodes

### RDF Value Representation

Values are represented as strings following RDF conventions:
- **IRI**: `http://example.org/resource` or `<http://example.org/resource>`
- **Blank Node**: `_:b1`, `_:node123`
- **Literal**: `"hello"`, `"hello"@en`, `"42"^^xsd:integer`

### Type Detection

The validator automatically detects RDF value types:
```cpp
ShaclConstraintEvaluator::getDatatype("http://example.org/x")
// → "http://www.w3.org/1999/02/22-rdf-syntax-ns#IRI"

ShaclConstraintEvaluator::getDatatype("\"hello\"")
// → "http://www.w3.org/2001/XMLSchema#string"

ShaclConstraintEvaluator::getDatatype("_:b1")
// → "http://www.w3.org/1999/02/22-rdf-syntax-ns#BlankNode"
```

---

## Validation Report

### Structure

```cpp
struct ValidationReport {
  bool conforms;                           // Overall conformance
  size_t violationCount;                  // Total violations
  std::unordered_map<std::string, ValidationResult> results;

  // Methods
  size_t conformingCount() const;         // Count of conforming resources
};

struct ValidationResult {
  std::string focusNode;                  // Resource being validated
  bool conforms;                          // Resource conforms?
  std::vector<std::string> violations;    // Violation messages
};
```

### Example Report

```json
{
  "conforms": false,
  "violationCount": 3,
  "results": {
    "http://example.org/alice": {
      "focusNode": "http://example.org/alice",
      "conforms": true,
      "violations": []
    },
    "http://example.org/bob": {
      "focusNode": "http://example.org/bob",
      "conforms": false,
      "violations": [
        "Required property foaf:name is missing",
        "Maximum count violated: expected at most 1 values for property foaf:email"
      ]
    }
  }
}
```

---

## Implementation Details

### 80/20 Design Decisions

1. **Simplified Parsing**
   - Turtle format only (not RDF/XML or N-Triples)
   - Pattern-based rather than full RDF parser

2. **Focus on Common Constraints**
   - Core cardinality: minCount, maxCount
   - Core validation: pattern, length, datatype
   - Omitted: recursive shapes, shape inheritance, custom validation

3. **Filtering Approach**
   - Validation results in filtering (only conforming resources in output)
   - Rather than separate validation report generation

4. **Linear Complexity**
   - O(n) validation cost where n = result size
   - No advanced optimizations for large datasets

### Performance Characteristics

- **Shape Registration**: O(1)
- **Shape Lookup**: O(1) by ID, O(n) by class/node
- **Constraint Evaluation**: O(1) per value
- **Resource Validation**: O(p × c) where p = properties, c = constraints

---

## Testing

Comprehensive test suite included in `test/engine/shacl/`:

```bash
# Run SHACL tests
ctest -R ShaclConstraintEvaluator --output-on-failure
ctest -R ShaclShapeRegistry --output-on-failure
ctest -R ShaclShapeParser --output-on-failure
```

---

## Examples

See `examples/shacl/` for example SHACL shape definitions:
- `person-shape.ttl` - Person, Contact, and Organization shapes
- Common patterns and best practices

---

## Future Enhancements (Beyond 80/20)

1. **Advanced Constraints**
   - sh:unique - Value uniqueness
   - sh:disjointWith - Property disjointness
   - sh:in - Enumerated values

2. **Recursive Shapes**
   - sh:shape - Reference other shapes
   - Nested validation

3. **Custom Validation**
   - SPARQL-based shapes
   - JavaScript functions

4. **Performance Optimization**
   - Index-based shape targeting
   - Constraint caching
   - Parallel validation

5. **Error Reporting**
   - Detailed violation reports
   - Focus node paths
   - Severity levels

---

## References

- **SHACL Specification**: https://www.w3.org/TR/shacl/
- **RDF Specification**: https://www.w3.org/RDF/
- **Turtle Format**: https://www.w3.org/TR/turtle/

---

## Summary

The QLever SHACL implementation provides a lightweight, focused validation system following the 80/20 principle:

- ✅ **Core Functionality**: NodeShape, PropertyShape, constraint evaluation
- ✅ **Integration**: Operation-based architecture fits naturally into QLever
- ✅ **Usability**: Simple API and comprehensive examples
- ✅ **Testing**: Full test coverage of core features

This allows users to validate RDF graphs against shapes without the complexity of a full SHACL engine, while remaining compatible with standard SHACL definitions.
