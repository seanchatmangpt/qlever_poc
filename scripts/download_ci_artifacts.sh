#!/usr/bin/env bash
# Download CI artifacts from GitHub Actions
# Agent 10: Integration artifact retrieval
# EPIC 11 Integration Phase

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}=== CI Artifact Downloader ===${NC}"

# Usage information
usage() {
  cat <<EOF
Usage: $0 [OPTIONS]

Download verification artifacts from GitHub Actions CI runs.

OPTIONS:
  -r, --run-id RUN_ID       GitHub Actions run ID (required)
  -o, --output DIR          Output directory (default: ./ci-artifacts)
  -a, --architecture ARCH   Download only specified architecture (x86_64, aarch64, or 'all')
  -t, --token TOKEN         GitHub token (or set GITHUB_TOKEN env var)
  -h, --help                Show this help message

EXAMPLES:
  # Download all artifacts from run 12345678
  $0 --run-id 12345678

  # Download only x86_64 artifacts
  $0 --run-id 12345678 --architecture x86_64

  # Use custom output directory
  $0 --run-id 12345678 --output /tmp/artifacts

ENVIRONMENT:
  GITHUB_TOKEN    GitHub personal access token with 'repo' scope
  GITHUB_REPO     Repository in format 'owner/repo' (default: seanchatmangpt/qlever)

PREREQUISITES:
  - gh (GitHub CLI) installed and authenticated
  OR
  - curl + jq + GITHUB_TOKEN set

EOF
  exit 1
}

# Default values
OUTPUT_DIR="./ci-artifacts"
ARCHITECTURE="all"
RUN_ID=""
GITHUB_REPO="${GITHUB_REPO:-seanchatmangpt/qlever}"
GITHUB_TOKEN="${GITHUB_TOKEN:-}"

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -r|--run-id)
      RUN_ID="$2"
      shift 2
      ;;
    -o|--output)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    -a|--architecture)
      ARCHITECTURE="$2"
      shift 2
      ;;
    -t|--token)
      GITHUB_TOKEN="$2"
      shift 2
      ;;
    -h|--help)
      usage
      ;;
    *)
      echo -e "${RED}Error: Unknown option $1${NC}"
      usage
      ;;
  esac
done

# Validate required arguments
if [[ -z "${RUN_ID}" ]]; then
  echo -e "${RED}Error: --run-id is required${NC}"
  usage
fi

# Check for gh CLI or curl+jq
if command -v gh &> /dev/null; then
  USE_GH_CLI=true
  echo -e "${BLUE}Using GitHub CLI (gh)${NC}"
elif command -v curl &> /dev/null && command -v jq &> /dev/null; then
  USE_GH_CLI=false
  echo -e "${BLUE}Using curl + jq${NC}"
  if [[ -z "${GITHUB_TOKEN}" ]]; then
    echo -e "${RED}Error: GITHUB_TOKEN environment variable required when using curl${NC}"
    exit 1
  fi
else
  echo -e "${RED}Error: Either 'gh' CLI or 'curl'+'jq' required${NC}"
  echo "Install gh: https://cli.github.com/"
  exit 1
fi

# Create output directory
mkdir -p "${OUTPUT_DIR}"
echo "Output directory: ${OUTPUT_DIR}"

# Function to download with gh CLI
download_with_gh() {
  local run_id=$1
  local output_dir=$2
  local arch_filter=$3

  echo -e "${YELLOW}Fetching artifacts for run ${run_id}...${NC}"

  # List artifacts
  ARTIFACTS=$(gh run view "${run_id}" --repo "${GITHUB_REPO}" --json artifacts --jq '.artifacts[] | "\(.name)"')

  if [[ -z "${ARTIFACTS}" ]]; then
    echo -e "${RED}No artifacts found for run ${run_id}${NC}"
    exit 1
  fi

  echo "Available artifacts:"
  echo "${ARTIFACTS}" | sed 's/^/  - /'
  echo ""

  # Download artifacts
  while IFS= read -r artifact_name; do
    # Filter by architecture if specified
    if [[ "${arch_filter}" != "all" ]]; then
      if [[ ! "${artifact_name}" =~ ${arch_filter} ]]; then
        echo -e "${BLUE}Skipping ${artifact_name} (architecture filter: ${arch_filter})${NC}"
        continue
      fi
    fi

    echo -e "${YELLOW}Downloading: ${artifact_name}${NC}"
    gh run download "${run_id}" \
      --repo "${GITHUB_REPO}" \
      --name "${artifact_name}" \
      --dir "${output_dir}/${artifact_name}"

    echo -e "${GREEN}  ✓ Downloaded to ${output_dir}/${artifact_name}${NC}"
  done <<< "${ARTIFACTS}"
}

