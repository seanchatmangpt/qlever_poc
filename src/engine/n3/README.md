# N3 Service Module - EPIC 5 Task 4

## Overview

The N3 Service module provides standardized N3 document verification with error taxonomy, guard enforcement, and machine-readable output. This is NOT full N3 rules evaluation - it's verification of N3 syntax and basic conformance.

## Components

### N3ErrorCode.h
- **Purpose**: Standardized error taxonomy for N3 verification
- **Features**:
  - Error codes organized by category (1xx parsing, 2xx unsupported features, 3xx guard violations, 4xx unknown/strict, 9xx general)
  - Stable string representation for reproducibility
  - Human-readable descriptions
  - ErrorRecord and WarningRecord structures

### N3Config.h
- **Purpose**: Configuration with resource guards
- **Default Guards** (fail closed):
  - `max_input_size_bytes`: 100MB (prevents excessive memory consumption)
  - `max_blank_nodes`: 1,000,000 (prevents blank node explosion)
  - `max_nesting_depth`: 100 (prevents stack overflow)
  - `max_memory_bytes`: 200MB (2x input size for processing overhead)
  - `timeout_ms`: 60,000ms (60 seconds)
  - `strict_mode`: false (lenient mode by default)

### N3VerifyResult.h
- **Purpose**: JSON-serializable verification result
- **Features**:
  - Success/failure status
  - List of errors and warnings
  - SHA-256 compliance digest for reproducibility
  - Verification statistics (input size, blank nodes, nesting depth, runtime, etc.)
  - JSON serialization/deserialization

### N3Service.h/cpp
- **Purpose**: Main verification service wrapping existing N3ComplianceVerifier
- **Features**:
  - Verify from file or string content
  - Enforce all guards (input size, blank nodes, nesting depth)
  - Calculate deterministic compliance digest
  - Map existing verifier issues to standardized error codes
  - JSON-serializable results

### N3ComplianceRunner.h/cpp
- **Purpose**: Conformance test runner
- **Features**:
  - Load test cases from JSON manifest
  - Run verification on all test cases
  - Compare actual vs expected results
  - Output JSON results for regression testing
  - Deterministic execution

## Supported N3 Subset

### ✅ Supported Features (Basic N3/Turtle Syntax)
- `@prefix` / `PREFIX` declarations
- `@base` / `BASE` declarations
- Typed literals (`"value"^^<type>`)
- Language tags (`"text"@lang`)
- Blank nodes (`[]`, `_:identifier`)
- Collections/lists (`( ... )`)
- Numeric literals (integers, decimals)
- Boolean literals (`true`, `false`)
- IRIs and URIs
- Basic triple patterns

### ❌ Unsupported Features (Noted in Manifest)
- **N3 Formulae/Quoted Graphs** (`{ ... }`): Not supported - these are advanced N3 features for reification
- **N3 Implication Rules** (`=>`, `<=`): Not supported - rules/logic predicates not in EPIC 5 scope
- **N3 Quantifiers** (`@forAll`, `@forSome`): Not supported - logic features beyond basic syntax
- **N3 Variables** (`?var`): Not supported - only SPARQL variables in queries, not in data
- **N3 Built-in Functions** (`log:`, `math:`, `string:`, etc.): Not supported - requires N3 reasoner
- **N3 Path Expressions** (`!`, `:^`): Not supported - use SPARQL property paths instead
- **Complex Nesting** (depth > 100): Guarded error for safety

### ⚠️ Guard Enforcement
All guards fail closed:
- Input exceeding `max_input_size_bytes` → INPUT_SIZE_EXCEEDED error
- Blank nodes exceeding `max_blank_nodes` → BLANK_NODE_LIMIT_EXCEEDED error
- Nesting exceeding `max_nesting_depth` → NESTING_DEPTH_EXCEEDED error
- Memory exceeding `max_memory_bytes` → MEMORY_LIMIT_EXCEEDED error (if detected)
- Timeout exceeding `timeout_ms` → timeout behavior (if implemented)

## Conformance Runner Behavior

### Test Case Format (JSON Manifest)
```json
{
  "tests": [
    {
      "name": "valid_turtle",
      "input_file": "path/to/file.n3",
      "should_pass": true,
      "description": "Valid Turtle document"
    },
    {
      "name": "invalid_formulae",
      "input_file": "path/to/invalid.n3",
      "should_pass": false,
      "description": "Document with unsupported formulae"
    }
  ]
}
```

