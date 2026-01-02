#!/bin/bash
# SessionStart hook: Guarantee complete build environment before agent starts
# Runs synchronously (60-90s) to ensure NO possibility of build failure
# Goal: Agent can write code and run make with guaranteed success

exec 2>&1

echo "QLever SessionStart: Installing complete build environment"
echo ""

# Run the full setup synchronously
# This installs:
#  - All system packages (CMake, compiler, Ninja, Boost, ICU, SSL, etc.)
#  - Python tools (pre-commit, test dependencies)
#  - Conan and its profile
# Result: Agent will never experience missing dependencies

bash "${CLAUDE_PROJECT_DIR}/scripts/setup-dev-env.sh" --no-background

echo ""
echo "✓ Environment setup complete"
echo "  Agent can now write code and run 'cmake' or 'make' without any setup"

