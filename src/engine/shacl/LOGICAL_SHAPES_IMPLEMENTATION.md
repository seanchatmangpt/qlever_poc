# Logical Shape Composition Implementation for SHACL

## Overview

This document describes the implementation of logical shape composition for SHACL in QLever, supporting `sh:and`, `sh:or`, `sh:not`, and `sh:xone` constraints as defined in the SHACL specification.

## Files Created/Modified

### New Files

1. **LogicalShapes.h** (`/home/user/qlever/src/engine/shacl/LogicalShapes.h`)
   - Defines logical shape constraint classes
   - Implements short-circuit evaluation for performance
   - Supports both shape references (by ID) and inline shapes

2. **LogicalShapes.cpp** (`/home/user/qlever/src/engine/shacl/LogicalShapes.cpp`)
   - Implements evaluation logic for all logical operators
   - Includes optimizations:
     - `sh:and`: short-circuits on first failure
     - `sh:or`: short-circuits on first success
     - `sh:not`: validates exactly one shape
     - `sh:xone`: counts conforming shapes (no short-circuit)

3. **LogicalShapesTest.cpp** (`/home/user/qlever/test/engine/shacl/LogicalShapesTest.cpp`)
   - Comprehensive test suite covering all logical operators
   - Tests edge cases: empty constraints, invalid references, nested compositions
   - Tests short-circuit behavior
   - Tests inline shapes vs shape references

### Modified Files

1. **ShaclShape.h**
   - Added forward declarations for logical constraint classes
   - Extended `NodeShape` class with:
     - `andConstraint`, `orConstraint`, `notConstraint`, `xoneConstraint` members
     - Setter methods for each logical constraint type
     - `hasLogicalConstraints()` helper method

2. **ShaclValidator.cpp**
   - Updated `validateResource()` to evaluate logical constraints
   - Logical constraints are evaluated before property shapes
   - Added include for `LogicalShapes.h`

3. **src/engine/CMakeLists.txt**
   - Added `shacl/LogicalShapes.cpp` to engine library

4. **test/CMakeLists.txt**
   - Added SHACL test suite entries:
     - `ShaclConstraintEvaluatorTest`
     - `ShaclShapeParserTest`
     - `ShaclShapeRegistryTest`
     - `LogicalShapesTest`

## Implementation Details

### Logical Constraint Types

#### 1. `sh:and` (AndConstraint)
- **Behavior**: All referenced shapes must conform
- **Short-circuit**: Stops on first failure
- **Use case**: Combine multiple shape requirements

```cpp
auto andConstraint = std::make_shared<AndConstraint>();
andConstraint->addShapeReference("PersonShape");
andConstraint->addShapeReference("EmployeeShape");
nodeShape.setAndConstraint(andConstraint);
```

#### 2. `sh:or` (OrConstraint)
- **Behavior**: At least one referenced shape must conform
- **Short-circuit**: Stops on first success
- **Use case**: Alternative shape requirements

```cpp
auto orConstraint = std::make_shared<OrConstraint>();
orConstraint->addShapeReference("EmailShape");
orConstraint->addShapeReference("PhoneShape");
nodeShape.setOrConstraint(orConstraint);
```

#### 3. `sh:not` (NotConstraint)
- **Behavior**: Referenced shape must NOT conform
- **Validation**: Requires exactly one shape
- **Use case**: Exclusion constraints

```cpp
auto notConstraint = std::make_shared<NotConstraint>();
notConstraint->addShapeReference("InvalidShape");
nodeShape.setNotConstraint(notConstraint);
```

#### 4. `sh:xone` (XoneConstraint)
- **Behavior**: Exactly one referenced shape must conform
- **No short-circuit**: Must evaluate all shapes to count conforming ones
- **Use case**: Exclusive alternatives

```cpp
auto xoneConstraint = std::make_shared<XoneConstraint>();
xoneConstraint->addShapeReference("TypeAShape");
xoneConstraint->addShapeReference("TypeBShape");
nodeShape.setXoneConstraint(xoneConstraint);
```

### Shape References

Shapes can be referenced in two ways:

1. **By ID** (references registry):
```cpp
constraint->addShapeReference("ShapeID");
```

2. **Inline** (self-contained):
```cpp
auto inlineShape = std::make_shared<NodeShape>();
// configure inlineShape...
constraint->addInlineShape(inlineShape);
```

### Performance Optimizations

1. **Short-circuit Evaluation**
   - `AND`: Early exit on first failure
   - `OR`: Early exit on first success
   - Reduces unnecessary validations

2. **Efficient Violation Reporting**
   - Violations collected hierarchically
   - Clear error messages with shape context

3. **Minimal Memory Overhead**
   - Shared pointers for shape references
   - Optional inline shapes only when needed

## Test Coverage

The test suite (`LogicalShapesTest.cpp`) includes:

- **Basic functionality tests** for each operator
- **Short-circuit verification**
- **Edge cases**:
  - Empty constraint lists
  - Non-existent shape references
  - Invalid configurations (e.g., NOT with multiple shapes)
- **Complex compositions**:
  - Nested logical constraints
  - Mixed inline and referenced shapes
- **Integration tests**:
  - With property shapes
  - With node constraints

## Usage Example

```cpp
#include "engine/shacl/LogicalShapes.h"
#include "engine/shacl/ShaclShapeRegistry.h"

// Create registry and register base shapes
ShaclShapeRegistry registry;

auto personShape = std::make_shared<NodeShape>();
personShape->shapeId = "PersonShape";
// ... configure person shape ...
registry.registerShape(*personShape);

auto employeeShape = std::make_shared<NodeShape>();
employeeShape->shapeId = "EmployeeShape";
// ... configure employee shape ...
registry.registerShape(*employeeShape);

// Create composite shape using AND
auto compositeShape = std::make_shared<NodeShape>();
compositeShape->shapeId = "EmployeePersonShape";

auto andConstraint = std::make_shared<AndConstraint>();
andConstraint->addShapeReference("PersonShape");
andConstraint->addShapeReference("EmployeeShape");

compositeShape->setAndConstraint(andConstraint);
registry.registerShape(*compositeShape);

// Validation will now check both PersonShape AND EmployeeShape
```

## SHACL Specification Compliance

This implementation follows the W3C SHACL specification:
- https://www.w3.org/TR/shacl/#core-components-logical

Supported features:
- ✅ sh:and (Section 4.1)
- ✅ sh:or (Section 4.2)
- ✅ sh:not (Section 4.3)
- ✅ sh:xone (Section 4.4)

## Future Enhancements

Potential improvements:
1. **Parser integration**: Extend `ShaclShapeParser` to parse logical constraints from Turtle
2. **Nested optimization**: Cache evaluation results for deeply nested logical constraints
3. **Parallel evaluation**: Evaluate independent shapes in `sh:or` and `sh:xone` in parallel
4. **Cycle detection**: Prevent infinite recursion in circular shape references

## Notes

- Logical constraints are evaluated **before** property shapes in validation
- Multiple logical constraints can coexist on the same shape
- Shape references are resolved through the `ShaclShapeRegistry`
- Inline shapes bypass the registry for self-contained definitions
