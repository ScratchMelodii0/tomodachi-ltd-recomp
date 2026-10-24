#pragma once
/**
 * AndroidInput — touch-to-controller mapping for the Android port.
 *
 * Maps multi-touch events from AInputEvent to Nintendo Switch Pro Controller
 * button/stick states that the suyu runtime expects.
 *
 * Layout (landscape, 1280×720 canonical):
 *
 *   Left half:          Right half:
 *   ┌──────────┐        ┌──────────┐
 *   │  ↑       │        │    Y     │
 *   │ ← ○ →   │        │  X   A   │
 *   │  ↓       │        │    B     │
 *   │  [L-stick]│       │  [R-stick]│
 *   │  ZL L   │        │  ZR R    │
 *   │  -       │        │  +       │
 *   └──────────┘        └──────────┘
 *
 * Each "button" is a fixed rectangle zone; the two sticks are drag areas that
 * output normalised [-1, 1] axis values.
 */

#include <android/input.h>
#include <cstdint>
#include <array>

// Switch Pro Controller button bitmask (matches suyu HID mapping).
enum class SwitchButton : uint64_t {
    A        = 1ULL << 0,
    B        = 1ULL << 1,
    X        = 1ULL << 2,
    Y        = 1ULL << 3,
    LStick   = 1ULL << 4,
    RStick   = 1ULL << 5,
    L        = 1ULL << 6,
    R        = 1ULL << 7,
    ZL       = 1ULL << 8,
    ZR       = 1ULL << 9,
    Plus     = 1ULL << 10,
    Minus    = 1ULL << 11,
    DLeft    = 1ULL << 12,
    DUp      = 1ULL << 13,
    DRight   = 1ULL << 14,
    DDown    = 1ULL << 15,
};

struct ControllerState {
    uint64_t buttons = 0;
    float    left_x  = 0.0f;
    float    left_y  = 0.0f;
    float    right_x = 0.0f;
    float    right_y = 0.0f;
};

class AndroidInput {
public:
    AndroidInput();

    // Returns true if the event was consumed.
    bool process(AInputEvent* event);

    // Read the current controller state (called each frame by android_main).
    ControllerState state() const;

    // Allow the touch layout to be rescaled when the window size is known.
    void set_surface_size(int width, int height);

private:
    void handle_touch_event(AInputEvent* event);

    // Active touch points (up to 10 pointers).
    struct TouchPoint {
        bool  active = false;
        float x      = 0.0f;
        float y      = 0.0f;
    };
    std::array<TouchPoint, 10> touches_{};

    // Mutable so state() can be const.
    mutable ControllerState cached_state_{};

    float surface_w_ = 1280.0f;
    float surface_h_ = 720.0f;

    void recompute_state();
};
