#!/bin/bash

# BonsAI Installer for Linux
# This script installs the 'bonsai' binary to /usr/local/bin

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

printf "${BLUE}BonsAI 🪴 Installer${NC}\n"

# Helper for sudo
run_as_root() {
  if [ "$EUID" -ne 0 ]; then
    sudo "$@"
  else
    "$@"
  fi
}

# Check if bonsai binary exists in the current directory (if running from repo)
if [ -f "./bonsai" ]; then
  printf "Found local 'bonsai' binary. Installing...\n"
  run_as_root cp ./bonsai /usr/local/bin/bonsai
  run_as_root chmod +x /usr/local/bin/bonsai
  printf "${GREEN}BonsAI installed successfully to /usr/local/bin/bonsai${NC}\n"
  exit 0
fi

# If not in repo, download from GitHub Releases
GITHUB_REPO="arenblake/bonsAI"
# Detect architecture
ARCH=$(uname -m)
PLATFORM="linux_${ARCH}"
BINARY_URL="https://github.com/${GITHUB_REPO}/releases/latest/download/bonsai-${PLATFORM}"

printf "Downloading latest release for ${PLATFORM} from GitHub...\n"

# Create a temporary file for the download
TMP_FILE=$(mktemp)

if curl -fL "${BINARY_URL}" -o "${TMP_FILE}"; then
  printf "Download complete. Installing to /usr/local/bin...\n"
  run_as_root mv "${TMP_FILE}" /usr/local/bin/bonsai
  run_as_root chmod +x /usr/local/bin/bonsai
  printf "${GREEN}BonsAI installed successfully to /usr/local/bin/bonsai${NC}\n"
else
  printf "${RED}Error: Failed to download binary from ${BINARY_URL}${NC}\n"
  printf "The release might not be published yet, or your architecture is not supported.\n"
  rm -f "${TMP_FILE}"
  exit 1
fi
