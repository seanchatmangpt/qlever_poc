# SHACL Shape Inheritance and Composition

This document describes the SHACL shape composition system implemented in QLever.

## Overview

The shape composition system enables:
1. **Shape Inheritance** via `sh:extends` (non-standard extension)
2. **Shape Composition** via multiple `sh:node` references (standard SHACL)
3. **Parameter Passing** between shapes
4. **Constraint Accumulation** from multiple sources
5. **Dependency Graph Management** with cycle detection
6. **Constraint Resolution** with configurable merge strategies

## Files

### Core Implementation

- **ShapeComposition.h**: Header file with all composition-related classes
- **ShapeComposition.cpp**: Implementation of composition engine
- **ShaclShapeRegistry.h/cpp**: Extended to support composable shapes

### Test Suite

- **ShapeCompositionTest.cpp**: Comprehensive tests covering:
  - Dependency graph management
  - Constraint merging strategies
  - Shape inheritance
  - Parameter substitution
  - Circular dependency detection
  - Complex composition hierarchies

## Architecture

### Key Components

#### 1. ComposableNodeShape

Extends `NodeShape` with composition features:
- `extends`: List of parent shapes (inheritance)
- `nodeReferences`: List of referenced shapes (composition)
- `parameters`: Parameters this shape accepts
- `getDependencies()`: Get all shape dependencies

#### 2. ShapeDependencyGraph

Manages shape dependency relationships:
- `addShape()`: Add shape and its dependencies
- `getResolutionOrder()`: Topological sort for resolution order
- `hasCycle()`: Detect circular dependencies
- `getDependencies()`: Get direct dependencies
- `getDependents()`: Get shapes that depend on a given shape

#### 3. ConstraintMerger

Handles constraint merging with three strategies:

**Override Strategy**:
- Child constraints override parent constraints
- Use when child shapes should completely replace parent behavior

**Accumulate Strategy**:
- All constraints are kept
- Use when constraints should be additive

**MostRestrictive Strategy** (default):
- For conflicting constraints, use the most restrictive
- MinCount: higher value is more restrictive
- MaxCount: lower value is more restrictive
- MinLength/MaxLength: similar logic
- In: smaller list is more restrictive

#### 4. ShapeCompositionEngine

Main composition engine:
- `resolveAllShapes()`: Resolve all composable shapes in registry
- `resolveShape()`: Resolve a single shape
- `applyParameterBindings()`: Substitute parameters in constraints
- `validateComposition()`: Check for errors (cycles, missing dependencies)

## Usage Examples

### Simple Inheritance

```cpp
// Create parent shape
NodeShape parent;
parent.shapeId = "PersonShape";
parent.nodeConstraints.push_back(minCountConstraint(1));

// Create child shape
ComposableNodeShape employee;
employee.shapeId = "EmployeeShape";
employee.addExtends(ShapeReference("PersonShape"));
employee.nodeConstraints.push_back(maxCountConstraint(100));

// Register and resolve
registry.registerShape(parent);
registry.registerComposableShape(employee);

ShapeCompositionEngine engine(&registry);
auto resolved = engine.resolveShape(employee);
// resolved now has constraints from both parent and child
```

### Multi-Level Inheritance

```cpp
// Grandparent -> Parent -> Child hierarchy
NodeShape grandparent;
grandparent.shapeId = "EntityShape";

ComposableNodeShape parent;
parent.shapeId = "PersonShape";
parent.addExtends(ShapeReference("EntityShape"));

ComposableNodeShape child;
child.shapeId = "EmployeeShape";
child.addExtends(ShapeReference("PersonShape"));

// Register all shapes
registry.registerShape(grandparent);
registry.registerComposableShape(parent);
registry.registerComposableShape(child);

// Resolve in order (dependencies first)
engine.resolveAllShapes();
```

### Multiple Inheritance (Diamond Pattern)

```cpp
//     Base
//    /    \
//  Left  Right
//    \    /
//     Top

NodeShape base;
base.shapeId = "BaseShape";

ComposableNodeShape left;
left.shapeId = "LeftShape";
left.addExtends(ShapeReference("BaseShape"));

ComposableNodeShape right;
right.shapeId = "RightShape";
right.addExtends(ShapeReference("BaseShape"));

ComposableNodeShape top;
top.shapeId = "TopShape";
top.addExtends(ShapeReference("LeftShape"));
top.addExtends(ShapeReference("RightShape"));

// Resolve with proper dependency ordering
engine.resolveAllShapes();
```

### Parameter Passing

