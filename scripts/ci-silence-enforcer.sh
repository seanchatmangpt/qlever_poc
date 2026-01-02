#!/bin/bash
# CI Silence Enforcer - EPIC 10.2 Construction Seal
# Fails build if forbidden logging constructs exist in hot paths
# Exit code 1 = violations found, 0 = clean

set -euo pipefail

# Hot path scopes
HOT_PATHS=(
  "src/engine/ingress"
  "src/engine/query"
  "src/engine"
  "src/index"
  "src/util/MemoryMap.cpp"
  "src/util/MemoryMap.h"
)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

VIOLATIONS_FOUND=0
VIOLATION_FILES=()

echo "=== EPIC 10.2 Silence Enforcer CI Gate ===" >&2
echo "Scanning hot paths for forbidden logging constructs..." >&2
echo "" >&2

# Function to check a file for violations
check_file() {
  local file="$1"
  local violations=""
  local file_has_violations=0

  # Skip if file doesn't exist
  [[ -f "$file" ]] || return 0

  # Skip if file already checked (deduplicate)
  for checked in "${VIOLATION_FILES[@]}"; do
    [[ "$checked" == "$file" ]] && return 0
  done

  # Check for std::cout (exclude comments more thoroughly)
  # Note: grep -n outputs "linenum:  content", so we filter for "linenum:  //" pattern
  if grep -nE 'std::cout' "$file" | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' > /dev/null 2>&1; then
    violations+=$(grep -nE 'std::cout' "$file" | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' | sed "s|^|  $file:|")
    violations+=$'\n'
    file_has_violations=1
  fi

  # Check for std::cerr (exclude comments more thoroughly)
  if grep -nE 'std::cerr' "$file" | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' > /dev/null 2>&1; then
    violations+=$(grep -nE 'std::cerr' "$file" | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' | sed "s|^|  $file:|")
    violations+=$'\n'
    file_has_violations=1
  fi

  # Check for printf (exclude comments more thoroughly)
  if grep -nE '\bprintf\(' "$file" | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' > /dev/null 2>&1; then
    violations+=$(grep -nE '\bprintf\(' "$file" | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' | sed "s|^|  $file:|")
    violations+=$'\n'
    file_has_violations=1
  fi

  # Check for LOG() macros excluding INIT and FATAL (exclude comments more thoroughly)
  if grep -nE 'LOG\(' "$file" | grep -vE 'LOG\((INIT|FATAL)\)' | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' > /dev/null 2>&1; then
    violations+=$(grep -nE 'LOG\(' "$file" | grep -vE 'LOG\((INIT|FATAL)\)' | grep -vE '^[0-9]+:\s*//|^[0-9]+:\s*/\*|^[0-9]+:\s*\*' | sed "s|^|  $file:|")
    violations+=$'\n'
    file_has_violations=1
  fi

  if [[ $file_has_violations -eq 1 ]]; then
    echo -e "${RED}Violations found in $file:${NC}" >&2
    echo "$violations" >&2
    VIOLATIONS_FOUND=$((VIOLATIONS_FOUND + 1))
    VIOLATION_FILES+=("$file")
  fi
}

# Scan each hot path
for path in "${HOT_PATHS[@]}"; do
  if [[ -d "$path" ]]; then
    echo "Scanning directory: $path" >&2
    while IFS= read -r -d '' file; do
      check_file "$file"
    done < <(find "$path" -type f \( -name "*.cpp" -o -name "*.h" \) -print0)
  elif [[ -f "$path" ]]; then
    echo "Scanning file: $path" >&2
    check_file "$path"
  else
    echo -e "${YELLOW}Warning: Hot path not found: $path${NC}" >&2
  fi
done

echo "" >&2
if [[ $VIOLATIONS_FOUND -eq 0 ]]; then
  echo -e "${GREEN}✓ CI Silence Enforcer PASSED${NC}" >&2
  echo "No forbidden logging constructs found in hot paths." >&2
  exit 0
else
  echo -e "${RED}✗ CI Silence Enforcer FAILED${NC}" >&2
  echo "Found violations in $VIOLATIONS_FOUND file(s):" >&2
  for file in "${VIOLATION_FILES[@]}"; do
    echo "  - $file" >&2
  done
  echo "" >&2
  echo "Forbidden constructs in hot paths:" >&2
  echo "  - std::cout" >&2
  echo "  - std::cerr" >&2
  echo "  - printf()" >&2
  echo "  - LOG(DEBUG), LOG(INFO), LOG(WARNING), LOG(ERROR)" >&2
  echo "" >&2
  echo "Allowed constructs:" >&2
  echo "  - LOG(INIT)" >&2
  echo "  - LOG(FATAL)" >&2
  exit 1
fi
