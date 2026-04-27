#!/bin/bash

# BonsAI Release Script
# This script builds the monolithic binary and packages it for release

set -e

VERSION=$(grep "project(BonsAI VERSION" CMakeLists.txt | cut -d ' ' -f 3 | tr -d ')')
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')_$(uname -m)
OUTPUT_NAME="bonsai-${VERSION}-${PLATFORM}"

printf "Building BonsAI version ${VERSION} for ${PLATFORM}...\n"

# 1. Configure workspace
./configure.sh

# 2. Build monolithic binary
cd LiteRT-LM
bazel build -c opt --define=LITERT_LM_FST_CONSTRAINTS_DISABLED=1 //bonsai:bonsai
cd ..

# 3. Copy and rename
cp LiteRT-LM/bazel-bin/bonsai/bonsai ./${OUTPUT_NAME}
strip ${OUTPUT_NAME}

printf "Successfully built ${OUTPUT_NAME}\n"
ls -lh ${OUTPUT_NAME}
