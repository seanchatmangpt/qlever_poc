#!/bin/bash
# EPIC 11.1 Failure Receipt Generator
# Generates deterministic failure receipts for migration rollback scenarios

set -euo pipefail

# Parse command line arguments
PHASE=""
GATE=""
ERROR=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --phase)
      PHASE="$2"
      shift 2
      ;;
    --gate)
      GATE="$2"
      shift 2
      ;;
    --error)
      ERROR="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      echo "Usage: $0 --phase <phase> --gate <gate> --error <error>" >&2
      exit 1
      ;;
  esac
done

# Validate required arguments
if [ -z "$PHASE" ] || [ -z "$GATE" ] || [ -z "$ERROR" ]; then
  echo "ERROR: Missing required arguments" >&2
  echo "Usage: $0 --phase <phase> --gate <gate> --error <error>" >&2
  echo "" >&2
  echo "Example:" >&2
  echo "  $0 --phase '3_move_verification_crates' \\" >&2
  echo "     --gate 'cargo check --package qlever-kernel-runner' \\" >&2
  echo "     --error 'could not find Cargo.toml'" >&2
  exit 1
fi

TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
RECEIPT_FILE="/tmp/epic11.1-failure-receipt.json"

echo "Generating EPIC 11.1 failure receipt..." >&2

# Capture system state
CURRENT_DIR=$(pwd)
GIT_BRANCH=$(git branch --show-current 2>/dev/null || echo "UNKNOWN")
GIT_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "UNKNOWN")

# Attempt to capture last successful phase (if any)
LAST_SUCCESS_PHASE="unknown"
if [ -f "/tmp/epic11.1-last-phase.txt" ]; then
  LAST_SUCCESS_PHASE=$(cat /tmp/epic11.1-last-phase.txt)
fi

# Check if backup exists
BACKUP_EXISTS=false
if ls /tmp/rust-backup-*.tar.gz 1> /dev/null 2>&1; then
  BACKUP_EXISTS=true
  BACKUP_FILE=$(ls -t /tmp/rust-backup-*.tar.gz | head -1)
  BACKUP_SIZE=$(du -h "$BACKUP_FILE" | cut -f1)
else
  BACKUP_FILE="NONE"
  BACKUP_SIZE="0"
fi

# Generate failure receipt JSON
cat > "$RECEIPT_FILE" <<EOF
{
  "receipt_type": "EPIC11.1_MIGRATION_FAILURE",
  "receipt_version": "1.0.0",
  "timestamp": "$TIMESTAMP",
  "failure_classification": "GATE_FAILURE",

  "failure_details": {
    "failed_phase": "$PHASE",
    "failed_gate": "$GATE",
    "error_message": "$ERROR",
    "last_successful_phase": "$LAST_SUCCESS_PHASE"
  },

  "system_state": {
    "working_directory": "$CURRENT_DIR",
    "git_branch": "$GIT_BRANCH",
    "git_commit": "$GIT_COMMIT",
    "backup_exists": $BACKUP_EXISTS,
    "backup_file": "$BACKUP_FILE",
    "backup_size": "$BACKUP_SIZE"
  },

  "rollback_status": {
    "rollback_required": true,
    "rollback_method": "tarball_restore",
    "rollback_command": "tar -xzf $BACKUP_FILE && git reset --hard HEAD && git clean -fd"
  },

  "investigation": {
    "reproduction_command": "$GATE",
    "expected_result": "exit code 0",
    "actual_result": "failure",
    "log_file": "/tmp/epic11.1-failure.log"
  },

  "next_steps": [
    "Execute rollback procedure immediately",
    "Review error message and system state",
    "Investigate root cause of gate failure",
    "Update specification if necessary",
    "Do NOT retry migration without understanding failure",
    "Report to bb80-specification-validator agent"
  ],

  "receipt_metadata": {
    "generator": "generate_failure_receipt.sh",
    "agent": "Agent 10 (Integration Verification)",
    "receipt_hash": "COMPUTED_AFTER_GENERATION"
  }
}
EOF

# Compute receipt hash
if command -v blake3 &> /dev/null; then
  RECEIPT_HASH=$(cat "$RECEIPT_FILE" | blake3 --no-names)
  HASH_METHOD="BLAKE3"
else
  RECEIPT_HASH=$(cat "$RECEIPT_FILE" | sha256sum | awk '{print $1}')
  HASH_METHOD="SHA256"
fi

# Update receipt with hash
jq --arg hash "$RECEIPT_HASH" \
   '.receipt_metadata.receipt_hash = $hash' \
   "$RECEIPT_FILE" > "${RECEIPT_FILE}.tmp" && mv "${RECEIPT_FILE}.tmp" "$RECEIPT_FILE"

# Output receipt
cat "$RECEIPT_FILE"

echo "" >&2
echo "=====================================" >&2
echo "MIGRATION FAILURE RECEIPT GENERATED" >&2
echo "=====================================" >&2
echo "" >&2
echo "Receipt: $RECEIPT_FILE" >&2
echo "Phase:   $PHASE" >&2
echo "Gate:    $GATE" >&2
echo "Error:   $ERROR" >&2
echo "" >&2
echo "Backup:  $BACKUP_FILE ($BACKUP_SIZE)" >&2
echo "Hash:    $RECEIPT_HASH ($HASH_METHOD)" >&2
echo "" >&2
echo "NEXT ACTIONS:" >&2
echo "1. Review failure details above" >&2
echo "2. Execute rollback procedure:" >&2
echo "   cd /home/user/qlever" >&2
echo "   tar -xzf $BACKUP_FILE" >&2
echo "   git reset --hard HEAD" >&2
echo "   git clean -fd" >&2
echo "3. Investigate root cause" >&2
echo "4. Do NOT retry without understanding failure" >&2
echo "" >&2
