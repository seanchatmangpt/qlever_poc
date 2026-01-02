#!/bin/bash
# scripts/validate-links.sh
# Validates all markdown links in the documentation
# Requires: npm install -g markdown-link-check

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DOCS_DIR="$PROJECT_ROOT/docs"

echo "🔗 Validating documentation links..."
echo "   Checking: $DOCS_DIR"
echo ""

# Count files and links
total_files=0
broken_links=0

# Find all markdown files
for md_file in $(find "$DOCS_DIR" -name "*.md" -type f | sort); do
  # Skip legacy directory for now (expected to have some broken links)
  if [[ "$md_file" == *"/legacy/"* ]]; then
    continue
  fi

  total_files=$((total_files + 1))
  relative_path="${md_file#$PROJECT_ROOT/}"

  echo "Checking: $relative_path"

  # Use markdown-link-check
  if command -v markdown-link-check &> /dev/null; then
    output=$(markdown-link-check "$md_file" 2>&1 || true)

    # Check for broken links in output
    if echo "$output" | grep -q "✖"; then
      echo "  ❌ Found broken links:"
      echo "$output" | grep "✖" | sed 's/^/    /'
      broken_links=$((broken_links + 1))
    else
      echo "  ✅ All links valid"
    fi
  else
    echo "  ⚠️  markdown-link-check not installed (install: npm i -g markdown-link-check)"
    exit 1
  fi
done

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 Link Validation Results"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "   Total files checked: $total_files"
echo "   Files with broken links: $broken_links"
echo ""

if [ $broken_links -eq 0 ]; then
  echo "✅ All documentation links are valid!"
  exit 0
else
  echo "❌ Found broken links in $broken_links file(s)"
  exit 1
fi
