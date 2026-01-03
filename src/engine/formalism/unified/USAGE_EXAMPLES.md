# Unified Result Schema - Usage Examples

**Author**: Agent 5 - EPIC 14.0 Formalism Convergence
**Date**: 2026-01-03

---

## Table of Contents

1. [Basic Usage](#basic-usage)
2. [SHACL Integration](#shacl-integration)
3. [N3 Integration](#n3-integration)
4. [Datalog Integration](#datalog-integration)
5. [Cross-Formalism Validation](#cross-formalism-validation)
6. [Output Formatting](#output-formatting)
7. [Advanced Queries](#advanced-queries)

---

## Basic Usage

### Creating a Unified Violation

```cpp
#include "engine/formalism/unified/UnifiedResult.h"

using namespace formalism;

// Create a SHACL-style validation violation
UnifiedViolation violation(FormalismType::SHACL, "ex:Alice");
violation.withResultPath("ex:age")
         .withSourceShape("ex:PersonShape")
         .withSourceConstraintComponent("sh:datatype")
         .withValue("\"twenty\"")
         .withExpectedValue("xsd:integer")
         .withMessage("Value 'twenty' is not of datatype xsd:integer")
         .withSeverity(SeverityLevel::Violation);

// Print summary
std::cout << violation.getSummary() << std::endl;
// Output: [Violation] ex:Alice / ex:age

// Print detailed description
std::cout << violation.getDetailedDescription();
```

### Creating a Validation Report

```cpp
UnifiedValidationReport report(FormalismType::SHACL, "data.ttl");

// Add violations
report.addResult(violation1);
report.addResult(std::move(violation2));

// Check conformance
if (!report.conforms) {
  std::cout << "Validation failed with " << report.violationCount
            << " violations" << std::endl;
}

// Get summary
std::cout << report.getSummary();
```

---

## SHACL Integration

### Convert Existing SHACL Violations

```cpp
#include "engine/shacl/ShaclViolation.h"
#include "engine/formalism/unified/UnifiedResult.h"

// Existing SHACL code
shacl::ShaclValidator validator;
shacl::DetailedValidationReport shaclReport = validator.validate(shapes, data);

// Convert to unified format
formalism::UnifiedValidationReport unifiedReport =
    formalism::fromShaclReport(shaclReport);

// Output in multiple formats
std::cout << formalism::ViolationFormatter::format(
    unifiedReport,
    formalism::OutputFormat::JSON
);
```

### Single Violation Conversion

```cpp
shacl::ShaclViolation shaclViolation;
shaclViolation.focusNode = "ex:Alice";
shaclViolation.resultPath = "ex:name";
shaclViolation.sourceConstraintComponent = "sh:minLength";
shaclViolation.severity = shacl::SeverityLevel::Warning;
shaclViolation.message = "Name is too short";

// Convert
formalism::UnifiedViolation unified =
    formalism::fromShaclViolation(shaclViolation);

// Access unified properties
assert(unified.formalism == formalism::FormalismType::SHACL);
assert(unified.resultType == formalism::ResultType::ValidationViolation);
assert(unified.focusNode == "ex:Alice");
```

---

## N3 Integration

### Convert N3 Compliance Issues

```cpp
#include "util/N3ComplianceVerifier.h"
#include "engine/formalism/unified/UnifiedResult.h"

// Existing N3 compliance check
ad_utility::N3ComplianceVerifier verifier;
verifier.analyzeFile("example.n3");

// Convert to unified format
formalism::UnifiedValidationReport unifiedReport =
    formalism::fromN3ComplianceVerifier(verifier);

// Output as Markdown (like existing N3 reports)
std::string markdownReport = formalism::ViolationFormatter::format(
    unifiedReport,
    formalism::OutputFormat::Markdown
);

// Also available: JSON, RDF, Text
std::string jsonReport = unifiedReport.toJSON(true);
```

### Single N3 Issue Conversion

```cpp
ad_utility::N3ComplianceIssue issue;
issue.feature = "formulae";
issue.description = "N3 formulae detected (not supported)";
issue.lineNumber = 42;
issue.lineContent = "@forAll :x :y { :x :likes :y }";
issue.isSupported = false;

// Convert
formalism::UnifiedViolation unified =
    formalism::fromN3ComplianceIssue(issue, "example.n3");

// Check location info
assert(unified.location.has_value());
assert(unified.location->lineNumber == 42);
assert(unified.location->filename == "example.n3");

// Check severity (unsupported = violation)
assert(unified.severity == formalism::SeverityLevel::Violation);
```

---

## Datalog Integration

### Convert Datalog Derivations

```cpp
#include "parser/DatalogRule.h"
#include "engine/formalism/unified/UnifiedResult.h"

// Existing Datalog rule
DatalogRule rule;
rule.headPredicate = "ancestor";
rule.headVariables = {Variable("?x"), Variable("?y")};
// ... (rule body setup)

// Derived tuples from fixpoint computation
std::vector<std::vector<std::string>> derivedTuples = {
    {"Alice", "Bob"},
    {"Alice", "Charlie"},
    {"Bob", "Charlie"}
};

std::vector<std::string> varNames = {"?x", "?y"};

// Create unified report for derivations
formalism::UnifiedValidationReport report =
    formalism::createDatalogDerivationReport(
        "ancestor",
        derivedTuples,
        varNames
    );

// All results are Info-level (derivations, not violations)
assert(report.conforms == true);
assert(report.infoCount == 3);
assert(report.violationCount == 0);

// Output as SPARQL result set
for (const auto& result : report.results) {
  std::cout << result.toSPARQLBinding() << std::endl;
}
```

### Single Tuple Conversion

```cpp
DatalogRule rule;
rule.headPredicate = "ancestor";
rule.headVariables = {Variable("?x"), Variable("?y")};

std::vector<std::string> tuple = {"Alice", "Charlie"};
std::vector<std::string> varNames = {"?x", "?y"};

formalism::UnifiedViolation unified =
    formalism::fromDatalogTuple(rule, tuple, varNames);

// Check properties
assert(unified.formalism == formalism::FormalismType::Datalog);
assert(unified.resultType == formalism::ResultType::RuleDerivation);
assert(unified.focusNode == "ancestor");
assert(unified.values == tuple);
assert(unified.severity == formalism::SeverityLevel::Info);
```

---

## Cross-Formalism Validation

### Validate with Multiple Formalisms

```cpp
#include "engine/formalism/unified/UnifiedResult.h"

// Validate same data with SHACL and ShEx (when ShEx is implemented)
formalism::UnifiedValidationReport shaclReport =
    validateWithSHACL(data, shapes);
formalism::UnifiedValidationReport shexReport =
    validateWithShEx(data, schemas);

// Merge reports
formalism::UnifiedValidationReport mergedReport(
    formalism::FormalismType::SHACL, // Primary formalism
    "combined_validation"
);

for (const auto& result : shaclReport.results) {
  mergedReport.addResult(result);
}
for (const auto& result : shexReport.results) {
  mergedReport.addResult(result);
}

// Compare violations across formalisms
std::cout << "SHACL violations: " << shaclReport.violationCount << std::endl;
std::cout << "ShEx violations: " << shexReport.violationCount << std::endl;
std::cout << "Total: " << mergedReport.violationCount << std::endl;
```

### Analyze Cross-Formalism Consistency

```cpp
// Check if same focus node fails in multiple formalisms
std::unordered_map<std::string, std::set<formalism::FormalismType>> failuresByNode;

for (const auto& result : mergedReport.results) {
  if (result.isError()) {
    failuresByNode[result.focusNode].insert(result.formalism);
  }
}

// Report nodes that fail in multiple formalisms
for (const auto& [node, formalisms] : failuresByNode) {
  if (formalisms.size() > 1) {
    std::cout << "Node " << node << " fails in " << formalisms.size()
              << " formalisms" << std::endl;
  }
}
```

---

## Output Formatting

### Text Format (Human-Readable)

```cpp
std::string textOutput = formalism::ViolationFormatter::format(
    report,
    formalism::OutputFormat::Text
);

std::cout << textOutput;
```

**Output**:
```
=== Validation Report ===
Source: data.ttl
Formalism: SHACL
Conforms: No
Violations: 2 | Warnings: 1 | Info: 0
========================

[Violation] ex:Alice / ex:age
  Constraint: sh:datatype
  Shape: ex:PersonShape
  Values: "twenty"
  Expected: xsd:integer
  Message: Value 'twenty' is not of datatype xsd:integer

[Warning] ex:Bob / ex:name
  Constraint: sh:minLength
  Shape: ex:PersonShape
  Values: "B"
  Expected: minLength 2
  Message: Name is too short
```

### JSON Format (API-Friendly)

```cpp
std::string jsonOutput = formalism::ViolationFormatter::format(
    report,
    formalism::OutputFormat::JSON
);

// Or with pretty-printing
std::string prettyJSON = report.toJSON(true);
```

**Output**:
```json
{
  "conforms": false,
  "formalism": "SHACL",
  "source": "data.ttl",
  "violationCount": 2,
  "warningCount": 1,
  "infoCount": 0,
  "results": [
    {
      "formalism": "SHACL",
      "resultType": "ValidationViolation",
      "severity": "Violation",
      "focusNode": "ex:Alice",
      "resultPath": "ex:age",
      "sourceShape": "ex:PersonShape",
      "sourceConstraintComponent": "sh:datatype",
      "values": ["twenty"],
      "expectedValue": "xsd:integer",
      "message": "Value 'twenty' is not of datatype xsd:integer"
    }
  ]
}
```

### RDF Format (W3C SHACL Compliance)

```cpp
std::string rdfOutput = formalism::ViolationFormatter::format(
    report,
    formalism::OutputFormat::RDF
);
```

**Output**:
```turtle
@prefix sh: <http://www.w3.org/ns/shacl#> .
@prefix qlever: <http://qlever.cs.uni-freiburg.de/formalism#> .
@prefix ex: <http://example.org/> .

_:report a sh:ValidationReport ;
  sh:conforms false ;
  qlever:formalism "SHACL" ;
  qlever:source "data.ttl" ;
  sh:result _:result0 .

_:result0 a sh:ValidationResult ;
  sh:focusNode ex:Alice ;
  sh:resultPath ex:age ;
  sh:sourceShape ex:PersonShape ;
  sh:sourceConstraintComponent sh:DatatypeConstraintComponent ;
  sh:value "twenty" ;
  sh:resultSeverity sh:Violation ;
  sh:resultMessage "Value 'twenty' is not of datatype xsd:integer" ;
  qlever:formalism "SHACL" ;
  qlever:resultType "ValidationViolation" .
```

### Markdown Format (Documentation)

```cpp
std::string markdownOutput = formalism::ViolationFormatter::format(
    report,
    formalism::OutputFormat::Markdown
);
```

**Output**:
```markdown
# Validation Report: data.ttl

**Formalism**: SHACL
**Conforms**: ❌ No
**Violations**: 2 | **Warnings**: 1 | **Info**: 0

---

## Violations

### [Violation] ex:Alice / ex:age

- **Constraint**: sh:datatype
- **Shape**: ex:PersonShape
- **Values**: `twenty`
- **Expected**: xsd:integer
- **Message**: Value 'twenty' is not of datatype xsd:integer

---

## Summary

- **Total Issues**: 3
- **Unique Focus Nodes**: 2
```

---

## Advanced Queries

### Query by Focus Node

```cpp
// Get all results for a specific focus node
const auto& aliceResults = report.getResultsForFocusNode("ex:Alice");

std::cout << "ex:Alice has " << aliceResults.size() << " issues:" << std::endl;
for (const auto& result : aliceResults) {
  std::cout << "  - " << result.getSummary() << std::endl;
}
```

### Query by Severity

```cpp
// Get all violations (errors only)
auto violations = report.getResultsBySeverity(
    formalism::SeverityLevel::Violation
);

std::cout << "Found " << violations.size() << " violations" << std::endl;

// Get all warnings
auto warnings = report.getResultsBySeverity(
    formalism::SeverityLevel::Warning
);
```

### Query by Result Type

```cpp
// Get all validation violations
auto validationViolations = report.getResultsByType(
    formalism::ResultType::ValidationViolation
);

// Get all compliance issues (N3)
auto complianceIssues = report.getResultsByType(
    formalism::ResultType::ComplianceIssue
);

// Get all rule derivations (Datalog)
auto derivations = report.getResultsByType(
    formalism::ResultType::RuleDerivation
);
```

### Get Statistics

```cpp
auto stats = report.getStatistics();

std::cout << "Total results: " << stats.totalResults << std::endl;
std::cout << "Violations: " << stats.violations << std::endl;
std::cout << "Warnings: " << stats.warnings << std::endl;
std::cout << "Infos: " << stats.infos << std::endl;
std::cout << "Unique focus nodes: " << stats.uniqueFocusNodes << std::endl;

// Results by constraint
std::cout << "\nResults by constraint:" << std::endl;
for (const auto& [constraint, count] : stats.resultsByConstraint) {
  std::cout << "  " << constraint << ": " << count << std::endl;
}

// Results by shape
std::cout << "\nResults by shape:" << std::endl;
for (const auto& [shape, count] : stats.resultsByShape) {
  std::cout << "  " << shape << ": " << count << std::endl;
}
```

### Grouped Formatting

```cpp
// Format results grouped by focus node
std::string byNode = formalism::ViolationFormatter::formatByFocusNode(report);

// Format results grouped by severity
std::string bySeverity = formalism::ViolationFormatter::formatBySeverity(report);

// Format results grouped by constraint type
std::string byConstraint = formalism::ViolationFormatter::formatByConstraint(report);

std::cout << byNode << std::endl;
std::cout << bySeverity << std::endl;
std::cout << byConstraint << std::endl;
```

---

## Integration Patterns

### Stream Output

```cpp
#include <fstream>

// Write to file
std::ofstream outFile("validation_report.json");
formalism::ViolationFormatter::format(
    report,
    outFile,
    formalism::OutputFormat::JSON
);
outFile.close();

// Write to cout
formalism::ViolationFormatter::format(
    report,
    std::cout,
    formalism::OutputFormat::Text
);
```

### API Response

```cpp
// REST API endpoint
std::string getValidationReport(const std::string& dataFile) {
  // Run validation
  auto report = validateData(dataFile);

  // Return JSON for API
  return formalism::ViolationFormatter::format(
      report,
      formalism::OutputFormat::JSON
  );
}
```

### Logging

```cpp
#include <glog/logging.h>

// Log violations
for (const auto& result : report.results) {
  if (result.isError()) {
    LOG(ERROR) << result.getSummary();
  } else if (result.isWarning()) {
    LOG(WARNING) << result.getSummary();
  } else {
    LOG(INFO) << result.getSummary();
  }
}
```

---

## Performance Tips

### Batch Processing

```cpp
// Create report once, add results in batch
formalism::UnifiedValidationReport report(
    formalism::FormalismType::SHACL,
    "large_dataset.ttl"
);

// Reserve space if you know the size
report.results.reserve(10000);

// Add results
for (const auto& violation : violations) {
  report.addResult(std::move(violation)); // Use move for large objects
}
```

### Lazy Serialization

```cpp
// Don't serialize until needed
formalism::UnifiedValidationReport report = runValidation();

// Only serialize if there are violations
if (!report.conforms) {
  std::string output = report.toJSON(true);
  writeToFile(output);
}
```

### Selective Output

```cpp
// Only output violations, skip warnings/info
formalism::UnifiedValidationReport filteredReport(
    report.formalism,
    report.source
);

for (const auto& result : report.results) {
  if (result.isError()) {
    filteredReport.addResult(result);
  }
}

std::cout << filteredReport.toJSON(true);
```

---

## Testing Examples

### Unit Test: Conversion

```cpp
TEST(UnifiedResultTest, ConvertShaclViolation) {
  shacl::ShaclViolation shaclViolation;
  shaclViolation.focusNode = "ex:Alice";
  shaclViolation.resultPath = "ex:age";
  shaclViolation.sourceConstraintComponent = "sh:datatype";
  shaclViolation.severity = shacl::SeverityLevel::Violation;

  formalism::UnifiedViolation unified =
      formalism::fromShaclViolation(shaclViolation);

  EXPECT_EQ(unified.formalism, formalism::FormalismType::SHACL);
  EXPECT_EQ(unified.focusNode, "ex:Alice");
  EXPECT_EQ(*unified.resultPath, "ex:age");
  EXPECT_EQ(unified.severity, formalism::SeverityLevel::Violation);
}
```

### Unit Test: Serialization

```cpp
TEST(UnifiedResultTest, JSONSerialization) {
  formalism::UnifiedViolation violation(
      formalism::FormalismType::SHACL,
      "ex:Alice"
  );
  violation.withResultPath("ex:age")
           .withMessage("Test violation");

  std::string json = violation.toJSON(false);

  EXPECT_TRUE(json.find("\"focusNode\": \"ex:Alice\"") != std::string::npos);
  EXPECT_TRUE(json.find("\"resultPath\": \"ex:age\"") != std::string::npos);
}
```

---

## Best Practices

1. **Use Builder Pattern**: Chain `.with*()` methods for clean violation construction
2. **Move Semantics**: Use `std::move()` when adding results to reports
3. **Selective Conversion**: Only convert to unified format when cross-formalism compatibility is needed
4. **Format Selection**: Choose format based on use case (JSON for APIs, Markdown for docs, RDF for compliance)
5. **Batch Operations**: Reserve space in reports if you know the result count
6. **Lazy Evaluation**: Don't serialize until output is actually needed

---

## See Also

- [Unified Result Design](UNIFIED_RESULT_DESIGN.md) - Full design specification
- [UnifiedResult.h](UnifiedResult.h) - Header documentation
- [EPIC 14.0 Formalism Delta Matrix](../../../audit/FORMALISM_DELTA_MATRIX.md)
- [W3C SHACL Specification](https://www.w3.org/TR/shacl/)
