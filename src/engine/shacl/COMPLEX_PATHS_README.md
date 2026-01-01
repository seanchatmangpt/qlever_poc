# Complex Property Paths in SHACL

This document describes the complex property path implementation for QLever's SHACL validation engine.

## Overview

The complex property path feature extends SHACL validation to support advanced path expressions beyond simple property IRIs. This implementation follows the SHACL specification for property paths and enables validation of complex graph patterns.

## Components

### 1. ComplexPropertyPaths (ComplexPropertyPaths.h/cpp)

Defines the property path type system supporting:

- **Simple Paths**: Direct property IRIs (`ex:property`)
- **Inverse Paths**: Reverse direction (`^ex:property`)
- **Sequence Paths**: Property chains (`ex:p1/ex:p2/ex:p3`)
- **Alternative Paths**: Multiple options (`ex:p1|ex:p2`)
- **Transitive Paths**:
  - Zero or more: `ex:property*`
  - One or more: `ex:property+`
  - Zero or one: `ex:property?`
- **Wildcard Paths**: Match any property (`*`)

### 2. PathResolver (PathResolver.h/cpp)

Resolves property paths against RDF data:

- **RdfDataProvider Interface**: Abstracts RDF data access
- **InMemoryRdfData**: Simple in-memory implementation for testing
- **PathResolver**: Main resolution engine with:
  - BFS-based transitive closure computation
  - Configurable maximum recursion depth
  - Efficient path existence checking
  - Value counting utilities

### 3. Extended PropertyShape (ShaclShape.h)

PropertyShape now supports both simple and complex paths:

```cpp
// Backward compatible: simple path
PropertyShape shape1("http://example.org/property");

// Complex path
PropertyPath complexPath = PropertyPath::parse("^http://example.org/parent");
PropertyShape shape2(complexPath);
```

## Usage Examples

### Creating Complex Paths

```cpp
#include "engine/shacl/ComplexPropertyPaths.h"

using namespace shacl;

// Simple path
auto simple = PropertyPath::simple("http://example.org/knows");

// Inverse path: ^knows
auto inverse = PropertyPath::inverse(simple);

// Sequence path: knows/knows
std::vector<PropertyPath> paths = {simple, simple};
auto sequence = PropertyPath::sequence(std::move(paths));

// Alternative path: knows|parent
auto knows = PropertyPath::simple("http://example.org/knows");
auto parent = PropertyPath::simple("http://example.org/parent");
std::vector<PropertyPath> altPaths = {knows, parent};
auto alternative = PropertyPath::alternative(std::move(altPaths));

// Transitive closure: parent*
auto transitive = PropertyPath::zeroOrMore(parent);

// Parse from string
auto parsed = PropertyPath::parse("^http://example.org/parent");
```

### Resolving Paths Against RDF Data

```cpp
#include "engine/shacl/PathResolver.h"

using namespace shacl;

// Create RDF data
InMemoryRdfData data;
data.addTriple("http://example.org/alice",
               "http://example.org/knows",
               "http://example.org/bob");
data.addTriple("http://example.org/bob",
               "http://example.org/knows",
               "http://example.org/charlie");

// Create resolver
PathResolver resolver(&data);

// Simple path resolution
auto knows = PropertyPath::simple("http://example.org/knows");
auto friends = resolver.resolve("http://example.org/alice", knows);
// Result: ["http://example.org/bob"]

// Sequence path: knows/knows (friends of friends)
std::vector<PropertyPath> paths = {knows, knows};
auto knowsKnows = PropertyPath::sequence(std::move(paths));
auto fof = resolver.resolve("http://example.org/alice", knowsKnows);
// Result: ["http://example.org/charlie"]

// Check path existence
bool pathExists = resolver.pathExists(
    "http://example.org/alice",
    "http://example.org/charlie",
    knowsKnows
);
// Result: true

// Count values reachable via path
size_t count = resolver.countValues("http://example.org/alice", knows);
// Result: 1
```

### Using Complex Paths in Property Shapes

```cpp
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ComplexPropertyPaths.h"

using namespace shacl;

// Create a property shape with inverse path
// Validates that the node is referenced by at least one parent
auto parentPath = PropertyPath::simple("http://example.org/parent");
auto inverseParent = PropertyPath::inverse(parentPath);
PropertyShape childShape(inverseParent);

// Add constraint: at least one parent must reference this node
ShaclConstraint minCount(ConstraintType::MinCount);
minCount.value = 1;
childShape.constraints.push_back(minCount);

// Check if shape uses complex path
if (childShape.hasComplexPath()) {
    std::cout << "Using complex path: "
              << childShape.propertyPath.toString() << std::endl;
}
```

