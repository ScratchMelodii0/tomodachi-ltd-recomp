package com.tomodachi.recomp

/**
 * Thin JNI bridge to the native library (libtomodachi_android.so).
 *
 * All functions declared here correspond to JNIEXPORT symbols defined in
 * src/main/cpp/jni_bridge.cpp.
 */
object TomodachiNative {

    init {
        System.loadLibrary("tomodachi_android")
    }

    /**
     * Run the install pipeline (extract game data, set up runtime directories).
     * Blocking; call from a background thread.
     *
     * @return 0 on success, non-zero on failure.
     */
    external fun install(
        nspPath: String,
        keysDir: String,
        dataDir: String,
        externalDir: String,
    ): Int

    /**
     * Called by GameActivity when window focus changes.
     * Forwards to the native audio/render pause/resume logic.
     */
    external fun onWindowFocusChanged(hasFocus: Boolean)

    /**
     * Query the version string of the native runtime for display in settings.
     */
    external fun nativeVersion(): String
}
