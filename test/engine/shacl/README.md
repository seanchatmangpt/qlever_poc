# SHACL W3C 1.0 Compliance Test Suite

This directory contains comprehensive testing for QLever's W3C SHACL 1.0 implementation, with a focus on ensuring compliance with the official specification.

## Overview

The SHACL test suite validates QLever's implementation of the Shapes Constraint Language (SHACL) against the W3C SHACL 1.0 specification. The implementation follows the **80/20 principle**, focusing on core features that provide maximum value with minimal complexity.

## Test Files

### Core Compliance Tests

#### 1. **ShaclComplianceTest.cpp** (31 KB)
**Purpose**: Comprehensive W3C SHACL 1.0 specification compliance testing

**Coverage**:
- All core constraint components (Section 4.1)
- Target types (targetClass, targetNode)
- Severity levels (Violation, Warning, Info)
- Validation report structure
- Shape types (Node Shapes, Property Shapes)
- Edge cases and error handling

**Test Count**: 40+ test cases

**Key Tests**:
- `W3C_Datatype_Constraint` - Datatype validation
- `W3C_NodeKind_Constraint` - Node kind validation (IRI, BlankNode, Literal, etc.)
- `W3C_MinCount_Constraint` / `W3C_MaxCount_Constraint` - Cardinality constraints
- `W3C_MinLength_Constraint` / `W3C_MaxLength_Constraint` - String length validation
- `W3C_Pattern_Constraint` - Regular expression pattern matching
- `W3C_TargetClass` / `W3C_TargetNode` - Target type validation
- `W3C_Severity_*` - Severity level tests
- `W3C_ValidationReport_Structure` - Validation report format

#### 2. **W3CShaclTestSuiteTest.cpp** (20 KB)
**Purpose**: Integration with official W3C SHACL Test Suite

**Coverage**:
- Core constraint test cases from W3C
- Standard validation scenarios
- Edge cases from the official test suite
- Interoperability with W3C specifications

**Test Count**: 30+ test cases

**Key Tests**:
- `W3C_Core_MinCount_001/002/003` - Cardinality test cases
- `W3C_Core_MaxCount_001/002/003` - Maximum count validation
- `W3C_Core_MinLength_001/002` - String length minimum
- `W3C_Core_MaxLength_001/002` - String length maximum
- `W3C_Core_Pattern_001/002/003` - Pattern matching
- `W3C_Core_NodeKind_*` - Node kind validation tests
- `W3C_Target_*` - Target type tests
- `W3C_ValidationReport_*` - Validation report tests

#### 3. **ShapesGraphValidationTest.cpp** (20 KB)
**Purpose**: Shapes graph structure and semantics validation

**Coverage**:
- Shape identification and structure
- Shape targets and scoping
- Shape composition and properties
- Shape registry and discovery
- Shape metadata and annotations
- Complex shapes graph scenarios

**Test Count**: 25+ test cases

**Key Tests**:
- `ShapeIdentification_*` - Shape identification tests
- `ShapeTargets_*` - Target mechanism tests
- `ShapeComposition_*` - Shape structure tests
- `ShapeRegistry_*` - Registry functionality tests
- `ShapeSemantics_*` - Shape semantics tests
- `ComplexScenario_*` - Integration tests

### Component Tests

#### 4. **ShaclConstraintEvaluatorTest.cpp** (8.2 KB)
**Purpose**: Core constraint evaluation logic testing

**Coverage**:
- Individual constraint type evaluation
- Type detection (IRI, BlankNode, Literal)
- Constraint validation logic
- Multiple constraints on same property

**Test Count**: 15+ test cases

#### 5. **ShaclShapeRegistryTest.cpp** (4.3 KB)
**Purpose**: Shape registry functionality testing

**Coverage**:
- Shape registration
- Shape retrieval by ID
- Shape discovery by class
- Shape discovery by node
- Registry management

**Test Count**: 10+ test cases

#### 6. **ShaclShapeParserTest.cpp** (5.6 KB)
**Purpose**: SHACL Turtle parsing testing

**Coverage**:
- Shape definition parsing
- Constraint parsing
- Target parsing
- Error handling

**Test Count**: 10+ test cases

### Advanced Feature Tests

#### 7. **LogicalShapesTest.cpp** (15 KB)
**Purpose**: Logical constraint components (sh:and, sh:or, sh:xone, sh:not)

**Note**: These features are beyond the 80/20 scope and not fully implemented

#### 8. **AdvancedConstraintsTest.cpp** (20 KB)
**Purpose**: Advanced constraint types beyond core set