# Function to download with curl
download_with_curl() {
  local run_id=$1
  local output_dir=$2
  local arch_filter=$3

  echo -e "${YELLOW}Fetching artifacts for run ${run_id}...${NC}"

  # Get artifacts list
  ARTIFACTS_JSON=$(curl -s \
    -H "Authorization: Bearer ${GITHUB_TOKEN}" \
    -H "Accept: application/vnd.github+json" \
    "https://api.github.com/repos/${GITHUB_REPO}/actions/runs/${run_id}/artifacts")

  # Check if successful
  if echo "${ARTIFACTS_JSON}" | jq -e '.artifacts' > /dev/null 2>&1; then
    :
  else
    echo -e "${RED}Failed to fetch artifacts. Response:${NC}"
    echo "${ARTIFACTS_JSON}"
    exit 1
  fi

  # Download each artifact
  echo "${ARTIFACTS_JSON}" | jq -r '.artifacts[] | "\(.name) \(.archive_download_url)"' | \
  while read -r artifact_name download_url; do
    # Filter by architecture if specified
    if [[ "${arch_filter}" != "all" ]]; then
      if [[ ! "${artifact_name}" =~ ${arch_filter} ]]; then
        echo -e "${BLUE}Skipping ${artifact_name} (architecture filter: ${arch_filter})${NC}"
        continue
      fi
    fi

    echo -e "${YELLOW}Downloading: ${artifact_name}${NC}"
    mkdir -p "${output_dir}/${artifact_name}"

    curl -L \
      -H "Authorization: Bearer ${GITHUB_TOKEN}" \
      -H "Accept: application/vnd.github+json" \
      "${download_url}" \
      -o "${output_dir}/${artifact_name}.zip"

    # Extract zip
    unzip -q "${output_dir}/${artifact_name}.zip" -d "${output_dir}/${artifact_name}"
    rm "${output_dir}/${artifact_name}.zip"

    echo -e "${GREEN}  ✓ Downloaded to ${output_dir}/${artifact_name}${NC}"
  done
}

# Download artifacts
if [[ "${USE_GH_CLI}" == "true" ]]; then
  download_with_gh "${RUN_ID}" "${OUTPUT_DIR}" "${ARCHITECTURE}"
else
  download_with_curl "${RUN_ID}" "${OUTPUT_DIR}" "${ARCHITECTURE}"
fi

echo ""
echo -e "${GREEN}=== Download Complete ===${NC}"
echo ""
echo "Artifacts saved to: ${OUTPUT_DIR}"
echo ""
echo "Contents:"
find "${OUTPUT_DIR}" -type f | head -20 | sed 's/^/  /'

# Count total files
TOTAL_FILES=$(find "${OUTPUT_DIR}" -type f | wc -l)
echo ""
echo "Total files: ${TOTAL_FILES}"

# Generate summary
SUMMARY_FILE="${OUTPUT_DIR}/DOWNLOAD_SUMMARY.txt"
cat > "${SUMMARY_FILE}" <<EOF
CI Artifact Download Summary
=============================

Run ID: ${RUN_ID}
Repository: ${GITHUB_REPO}
Architecture Filter: ${ARCHITECTURE}
Download Time: $(date -u +"%Y-%m-%d %H:%M:%S UTC")

Downloaded Artifacts:
---------------------
$(find "${OUTPUT_DIR}" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort)

Total Files: ${TOTAL_FILES}
Total Size: $(du -sh "${OUTPUT_DIR}" | cut -f1)

Next Steps:
-----------
1. Verify receipt integrity:
   find ${OUTPUT_DIR} -name "*.cbor" -exec b3sum {} \;

2. Check verdicts:
   find ${OUTPUT_DIR} -name "verdict.json" -exec cat {} \;

3. Compare cross-architecture results:
   diff ${OUTPUT_DIR}/x86_64-*/verdict.json ${OUTPUT_DIR}/aarch64-*/verdict.json

EOF

echo ""
echo "Summary saved to: ${SUMMARY_FILE}"
cat "${SUMMARY_FILE}"

exit 0
