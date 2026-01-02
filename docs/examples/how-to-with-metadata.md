---
diataxis_type: how-to
title: "Configure Claude Code SessionStart Hooks"
description: "Set up automatic dependency installation for Claude Code on the web using SessionStart hooks"
audience: humans
status: complete
last_updated: 2026-01-02
difficulty: intermediate
estimated_time: "15 minutes"
prerequisites:
  - "GitHub repository with .claude/ directory"
  - "Basic shell scripting knowledge"
  - "Familiarity with JSON configuration files"
related_docs:
  - "reference/cli.md"
  - "how-to/native-setup.md"
  - "tutorials/01-quickstart.md"
keywords:
  - Claude Code
  - SessionStart hooks
  - environment setup
  - automation
  - dependencies
  - configuration
  - CI/CD
  - development environment
semantic_tags:
  - "configuration/claude-code"
  - "automation/hooks"
  - "development/setup"
  - "ci-cd/automation"
agent_priority: medium
search_boost: 1.5
---

# Configure Claude Code SessionStart Hooks

> Automatically install dependencies and configure your development environment when Claude Code sessions start

## Problem

Every time you start a new Claude Code session on the web, you need to manually install dependencies, configure build tools, and set environment variables. This wastes time and can lead to inconsistent environments.

## Solution

Use SessionStart hooks to automatically run setup scripts when your session initializes.

## Prerequisites

- GitHub repository with `.claude/` directory
- Basic shell scripting knowledge
- Familiarity with JSON configuration files

## Steps

### 1. Create Configuration File

Create or update `.claude/settings.json` in your repository root:

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

**Important**: The `"matcher"` field **must be an empty string** (`""`), not `"startup"` or any tool name.

### 2. Create Setup Script

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

### 3. Make Script Executable

```bash
chmod +x scripts/setup-dev-env.sh
```

### 4. Test Locally (Optional)

```bash
# Test the script manually
./scripts/setup-dev-env.sh
```

### 5. Commit and Push

```bash
git add .claude/settings.json scripts/setup-dev-env.sh
git commit -m "feat: add SessionStart hook for automatic environment setup"
git push
```

### 6. Verify in Claude Code

Start a new Claude Code session and check that:
- The setup script runs automatically
- Dependencies are installed
- No errors appear in the output

## Environment Variables

SessionStart hooks have access to special environment variables:

- `CLAUDE_CODE_REMOTE` - set to `"true"` in web environments
- `CLAUDE_PROJECT_DIR` - path to your project root
- `CLAUDE_ENV_FILE` - file to write environment variables for subsequent commands

## Advanced: Environment-Specific Setup

Customize behavior based on environment:

```bash
#!/bin/bash
set -e

# Run only in remote (web) environments
if [ "$CLAUDE_CODE_REMOTE" = "true" ]; then
    echo "Installing dependencies for web environment..."
    npm install
else
    echo "Local environment detected, skipping npm install"
fi

# Run everywhere
conan profile detect --force

exit 0
```

## Advanced: Persist Environment Variables

For subsequent bash commands to use environment variables set during SessionStart:

```bash
#!/bin/bash

# Write to CLAUDE_ENV_FILE for subsequent commands
if [ -n "$CLAUDE_ENV_FILE" ]; then
    echo "MY_VAR=value" >> "$CLAUDE_ENV_FILE"
fi

exit 0
```

## Troubleshooting

### Hook Not Running

**Problem**: SessionStart hook doesn't execute

**Check**:
- `"matcher"` is an empty string: `""`
- File is `.claude/settings.json` in repository root
- Configuration is valid JSON

**Fix**:
```bash
# Validate JSON syntax
cat .claude/settings.json | jq .
```

### Script Fails Silently

**Problem**: Script errors are not visible

**Fix**: Use `set -e` and add debug output:

```bash
#!/bin/bash
set -e

echo "Starting setup..."
npm install || { echo "npm install failed"; exit 1; }
echo "Setup complete!"
```

### Permissions Issues

**Problem**: "Permission denied" error

**Fix**:
```bash
chmod +x scripts/setup-dev-env.sh
git add scripts/setup-dev-env.sh
git commit -m "fix: make setup script executable"
```

## Related Resources

- [Claude Code hooks documentation](/en/hooks)
- [SessionStart hook reference](/en/hooks#sessionstart)
- [Claude Code on the web](/en/claude-code-web)
- [Native Setup Guide](./native-setup.md)

## See Also

- **[Native Setup](./native-setup.md)** — Manual development environment setup
- **[CLI Reference](../reference/cli.md)** — Command-line tools and options
- **[Troubleshooting](./troubleshooting.md)** — Solutions to common problems

---

**Result**: Automated, reproducible development environment setup that runs every time you start a Claude Code session.