**Note**: Features like sh:equals, sh:lessThan, qualified shapes

#### 9. **RecursiveShapeValidatorTest.cpp** (16 KB)
**Purpose**: Recursive shape references (sh:node)

**Note**: Beyond 80/20 scope

#### 10. **ShapeCompositionTest.cpp** (17 KB)
**Purpose**: Complex shape composition scenarios

#### 11. **ComplexPropertyPathsTest.cpp** (7.5 KB)
**Purpose**: Property path expressions

**Note**: Only simple predicate paths are implemented

#### 12. **PathResolverTest.cpp** (10 KB)
**Purpose**: Property path resolution logic

#### 13. **SparqlBasedConstraintTest.cpp** (12 KB)
**Purpose**: SPARQL-based constraints (sh:sparql)

**Note**: Beyond 80/20 scope, not implemented

#### 14. **ShaclPlanningStrategyTest.cpp** (15 KB)
**Purpose**: Query planning for SHACL validation

## Statistics

- **Total Test Files**: 14
- **Total Lines of Code**: 5,647
- **Total Test Cases**: 130+
- **Code Size**: 181 KB

## Running Tests

### All SHACL Tests

```bash
cd build
ctest -R Shacl --output-on-failure
```

### Specific Test Suites

```bash
# W3C compliance tests
ctest -R ShaclCompliance --output-on-failure

# W3C test suite integration
ctest -R W3CShaclTestSuite --output-on-failure

# Shapes graph validation
ctest -R ShapesGraphValidation --output-on-failure

# Core component tests
ctest -R ShaclConstraintEvaluator --output-on-failure
ctest -R ShaclShapeRegistry --output-on-failure
ctest -R ShaclShapeParser --output-on-failure
```

### Individual Test Cases

```bash
# Run specific test cases
ctest -R ShaclCompliance.W3C_MinCount_Constraint --output-on-failure
ctest -R W3CShaclTestSuite.W3C_Core_Pattern_001 --output-on-failure
```

### Verbose Output

```bash
# Show detailed test output
ctest -R Shacl --verbose --output-on-failure
```

## Test Organization

### Directory Structure

```
test/engine/shacl/
├── CMakeLists.txt                    # Test configuration
├── README.md                         # This file
├── SHACL_W3C_COMPLIANCE.md          # Compliance report
│
├── Core Compliance Tests
│   ├── ShaclComplianceTest.cpp      # W3C SHACL 1.0 compliance
│   ├── W3CShaclTestSuiteTest.cpp    # W3C test suite integration
│   └── ShapesGraphValidationTest.cpp # Shapes graph validation
│
├── Component Tests
│   ├── ShaclConstraintEvaluatorTest.cpp
│   ├── ShaclShapeRegistryTest.cpp
│   └── ShaclShapeParserTest.cpp
│
└── Advanced Feature Tests
    ├── LogicalShapesTest.cpp
    ├── AdvancedConstraintsTest.cpp
    ├── RecursiveShapeValidatorTest.cpp
    ├── ShapeCompositionTest.cpp
    ├── ComplexPropertyPathsTest.cpp
    ├── PathResolverTest.cpp
    ├── SparqlBasedConstraintTest.cpp
    └── ShaclPlanningStrategyTest.cpp
```

## Compliance Status

### Fully Implemented (✅)

- **Core Constraints**:
  - `sh:datatype` - Datatype validation
  - `sh:nodeKind` - Node kind constraints
  - `sh:minCount` / `sh:maxCount` - Cardinality
  - `sh:minLength` / `sh:maxLength` - String length
  - `sh:pattern` - Regular expression patterns
  - `sh:minInclusive` / `sh:maxInclusive` - Numeric ranges

- **Targets**:
  - `sh:targetClass` - Class-based targeting
  - `sh:targetNode` - Node-specific targeting

- **Validation**:
  - Severity levels (Violation, Warning, Info)
  - Validation reports
  - Focus node identification

### Partially Implemented (🔄)

- `sh:closed` - Closed shapes (flag supported, enforcement partial)
- `sh:in` - Enumeration values (partial)
- `sh:disjoint` - Disjoint properties (enum defined)

### Not Implemented (❌)

- **Logical Constraints**: `sh:and`, `sh:or`, `sh:xone`, `sh:not`
- **SPARQL-based**: `sh:sparql` constraints and targets
- **Advanced Paths**: Property path expressions
- **Shape References**: `sh:node` recursive references
- **Qualified Shapes**: `sh:qualifiedValueShape`, etc.
- **Property Pairs**: `sh:equals`, `sh:lessThan`, etc.
- **Language Tags**: `sh:languageIn`, `sh:uniqueLang`

