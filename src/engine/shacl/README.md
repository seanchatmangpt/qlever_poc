# SHACL Implementation for QLever

This directory contains the SHACL (Shapes Constraint Language) implementation for QLever, following the 80/20 principle to deliver core RDF validation functionality.

## Files

### Data Structures
- **ShaclShape.h** - Core data structures (NodeShape, PropertyShape, ShaclConstraint, ValidationReport)

### Parsing
- **ShaclShapeParser.h/cpp** - Parse SHACL shape definitions from Turtle format

### Evaluation
- **ShaclConstraintEvaluator.h/cpp** - Evaluate constraints against RDF values
  - Type detection (IRI, BlankNode, Literal)
  - Constraint evaluation (cardinality, datatype, pattern, length)

### Registry
- **ShaclShapeRegistry.h/cpp** - Manage registered shapes
  - Register/retrieve shapes by ID
  - Discover shapes for classes and nodes

### Integration
- **ShaclValidator.h/cpp** - Query execution operation
  - Integrates validation into SPARQL execution
  - Extends Operation base class

## Supported Features (80/20)

### Constraints
- ✅ sh:minCount - Minimum property values
- ✅ sh:maxCount - Maximum property values
- ✅ sh:datatype - Required RDF datatype
- ✅ sh:pattern - Regular expression matching
- ✅ sh:minLength - Minimum string length
- ✅ sh:maxLength - Maximum string length
- ✅ sh:minInclusive / sh:maxInclusive - Numeric ranges
- ✅ sh:nodeKind - Node type constraints (IRI, Literal, BlankNode)

### Shape Targeting
- ✅ sh:targetClass - Target nodes of a class
- ✅ sh:targetNode - Target specific nodes
- ✅ sh:property - Property shape definitions

### Not Included (Beyond 80/20)
- Recursive shapes (sh:shape)
- Shape inheritance
- SPARQL-based validation
- Advanced disjointness constraints

## Quick Example

```cpp
#include "ShaclShapeRegistry.h"
#include "ShaclConstraintEvaluator.h"

// Create and register a shape
shacl::NodeShape personShape;
personShape.shapeId = "PersonShape";
personShape.targetClasses.push_back("http://example.org/Person");

shacl::PropertyShape nameProperty("http://xmlns.com/foaf/0.1/name");
shacl::ShaclConstraint minCount;
minCount.type = shacl::ConstraintType::MinCount;
minCount.value = 1;
nameProperty.constraints.push_back(minCount);

personShape.propertyShapes.push_back(nameProperty);

// Register shape
shacl::ShaclShapeRegistry registry;
registry.registerShape(personShape);

// Validate a value
std::vector<std::string> values = {"\"Alice\""};
auto result = shacl::ShaclConstraintEvaluator::evaluatePropertyShape(
    "http://example.org/alice",
    nameProperty,
    values
);

if (result.conforms) {
    std::cout << "Resource conforms!\n";
}
```

## Testing

Comprehensive test suite in `test/engine/shacl/`:

```bash
# Test constraint evaluation
ctest -R ShaclConstraintEvaluator --output-on-failure

# Test shape registry
ctest -R ShaclShapeRegistry --output-on-failure

# Test parser
ctest -R ShaclShapeParser --output-on-failure
```

## Performance

- Shape registration: O(1)
- Shape lookup by ID: O(1)
- Constraint evaluation: O(1) per value
- Resource validation: O(p × c) where p = properties, c = constraints

## Integration Pattern

ShaclValidator integrates as an Operation in the query execution tree:

```
QueryExecutionTree
└── Operation (ShaclValidator)
    └── Subtree (produces results to validate)
```

Result rows that don't conform to shapes are filtered from the output, with a warning added to the runtime information.

## Architecture Diagram

```
ShaclShapeParser
    ↓
  Parses SHACL Turtle
    ↓
ShaclShapeRegistry
    ↓
  Stores/retrieves shapes
    ↓
ShaclValidator (Operation)
    ↓
  Validates results
    ↓
ShaclConstraintEvaluator
    ↓
  Checks constraints against values
```

## References

- SHACL W3C Specification: https://www.w3.org/TR/shacl/
- Examples: See `examples/shacl/` directory

## Development Notes

### Adding New Constraint Types

1. Add enum value to `ConstraintType` in ShaclShape.h
2. Add variant case in `ShaclConstraint.value`
3. Implement evaluation in `ShaclConstraintEvaluator`
4. Add parser support in `ShaclShapeParser`
5. Add test cases in `test/engine/shacl/`

### Thread Safety

- ShaclShapeRegistry is not thread-safe; synchronize externally
- ShaclConstraintEvaluator is stateless and thread-safe
- ShaclValidator is safe when called on different instances
