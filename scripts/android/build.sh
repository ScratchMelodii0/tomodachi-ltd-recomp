#!/usr/bin/env bash
# scripts/android/build.sh
#
# Cross-compile the suyu/mk8-recomp native runtime for Android arm64-v8a.
#
# This script handles the parts of the build pipeline that cannot live inside
# Gradle (the suyu C++ host-tools build, AOT export, and module compilation).
# Android Studio / Gradle calls this script during the externalNativeBuild
# step, or you can run it manually.
#
# Prerequisites:
#   - ANDROID_NDK  set to the NDK root (e.g. ~/Android/Sdk/ndk/26.3.11579264)
#   - cmake, ninja available on PATH
#   - The repo has been cloned with submodules (scripts/install.py fetch step)
#
# Usage:
#   ./scripts/android/build.sh [--jobs N] [--configure-only] [--debug]
#
# The script writes:
#   upstream/mk8-recomp/build/android-arm64/   — suyu native build tree
#   android/app/src/main/jniLibs/arm64-v8a/   — libtomodachi_android.so

set -euo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
UPSTREAM="$REPO/upstream/mk8-recomp"
SUYU="$UPSTREAM/third_party/suyu"
BUILD="$UPSTREAM/build/android-arm64"
JNILIB="$REPO/android/app/src/main/jniLibs/arm64-v8a"

# ── Defaults ────────────────────────────────────────────────────────────────
JOBS="${JOBS:-$(nproc --ignore=2)}"
CONFIGURE_ONLY=0
BUILD_TYPE="Release"

while [[ $# -gt 0 ]]; do
    case $1 in
        --jobs)       JOBS="$2";         shift 2 ;;
        --configure-only) CONFIGURE_ONLY=1; shift ;;
        --debug)      BUILD_TYPE="Debug"; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ── Validate environment ─────────────────────────────────────────────────────
if [[ -z "${ANDROID_NDK:-}" ]]; then
    # Try to find it in the common SDK location.
    SDK="${ANDROID_HOME:-$HOME/Android/Sdk}"
    NDK_DIR=$(ls -d "$SDK"/ndk/26.* 2>/dev/null | sort -V | tail -1 || true)
    if [[ -d "$NDK_DIR" ]]; then
        ANDROID_NDK="$NDK_DIR"
        echo "INFO: ANDROID_NDK not set; using $ANDROID_NDK"
    else
        echo "ERROR: ANDROID_NDK is not set and could not be found in $SDK/ndk/"
        echo "  Install NDK 26.x via Android Studio SDK Manager, then set ANDROID_NDK."
        exit 1
    fi
fi

TOOLCHAIN="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
if [[ ! -f "$TOOLCHAIN" ]]; then
    echo "ERROR: NDK toolchain file not found: $TOOLCHAIN"
    exit 1
fi

if [[ ! -d "$SUYU/.git" ]]; then
    echo "ERROR: suyu submodule not initialised."
    echo "  Run './install.sh' through step 3 (fetch) first, or:"
    echo "  git -C upstream/mk8-recomp submodule update --init --recursive"
    exit 1
fi

echo "==> Building for Android arm64-v8a"
echo "    NDK:        $ANDROID_NDK"
echo "    Build dir:  $BUILD"
echo "    Build type: $BUILD_TYPE"
echo "    Jobs:       $JOBS"

mkdir -p "$BUILD" "$JNILIB"

# ── Configure ────────────────────────────────────────────────────────────────
cmake \
    -S "$REPO/android/app/src/main/cpp" \
    -B "$BUILD" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-29 \
    -DANDROID_NDK="$ANDROID_NDK" \
    -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -C "$REPO/cmake/android-ndk.cmake"

if [[ "$CONFIGURE_ONLY" -eq 1 ]]; then
    echo "==> Configure-only: done."
    exit 0
fi

# ── Build ─────────────────────────────────────────────────────────────────────
cmake --build "$BUILD" --target tomodachi_android -- -j"$JOBS"

# ── Copy the shared library to the Gradle jniLibs directory ──────────────────
SO="$BUILD/libtomodachi_android.so"
if [[ -f "$SO" ]]; then
    cp "$SO" "$JNILIB/"
    echo "==> Copied: $JNILIB/libtomodachi_android.so"
else
    echo "ERROR: Build succeeded but $SO not found."
    exit 1
fi

echo "==> Android native build complete."
echo "    Open the 'android/' directory in Android Studio and run the app,"
echo "    or run: cd android && ./gradlew assembleDebug"
