# W3C ShEx Test Suite Integration

This directory contains the W3C ShEx test suite integration for QLever's ShEx implementation.

## Overview

The W3C ShEx test suite provides comprehensive conformance testing for ShEx implementations. This integration:

- Parses W3C JSON-LD test manifests
- Executes validation, syntax, and structure tests
- Tracks compliance metrics by feature and category
- Generates JSON, HTML, and CSV compliance reports
- Supports skip lists and expected failure tracking
- Provides regression detection across test runs

## Directory Structure

```
shex/
├── W3CManifestParser.h/cpp    # Parse W3C JSON-LD test manifests
├── W3CTestRunner.h/cpp        # Execute W3C test cases
├── ComplianceTracker.h/cpp    # Track and report compliance
├── W3CShExTestSuite.cpp       # Google Test integration
├── skip-list.json             # Tests to skip (unimplemented features)
├── xfail-list.json            # Expected failures (known issues)
├── feature-mapping.json       # Test-to-feature mapping
├── test-data/                 # W3C test repository (submodule)
│   ├── manifest.json          # Main test manifest
│   ├── schemas/               # ShEx schema files
│   └── validation/            # RDF data for validation tests
└── reports/                   # Generated compliance reports
    ├── compliance-report.json
    ├── compliance-report.html
    └── feature-coverage.csv
```

## Setup

### Clone W3C Test Repository

```bash
cd test/parser/shex
git clone https://github.com/shexSpec/shexTest.git test-data
```

Alternatively, add as a git submodule:

```bash
git submodule add https://github.com/shexSpec/shexTest.git test/parser/shex/test-data
git submodule update --init --recursive
```

## Running Tests

### Run All W3C Tests

```bash
cd build
ctest -R W3CShExParameterizedTest --output-on-failure
```

### Run Specific Feature Tests

```bash
# Test cardinality feature
ctest -R W3CShExTestSuite.TestCardinalityFeature --output-on-failure

# Test nodeKind feature
ctest -R W3CShExTestSuite.TestNodeKindFeature --output-on-failure
```

### Generate Compliance Summary

```bash
ctest -R W3CShExTestSuite.GenerateComplianceSummary --output-on-failure
```

This generates:
- `reports/compliance-report.json` - Machine-readable compliance data
- `reports/compliance-report.html` - Human-readable dashboard
- `reports/feature-coverage.csv` - Feature coverage matrix

### View Compliance Report

Open the HTML report in a browser:

```bash
xdg-open test/parser/shex/reports/compliance-report.html
```

## Configuration

### Skip List (`skip-list.json`)

Specifies tests to skip (unimplemented features):

```json
{
  "skip": [
    {
      "id": "semanticAction.*",
      "reason": "Semantic actions not implemented",
      "pattern": true
    }
  ]
}
```

### Expected Failures (`xfail-list.json`)

Specifies known failures:

```json
{
  "xfail": [
    {
      "id": "complex-regex-pattern-01",
      "reason": "Complex regex patterns not supported",
      "issue": "https://github.com/..."
    }
  ]
}
```

### Feature Mapping (`feature-mapping.json`)

Maps tests to implementation features:

```json
{
  "features": {
    "cardinality": {
      "implemented": true,
      "phase": "1",
      "tests": ["cardinality-*"],
      "implementation": "src/parser/ShEx.h:Cardinality"
    }
  }
}
```

## Compliance Metrics

### Overall Statistics

- **Total Tests**: Total W3C test cases
- **Passed**: Tests that passed
- **Failed**: Tests that failed unexpectedly
- **Skipped**: Tests skipped (unimplemented features)
- **Expected Failures**: Tests that failed as expected
- **Errors**: Tests that encountered errors

### Compliance Percentage

```
Compliance % = (Passed + Expected Failures) / Total Tests × 100
```

### Pass Rate (excluding skipped)

```
Pass Rate % = (Passed + Expected Failures) / (Total - Skipped) × 100
```

### Feature Coverage

For each feature:
- Total tests covering the feature
- Passed tests
- Failed tests
- Coverage percentage
- Implementation status

## Expected Compliance Levels

Based on current implementation phases:

