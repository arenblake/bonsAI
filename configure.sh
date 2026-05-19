#!/bin/bash

# BonsAI Configuration Script
# Sets up symlinks and internal structure for monolithic Bazel build

set -e

echo "🪴 Configuring BonsAI workspace..."

# 1. Ensure LiteRT-LM is present
if [ ! -d "LiteRT-LM" ]; then
    echo "Error: LiteRT-LM directory not found. Please ensure it was included in the clone."
    exit 1
fi

# 2. Setup internal bonsai package in LiteRT-LM
echo "Setting up Bazel package links..."
mkdir -p LiteRT-LM/bonsai
cd LiteRT-LM/bonsai
ln -sf ../../include include
ln -sf ../../src src
cd ../..

# 3. Create necessary directories
mkdir -p include/oatpp
mkdir -p include/oatpp-test

echo "✅ Workspace configured."
echo "You can now build with: cd LiteRT-LM && bazel build -c opt --define=LITERT_LM_FST_CONSTRAINTS_DISABLED=1 //bonsai:bonsai"