```cpp
// Define parameterized shape
NodeShape template;
template.shapeId = "AgeRestrictionShape";

ShaclConstraint minAge(ConstraintType::MinInclusive);
minAge.value = std::string("${minAge}");
minAge.message = "Must be at least ${minAge} years old";
template.nodeConstraints.push_back(minAge);

// Use with parameters
ComposableNodeShape adult;
adult.shapeId = "AdultShape";

ShapeReference ref("AgeRestrictionShape");
ref.bindParameter("minAge", "18");
adult.addExtends(ref);

// Resolve with parameter substitution
auto resolved = engine.resolveShape(adult);
// minAge constraint now has value "18" instead of "${minAge}"
```

### Node References (Standard SHACL)

```cpp
// Create reusable constraint set
NodeShape addressConstraints;
addressConstraints.shapeId = "AddressConstraints";
addressConstraints.nodeConstraints.push_back(streetConstraint);
addressConstraints.nodeConstraints.push_back(cityConstraint);

// Reference in main shape
ComposableNodeShape person;
person.shapeId = "PersonShape";
person.addNodeReference(ShapeReference("AddressConstraints"));

// Resolve accumulates constraints
auto resolved = engine.resolveShape(person);
```

### Merge Strategy Configuration

```cpp
ShapeCompositionEngine engine(&registry);

// Use override strategy (child wins)
engine.setMergeStrategy(ConstraintMerger::MergeStrategy::Override);

// Use accumulate strategy (keep all)
engine.setMergeStrategy(ConstraintMerger::MergeStrategy::Accumulate);

// Use most restrictive strategy (default)
engine.setMergeStrategy(ConstraintMerger::MergeStrategy::MostRestrictive);
```

### Validation

```cpp
// Check for composition errors
auto errors = engine.validateComposition();

if (!errors.empty()) {
  for (const auto& error : errors) {
    std::cerr << "Composition error: " << error << std::endl;
  }
}

// Check for cycles
auto depGraph = registry.getDependencyGraph();
if (depGraph && depGraph->hasCycle()) {
  std::cerr << "Circular dependency detected!" << std::endl;
}
```

## Constraint Resolution Order

The system ensures correct resolution order:

1. **Build Dependency Graph**: Track which shapes depend on which
2. **Topological Sort**: Determine resolution order (dependencies first)
3. **Cycle Detection**: Fail if circular dependencies exist
4. **Sequential Resolution**: Process shapes in dependency order
5. **Cache Results**: Avoid redundant resolution

## Property Shape Merging

When parent and child define constraints for the same property:

```cpp
// Parent defines minLength
PropertyShape parentName("http://example.org/name");
parentName.constraints.push_back(minLength(1));

// Child defines maxLength
PropertyShape childName("http://example.org/name");
childName.constraints.push_back(maxLength(100));

// Merged result has both constraints
PropertyShape merged;
merged.path = "http://example.org/name";
merged.constraints = [minLength(1), maxLength(100)];
```

## Error Handling

The system detects and reports:

- **Circular Dependencies**: Shapes that form dependency cycles
- **Missing Dependencies**: References to non-existent shapes
- **Conflicting Constraints**: Incompatible constraint values
- **Invalid Parameters**: Required parameters not provided

## Performance Considerations

- **Caching**: Resolved shapes are cached to avoid redundant work
- **Lazy Resolution**: Shapes resolved only when needed
- **Topological Ordering**: Ensures each shape resolved exactly once
- **Early Validation**: Catches errors before resolution begins

## Integration with Registry

The extended `ShaclShapeRegistry` provides:

```cpp
// Register composable shapes
registry.registerComposableShape(composableShape);

// Resolve all at once
registry.resolveComposableShapes();

// Validate before resolution
auto errors = registry.validateComposition();

// Access dependency graph
auto* graph = registry.getDependencyGraph();
```

## Testing

The test suite (`ShapeCompositionTest.cpp`) covers:

1. **Dependency Graph Tests**:
   - Basic dependencies
   - Multiple dependencies (diamond pattern)
   - Circular dependency detection
   - Self-references

2. **Constraint Merger Tests**:
   - Override strategy
   - Accumulate strategy
   - Most restrictive strategy
   - Property shape merging

3. **Shape Composition Tests**:
   - Simple inheritance
   - Multi-level inheritance
   - Multiple inheritance
   - Parameter substitution
   - Node references
   - Complex hierarchies

4. **Validation Tests**:
   - Success cases
   - Missing dependencies
   - Error reporting

## Future Enhancements

Potential improvements:

1. **SPARQL Query Integration**: Use shapes in query optimization
2. **Shape Validation Caching**: Cache validation results
3. **Incremental Resolution**: Support dynamic shape updates
4. **Advanced Parameters**: Support complex parameter types
5. **Constraint Conflict Resolution**: More sophisticated conflict handling
6. **Performance Profiling**: Optimize resolution for large hierarchies

## References

- SHACL Specification: https://www.w3.org/TR/shacl/
- QLever Documentation: See `/home/user/qlever/CLAUDE.md`
- Shape Files: `/home/user/qlever/src/engine/shacl/`
- Tests: `/home/user/qlever/test/engine/shacl/`