### Output Format (JSON Results)
```json
{
  "total_tests": 2,
  "passed_tests": 2,
  "failed_tests": 0,
  "total_runtime_ms": 150,
  "test_results": [
    {
      "test_name": "valid_turtle",
      "passed": true,
      "verify_result": {
        "ok": true,
        "compliance_digest": "sha256_hash_here",
        "errors": [],
        "warnings": [],
        "stats": { ... }
      },
      "error_message": ""
    }
  ]
}
```

### Determinism
- Compliance digest is SHA-256 hash of normalized document
- Normalization: remove comments, normalize whitespace, sort prefixes
- Same input always produces same digest
- Results are reproducible across runs

## Usage Examples

### Basic Verification
```cpp
#include "engine/n3/N3Service.h"

using namespace ad_engine::n3;

N3Service service;
N3VerifyResult result = service.verifyFile("data.n3");

if (result.ok) {
  std::cout << "Verification passed: " << result.compliance_digest << std::endl;
} else {
  std::cout << "Verification failed with " << result.errors.size() << " errors" << std::endl;
  for (const auto& error : result.errors) {
    std::cout << "  Line " << error.line_number << ": "
              << errorCodeToString(error.code) << " - "
              << error.message << std::endl;
  }
}
```

### Strict Mode Verification
```cpp
N3Config config = N3Config::strictConfig();
N3Service service(config);
N3VerifyResult result = service.verifyFile("data.n3");
// Fails on any unknown or uncertain feature
```

### Conformance Testing
```cpp
#include "engine/n3/N3ComplianceRunner.h"

N3ComplianceRunner runner;
N3ConformanceResult result = runner.runConformanceTests("manifest.json");

std::cout << result.summary() << std::endl;
runner.writeResults(result, "runner_output.json");
```

### JSON Serialization
```cpp
N3VerifyResult result = service.verifyFile("data.n3");
nlohmann::json j = result.toJson();
std::cout << j.dump(2) << std::endl;  // Pretty print

// Deserialize
N3VerifyResult result2 = N3VerifyResult::fromJson(j);
```

## Integration with Existing Code

### Wraps Existing N3ComplianceVerifier
The N3Service wraps the existing `ad_utility::N3ComplianceVerifier` from `src/util/N3ComplianceVerifier.h`:
- Converts `N3ComplianceIssue` to standardized `ErrorRecord`
- Maps feature names to `N3ErrorCode` enum
- Adds guard enforcement (size, blank nodes, nesting)
- Calculates compliance digest

### No Rewriting of Existing Parser
- Uses existing N3 parser/verifier as-is
- Adds standardized error taxonomy layer
- Adds guard enforcement layer
- Adds JSON serialization layer

## Testing

Tests are located in `test/engine/n3/N3ServiceTest.cpp`:
- Configuration tests (default, strict, permissive, minimal)
- Error code tests (string conversion, descriptions)
- Valid document verification
- Unsupported feature detection (formulae, implication, etc.)
- Guard enforcement (input size, blank nodes, nesting depth)
- File not found handling
- JSON serialization/deserialization
- Digest determinism
- Conformance runner basic functionality
- Result summary

## Building

The N3 module is integrated into the main QLever build:
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build .
ctest -R N3ServiceTest --output-on-failure
```

## Error Code Reference

| Code | Category | Description |
|------|----------|-------------|
| 100 | Parsing | Parse failed |
| 101 | Parsing | Invalid syntax |
| 102 | Parsing | Invalid IRI |
| 103 | Parsing | Invalid literal |
| 104 | Parsing | Invalid prefix |
| 105 | Parsing | Invalid base |
| 200 | Unsupported | Formulae not supported |
| 201 | Unsupported | Implication not supported |
| 202 | Unsupported | Quantifier not supported |
| 203 | Unsupported | Variable not supported |
| 204 | Unsupported | Built-in function not supported |
| 205 | Unsupported | N3 path not supported |
| 206 | Unsupported | Rules not supported |
| 300 | Guard | Input size exceeded |
| 301 | Guard | Blank node limit exceeded |
| 302 | Guard | Nesting depth exceeded |
| 303 | Guard | Memory limit exceeded |
| 400 | Unknown | Unknown feature |
| 401 | Strict | Strict mode violation |
| 900 | General | File not found |
| 901 | General | I/O error |
| 999 | General | Internal error |

## Future Enhancements

Potential improvements not in current scope:
- Full N3 rules evaluation (requires reasoner)
- Streaming verification for very large files
- Parallel verification of multiple files
- More detailed syntax error reporting
- Custom error handlers
- Caching of verification results
- Integration with RDF parser directly (avoid temp files)
