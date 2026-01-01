# ShEx Implementation - Quick Reference

## Files Overview

### Header: `ShEx.h`
Defines core data structures and interfaces:

- **ValueSetConstraint** (lines 21-26): Constraints on RDF values
  - `valueType`: IRI, LITERAL, or BNODE
  - `allowedIris`: Whitelist specific IRIs
  - `datatypeRestriction`: Datatype validation

- **PropertyShape** (lines 28-35): Property constraints
  - `predicate`: RDF predicate URI
  - `valueConstraint`: How values must look
  - `cardinality`: How many times property can appear

- **Shape** (lines 37-54): Complete shape definition
  - `id`: Shape identifier
  - `properties`: Vector of PropertyShapes
  - `validate()`: Check RDF node against this shape

- **ShExSchema** (lines 56-66): Container for shapes
  - `addShape()`: Register a shape
  - `getShape()`: Look up shape by ID
  - `hasShape()`: Check if shape exists

- **ShExParser** (lines 68-88): Parse text to shapes
  - `parse()`: Convert ShEx text to ShExSchema
  - Input format: `shape ShapeId { prop predicate Type card; ... }`
  - Simple hand-written recursive descent parser

- **ShExValidator** (lines 90-119): Validate RDF against shapes
  - `validateNode()`: Check one node against a shape
  - `validateDataset()`: Check multiple nodes
  - Returns `ValidationReport` with conformance status and errors

### Implementation: `ShEx.cpp`
Implements the logic (~270 lines):

- **Constraint Validation** (lines 24-40): Check if value matches constraint
- **Shape Validation** (lines 54-103): Verify all properties match cardinality/constraints
- **Schema Management** (lines 108-122): Shape storage and lookup
- **Parser** (lines 127-280): Convert text to shapes
- **Validator** (lines 285-340): Validation logic

### Tests: `test/parser/ShExTest.cpp`
39 comprehensive tests covering:

- Constraint validation (type checking, IRI whitelisting)
- Cardinality validation (exactly-one, zero-or-one, zero-or-more, one-or-more)
- Parser functionality (simple shapes, multiple shapes, type constraints)
- Node and dataset validation
- Integration tests (parse + validate together)

## Quick Start

### Basic Usage

```cpp
// 1. Create a shape
Shape person("PersonShape");
PropertyShape name("http://example.org/name");
name.valueConstraint.valueType = ValueType::LITERAL;
name.cardinality = Cardinality::EXACTLY_ONE;
person.addProperty(name);

// 2. Create schema
ShExSchema schema;
schema.addShape(person);

// 3. Prepare data
std::map<string, vector<pair<string, ValueType>>> nodeData;
nodeData["http://example.org/name"].push_back({"John", ValueType::LITERAL});

// 4. Validate
ShExValidator validator(schema);
auto report = validator.validateNode("http://example.org/john",
                                     "PersonShape", nodeData);
if (report.conforms) {
    cout << "Valid!" << endl;
}
```

### Parser Usage

```cpp
string shex = R"(
  shape PersonShape {
    http://example.org/name LITERAL ;
    http://example.org/age LITERAL ?
  }
)";

ShExParser parser;
auto schema = parser.parse(shex);
if (schema) {
    // Use schema for validation...
}
```

## Cardinality Options

| Syntax | Name | Occurrences | Default | Marker |
|--------|------|-------------|---------|--------|
| (none) | EXACTLY_ONE | 1 | Yes | (default) |
| `?` | ZERO_OR_ONE | 0 or 1 | No | Optional |
| `*` | ZERO_OR_MORE | 0+ | No | * in regex |
| `+` | ONE_OR_MORE | 1+ | No | + in regex |

## Value Type Constraints

| Type | Matches |
|------|---------|
| `ValueType::IRI` | RDF IRIs/URIs |
| `ValueType::LITERAL` | RDF string/typed literals |
| `ValueType::BNODE` | Blank nodes |

## Parser Grammar (EBNF-style)

```
Schema    ::= Shape*
Shape     ::= 'shape' ID '{' Property* '}'
Property  ::= Predicate [Constraint] [Cardinality] ';'
Predicate ::= ID (IRI format)
Constraint::= 'IRI' | 'LITERAL' | 'BNODE' | '<IRI>'
Cardinality::= '?' | '*' | '+'
ID        ::= [a-zA-Z0-9_/:#\-\.]+
```

## Design Decisions

### 80/20 Focus
- ✅ Core shapes, properties, cardinality, types
- ❌ Semantic actions, annotations, closed shapes, negation

### Hand-Written Parser
- Simpler than ANTLR for simple syntax
- ~150 lines of code
- Easy to extend for Phase 2 features
- Matches existing QLever style

### Optional-Based Error Handling
- Uses `std::optional` instead of exceptions
- Matches C++ modern best practices
- Returns error messages in `getLastError()`

### Data-Driven Validation
- Validation logic independent from data structures
- Easy to test different scenarios
- Supports arbitrary RDF graph representations

## Testing Strategy

Run specific tests:
```bash
# All ShEx tests
ctest -R ShExTest --output-on-failure

# Specific test class
ctest -R ShExTest.ValidateExactlyOneCardinality --output-on-failure

# With verbose output
ctest -R ShExTest -V
```

## Integration Points

### Current
- Part of `src/parser/` module
- Linked with main `parser` library
- Tested alongside other parser functionality

### Future (Phase 2+)
- Query execution context integration
- Index metadata storage
- SPARQL result validation
- Schema inference

## Files Modified for Integration

1. `src/parser/CMakeLists.txt` - Added ShEx.cpp to library
2. `test/parser/CMakeLists.txt` - Added ShExTest to test suite
3. `src/parser/ShEx.h` - New header (150 lines)
4. `src/parser/ShEx.cpp` - New implementation (270 lines)
5. `test/parser/ShExTest.cpp` - New test suite (340 lines)

## Code Style

- **C++20**: Modern features (std::optional, structured bindings)
- **Google Style**: Matches QLever conventions (100-char line length)
- **Pre-commit**: Automatically formatted with clang-format
- **No External Dependencies**: Uses only Abseil and standard library

## Performance Notes

- **Validation**: O(n) where n = total property constraints
- **Parsing**: O(m) where m = input string length
- **Memory**: O(s) where s = total shapes + properties
- **No Allocation in Validation**: Validates provided data in-place

## Next Steps

See `docs/ShEx.md` for:
- Full implementation details
- Architecture documentation
- Usage examples
- Future enhancement phases
- Standards references
