#pragma once
/**
 * AndroidWindow — Vulkan surface lifecycle wrapper.
 *
 * Wraps the ANativeWindow pointer provided by GameActivity and manages the
 * VkSurfaceKHR lifecycle.  The suyu Vulkan backend creates a surface via
 * vkCreateAndroidSurfaceKHR(); this class provides and invalidates the
 * window handle when the Android surface is created/destroyed.
 */

#include <android/native_window.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <atomic>

class AndroidWindow {
public:
    AndroidWindow() = default;
    ~AndroidWindow() { detach(); }

    // Called on APP_CMD_INIT_WINDOW.
    void attach(ANativeWindow* window);
    // Called on APP_CMD_TERM_WINDOW.
    void detach();

    // True when a window is present and the Vulkan surface is valid.
    bool ready() const { return window_ != nullptr; }

    // The raw ANativeWindow — passed to vkCreateAndroidSurfaceKHR.
    ANativeWindow* native_window() const { return window_; }

    // Surface dimensions (updated on attach).
    int width()  const { return width_;  }
    int height() const { return height_; }

private:
    ANativeWindow*     window_  = nullptr;
    int                width_   = 0;
    int                height_  = 0;
};
