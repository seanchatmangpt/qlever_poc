#!/bin/bash
# Ultra-fast SessionStart hook for Claude Code on the web
# In cloud: all tools pre-installed, just launch background setup
# Local: fallback to full verification (only if not CLAUDE_CODE_REMOTE)

exec 2>&1

echo "QLever SessionStart: Starting environment setup"

# Cloud path: assume all tools pre-installed, launch background setup immediately
if [ "$CLAUDE_CODE_REMOTE" = "true" ]; then
    echo "  Running in Claude Code remote environment (cloud)"
    echo ""
    echo "  All build tools are pre-installed:"
    echo "    ✓ CMake ✓ C++ Compiler ✓ Ninja ✓ Conan"
    echo ""
    echo "  Launching background installation phase..."

    # Launch background installation script
    # Writes completion marker to /tmp/qlever-setup.ready
    (
        exec >/tmp/qlever-setup.log 2>&1
        bash "${CLAUDE_PROJECT_DIR}/scripts/setup-dev-env.sh" --background
    ) &

    echo $! > /tmp/qlever-setup.pid
    echo ""
    echo "✓ SessionStart complete. Agent starting..."
    exit 0
fi

# Local fallback: do full verification if not in cloud
echo "Local environment detected. Running full setup..."
bash "${CLAUDE_PROJECT_DIR}/scripts/setup-dev-env.sh" --no-background

