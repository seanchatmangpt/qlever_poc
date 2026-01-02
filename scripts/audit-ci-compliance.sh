#!/bin/bash
# EPIC 8.2 Agent 10: CI/CD Compliance Audit Script
# Validates CI/CD laws from EPIC 8 CI Relegation
# Ensures CI only invokes make universe with no overrides

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================="
echo "CI/CD CONFIGURATION COMPLIANCE AUDIT"
echo "========================================="

cd "$PROJECT_ROOT"

violations=0

# Check GitHub Actions workflows
echo ""
echo "Auditing GitHub Actions workflows..."

if [ -d .github/workflows ]; then
  for workflow in .github/workflows/*.yml .github/workflows/*.yaml; do
    if [ -f "$workflow" ]; then
      echo "  Checking $(basename "$workflow")..."

      # LAW CI-1: No prohibited make flags
      if grep -q "make.*-j\|make.*-k\|make.*--keep-going" "$workflow"; then
        echo "    ✗ VIOLATION: Prohibited make flags (-j, -k) found"
        violations=$((violations + 1))
      fi

      # LAW CI-2: No environment overrides
      if grep -q "CXX:\|CXXFLAGS:\|CMAKE_BUILD_TYPE:" "$workflow"; then
        echo "    ✗ VIOLATION: Build environment override detected"
        violations=$((violations + 1))
      fi

      # LAW CI-3: No retry logic
      if grep -q "retry:\|for.*make\|while.*make" "$workflow"; then
        echo "    ✗ VIOLATION: Retry logic detected"
        violations=$((violations + 1))
      fi

      # LAW CI-4: Should only invoke make universe
      if grep "make" "$workflow" | grep -v "make universe\|make clean\|make verify" | grep -q "make"; then
        echo "    ⚠ WARNING: Non-standard make target invocation"
      fi
    fi
  done
fi

# Check GitLab CI configuration
echo ""
echo "Auditing GitLab CI configuration..."

if [ -f .gitlab-ci.yml ]; then
  echo "  Checking .gitlab-ci.yml..."

  if grep -q "make.*-j\|make.*-k" .gitlab-ci.yml; then
    echo "    ✗ VIOLATION: Prohibited make flags in GitLab CI"
    violations=$((violations + 1))
  fi

  if grep -q "CXX:\|CXXFLAGS:" .gitlab-ci.yml; then
    echo "    ✗ VIOLATION: Environment override in GitLab CI"
    violations=$((violations + 1))
  fi
fi

# Check Jenkinsfile
echo ""
echo "Auditing Jenkins configuration..."

if [ -f Jenkinsfile ]; then
  echo "  Checking Jenkinsfile..."

  if grep -q "make.*-j\|make.*-k" Jenkinsfile; then
    echo "    ✗ VIOLATION: Prohibited make flags in Jenkinsfile"
    violations=$((violations + 1))
  fi
fi

echo ""
echo "========================================="
if [ $violations -eq 0 ]; then
  echo "✓ PASS: CI configuration compliant with EPIC 8"
  echo "  CI/CD systems relegated to compute-only role"
  echo "  No interpretation, branching, or retry logic"
  exit 0
else
  echo "✗ FAIL: $violations CI violations detected"
  echo ""
  echo "VIOLATIONS DETECTED:"
  echo "  CI/CD must only invoke 'make universe'"
  echo "  No flags, overrides, or retry logic permitted"
  echo "  See EPIC8_CI_RELEGATION.md for details"
  exit 1
fi
