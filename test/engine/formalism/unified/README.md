# Unified Formalism Testing Framework

**Agent 9 - EPIC 14.0 Formalism Convergence**

## Quick Start

This testing framework addresses critical gaps identified in the formalism convergence audit:

🔴 **Determinism Testing Gap** (HIGH SEVERITY)
🔴 **Coverage Gap in N3/Datalog** (HIGH SEVERITY)
🟡 **Negative Test Coverage Imbalance** (MEDIUM)
🆕 **Cross-Formalism Equivalence** (NEW)

## Usage Examples

### 1. Running Golden Tests

```cpp
#include "test/engine/formalism/unified/UnifiedFormalismTestFramework.h"

TEST(UnifiedGoldenTests, SHACL_W3C_MinCount) {
  TestCorpusLoader loader("/path/to/corpus");
  auto goldenTests = loader.loadGoldenTests(FormalismType::SHACL);

  UnifiedTestExecutor executor(ctx);

  for (const auto& testCase : goldenTests) {
    auto result = executor.executeGoldenTest(testCase);

    EXPECT_EQ(result.conforms, testCase.expectedConforms)
        << "Test " << testCase.testId << " failed";
  }
}
```

### 2. Running Determinism Tests

```cpp
#include "test/engine/formalism/unified/DeterminismTestSuite.cpp"

// Execute determinism test suite
// Tests run automatically with GTest
```

### 3. Running Negative Tests

```cpp
TEST(UnifiedNegativeTests, SHACL_InvalidSyntax) {
  TestCorpusLoader loader("/path/to/corpus");
  auto negativeTests = loader.loadNegativeTests(FormalismType::SHACL);

  UnifiedTestExecutor executor(ctx);

  for (const auto& testCase : negativeTests) {
    bool caught = executor.executeNegativeTest(testCase);

    EXPECT_TRUE(caught)
        << "Expected error not caught for test " << testCase.testId;
  }
}
```

### 4. Running Equivalence Tests

```cpp
TEST(UnifiedEquivalenceTests, SHACL_vs_Datalog_Transitive) {
  TestCorpusLoader loader("/path/to/corpus");
  auto equivTests = loader.loadEquivalenceTests();

  UnifiedTestExecutor executor(ctx);

  for (const auto& testCase : equivTests) {
    auto result = executor.executeEquivalenceTest(testCase);

    EXPECT_TRUE(result.areEquivalent)
        << "Formalisms not equivalent for test " << testCase.testId;
  }
}
```

## Test Corpus Structure

```
corpus/
├── golden/                    # Reference-standard tests
│   ├── shacl_w3c_core.json   # 8 W3C SHACL tests
│   ├── datalog_basic.json    # 5 Datalog tests
│   ├── n3_basic.json         # 5 N3 Turtle tests
│   └── shex_basic.json       # Placeholder
│
├── negative/                  # Invalid input tests
│   ├── shacl_invalid.json    # 6 SHACL error cases
│   ├── datalog_invalid.json  # 7 Datalog error cases
│   ├── n3_invalid.json       # 7 N3 error cases
│   └── shex_invalid.json     # Placeholder
│
└── equivalence/               # Cross-formalism tests
    └── cross_formalism.json  # 5 equivalence tests
```

## Test Coverage

| Formalism | Golden Tests | Determinism Tests | Negative Tests | Equivalence Tests |
|-----------|-------------|-------------------|----------------|-------------------|
| **SHACL** | 8 (W3C) | 5 rule + 2 query | 6 error cases | 5 cross-formalism |
| **Datalog** | 5 (custom) | 3 rule + 1 query | 7 error cases | 5 cross-formalism |
| **N3** | 5 (Turtle) | 1 parse | 7 error cases | N/A |
| **ShEx** | 0 (stub) | 0 | 0 | 0 |

## Running Tests

### Build and Run All Tests

```bash
cd /home/user/qlever
make build
make test
```

### Run Specific Test Suite

```bash
cd build
./test/UnifiedFormalismTest
```

### Run Determinism Tests Only

```bash
cd build
./test/UnifiedFormalismTest --gtest_filter="DeterminismTestSuite.*"
```

### Run Negative Tests Only

```bash
cd build
./test/UnifiedFormalismTest --gtest_filter="NegativeTestSuite.*"
```

## Adding New Tests

### Add Golden Test

