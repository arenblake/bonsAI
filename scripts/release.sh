#!/bin/bash

# BonsAI Release Script
# This script builds the monolithic binary and packages it for release

set -e

# Version is still used for internal metadata or logging
VERSION=$(grep "project(BonsAI VERSION" CMakeLists.txt | cut -d ' ' -f 3 | tr -d ')')
# Fixed name for easier one-liner installation
PLATFORM="linux_$(uname -m)"
OUTPUT_NAME="bonsai-${PLATFORM}"

printf "Building BonsAI version ${VERSION} for ${PLATFORM}...\n"

# 1. Configure workspace
./configure.sh

# 2. Build monolithic binary
cd LiteRT-LM
bazel build -c opt --define=LITERT_LM_FST_CONSTRAINTS_DISABLED=1 //bonsai:bonsai
cd ..

# 3. Copy and rename
# Note: sudo is used because Bazel outputs are often read-only/owned by root in some environments
sudo cp LiteRT-LM/bazel-bin/bonsai/bonsai ./${OUTPUT_NAME}
sudo chown $(id -u):$(id -g) ./${OUTPUT_NAME}
chmod +w ./${OUTPUT_NAME}
strip ${OUTPUT_NAME}

printf "Successfully built ${OUTPUT_NAME}\n"
ls -lh ${OUTPUT_NAME}
