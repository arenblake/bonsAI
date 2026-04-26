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
PLATFORM="linux_x86_64" # Default for now

printf "Fetching latest release from GitHub...\n"
LATEST_RELEASE=$(curl -s https://api.github.com/repos/${GITHUB_REPO}/releases/latest | grep "tag_name" | cut -d '"' -f 4)

if [ -z "${LATEST_RELEASE}" ]; then
  printf "${RED}No releases found on GitHub.${NC}\n"
  printf "Please build BonsAI first using './configure.sh' and the Bazel build command.\n"
  exit 1
fi

VERSION=$(echo ${LATEST_RELEASE} | sed 's/^v//')
BINARY_URL="https://github.com/${GITHUB_REPO}/releases/download/${LATEST_RELEASE}/bonsai-${VERSION}-${PLATFORM}"

printf "Downloading ${LATEST_RELEASE}...\n"
curl -L "${BINARY_URL}" -o /usr/local/bin/bonsai
chmod +x /usr/local/bin/bonsai

printf "${GREEN}BonsAI ${LATEST_RELEASE} installed successfully to /usr/local/bin/bonsai${NC}\n"
exit 0