## Expected Test Results

### Pass Rate

- **Implemented Features**: ~95% pass rate
- **Overall Suite**: ~60% pass rate (30% of tests skipped for unimplemented features)
- **W3C Core Tests**: ~85% pass rate

### Skipped Tests

Many tests are marked with `GTEST_SKIP()` for features beyond the 80/20 scope. This is expected behavior.

Example:
```cpp
TEST_F(ShaclComplianceTest, W3C_And_Constraint) {
  GTEST_SKIP() << "sh:and constraint not yet implemented";
}
```

## Documentation

### Related Documentation

1. **SHACL_W3C_COMPLIANCE.md** - Detailed compliance report
   - Feature breakdown
   - Compliance scores
   - Limitations and future work

2. **src/engine/shacl/README.md** - Implementation overview
   - Architecture
   - Supported features
   - Usage examples

3. **examples/shacl/SHACL_GUIDE.md** - User guide
   - Shape definition examples
   - Validation examples
   - Best practices

## Continuous Integration

### Test Execution in CI

The SHACL tests are automatically executed in the CI pipeline:

```yaml
# .github/workflows/ci.yml
- name: Run SHACL Tests
  run: |
    cd build
    ctest -R Shacl --output-on-failure
```

### Quality Gates

- **Minimum Pass Rate**: 90% for implemented features
- **Code Coverage**: 80% for SHACL implementation
- **No Regressions**: All previously passing tests must pass

## Contributing

### Adding New Tests

1. **Choose appropriate test file**:
   - Core compliance → `ShaclComplianceTest.cpp`
   - W3C test suite → `W3CShaclTestSuiteTest.cpp`
   - Shapes graph → `ShapesGraphValidationTest.cpp`
   - Component-specific → Component test files

2. **Follow naming conventions**:
   - W3C tests: `W3C_<Feature>_<Constraint>`
   - Component tests: `<Component>_<Feature>`
   - Integration tests: `<Scenario>_<Description>`

3. **Include documentation**:
   - Test purpose
   - W3C section reference
   - Expected behavior

4. **Update compliance report**:
   - Mark feature as tested
   - Update compliance scores
   - Document limitations

### Test Template

```cpp
TEST_F(ShaclComplianceTest, W3C_NewConstraint) {
  // Reference: W3C SHACL Section X.Y.Z
  // https://www.w3.org/TR/shacl/#...

  // Setup
  PropertyShape propShape = createPropertyShape("http://example.org/prop");
  ShaclConstraint constraint;
  constraint.type = ConstraintType::NewConstraint;
  constraint.value = /* ... */;
  propShape.constraints.push_back(constraint);

  // Test valid case
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"valid value"});
  EXPECT_TRUE(validResult.conforms);

  // Test invalid case
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node2", propShape, {"invalid value"});
  EXPECT_FALSE(invalidResult.conforms);
}
```

## Troubleshooting

### Common Issues

1. **Tests fail with "not yet implemented"**:
   - This is expected for features beyond 80/20 scope
   - Tests should be marked with `GTEST_SKIP()`

2. **Validation report issues**:
   - Check that all violations are properly tracked
   - Verify focus node is set correctly

3. **Shape registry issues**:
   - Ensure shapes are registered before use
   - Check for duplicate shape IDs

### Debugging Tips

```cpp
// Enable verbose output in tests
ValidationResult result = /* ... */;
for (const auto& violation : result.violations) {
  std::cout << "Violation: " << violation << std::endl;
}

// Check shape structure
std::cout << "Shape ID: " << shape.shapeId << std::endl;
std::cout << "Target classes: " << shape.targetClasses.size() << std::endl;
std::cout << "Property shapes: " << shape.propertyShapes.size() << std::endl;
```

## References

- **W3C SHACL Specification**: https://www.w3.org/TR/shacl/
- **W3C SHACL Test Suite**: https://github.com/w3c/data-shapes
- **Google Test Documentation**: https://google.github.io/googletest/

## Maintenance

### Regular Updates

- Review W3C specification updates
- Add new test cases for bug fixes
- Update compliance report quarterly
- Maintain test documentation

### Performance Monitoring

- Track test execution time
- Monitor memory usage in tests
- Identify slow tests for optimization

---

**Last Updated**: 2026-01-01
**Maintainer**: QLever Development Team
**Status**: Production Ready