1. Open `corpus/golden/{formalism}_*.json`
2. Add new test case:
```json
{
  "testId": "my-new-test-001",
  "description": "What this test validates",
  "inputFormat": "turtle",
  "input": "...",
  "expectedConforms": true,
  "expectedViolations": [],
  "source": "W3C",
  "sourceUrl": "https://...",
  "tags": ["category", "feature"]
}
```
3. Run corpus validation:
```cpp
TestCorpusLoader loader("/path/to/corpus");
EXPECT_TRUE(loader.validateCorpusStructure());
```

### Add Negative Test

1. Open `corpus/negative/{formalism}_invalid.json`
2. Add new error case:
```json
{
  "testId": "invalid-syntax-new-001",
  "description": "What is invalid",
  "invalidInput": "...",
  "expectedErrorType": "ParseException",
  "expectedErrorPattern": "Expected.*semicolon",
  "tags": ["syntax", "parse-error"]
}
```

### Add Equivalence Test

1. Open `corpus/equivalence/cross_formalism.json`
2. Add new equivalence case:
```json
{
  "testId": "equiv-new-001",
  "description": "What equivalence is tested",
  "equivalenceType": "semantic",
  "inputs": [
    {
      "formalism": "SHACL",
      "input": "..."
    },
    {
      "formalism": "Datalog",
      "input": "..."
    }
  ],
  "dataGraph": "...",
  "tags": ["category"]
}
```

## Test Result Analysis

### Determinism Report

```cpp
DeterminismTester tester;
auto result = tester.testRuleDeterminism(
    FormalismType::SHACL,
    shaclInput,
    dataGraph
);

if (!result.isDeterministic) {
  std::cout << "Non-determinism detected!" << std::endl;
  std::cout << "Cause: " << result.nonDeterminismCause.value() << std::endl;
  std::cout << "Fingerprint sequence: " << std::endl;
  for (const auto& fp : result.fingerprintSequence) {
    std::cout << "  " << fp << std::endl;
  }
}
```

### Equivalence Report

```cpp
EquivalenceTester tester;
auto result = tester.testSemanticEquivalence(testCase);

if (!result.areEquivalent) {
  std::cout << "Equivalence failed!" << std::endl;
  std::cout << "Formalism 1: " << toString(result.formalism1) << std::endl;
  std::cout << "Formalism 2: " << toString(result.formalism2) << std::endl;
  std::cout << "Divergence: " << result.divergenceDescription.value() << std::endl;
}
```

## Design Documentation

See [DESIGN.md](./DESIGN.md) for:
- Architecture overview
- Component design
- API documentation
- Implementation roadmap
- Success criteria

## Integration with Existing Tests

This framework **complements** existing tests:

- **Existing tests** remain in `test/engine/shacl/`, `test/engine/datalog/`, etc.
- **Unified tests** live in `test/engine/formalism/unified/`
- Both test suites run in parallel
- No existing tests are modified

## CI Integration

### Test Pipeline

```yaml
# .github/workflows/test.yml
- name: Run Unified Formalism Tests
  run: |
    cd build
    ./test/UnifiedFormalismTest --gtest_output=xml:unified_test_results.xml
```

### Coverage Report

```bash
# Generate test coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

## FAQ

### Why a separate unified framework?

The existing tests are formalism-specific. The unified framework provides:
1. Cross-formalism consistency
2. Determinism validation across all formalisms
3. Explicit negative testing
4. Semantic equivalence validation

### Why JSON test corpus?

JSON allows:
1. Easy extension without code changes
2. Schema validation
3. External tool integration
4. Version control friendly

### How do I debug a failing test?

1. Check test ID in corpus JSON
2. Read `description` and `tags`
3. Examine `input` and `expectedViolations`
4. Run test with `--gtest_filter="TestName"` for isolation
5. Enable verbose output: `--gtest_verbose`

### How do I contribute tests?

1. Identify gap in test coverage
2. Add test case to appropriate JSON corpus
3. Validate corpus structure
4. Submit PR with test case

## References

- **W3C SHACL**: https://www.w3.org/TR/shacl/
- **N3 Submission**: https://www.w3.org/TeamSubmission/n3/
- **EPIC 14.0 Audit**: `audit/MURA_DELTA_SEVERITY.md`
- **Design Document**: `DESIGN.md`

---

**Status**: Framework Complete — Implementation In Progress
**Next Phase**: EPIC 14.1 Core Implementation
