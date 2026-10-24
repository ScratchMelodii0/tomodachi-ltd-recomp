/**
 * android_window.cpp — Vulkan surface lifecycle implementation.
 */

#include "android_window.h"
#include <android/log.h>
#include <android/native_window.h>

#define LOG_TAG "AndroidWindow"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

void AndroidWindow::attach(ANativeWindow* window) {
    if (window_ == window) return;
    detach();
    window_  = window;
    width_   = ANativeWindow_getWidth(window);
    height_  = ANativeWindow_getHeight(window);
    LOGI("window attached: %d×%d", width_, height_);
}

void AndroidWindow::detach() {
    if (!window_) return;
    LOGI("window detached");
    window_ = nullptr;
    width_  = 0;
    height_ = 0;
}
