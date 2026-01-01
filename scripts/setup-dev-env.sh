#!/bin/bash
# Setup QLever development environment
# This script installs dependencies and configures pre-commit hooks

set -e

echo "Setting up QLever development environment..."

# Check for required tools
if ! command -v pip &> /dev/null; then
    echo "ERROR: pip is not installed"
    exit 1
fi

# Install pre-commit
echo "Installing pre-commit..."
pip install pre-commit

# Setup pre-commit hooks
echo "Setting up git hooks..."
pre-commit install

echo ""
echo "✓ Development environment setup complete!"
echo ""
echo "Next steps:"
echo "  1. mkdir build && cd build"
echo "  2. cmake -DCMAKE_BUILD_TYPE=Release -GNinja .."
echo "  3. cmake --build ."
echo "  4. ctest --output-on-failure"
echo ""
echo "For more options and scenarios, see: ./scripts/build-release.sh"
