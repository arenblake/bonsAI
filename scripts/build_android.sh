#!/bin/bash
# Builds BonsAI for Android (both Standalone Binary and JNI Shared Library)

set -e

# Default to android_arm64. Set NDK environment if needed.
# e.g., export ANDROID_NDK_HOME=/path/to/android-ndk

if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "Warning: ANDROID_NDK_HOME is not set."
    echo "Bazel may fail if it cannot automatically find the NDK."
    echo "Consider setting it: export ANDROID_NDK_HOME=/opt/android-ndk-r26b"
fi

echo "🪴 Building BonsAI for Android (android_arm64)..."

cd LiteRT-LM

# Build Standalone Binary
echo "Building standalone binary..."
bazel build -c opt --config=android_arm64 --define=LITERT_LM_FST_CONSTRAINTS_DISABLED=1 //bonsai:bonsai

# Build JNI Shared Library
echo "Building JNI shared library..."
bazel build -c opt --config=android_arm64 --define=LITERT_LM_FST_CONSTRAINTS_DISABLED=1 //bonsai:libbonsai_jni.so

echo "✅ Android build complete!"
echo "Artifacts located at:"
echo " - Standalone Binary: LiteRT-LM/bazel-bin/bonsai/bonsai"
echo " - JNI Shared Library: LiteRT-LM/bazel-bin/bonsai/libbonsai_jni.so"
