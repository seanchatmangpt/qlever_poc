#!/bin/bash
# Run QLever tests locally

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build directory not found at $BUILD_DIR"
    echo "First run: ./scripts/build-release.sh"
    exit 1
fi

cd "$BUILD_DIR"

echo "Running tests..."
echo ""

# Parse arguments
if [ $# -eq 0 ]; then
    # No arguments: run all tests
    echo "Running all tests (use -h for options)"
    ctest --output-on-failure -j$(nproc)
elif [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [TEST_PATTERN]"
    echo ""
    echo "Examples:"
    echo "  $0                    # Run all tests"
    echo "  $0 JoinTest           # Run tests matching 'JoinTest'"
    echo "  $0 'Join.*'           # Run tests matching regex 'Join.*'"
    echo "  $0 --verbose          # Run all tests with verbose output"
    echo ""
    echo "Common test patterns:"
    echo "  - Engine tests: EngineTest, OperationTest, JoinTest, etc."
    echo "  - Index tests: IndexTest, VocabularyTest"
    echo "  - Parser tests: ParserTest"
    echo ""
    echo "View available tests: ctest --verbose (shows all tests)"
    exit 0
else
    # Run specific test pattern
    echo "Running tests matching: $*"
    ctest -R "$*" --output-on-failure -j$(nproc)
fi

echo ""
echo "✓ Test run complete!"
