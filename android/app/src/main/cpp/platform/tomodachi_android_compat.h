#pragma once
/**
 * tomodachi_android_compat.h
 *
 * Included by suyu source files that are compiled in the Android build
 * (guarded by #include "tomodachi_android_compat.h").  Defines the
 * TOMODACHI_ANDROID macro and provides minimal stubs for desktop-only
 * symbols that are otherwise unreachable on Android.
 *
 * This header is found via the include path injected by the Android
 * CMakeLists.txt:
 *   target_include_directories(...  platform  ...)
 *
 * It must not pull in any Android SDK headers at the top level, because
 * suyu source files may include it before their own system includes.
 */

// ── Platform tag ──────────────────────────────────────────────────────────────
#if !defined(TOMODACHI_ANDROID)
#  define TOMODACHI_ANDROID 1
#endif

// ── Path injection API (implemented in android_storage.cpp) ───────────────────
#ifdef __cplusplus
#include <filesystem>
namespace Common::FS {
    enum class UserPath;
    std::filesystem::path GetAndroidPath(UserPath user_path);
    void android_path_init(const char* data_dir, const char* external_dir);
} // namespace Common::FS
#endif // __cplusplus

// ── ANativeWindow injection (implemented in android_window.cpp) ───────────────
struct ANativeWindow;
#ifdef __cplusplus
extern "C" {
#endif
void suyu_android_set_native_window(ANativeWindow* window);
#ifdef __cplusplus
}
#endif

// ── suyu-cmd Android entry points (declared for android_main.cpp) ─────────────
#ifdef __cplusplus
extern "C" {
#endif
int  suyu_android_init(const char* data_dir, const char* game_path);
void suyu_android_run_frame(void);
void suyu_android_pause(void);
void suyu_android_resume(void);
void suyu_android_shutdown(void);
#ifdef __cplusplus
}
#endif
