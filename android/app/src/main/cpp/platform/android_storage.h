#pragma once
/**
 * AndroidStorage — path utilities for Android scoped-storage layout.
 *
 * Provides helper functions for resolving app-specific paths from the
 * android_app struct, replacing the desktop filesystem assumptions in the
 * suyu runtime.
 *
 * Android storage layout used by this port:
 *
 *   Internal (Context.getFilesDir):
 *     <data_dir>/game/       — extracted game binaries (main, rtld, sdk NSOs)
 *     <data_dir>/keys/       — prod.keys (written by SetupActivity)
 *     <data_dir>/user/       — save data, NAND layout
 *     <data_dir>/install.ok  — marker file; its presence means the install
 *                              pipeline has run successfully.
 *
 *   External (Context.getExternalFilesDir):
 *     <ext_dir>/logs/        — runtime logs (accessible via adb pull)
 *     <ext_dir>/screenshots/ — screenshots
 *
 * These paths are set once on startup by SetupActivity and passed to
 * android_main() through the android_app->activity->internalDataPath and
 * android_app->activity->externalDataPath fields.
 */

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <string>

namespace AndroidStorage {

// Returns the internal data directory (Context.getFilesDir).
inline std::string data_dir(android_app* app) {
    return app->activity->internalDataPath
               ? app->activity->internalDataPath
               : "";
}

// Returns the external data directory (Context.getExternalFilesDir).
inline std::string external_dir(android_app* app) {
    return app->activity->externalDataPath
               ? app->activity->externalDataPath
               : data_dir(app);
}

// Returns the path to the extracted main NSO binary.
inline std::string game_path(android_app* app) {
    return data_dir(app) + "/game/main";
}

// Returns the directory containing prod.keys.
inline std::string keys_dir(android_app* app) {
    return data_dir(app) + "/keys";
}

// Returns the user data directory (saves, NAND layout).
inline std::string user_dir(android_app* app) {
    return data_dir(app) + "/user";
}

// Returns the log directory.
inline std::string log_dir(android_app* app) {
    return external_dir(app) + "/logs";
}

} // namespace AndroidStorage
