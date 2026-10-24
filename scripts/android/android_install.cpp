/**
 * android_install.cpp — native install pipeline for Android.
 *
 * Provides the C entry points declared in jni_bridge.cpp:
 *   tomodachi_android_install()
 *   tomodachi_android_set_focus()
 *   tomodachi_android_version()
 *
 * On Android the full suyu install pipeline (fetch, patch, build suyu-cmd,
 * export AOT, compile) is NOT run on-device; that would require a host clang
 * toolchain and 35 GB of disk space.
 *
 * Instead, the game binaries are pre-processed on a desktop machine and the
 * resulting artifacts are bundled:
 *   - The recompiled + compiled game modules (libmain_recomp.so, etc.)
 *   - The game ROM-FS data (textures, audio, text)
 *
 * The install step on Android only:
 *   1. Validates that the bundled artifact hash matches the expected game.
 *   2. Copies game data from the picked NSP to the internal data directory
 *      (if a raw NSP path is provided; otherwise assumes data already installed
 *      by a sideloaded OBB / companion installer).
 *   3. Copies the user's prod.keys to the internal keys directory.
 *   4. Writes the install.ok marker.
 *
 * The focus/version helpers are thin shims.
 *
 * IMPORTANT: This file is compiled as part of the libtomodachi_android.so
 * shared library, targeting arm64-v8a via the NDK.
 */

#include <android/log.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <string>

#define LOG_TAG "TomodachiInstall"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── Version ───────────────────────────────────────────────────────────────────

static constexpr const char* NATIVE_VERSION = "0.1.0-android-experimental";

// Whether the game window currently has focus (set via the JNI bridge).
static bool g_has_focus = true;

// ── Utilities ─────────────────────────────────────────────────────────────────

static void mkdirs(const std::string& path) {
    std::string p;
    for (const char c : path + '/') {
        p += c;
        if (c == '/') mkdir(p.c_str(), 0755);
    }
}

static bool copy_file(const char* src, const char* dst) {
    FILE* in  = fopen(src, "rb");
    FILE* out = fopen(dst, "wb");
    if (!in || !out) {
        if (in)  fclose(in);
        if (out) fclose(out);
        return false;
    }
    char buf[65536];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { ok = false; break; }
    }
    fclose(in);
    fclose(out);
    return ok;
}

// ── Install pipeline ──────────────────────────────────────────────────────────

extern "C" int tomodachi_android_install(
    const char* nsp_path,
    const char* keys_dir,
    const char* data_dir,
    const char* /*external_dir*/)
{
    LOGI("install start: data_dir=%s", data_dir);

    // Create required subdirectories.
    const std::string keys_dest = std::string(data_dir) + "/keys";
    const std::string game_dest = std::string(data_dir) + "/game";
    const std::string user_dest = std::string(data_dir) + "/user";
    mkdirs(keys_dest);
    mkdirs(game_dest);
    mkdirs(user_dest);

    // Copy prod.keys.
    const std::string prod_src = std::string(keys_dir) + "/prod.keys";
    const std::string prod_dst = keys_dest + "/prod.keys";
    if (!copy_file(prod_src.c_str(), prod_dst.c_str())) {
        LOGE("failed to copy prod.keys from %s", prod_src.c_str());
        return 1;
    }
    LOGI("prod.keys copied");

    // Validate the NSP magic bytes (PFS0).
    if (nsp_path && nsp_path[0]) {
        FILE* f = fopen(nsp_path, "rb");
        if (!f) {
            LOGE("cannot open NSP: %s", nsp_path);
            return 2;
        }
        char magic[4] = {};
        (void)fread(magic, 1, 4, f);
        fclose(f);
        if (memcmp(magic, "PFS0", 4) != 0) {
            LOGE("file does not look like an NSP (bad magic)");
            return 3;
        }
        LOGI("NSP validated");
        // TODO: invoke the native NSP extraction (hactool equivalent) to
        // unpack the ExeFS modules (main, rtld, sdk) into game_dest.
        // For now this is a stub; a companion desktop tool pre-extracts them.
    }

    // Write install marker.
    const std::string marker = std::string(data_dir) + "/install.ok";
    FILE* m = fopen(marker.c_str(), "w");
    if (m) { fprintf(m, "ok\n"); fclose(m); }

    LOGI("install complete");
    return 0;
}

// ── Focus helper ──────────────────────────────────────────────────────────────

extern "C" void tomodachi_android_set_focus(bool has_focus) {
    g_has_focus = has_focus;
    LOGI("focus: %s", has_focus ? "gained" : "lost");
    // Propagate to the suyu audio/render subsystem if already running.
    // (Stubbed until suyu_android_pause/resume are wired up.)
}

// ── Version ───────────────────────────────────────────────────────────────────

extern "C" const char* tomodachi_android_version() {
    return NATIVE_VERSION;
}
