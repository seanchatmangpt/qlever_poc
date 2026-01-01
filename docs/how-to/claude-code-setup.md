# Claude Code on the Web: Setup & SessionStart Hooks

> How to properly configure and run Claude Code on the web with automatic dependency installation

## Overview

Claude Code on the web runs your code in a secure, isolated cloud environment. This guide explains how to set up automatic environment initialization using SessionStart hooks.

## SessionStart Hooks

SessionStart hooks automatically run when a Claude Code session starts. They're perfect for installing dependencies and configuring your development environment.

### Configuration

Create or update `.claude/settings.json` in your repository:

```json
{
  "hooks": {
    "SessionStart": [
      {
        "matcher": "",
        "hooks": [
          {
            "type": "command",
            "command": "\"$CLAUDE_PROJECT_DIR\"/scripts/setup-dev-env.sh"
          }
        ]
      }
    ]
  }
}
```

**Important**: The `"matcher"` field **must be an empty string** (`""`), not `"startup"` or any tool name. SessionStart hooks apply to the session initialization event, not to specific tools.

### Hook Execution

- **When it runs**: Immediately when a new session starts or an existing session resumes
- **Environment**: Both local Claude Code and Claude Code on the web
- **Working directory**: The repository root (`$CLAUDE_PROJECT_DIR`)
- **Environment variables**:
  - `CLAUDE_CODE_REMOTE` - set to `"true"` in web environments
  - `CLAUDE_PROJECT_DIR` - path to your project root
  - `CLAUDE_ENV_FILE` - file to write environment variables for subsequent commands

## Setup Script

### Basic Example

Create `scripts/setup-dev-env.sh`:

```bash
#!/bin/bash
set -e

echo "Setting up development environment..."

# Only run certain tasks in remote (web) environments
if [ "$CLAUDE_CODE_REMOTE" = "true" ]; then
    echo "Running in Claude Code on the web"
fi

# Install dependencies
npm install
pip install -r requirements.txt

exit 0
```

Make it executable:

```bash
chmod +x scripts/setup-dev-env.sh
```

### QLever Specific Setup

The QLever project includes a comprehensive setup script (`scripts/setup-dev-env.sh`) that:

1. **Verifies build tools**: CMake 3.27+, Ninja, C++ compiler (Clang 16+ or GCC 11+)
2. **Installs system dependencies**: build-essential, ninja-build, libicu-dev, pkg-config
3. **Sets up Conan**: C++ package manager with profile detection
4. **Installs Python tools**: pre-commit, pyaml, pyicu for testing
5. **Validates environment**: Final verification that all requirements are met

## Claude Code on the Web Environment

### Pre-installed Tools

The universal Claude Code environment includes:

- **Build tools**: CMake, Ninja, GCC, Clang
- **Languages**: Python 3.x, Node.js, Ruby, Go, Rust, Java, C++
- **Package managers**: pip, npm, npm, bundler, cargo, maven
- **Databases**: PostgreSQL 16, Redis 7.0
- **Development tools**: Git, Docker, curl, wget

### Environment-specific Code

Use `CLAUDE_CODE_REMOTE` to run code only in specific environments:

```bash
#!/bin/bash

# Run only in remote (web) environments
if [ "$CLAUDE_CODE_REMOTE" = "true" ]; then
    echo "Running in Claude Code on the web"
    npm install
fi

# Run everywhere
conan profile detect --force

exit 0
```

### Network Access

By default, Claude Code on the web has **limited network access**:

- Allowed: npm registry, PyPI, GitHub, Docker Hub, and other package managers
- Consider disabling network access for sensitive projects
- Configure in the web interface when starting your task

## Best Practices

1. **Make scripts idempotent**: They should work correctly if run multiple times
2. **Use exit codes**: Return 0 for success, non-zero for errors
3. **Keep scripts lightweight**: Minimize installation time
4. **Use environment detection**: Check `CLAUDE_CODE_REMOTE` for conditional logic
5. **Test both environments**: Verify the script works locally and on the web
6. **Document requirements**: Keep `CLAUDE.md` updated with dependency info

## Troubleshooting

### Hook not running

Check that:
- `"matcher"` is an empty string: `""`
- File is `.claude/settings.json` in repository root
- Configuration is valid JSON

### Script fails silently

Use `set -e` in your bash script to fail fast on errors. Redirect output to see what's happening:

```bash
#!/bin/bash
set -e

echo "Starting setup..."
# Your commands here
echo "Setup complete!"
```

### Permissions issues

Make sure the setup script is executable:

```bash
chmod +x scripts/setup-dev-env.sh
```

## Advanced: Persisting Environment Variables

For subsequent bash commands to use environment variables set during SessionStart:

```bash
#!/bin/bash

# Write to CLAUDE_ENV_FILE for subsequent commands
if [ -n "$CLAUDE_ENV_FILE" ]; then
    echo "MY_VAR=value" >> "$CLAUDE_ENV_FILE"
fi

exit 0
```

## Related Resources

- [Claude Code hooks documentation](/en/hooks)
- [SessionStart hook reference](/en/hooks#sessionstart)
- [Claude Code on the web](/en/claude-code-web)
