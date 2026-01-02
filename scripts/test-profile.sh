#!/bin/bash
#
# Profile test execution to identify slow tests
# Helps optimize testing time by finding bottlenecks
#
# Usage:
#   ./scripts/test-profile.sh           # Profile all tests
#   ./scripts/test-profile.sh 10        # Show top 10 slowest tests
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Default: show top 20 slowest tests
N_SLOWEST=${1:-20}

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}▶ Test Performance Profiling${NC}"
echo "  Measuring execution time of all tests..."
echo ""

cd "$BUILD_DIR"

# Run all tests with timing, capture individual test times
test_log=$(mktemp)
if ctest \
    -j1 \
    --output-on-failure \
    --timeout 600 \
    2>&1 | tee "$test_log"; then
    test_status=0
else
    test_status=$?
fi

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Top ${N_SLOWEST} Slowest Tests:${NC}"
echo ""

# Extract test timing information and sort
# Format: "Test project QLever: time=X.XX sec"
grep -E "Test project|Test.*Passed|Test.*FAILED|Real time|CPU time" "$test_log" | \
    awk '
        /Test project/ {
            test = $3;
            getline;
            time = $NF;
            sub("sec", "", time);
            printf "%f %s\n", time, test
        }
    ' | \
    sort -rn | \
    head -n "$N_SLOWEST" | \
    awk '{
        printf "%-6.2fs  %s\n", $1, $2
    }' | \
    nl

# Calculate total test time
total_time=$(grep -E "Total Test time" "$test_log" | awk '{print $4}' || echo "unknown")
echo ""
echo -e "${YELLOW}Total test time: ${total_time}${NC}"
echo ""

if [ $test_status -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed${NC}"
else
    echo -e "${RED}✗ Some tests failed${NC}"
fi

# Cleanup
rm -f "$test_log"

# Print recommendations
echo ""
echo -e "${YELLOW}Recommendations:${NC}"
echo "  - Tests taking > 5s should be reviewed for optimization"
echo "  - Consider splitting large tests into smaller units"
echo "  - Check if slow tests can use mock data instead of full datasets"
echo "  - Use 'make fast-build' for development iteration"
echo ""
echo "For individual test timing:"
echo "  ctest -R 'TestName' --verbose"
