#!/bin/bash
#
# One-time developer environment setup
# Installs pre-commit hooks, configures git, sets up IDE templates
#
# Usage:
#   ./scripts/dev-setup.sh
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}▶ QLever Developer Setup${NC}"
echo ""

# Check if we're in the right directory
if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    echo -e "${RED}✗ CMakeLists.txt not found. Are you in the QLever root directory?${NC}"
    exit 1
fi

# 1. Install pre-commit hooks
echo -e "${BLUE}[1/5] Installing pre-commit hooks...${NC}"
if command -v pre-commit &> /dev/null; then
    cd "$PROJECT_ROOT"
    pre-commit install
    echo -e "${GREEN}✓ Pre-commit hooks installed${NC}"
else
    echo -e "${YELLOW}⚠ pre-commit not found. Install with: pip install pre-commit${NC}"
fi

# 2. Configure git
echo ""
echo -e "${BLUE}[2/5] Configuring git...${NC}"
cd "$PROJECT_ROOT"
git config core.hooksPath .git/hooks
echo -e "${GREEN}✓ Git hooks path configured${NC}"

# 3. Create VS Code settings directory
echo ""
echo -e "${BLUE}[3/5] Setting up IDE configurations...${NC}"
mkdir -p .vscode
if [ ! -f ".vscode/settings.json" ]; then
    cat > .vscode/settings.json << 'EOF'
{
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
  "C_Cpp.default.cppStandard": "c++20",
  "C_Cpp.default.cStandard": "c11",
  "C_Cpp.intelliSenseEngine": "tag-parser",
  "editor.formatOnSave": true,
  "[cpp]": {
    "editor.defaultFormatter": "xaver.clang-format",
    "editor.formatOnSave": true
  },
  "clangFormat.style": "file",
  "clangFormat.fallbackStyle": "Google",
  "cmake.generator": "Ninja",
  "cmake.configureOnEdit": false,
  "cmake.configureOnOpen": true,
  "cmake.sourceDirectory": "${workspaceFolder}",
  "cmake.buildDirectory": "${workspaceFolder}/build"
}
EOF
    echo -e "${GREEN}✓ Created .vscode/settings.json${NC}"
else
    echo -e "${YELLOW}⚠ .vscode/settings.json already exists (skipped)${NC}"
fi

# 4. Create build directory
echo ""
echo -e "${BLUE}[4/5] Creating build directory...${NC}"
mkdir -p build
echo -e "${GREEN}✓ Build directory ready${NC}"

# 5. Print next steps
echo ""
echo -e "${BLUE}[5/5] Verification${NC}"
echo ""
echo -e "${GREEN}✓ Developer environment setup complete!${NC}"
echo ""
echo "Next steps:"
echo ""
echo -e "  ${YELLOW}Build the project:${NC}"
echo "    cd build && cmake -GNinja -DCMAKE_BUILD_TYPE=Release .."
echo "    cmake --build ."
echo ""
echo -e "  ${YELLOW}Or use convenience targets:${NC}"
echo "    make build          # Build release binary"
echo "    make fast-build     # Quick build & test"
echo "    make test           # Run all tests"
echo "    make lint           # Run clang-tidy"
echo "    make format-check   # Check code style"
echo ""
echo -e "  ${YELLOW}IDE setup:${NC}"
echo "    - VS Code: Install C++, CMake Tools, clang-format extensions"
echo "    - CLion: Open project root, auto-detects configuration"
echo "    - Vim/Neovim: See docs/how-to/ide-setup.md for LSP setup"
echo ""
echo -e "  ${YELLOW}Documentation:${NC}"
echo "    - Contributing: See CONTRIBUTING.md"
echo "    - Setup: See docs/how-to/quick-start.md"
echo "    - IDE Setup: See docs/how-to/ide-setup.md"
echo ""
echo "Questions? Open an issue or see CONTRIBUTING.md"
