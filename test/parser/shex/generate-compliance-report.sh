#!/bin/bash
# Generate ShEx W3C Compliance Report
# Usage: ./generate-compliance-report.sh [output-dir]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${1:-$SCRIPT_DIR/reports}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/../../../build}"

echo "=== ShEx W3C Compliance Report Generator ==="
echo "Output directory: $OUTPUT_DIR"
echo "Build directory: $BUILD_DIR"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if W3C test data exists
if [ ! -d "$SCRIPT_DIR/test-data" ]; then
    echo "ERROR: W3C test data not found at $SCRIPT_DIR/test-data"
    echo ""
    echo "To download W3C test data, run:"
    echo "  cd $SCRIPT_DIR"
    echo "  git clone https://github.com/shexSpec/shexTest.git test-data"
    echo ""
    exit 1
fi

echo "W3C test data found: $SCRIPT_DIR/test-data"
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory not found at $BUILD_DIR"
    echo ""
    echo "To build the project, run:"
    echo "  mkdir -p build && cd build"
    echo "  cmake -DCMAKE_BUILD_TYPE=Release -GNinja .."
    echo "  cmake --build ."
    echo ""
    exit 1
fi

echo "Build directory found: $BUILD_DIR"
echo ""

# Build the test executable
echo "Building W3C test suite..."
cd "$BUILD_DIR"
cmake --build . --target W3CShExTestSuite
echo "Build complete."
echo ""

# Run the compliance summary test
echo "Running compliance tests..."
cd "$BUILD_DIR"
ctest -R W3CShExTestSuite.GenerateComplianceSummary --output-on-failure || true
echo ""

# Copy reports to output directory
REPORT_SOURCE="$SCRIPT_DIR/reports"
if [ -d "$REPORT_SOURCE" ]; then
    echo "Copying reports to $OUTPUT_DIR..."
    cp -v "$REPORT_SOURCE"/*.json "$OUTPUT_DIR/" 2>/dev/null || true
    cp -v "$REPORT_SOURCE"/*.html "$OUTPUT_DIR/" 2>/dev/null || true
    cp -v "$REPORT_SOURCE"/*.csv "$OUTPUT_DIR/" 2>/dev/null || true
    echo ""
fi

# Display summary
if [ -f "$OUTPUT_DIR/compliance-report.json" ]; then
    echo "=== Compliance Summary ==="
    echo ""

    # Extract key metrics using jq if available
    if command -v jq &> /dev/null; then
        TOTAL=$(jq -r '.summary.total_tests' "$OUTPUT_DIR/compliance-report.json")
        PASSED=$(jq -r '.summary.passed' "$OUTPUT_DIR/compliance-report.json")
        FAILED=$(jq -r '.summary.failed' "$OUTPUT_DIR/compliance-report.json")
        SKIPPED=$(jq -r '.summary.skipped' "$OUTPUT_DIR/compliance-report.json")
        COMPLIANCE=$(jq -r '.summary.compliance_percentage' "$OUTPUT_DIR/compliance-report.json")

        echo "Total Tests: $TOTAL"
        echo "Passed: $PASSED"
        echo "Failed: $FAILED"
        echo "Skipped: $SKIPPED"
        echo "Compliance: ${COMPLIANCE}%"
        echo ""

        echo "Feature Coverage:"
        jq -r '.feature_coverage[] | "  \(.name): \(.passed_tests)/\(.total_tests) (\(.coverage_percentage | tonumber | floor)%)"' \
            "$OUTPUT_DIR/compliance-report.json" | head -10
        echo ""
    else
        echo "Install 'jq' for detailed summary display"
        echo "Report available at: $OUTPUT_DIR/compliance-report.json"
        echo ""
    fi
fi

# Display report locations
echo "=== Generated Reports ==="
echo "JSON Report: $OUTPUT_DIR/compliance-report.json"
echo "HTML Report: $OUTPUT_DIR/compliance-report.html"
echo "CSV Report: $OUTPUT_DIR/feature-coverage.csv"
echo ""

# Offer to open HTML report
if [ -f "$OUTPUT_DIR/compliance-report.html" ]; then
    echo "To view the HTML report, run:"
    echo "  xdg-open $OUTPUT_DIR/compliance-report.html"
    echo ""
fi

echo "=== Complete ==="
