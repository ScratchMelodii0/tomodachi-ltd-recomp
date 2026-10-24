# cmake/android-ndk.cmake
#
# Supplementary CMake configuration for cross-compiling suyu/mk8-recomp
# for Android arm64-v8a using the Android NDK.
#
# Usage (invoked by scripts/android/build.sh, not directly by Android Studio):
#
#   cmake -S <suyu_src> -B <build_dir> \
#         -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
#         -DANDROID_ABI=arm64-v8a \
#         -DANDROID_PLATFORM=android-29 \
#         -DANDROID_NDK=$ANDROID_NDK \
#         -DCMAKE_BUILD_TYPE=Release \
#         -C <repo>/cmake/android-ndk.cmake
#
# This file sets CMake cache variables consumed by suyu's CMakeLists.txt and
# by the Android build system.  It does NOT replace the NDK toolchain file.

# ── Target platform ──────────────────────────────────────────────────────────
set(ANDROID_ABI         "arm64-v8a" CACHE STRING "" FORCE)
set(ANDROID_PLATFORM    "android-29" CACHE STRING "" FORCE)
set(ANDROID_STL         "c++_shared" CACHE STRING "" FORCE)
set(ANDROID_ARM_NEON    ON CACHE BOOL "" FORCE)

# ── Disable Qt (no Qt on Android in this build) ───────────────────────────────
set(ENABLE_QT               OFF CACHE BOOL "" FORCE)
set(YUZU_USE_BUNDLED_QT     OFF CACHE BOOL "" FORCE)
set(ENABLE_QT_TRANSLATION   OFF CACHE BOOL "" FORCE)

# ── Disable desktop-only features ─────────────────────────────────────────────
set(YUZU_TESTS              OFF CACHE BOOL "" FORCE)
set(ENABLE_WEB_SERVICE      OFF CACHE BOOL "" FORCE)
set(YUZU_ROOM               OFF CACHE BOOL "" FORCE)
set(YUZU_ROOM_STANDALONE    OFF CACHE BOOL "" FORCE)
set(USE_DISCORD_PRESENCE    OFF CACHE BOOL "" FORCE)
set(ENABLE_LIBUSB           OFF CACHE BOOL "" FORCE)
# breakpad crash reporter: Android has its own (not wired up here yet).
set(ENABLE_BREAKPAD         OFF CACHE BOOL "" FORCE)

# ── No JIT (W^X policy on Android) ───────────────────────────────────────────
set(SUYU_NO_JIT             ON  CACHE BOOL "" FORCE)
set(SUYU_RECOMP_HYBRID      OFF CACHE BOOL "" FORCE)
# dynarmic JIT: also disabled.
set(ENABLE_DYNARMIC         OFF CACHE BOOL "" FORCE)

# ── Graphics ──────────────────────────────────────────────────────────────────
# Use the system Vulkan loader (libvulkan.so) rather than building one.
set(VULKAN_SDK              "" CACHE STRING "" FORCE)
set(ENABLE_VULKAN           ON  CACHE BOOL "" FORCE)
# OpenGL ES 3 is used for the shader-compilation fallback only.
set(ENABLE_OPENGL           ON  CACHE BOOL "" FORCE)
# libadrenotools is already a submodule; enable it for Adreno custom drivers.
set(ENABLE_ADRENO_TOOLS     ON  CACHE BOOL "" FORCE)

# ── Audio ─────────────────────────────────────────────────────────────────────
# cubeb can use AAudio on Android (backend selected at runtime).
# SDL3 audio is not used on Android; cubeb is the only backend.
set(USE_SDL2                OFF CACHE BOOL "" FORCE)
set(USE_SDL3                OFF CACHE BOOL "" FORCE)   # SDL has its own entry point; we use GameActivity

# ── Bundled dependencies ──────────────────────────────────────────────────────
set(fmt_FORCE_BUNDLED       ON  CACHE BOOL "" FORCE)
set(OPUS_FORCE_BUNDLED      ON  CACHE BOOL "" FORCE)
# ffmpeg: use the prebuilt Android version from the NDK's sysroot if available,
# or build from source.  The suyu ffmpeg submodule supports Android cross-build.
set(FFMPEG_FORCE_BUNDLED    ON  CACHE BOOL "" FORCE)

# ── Android-specific port flag ────────────────────────────────────────────────
set(TOMODACHI_ANDROID       ON  CACHE BOOL "" FORCE)
