cmake_minimum_required(VERSION 3.22.1)
project(TomodachiAndroid LANGUAGES C CXX)

# ── Sanity check ────────────────────────────────────────────────────────────────
if(NOT ANDROID)
    message(FATAL_ERROR "This CMakeLists.txt is only for the Android (NDK) build.")
endif()
if(NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "Only arm64-v8a is supported; got '${CMAKE_ANDROID_ARCH_ABI}'.")
endif()

# ── Paths ────────────────────────────────────────────────────────────────────────
# Resolve the root of the repository (four levels up from this file:
#   android/app/src/main/cpp/CMakeLists.txt → repo root)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
set(UPSTREAM_ROOT "${REPO_ROOT}/upstream/mk8-recomp")
set(SUYU_ROOT     "${UPSTREAM_ROOT}/third_party/suyu")

# ── C++ standard ────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Android NDK libraries (always available) ─────────────────────────────────────
find_library(log-lib    log)
find_library(android-lib android)
find_library(EGL-lib    EGL)
find_library(GLESv3-lib GLESv3)

# ── Optional: Vulkan from the NDK headers ────────────────────────────────────────
# The Vulkan loader on Android is the system libvulkan.so; we link against it directly.
find_library(vulkan-lib vulkan)

# ── GameActivity support library ─────────────────────────────────────────────────
# Provided by the Gradle dependency androidx.games:games-activity; the prefab AAR
# exposes the CMake package.
find_package(games-activity REQUIRED CONFIG)

# ── fmt (header-only bundled) ────────────────────────────────────────────────────
# Suyu bundles fmt; reuse the same source.
add_subdirectory("${SUYU_ROOT}/externals/fmt" "${CMAKE_BINARY_DIR}/fmt" EXCLUDE_FROM_ALL)

# ── AAudio backend ───────────────────────────────────────────────────────────────
find_library(aaudio-lib aaudio)

# ── Our platform adaptation library ─────────────────────────────────────────────
add_library(tomodachi_platform STATIC
    platform/android_audio.cpp
    platform/android_input.cpp
    platform/android_window.cpp
    platform/android_storage.cpp
)
target_include_directories(tomodachi_platform PUBLIC
    platform
    "${SUYU_ROOT}/src"
    "${SUYU_ROOT}/externals/fmt/include"
)
target_link_libraries(tomodachi_platform
    ${log-lib}
    ${android-lib}
    ${aaudio-lib}
    ${EGL-lib}
    ${vulkan-lib}
    fmt::fmt
)
target_compile_definitions(tomodachi_platform PUBLIC
    TOMODACHI_ANDROID=1
    # Suppress desktop-only platform paths inside suyu headers.
    SUYU_ANDROID=1
)

# ── JNI bridge shared library ────────────────────────────────────────────────────
# This is the .so that Gradle packages and that GameActivity loads via
#   android:name="android.app.lib_name" value="tomodachi_android"
add_library(tomodachi_android SHARED
    jni_bridge.cpp
    android_main.cpp
)
target_include_directories(tomodachi_android PRIVATE
    platform
    "${SUYU_ROOT}/src"
    "${SUYU_ROOT}/externals/fmt/include"
)
target_link_libraries(tomodachi_android
    tomodachi_platform
    games-activity::games-activity
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
    ${vulkan-lib}
    ${aaudio-lib}
    fmt::fmt
)

# ── Compiler flags ───────────────────────────────────────────────────────────────
target_compile_options(tomodachi_android PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    # ARM64 performance: use NEON; align stacks correctly.
    -march=armv8-a
    -fvisibility=hidden
    # Keep frame pointers for Android's profiler.
    -fno-omit-frame-pointer
)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(tomodachi_android PRIVATE -O2 -DNDEBUG)
else()
    target_compile_options(tomodachi_android PRIVATE -O0 -g)
endif()
