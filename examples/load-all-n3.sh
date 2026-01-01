#!/bin/bash
# load-all-n3.sh - Load all N3 examples into QLever
# =====================================================
#
# This script builds QLever indexes from all N3 example files.
# It creates separate indexes for each category and a combined index.
#
# Usage:
#   ./load-all-n3.sh [options]
#
# Options:
#   --combined-only   Build only the combined index
#   --tutorial-only   Build only tutorial examples
#   --realworld-only  Build only real-world examples
#   --advanced-only   Build only advanced examples
#   --help            Show this help message

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"
INDEXES_DIR="${SCRIPT_DIR}/indexes"

# Check if IndexBuilderMain exists
if [ ! -f "${BUILD_DIR}/IndexBuilderMain" ]; then
    echo -e "${RED}Error: IndexBuilderMain not found in ${BUILD_DIR}${NC}"
    echo "Please build QLever first:"
    echo "  cd $(dirname ${SCRIPT_DIR})"
    echo "  mkdir -p build && cd build"
    echo "  cmake -DCMAKE_BUILD_TYPE=Release .."
    echo "  make IndexBuilderMain"
    exit 1
fi

# Create indexes directory
mkdir -p "${INDEXES_DIR}"

# Function to print colored output
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# Function to count triples in a file
count_triples() {
    local file=$1
    # Simple estimate: count lines with '.' at the end (subject to false positives)
    grep -c '\.$' "$file" || echo "unknown"
}

# Function to build index
build_index() {
    local index_name=$1
    local input_files=$2
    local description=$3

    print_header "Building index: ${index_name}"
    print_info "${description}"

    local index_path="${INDEXES_DIR}/${index_name}"

    # Count total files
    local file_count=$(echo ${input_files} | wc -w)
    print_info "Processing ${file_count} file(s)..."

    # Build the index
    if "${BUILD_DIR}/IndexBuilderMain" \
        -i "${index_path}" \
        -F ttl \
        -f ${input_files} \
        2>&1 | tee "${index_path}.log"; then
        print_success "Index built successfully: ${index_name}"
        print_info "Index location: ${index_path}"
        print_info "Log file: ${index_path}.log"

        # Show index size
        if [ -d "${index_path}" ]; then
            local size=$(du -sh "${index_path}" | cut -f1)
            print_info "Index size: ${size}"
        fi
    else
        print_error "Failed to build index: ${index_name}"
        return 1
    fi

    echo ""
}

# Function to show usage
show_help() {
    cat << EOF
Load all N3 examples into QLever

Usage: $0 [options]

Options:
  --combined-only   Build only the combined index with all examples
  --tutorial-only   Build only tutorial examples
  --realworld-only  Build only real-world examples
  --advanced-only   Build only advanced examples
  --help            Show this help message

Examples:
  $0                    # Build all indexes
  $0 --combined-only    # Build only combined index
  $0 --tutorial-only    # Build only tutorial examples

After building indexes, start QLever server with:
  ServerMain -i indexes/n3-combined -p 7001

EOF
}

# Parse command line arguments
BUILD_TUTORIAL=true
BUILD_REALWORLD=true
BUILD_ADVANCED=true
BUILD_COMBINED=true

if [ $# -gt 0 ]; then
    case "$1" in
        --help)
            show_help
            exit 0
            ;;
        --combined-only)
            BUILD_TUTORIAL=false
            BUILD_REALWORLD=false
            BUILD_ADVANCED=false
            ;;
        --tutorial-only)
            BUILD_REALWORLD=false
            BUILD_ADVANCED=false
            BUILD_COMBINED=false
            ;;
        --realworld-only)
            BUILD_TUTORIAL=false
            BUILD_ADVANCED=false
            BUILD_COMBINED=false
            ;;
        --advanced-only)
            BUILD_TUTORIAL=false
            BUILD_REALWORLD=false
            BUILD_COMBINED=false
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
fi

# Start
print_header "QLever N3 Examples - Index Builder"
echo -e "Script directory: ${SCRIPT_DIR}"
echo -e "Build directory: ${BUILD_DIR}"
echo -e "Indexes directory: ${INDEXES_DIR}"
echo ""

# Build tutorial index
if [ "$BUILD_TUTORIAL" = true ]; then
    TUTORIAL_FILES="${SCRIPT_DIR}/n3-tutorial/*.n3"
    build_index "n3-tutorial" "$TUTORIAL_FILES" "Tutorial examples (01-basic to 05-large-dataset)"
fi

# Build real-world index
if [ "$BUILD_REALWORLD" = true ]; then
    REALWORLD_FILES="${SCRIPT_DIR}/n3-real-world/*.n3"
    build_index "n3-realworld" "$REALWORLD_FILES" "Real-world examples (FOAF, Schema.org, Linked Data, Knowledge Graph)"
fi

# Build advanced index
if [ "$BUILD_ADVANCED" = true ]; then
    ADVANCED_FILES="${SCRIPT_DIR}/n3-advanced/*.n3"
    build_index "n3-advanced" "$ADVANCED_FILES" "Advanced examples (Collections, Property Lists, Blank Nodes, Language Tags)"
fi

# Build combined index
if [ "$BUILD_COMBINED" = true ]; then
    COMBINED_FILES="${SCRIPT_DIR}/n3-tutorial/*.n3 ${SCRIPT_DIR}/n3-real-world/*.n3 ${SCRIPT_DIR}/n3-advanced/*.n3"
    build_index "n3-combined" "$COMBINED_FILES" "All N3 examples combined (~5000+ triples)"
fi

# Summary
print_header "Summary"
echo "Indexes created in: ${INDEXES_DIR}"
echo ""
echo "Available indexes:"
if [ "$BUILD_TUTORIAL" = true ] || [ -d "${INDEXES_DIR}/n3-tutorial" ]; then
    echo "  - n3-tutorial    : Tutorial examples"
fi
if [ "$BUILD_REALWORLD" = true ] || [ -d "${INDEXES_DIR}/n3-realworld" ]; then
    echo "  - n3-realworld   : Real-world examples"
fi
if [ "$BUILD_ADVANCED" = true ] || [ -d "${INDEXES_DIR}/n3-advanced" ]; then
    echo "  - n3-advanced    : Advanced examples"
fi
if [ "$BUILD_COMBINED" = true ] || [ -d "${INDEXES_DIR}/n3-combined" ]; then
    echo "  - n3-combined    : All examples combined"
fi
echo ""
print_info "To start QLever server with an index:"
echo "  ${BUILD_DIR}/ServerMain -i ${INDEXES_DIR}/n3-combined -p 7001"
echo ""
print_info "To query via curl:"
echo '  curl -X POST http://localhost:7001/ \'
echo '    -H "Content-Type: application/sparql-query" \'
echo '    --data "SELECT * WHERE { ?s ?p ?o } LIMIT 10"'
echo ""
print_info "See README-N3.md for more information and sample queries"
echo ""
print_success "Done!"
