/**
 * android_input.cpp — touch-to-controller implementation.
 *
 * Zone geometry is defined in a 1280×720 normalised coordinate system and
 * scaled to the actual surface dimensions at runtime.
 */

#include "android_input.h"
#include <android/log.h>
#include <cmath>

#define LOG_TAG "AndroidInput"

// ── Touch zone definitions ─────────────────────────────────────────────────────
// All coordinates are in the 1280×720 canonical space.

struct Rect { float x, y, w, h; };

static bool in_rect(float px, float py, const Rect& r) {
    return px >= r.x && px < r.x + r.w &&
           py >= r.y && py < r.y + r.h;
}

// D-pad
static constexpr Rect ZONE_DUP   = { 50,  80, 80, 80  };
static constexpr Rect ZONE_DDOWN = { 50, 240, 80, 80  };
static constexpr Rect ZONE_DLEFT = {  0, 160, 80, 80  };
static constexpr Rect ZONE_DRIGHT= {130, 160, 80, 80  };

// Face buttons
static constexpr Rect ZONE_A     = {1150, 160, 80, 80 };
static constexpr Rect ZONE_B     = {1070, 240, 80, 80 };
static constexpr Rect ZONE_X     = {1070,  80, 80, 80 };
static constexpr Rect ZONE_Y     = { 990, 160, 80, 80 };

// Shoulder buttons
static constexpr Rect ZONE_L     = {   0,   0, 160, 50 };
static constexpr Rect ZONE_ZL    = {   0,   0,  80, 30 };
static constexpr Rect ZONE_R     = {1120,   0, 160, 50 };
static constexpr Rect ZONE_ZR    = {1200,   0,  80, 30 };

// Menu buttons
static constexpr Rect ZONE_PLUS  = { 700,  10,  60, 40 };
static constexpr Rect ZONE_MINUS = { 520,  10,  60, 40 };

// Analog stick drag areas (center + radius)
static constexpr float LSTICK_CX = 200.0f, LSTICK_CY = 480.0f, LSTICK_R = 100.0f;
static constexpr float RSTICK_CX =1080.0f, RSTICK_CY = 480.0f, RSTICK_R = 100.0f;

// ── AndroidInput ─────────────────────────────────────────────────────────────

AndroidInput::AndroidInput() = default;

void AndroidInput::set_surface_size(int width, int height) {
    surface_w_ = static_cast<float>(width);
    surface_h_ = static_cast<float>(height);
}

bool AndroidInput::process(AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
        return false;
    handle_touch_event(event);
    return true;
}

void AndroidInput::handle_touch_event(AInputEvent* event) {
    const int32_t action      = AMotionEvent_getAction(event);
    const int32_t action_code = action & AMOTION_EVENT_ACTION_MASK;
    const int32_t pointer_idx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                                 >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    const size_t count = AMotionEvent_getPointerCount(event);

    for (size_t i = 0; i < count && i < touches_.size(); ++i) {
        const int32_t pid = AMotionEvent_getPointerId(event, i);
        if (pid < 0 || static_cast<size_t>(pid) >= touches_.size()) continue;

        const float raw_x = AMotionEvent_getX(event, i);
        const float raw_y = AMotionEvent_getY(event, i);
        // Map to 1280×720 canonical space.
        const float cx = raw_x * 1280.0f / surface_w_;
        const float cy = raw_y *  720.0f / surface_h_;

        if (action_code == AMOTION_EVENT_ACTION_UP
         || action_code == AMOTION_EVENT_ACTION_CANCEL
         || (action_code == AMOTION_EVENT_ACTION_POINTER_UP
             && static_cast<int32_t>(i) == pointer_idx)) {
            touches_[pid].active = false;
        } else {
            touches_[pid] = { true, cx, cy };
        }
    }

    if (action_code == AMOTION_EVENT_ACTION_UP
     || action_code == AMOTION_EVENT_ACTION_CANCEL) {
        for (auto& t : touches_) t.active = false;
    }

    recompute_state();
}

void AndroidInput::recompute_state() {
    ControllerState s{};

    for (const auto& t : touches_) {
        if (!t.active) continue;
        const float x = t.x, y = t.y;

        // D-pad
        if (in_rect(x, y, ZONE_DUP))    s.buttons |= static_cast<uint64_t>(SwitchButton::DUp);
        if (in_rect(x, y, ZONE_DDOWN))  s.buttons |= static_cast<uint64_t>(SwitchButton::DDown);
        if (in_rect(x, y, ZONE_DLEFT))  s.buttons |= static_cast<uint64_t>(SwitchButton::DLeft);
        if (in_rect(x, y, ZONE_DRIGHT)) s.buttons |= static_cast<uint64_t>(SwitchButton::DRight);
        // Face buttons
        if (in_rect(x, y, ZONE_A))      s.buttons |= static_cast<uint64_t>(SwitchButton::A);
        if (in_rect(x, y, ZONE_B))      s.buttons |= static_cast<uint64_t>(SwitchButton::B);
        if (in_rect(x, y, ZONE_X))      s.buttons |= static_cast<uint64_t>(SwitchButton::X);
        if (in_rect(x, y, ZONE_Y))      s.buttons |= static_cast<uint64_t>(SwitchButton::Y);
        // Shoulder
        if (in_rect(x, y, ZONE_L))      s.buttons |= static_cast<uint64_t>(SwitchButton::L);
        if (in_rect(x, y, ZONE_ZL))     s.buttons |= static_cast<uint64_t>(SwitchButton::ZL);
        if (in_rect(x, y, ZONE_R))      s.buttons |= static_cast<uint64_t>(SwitchButton::R);
        if (in_rect(x, y, ZONE_ZR))     s.buttons |= static_cast<uint64_t>(SwitchButton::ZR);
        // Menu
        if (in_rect(x, y, ZONE_PLUS))   s.buttons |= static_cast<uint64_t>(SwitchButton::Plus);
        if (in_rect(x, y, ZONE_MINUS))  s.buttons |= static_cast<uint64_t>(SwitchButton::Minus);

        // Left stick drag
        const float ldx = x - LSTICK_CX, ldy = y - LSTICK_CY;
        const float ld = std::sqrt(ldx * ldx + ldy * ldy);
        if (ld < LSTICK_R) {
            s.left_x += ldx / LSTICK_R;
            s.left_y += ldy / LSTICK_R;
        }
        // Right stick drag
        const float rdx = x - RSTICK_CX, rdy = y - RSTICK_CY;
        const float rd = std::sqrt(rdx * rdx + rdy * rdy);
        if (rd < RSTICK_R) {
            s.right_x += rdx / RSTICK_R;
            s.right_y += rdy / RSTICK_R;
        }
    }

    // Clamp stick axes to [-1, 1].
    auto clamp = [](float v) { return v < -1.0f ? -1.0f : v > 1.0f ? 1.0f : v; };
    s.left_x  = clamp(s.left_x);
    s.left_y  = clamp(s.left_y);
    s.right_x = clamp(s.right_x);
    s.right_y = clamp(s.right_y);

    cached_state_ = s;
}

ControllerState AndroidInput::state() const {
    return cached_state_;
}
