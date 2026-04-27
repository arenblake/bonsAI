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

# Check if running as root
if [ "$EUID" -ne 0 ]; then
  printf "${RED}Please run as root (use sudo)${NC}\n"
  exit 1
fi

# Check if bonsai binary exists in the current directory (if running from repo)
if [ -f "./bonsai" ]; then
  printf "Found local 'bonsai' binary. Installing...\n"
  cp ./bonsai /usr/local/bin/bonsai
  chmod +x /usr/local/bin/bonsai
  printf "${GREEN}BonsAI installed successfully to /usr/local/bin/bonsai${NC}\n"
  exit 0
fi

# If not in repo, try to download from GitHub
GITHUB_REPO="arenblake/bonsAI"
PLATFORM="linux_x86_64"

printf "Fetching latest release from GitHub...\n"
# Fetch tag_name using a more robust regex
LATEST_RELEASE=$(curl -s https://api.github.com/repos/${GITHUB_REPO}/releases/latest | grep '"tag_name":' | sed -E 's/.*"tag_name": "([^"]+)".*/\1/')

if [ -z "${LATEST_RELEASE}" ]; then
  printf "${RED}Error: Could not find any releases on GitHub.${NC}\n"
  printf "Please ensure the repository '${GITHUB_REPO}' has at least one published release.\n"
  exit 1
fi

VERSION=$(echo ${LATEST_RELEASE} | sed 's/^v//')
BINARY_URL="https://github.com/${GITHUB_REPO}/releases/download/${LATEST_RELEASE}/bonsai-${VERSION}-${PLATFORM}"

printf "Found release ${LATEST_RELEASE}. Downloading from:\n${BINARY_URL}\n"

if ! curl -fL "${BINARY_URL}" -o /usr/local/bin/bonsai; then
  printf "${RED}Error: Failed to download binary.${NC}\n"
  printf "The release might be in progress or the platform '${PLATFORM}' is not supported yet.\n"
  exit 1
fi

chmod +x /usr/local/bin/bonsai
printf "${GREEN}BonsAI ${LATEST_RELEASE} installed successfully to /usr/local/bin/bonsai${NC}\n"
exit 0
