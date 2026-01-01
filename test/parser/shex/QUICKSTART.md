# W3C ShEx Test Suite - Quick Start Guide

## 1. Download W3C Test Data

```bash
cd /home/user/qlever/test/parser/shex
git clone https://github.com/shexSpec/shexTest.git test-data
```

## 2. Build the Project

```bash
cd /home/user/qlever
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build . --target W3CShExTestSuite
```

## 3. Run Compliance Tests

### Run All W3C Tests

```bash
cd /home/user/qlever/build
ctest -R W3CCompliance --output-on-failure
```

### Generate Compliance Report

```bash
cd /home/user/qlever/build
ctest -R GenerateComplianceSummary --output-on-failure
```

Or use the helper script:

```bash
cd /home/user/qlever/test/parser/shex
./generate-compliance-report.sh
```

## 4. View Results

### Terminal Output

The compliance summary test displays:
- Total tests, passed, failed, skipped
- Compliance percentage
- Feature-by-feature coverage

### HTML Dashboard

```bash
xdg-open /home/user/qlever/test/parser/shex/reports/compliance-report.html
```

### JSON Report (Machine-Readable)

```bash
cat /home/user/qlever/test/parser/shex/reports/compliance-report.json | jq
```

### CSV Feature Coverage

```bash
cat /home/user/qlever/test/parser/shex/reports/feature-coverage.csv
```

## 5. Run Specific Feature Tests

### Test Cardinality

```bash
ctest -R TestCardinalityFeature --output-on-failure
```

### Test Node Kind

```bash
ctest -R TestNodeKindFeature --output-on-failure
```

## 6. Update Configuration

### Skip Unimplemented Features

Edit `skip-list.json`:

```bash
vim /home/user/qlever/test/parser/shex/skip-list.json
```

### Mark Known Failures

Edit `xfail-list.json`:

```bash
vim /home/user/qlever/test/parser/shex/xfail-list.json
```

### Update Feature Mapping

Edit `feature-mapping.json`:

```bash
vim /home/user/qlever/test/parser/shex/feature-mapping.json
```

## 7. Expected Results (Phase 2)

Current implementation is **Phase 2 (Extended)**:

- **Core Features (Phase 1)**: ✓ Implemented
  - Cardinality constraints
  - Node kind constraints
  - Closed shapes

- **Extended Features (Phase 2)**: ✓ Partially Implemented
  - Datatype constraints (basic)
  - Value sets (partial)
  - EXTENDS
  - EXTRA / !EXTRA

- **Advanced Features (Phase 3)**: ✗ Not Yet Implemented
  - String/numeric facets
  - OR operator
  - NOT operator

**Expected Compliance: 40-50%** of ~3000 W3C tests

## 8. Troubleshooting

### "W3C test data not found"

Clone the test repository:

```bash
cd /home/user/qlever/test/parser/shex
git clone https://github.com/shexSpec/shexTest.git test-data
```

### "No test cases found"

Check manifest file exists:

```bash
ls /home/user/qlever/test/parser/shex/test-data/manifest.json*
```

### Build Errors

Rebuild from scratch:

```bash
cd /home/user/qlever
rm -rf build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build .
```

## 9. CI/CD Integration

Add to your workflow:

```yaml
- name: Run W3C ShEx Compliance
  run: |
    cd build
    ctest -R W3CShExTestSuite --output-on-failure

- name: Upload Compliance Report
  uses: actions/upload-artifact@v3
  with:
    name: shex-compliance
    path: test/parser/shex/reports/
```

## 10. Development Workflow

When implementing a new feature:

1. Update `feature-mapping.json` - mark as implemented
2. Update `skip-list.json` - remove skipped tests
3. Run feature-specific tests
4. Fix implementation as needed
5. Update `xfail-list.json` for known issues
6. Generate compliance report
7. Commit changes with compliance metrics

## Quick Command Reference

```bash
# Download test data
git clone https://github.com/shexSpec/shexTest.git test/parser/shex/test-data

# Build
cmake --build build --target W3CShExTestSuite

# Run all W3C tests
ctest -R W3CCompliance

# Generate report
ctest -R GenerateComplianceSummary

# View HTML report
xdg-open test/parser/shex/reports/compliance-report.html

# Run specific feature
ctest -R TestCardinalityFeature
```

## Support

- Full documentation: [README.md](README.md)
- Implementation guide: `/home/user/qlever/src/parser/ShEx_README.md`
- Project guide: `/home/user/qlever/CLAUDE.md`