| Phase | Features | Expected Compliance |
|-------|----------|---------------------|
| Phase 1 (Core) | Cardinality, NodeKind, Closed Shapes | 15-20% |
| Phase 2 (Extended) | Datatypes, ValueSet, EXTENDS, EXTRA | 40-50% |
| Phase 3 (Advanced) | Facets, OR, NOT | 70-80% |
| Phase 4 (Complete) | All features | 85-95% |

Current implementation is Phase 2 → **Target: 40-50% compliance**

## Test Types

### Validation Tests

Test RDF data validation against ShEx schemas:
- Positive validation (data conforms)
- Negative validation (data does not conform)

### Negative Syntax Tests

Test that invalid ShEx syntax is rejected by the parser.

### Negative Structure Tests

Test that syntactically valid but structurally invalid schemas are rejected.

### Positive Syntax Tests

Test that valid ShEx schemas are parsed successfully.

## Adding New Tests to Skip/Xfail Lists

### When to Skip

Skip tests for **unimplemented features**:

```json
{
  "id": "feature-test-id",
  "reason": "Feature not yet implemented (Phase 4)",
  "pattern": false
}
```

### When to Mark as Expected Failure

Mark tests as xfail for **known bugs/limitations**:

```json
{
  "id": "edge-case-test-id",
  "reason": "Edge case in current implementation",
  "issue": "GitHub issue URL"
}
```

### Pattern Matching

Use regex patterns for multiple tests:

```json
{
  "id": ".*semanticAction.*",
  "reason": "All semantic action tests",
  "pattern": true
}
```

## Regression Detection

Track compliance over time:

```bash
# Save current compliance baseline
cp reports/compliance-report.json reports/baseline.json

# After making changes, compare
./scripts/compare-compliance.sh reports/baseline.json reports/compliance-report.json
```

## Integration with CI/CD

Add to `.github/workflows/`:

```yaml
- name: Run W3C ShEx Compliance Tests
  run: |
    cd build
    ctest -R W3CShExTestSuite --output-on-failure

- name: Upload Compliance Report
  uses: actions/upload-artifact@v3
  with:
    name: shex-compliance-report
    path: test/parser/shex/reports/
```

## Troubleshooting

### Test Data Not Found

```
Warning: W3C test data not found
```

**Solution**: Clone the test repository:

```bash
cd test/parser/shex
git clone https://github.com/shexSpec/shexTest.git test-data
```

### No Test Cases Loaded

```
Warning: No test cases found in manifest
```

**Solution**: Check manifest file exists:

```bash
ls test/parser/shex/test-data/manifest.json
```

If using a different manifest structure, update the path in `W3CShExTestSuite.cpp`.

### Test Execution Errors

Check individual test output:

```bash
ctest -R W3CCompliance.test_id --verbose
```

## Development Workflow

### Implementing a New Feature

1. **Update feature-mapping.json** - Mark feature as "implemented"
2. **Update skip-list.json** - Remove skipped tests for this feature
3. **Run tests** - Execute W3C tests for the feature
4. **Fix failures** - Implement missing functionality
5. **Update xfail-list.json** - Mark any expected failures with reasons
6. **Generate report** - Run compliance summary test
7. **Commit** - Include compliance changes in PR

### Example: Implementing String Facets

```bash
# 1. Update feature-mapping.json
vim test/parser/shex/feature-mapping.json
# Set "stringFacet": { "implemented": true }

# 2. Update skip-list.json
vim test/parser/shex/skip-list.json
# Remove stringFacet pattern

# 3. Run string facet tests
ctest -R ".*stringFacet.*" --output-on-failure

# 4. Fix implementation
# (implement string facet validation in ShEx.cpp)

# 5. Re-run tests and check compliance
ctest -R W3CShExTestSuite.GenerateComplianceSummary

# 6. Commit changes
git add test/parser/shex/*.json src/parser/ShEx.*
git commit -m "feat: Implement string facets for ShEx validation"
```

## References

- **W3C ShEx Spec**: https://shex.io/shex-semantics/
- **W3C Test Repository**: https://github.com/shexSpec/shexTest
- **ShEx Primer**: https://shex.io/shex-primer/
- **QLever ShEx Implementation**: `/home/user/qlever/src/parser/ShEx.h`

## Contact

For questions about W3C test integration, refer to:
- Project documentation: `CLAUDE.md`
- ShEx implementation guide: `src/parser/ShEx_README.md`
- GitHub issues: https://github.com/seanchatmangpt/qlever/issues
