/**
 * android_main.cpp — GameActivity entry point.
 *
 * Called by the GameActivity C runtime on the game thread after the Android
 * window has been created and is ready to render.  This replaces the desktop
 * main() / SDL event loop.
 *
 * Responsibilities:
 *  - Set up the Vulkan surface from the ANativeWindow provided by GameActivity.
 *  - Start the audio backend (AAudio).
 *  - Forward Android lifecycle events (pause/resume/destroy) to the suyu runtime.
 *  - Run the suyu-cmd main loop (recompiled game execution).
 *  - Forward touch events as emulated controller input.
 */

#include <android/log.h>
#include <game-activity/GameActivity.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "platform/android_audio.h"
#include "platform/android_input.h"
#include "platform/android_window.h"
#include "platform/android_storage.h"

#define LOG_TAG "TomodachiNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ── Forward declarations for suyu-cmd entry points ────────────────────────────
// These are resolved at link time from the suyu-cmd-static archive that
// cmake/android_suyu.cmake adds to this project.
extern "C" {
    int  suyu_android_init(const char* data_dir, const char* game_path);
    void suyu_android_run_frame();
    void suyu_android_pause();
    void suyu_android_resume();
    void suyu_android_shutdown();
}

// ── Internal state ────────────────────────────────────────────────────────────

struct AppState {
    bool             running      = false;
    bool             paused       = false;
    AndroidWindow    window;
    AndroidAudio     audio;
    AndroidInput     input;
    std::string      data_dir;
    std::string      game_path;
};

static AppState g_app;

// ── GameActivity / native_app_glue callbacks ──────────────────────────────────

static void handle_cmd(android_app* app, int32_t cmd) {
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        LOGI("APP_CMD_INIT_WINDOW");
        if (app->window) {
            g_app.window.attach(app->window);
            if (!g_app.running) {
                // First window: initialise the runtime and start the game.
                const int rc = suyu_android_init(g_app.data_dir.c_str(),
                                                  g_app.game_path.c_str());
                if (rc != 0) {
                    LOGE("suyu_android_init failed: %d", rc);
                    ANativeActivity_finish(app->activity);
                    return;
                }
                g_app.audio.start();
                g_app.running = true;
            }
        }
        break;

    case APP_CMD_TERM_WINDOW:
        LOGI("APP_CMD_TERM_WINDOW");
        g_app.window.detach();
        break;

    case APP_CMD_GAINED_FOCUS:
        LOGI("APP_CMD_GAINED_FOCUS");
        if (g_app.paused) {
            suyu_android_resume();
            g_app.audio.resume();
            g_app.paused = false;
        }
        break;

    case APP_CMD_LOST_FOCUS:
        LOGI("APP_CMD_LOST_FOCUS");
        if (!g_app.paused) {
            suyu_android_pause();
            g_app.audio.pause();
            g_app.paused = true;
        }
        break;

    case APP_CMD_DESTROY:
        LOGI("APP_CMD_DESTROY");
        g_app.running = false;
        break;

    default:
        break;
    }
}

static int32_t handle_input(android_app* /*app*/, AInputEvent* event) {
    return g_app.input.process(event) ? 1 : 0;
}

// ── Entry point ───────────────────────────────────────────────────────────────

/**
 * android_main is called by the GameActivity native glue on the game thread.
 * It must not return until the application is ready to exit.
 */
void android_main(android_app* app) {
    LOGI("android_main start");

    // Retrieve paths written by SetupActivity / the Java install flow.
    g_app.data_dir  = AndroidStorage::data_dir(app);
    g_app.game_path = g_app.data_dir + "/game/main";   // extracted NSO main module

    app->onAppCmd    = handle_cmd;
    app->onInputEvent = handle_input;

    // Main loop: pump Android events and run one game frame per iteration.
    while (!app->destroyRequested) {
        int                  events;
        android_poll_source* source;

        // Poll with a timeout of 0 when running, or block when paused.
        const int timeout = (g_app.running && !g_app.paused) ? 0 : -1;
        while (ALooper_pollOnce(timeout, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) break;
        }

        if (g_app.running && !g_app.paused && g_app.window.ready()) {
            suyu_android_run_frame();
        }
    }

    LOGI("android_main: shutting down");
    if (g_app.running) {
        g_app.audio.stop();
        suyu_android_shutdown();
    }
    LOGI("android_main: done");
}
