# Phase 2D Quick Reference

## Core Usage

### 1. Shape Reference Syntax

```shex
shape PersonShape {
  name LITERAL ;
  address @AddressShape ;      # Shape reference
  spouse @PersonShape ?         # Recursive reference
}

shape AddressShape {
  city LITERAL ;
  country IRI
}
```

### 2. Detecting Cycles

```cpp
#include "parser/ShEx.h"
#include "parser/ShExShapeReference.h"

// Parse schema
ShExParser parser;
auto schema = parser.parse(shexInput).value();

// Build dependency graph
auto graph = schema.buildDependencyGraph();

// Detect cycles
auto cycles = graph.detectCycles();

// Check if specific shape is recursive
bool isRecursive = graph.isShapeRecursive("PersonShape");
```

### 3. Validating with Recursion

```cpp
// Create validation context
ValidationContext context;

// Validate with cycle protection
ShExValidator validator(schema);
auto report = validator.validateNode(
    nodeIri,
    targetShapeId,
    nodeData,
    context  // Tracks validation path
);

// Check depth
if (context.getDepth() > 100) {
    std::cout << "Deep recursion detected\n";
}
```

### 4. Using ValidationGuard (RAII)

```cpp
void validateRecursive(const std::string& shapeId,
                      ValidationContext& ctx) {
    ValidationGuard guard(ctx, shapeId);
    
    if (!guard.isValid()) {
        // Cycle detected - optimistic validation
        return;
    }
    
    // Perform validation...
    // guard.~ValidationGuard() called automatically
}
```

## Key Classes

### ValidationContext
- `enterShape(id)` - Returns false if cycle
- `exitShape(id)` - Removes from stack
- `isDepthExceeded()` - Check if > MAX_RECURSION_DEPTH
- `getPath()` - Get validation path for errors

### ShapeDependencyGraph
- `addDependency(from, to)` - Add edge
- `detectCycles()` - O(V+E) DFS detection
- `isShapeRecursive(id)` - Check if in cycle
- `getDependencies(id)` - Get direct dependencies

### ShapeReference
- `targetShapeId` - Referenced shape
- `isRecursive` - Marked if part of cycle

## Test Examples

### Basic Reference
```cpp
TEST_F(ShapeReferenceTest, BasicShapeReference) {
  auto schema = parser.parse(R"(
    shape PersonShape {
      address @AddressShape
    }
    shape AddressShape {
      city LITERAL
    }
  )");
  
  ASSERT_TRUE(schema.has_value());
  const Shape* person = schema->getShape("PersonShape");
  EXPECT_TRUE(person->properties[0].isShapeReference());
  EXPECT_EQ(person->properties[0].getReferencedShapeId().value(),
            "AddressShape");
}
```

### Recursive Shape
```cpp
TEST_F(ShapeReferenceTest, SelfRecursive) {
  auto schema = parser.parse(R"(
    shape PersonShape {
      children @PersonShape *
    }
  )");
  
  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("PersonShape"));
}
```

### Cycle Detection
```cpp
TEST_F(ShapeReferenceTest, ThreeShapeCycle) {
  auto schema = parser.parse(R"(
    shape A { refToB @B }
    shape B { refToC @C }
    shape C { refToA @A }
  )");
  
  auto cycles = schema->buildDependencyGraph().detectCycles();
  EXPECT_TRUE(cycles.contains("A"));
  EXPECT_TRUE(cycles.contains("B"));
  EXPECT_TRUE(cycles.contains("C"));
}
```

## Performance Characteristics

| Operation | Time | Space |
|-----------|------|-------|
| Parse `@Shape` | O(1) | O(1) |
| Build graph | O(P) | O(V+E) |
| Detect cycles | O(V+E) | O(V) |
| Enter shape | O(1) | O(d) |
| Validate (no rec) | O(P) | O(1) |
| Validate (rec) | O(V·P) | O(d) |

P = properties, V = shapes, E = references, d = depth

## Configuration

```cpp
// Maximum recursion depth (configurable)
ValidationContext::MAX_RECURSION_DEPTH = 1000;  // default

// Check depth limit
if (context.isDepthExceeded()) {
    // Handle deep recursion
}
```

## Error Messages

**Missing Shape:**
```
Shape 'PersonShape' references non-existent shape 'AddressShape'
in property 'http://example.org/address'
```

**Cycle Info:**
```
Recursive validation cycle detected for shape: PersonShape
Path: PersonShape → AddressShape → ContactShape → PersonShape
```

**Depth Limit:**
```
Maximum recursion depth exceeded for shape: NodeShape
Current depth: 1001
```

## Common Patterns

### Family Tree
```shex
shape PersonShape {
  name LITERAL ;
  parent @PersonShape ? ;
  children @PersonShape *
}
```

### Organizational Chart
```shex
shape OrgUnitShape {
  name LITERAL ;
  parentUnit @OrgUnitShape ? ;
  subUnits @OrgUnitShape *
}
```

### Linked List
```shex
shape NodeShape {
  value LITERAL ;
  next @NodeShape ?
}
```

### Binary Tree
```shex
shape TreeNodeShape {
  value LITERAL ;
  left @TreeNodeShape ? ;
  right @TreeNodeShape ?
}
```

---

See PHASE_2D_IMPLEMENTATION_REPORT.md for complete documentation.
