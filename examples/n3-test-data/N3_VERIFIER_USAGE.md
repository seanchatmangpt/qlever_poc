# N3 Compliance Verifier Usage Guide

The N3 Compliance Verifier is a utility tool that analyzes N3/Turtle files to determine their compatibility with QLever's N3 parser.

## Quick Start

### Building

The verifier is built automatically with QLever:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build .
```

The executable will be created at: `build/N3VerifierMain`

### Basic Usage

```bash
# Verify a file and display report on console
./N3VerifierMain data.n3

# Save report to markdown file
./N3VerifierMain data.n3 --report report.md

# Use strict mode (exit with error if incompatible)
./N3VerifierMain data.n3 --strict
```

## What It Checks

### Supported Features (Compatible with QLever)

The verifier detects and confirms these standard Turtle/N3 features:

- **Prefixes**: `@prefix` and `PREFIX` declarations
- **Base URI**: `@base` and `BASE` declarations
- **IRIs**: Full `<http://...>` and prefixed `ex:name` forms
- **Literals**: Plain, typed (`"value"^^type`), and language-tagged (`"text"@en`)
- **Blank nodes**: `[]` and `_:name` syntax
- **Collections**: `( item1 item2 ...)` syntax
- **Numeric literals**: Integer and decimal values
- **Boolean literals**: `true` and `false`
- **Comments**: `#` comment lines

### Unsupported Features (Will Cause Parsing Errors)

The verifier detects these advanced N3 features that QLever doesn't support:

- **Formulae/Quoted Graphs**: `{ triple triple }` syntax
- **Implication Rules**: `=>` (implies) and `<=` (is implied by)
- **Quantifiers**: `@forAll` and `@forSome`
- **Variables**: `?var` syntax (N3 style, different from SPARQL)
- **Built-in Functions**: `log:`, `math:`, `string:`, etc.
- **N3 Paths**: `!` and `^` path operators

## Example Reports

### Compatible File

```
# N3 Format Compliance Report

**File:** `basic.n3`

**Total Lines:** 37

**Compatibility Status:** COMPATIBLE

This file uses only supported Turtle/N3 features and can be loaded by QLever.

## Feature Matrix

| Feature | Used in File | Supported by QLever |
|---------|--------------|---------------------|
| @prefix / PREFIX | Yes | Yes |
| @base / BASE | Yes | Yes |
| Typed Literals | Yes | Yes |
| Language Tags | Yes | Yes |
| Blank Nodes | Yes | Yes |
```

### Incompatible File

```
# N3 Format Compliance Report

**File:** `rules.n3`

**Compatibility Status:** INCOMPATIBLE

This file uses 3 unsupported N3 feature(s) that will cause parsing errors.

## Detected Issues

### Line 15: Implication Rules

**Description:** N3 implications (=>, <=) are not supported

**Line content:**
```
{ :x :parent :y } => { :y :child :x } .
```

## Recommendations

To make this file compatible with QLever:

- **Implications:** Materialize rule results or use external reasoner
- Use a full N3 reasoner (like EYE or cwm) to process this file
  and convert the results to Turtle format before loading into QLever.
```

## Integration with IndexBuilderMain

The verifier can be used as a pre-flight check before indexing:

```bash
# Check file first
./N3VerifierMain data.n3 --strict

# If successful (exit code 0), proceed with indexing
if [ $? -eq 0 ]; then
  ./IndexBuilderMain -F n3 -f data.n3 -i myindex
fi
```

## Programmatic Usage

You can also use the verifier from C++ code:

```cpp
#include "util/N3ComplianceVerifier.h"

ad_utility::N3ComplianceVerifier verifier;
verifier.analyzeFile("data.n3");

if (verifier.isCompatible()) {
  std::cout << "File is compatible!\n";
  // Proceed with loading
} else {
  std::cerr << "Found " << verifier.getUnsupportedFeatureCount()
            << " incompatibilities\n";
  std::cout << verifier.generateReport();
}

// Get feature matrix
auto features = verifier.featureMatrix();
for (const auto& [feature, used] : features) {
  std::cout << feature << ": " << (used ? "used" : "not used") << "\n";
}
```

## Test Files

This directory contains test files you can use to verify the tool:

- `basic.n3`: Simple compatible file with common features
- `people-dataset.n3`: Larger compatible dataset (100 people)
- `people-dataset.ttl`: Same data in Turtle format

All these files should verify as COMPATIBLE.

## Performance

The verifier is designed for fast analysis:

- Typical files: < 1 second
- Large files (100K+ lines): < 5 seconds
- Minimal memory footprint

## Exit Codes

- `0`: File is compatible (or help displayed)
- `1`: Error (file not found, invalid arguments)
- `2`: File is incompatible (only in `--strict` mode)

## See Also

- QLever Documentation: https://github.com/ad-freiburg/qlever
- N3 Specification: https://www.w3.org/TeamSubmission/n3/
- Turtle Specification: https://www.w3.org/TR/turtle/