## Implementation Details

### Path Type System

Paths are represented using `std::variant` for type-safe discrimination:

```cpp
using PathVariant = std::variant<
    SimplePath, InversePath, SequencePath, AlternativePath,
    ZeroOrMorePath, OneOrMorePath, ZeroOrOnePath, WildcardPath
>;
```

### Path Resolution Algorithm

1. **Simple Paths**: Direct lookup in RDF data
2. **Inverse Paths**: Reverse lookup (subject ← predicate ← object)
3. **Sequence Paths**: Iterative resolution through each segment
4. **Alternative Paths**: Union of results from all alternatives
5. **Transitive Paths**: BFS with configurable depth limit

### Performance Considerations

- **Transitive Closure**: Uses BFS with depth limiting (default: 100 hops)
- **Caching**: Results are not cached by default (implement caching layer if needed)
- **Inverse Paths**: For complex inner paths, requires checking all potential subjects (expensive)

### Configuration

```cpp
PathResolver resolver(&data);

// Set maximum recursion depth for transitive paths
resolver.setMaxDepth(50);  // Default: 100
```

## Testing

Comprehensive test suite included:

```bash
# Run complex path tests
ctest -R ComplexPropertyPathsTest --output-on-failure

# Run path resolver tests
ctest -R PathResolverTest --output-on-failure

# Run all SHACL tests
ctest -R Shacl --output-on-failure
```

## Limitations and Future Work

### Current Limitations

1. **Inverse of Complex Paths**: Inverse of complex paths (except simple paths) requires full graph scan
2. **No Negation**: Negated property sets not yet implemented
3. **No Path Algebra**: Advanced path expressions (e.g., `(p1|p2)*/(p3|p4)`) may be expensive

### Future Enhancements

1. **Index Integration**: Direct integration with QLever's index for efficient path queries
2. **Query Optimization**: Path query optimization and rewriting
3. **Materialized Paths**: Pre-computed transitive closures for frequently used paths
4. **SPARQL Property Paths**: Full alignment with SPARQL 1.1 property path syntax

## SHACL Compliance

This implementation follows the SHACL W3C Recommendation:
- https://www.w3.org/TR/shacl/#property-paths

Supported SHACL path features:
- ✅ Predicate paths (`sh:path ex:property`)
- ✅ Inverse paths (`sh:inversePath`)
- ✅ Sequence paths (`sh:path (ex:p1 ex:p2)`)
- ✅ Alternative paths (`sh:path [sh:alternativePath (...)]`)
- ✅ Zero or more paths (`sh:zeroOrMorePath`)
- ✅ One or more paths (`sh:oneOrMorePath`)
- ✅ Zero or one paths (`sh:zeroOrOnePath`)

Not yet implemented:
- ⏳ Negated property sets
- ⏳ Complex RDF list-based path syntax parsing

## Integration Example

Complete example integrating with SHACL validation:

```cpp
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ComplexPropertyPaths.h"
#include "engine/shacl/PathResolver.h"
#include "engine/shacl/ShaclConstraintEvaluator.h"

using namespace shacl;

// Define shape: validates organizational hierarchy
NodeShape orgShape;
orgShape.shapeId = "http://example.org/OrgShape";
orgShape.targetClasses.push_back("http://example.org/Employee");

// Property shape: every employee must have a manager
// via the "reportsTo" property
auto reportsTo = PropertyPath::simple("http://example.org/reportsTo");
PropertyShape managerShape(reportsTo);

ShaclConstraint minCount(ConstraintType::MinCount);
minCount.value = 1;
managerShape.constraints.push_back(minCount);

orgShape.propertyShapes.push_back(managerShape);

// Property shape: CEO should have no manager
// (inverse of reportsTo should have no values)
auto inverseReportsTo = PropertyPath::inverse(reportsTo);
PropertyShape ceoShape(inverseReportsTo);

ShaclConstraint maxCount(ConstraintType::MaxCount);
maxCount.value = 0;
ceoShape.constraints.push_back(maxCount);

// This shape applies only to CEOs
NodeShape ceoNodeShape;
ceoNodeShape.shapeId = "http://example.org/CEOShape";
ceoNodeShape.targetClasses.push_back("http://example.org/CEO");
ceoNodeShape.propertyShapes.push_back(ceoShape);
```

## References

- SHACL Specification: https://www.w3.org/TR/shacl/
- SPARQL Property Paths: https://www.w3.org/TR/sparql11-query/#propertypaths
- QLever SHACL Implementation: `/src/engine/shacl/README.md`
