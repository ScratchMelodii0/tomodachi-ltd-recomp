#pragma once
/**
 * AndroidAudio — AAudio-based audio backend for the Android port.
 *
 * Replaces suyu's cubeb/SDL audio backend on Android.  The game produces
 * signed 16-bit stereo PCM at 48 kHz; this class opens a low-latency AAudio
 * stream and writes into it from a callback on AAudio's own thread.
 *
 * Thread-safety: start/pause/resume/stop are called from the main thread;
 * the data callback runs on AAudio's internal thread.  The ring buffer
 * between them is lock-free.
 */

#include <aaudio/AAudio.h>
#include <atomic>
#include <cstdint>
#include <vector>

class AndroidAudio {
public:
    static constexpr int    SAMPLE_RATE     = 48000;
    static constexpr int    CHANNEL_COUNT   = 2;
    static constexpr int    FRAMES_PER_BURST = 512;
    static constexpr size_t RING_FRAMES     = 8192;   // ~170 ms buffer

    AndroidAudio();
    ~AndroidAudio();

    // Called once the window is ready.
    void start();
    // Called on APP_CMD_LOST_FOCUS.
    void pause();
    // Called on APP_CMD_GAINED_FOCUS.
    void resume();
    // Called on shutdown.
    void stop();

    // Called by suyu's audio thread to push PCM frames.
    // Returns the number of frames actually written (may be less if the ring
    // buffer is full — the caller should drop or retry).
    size_t write(const int16_t* frames, size_t frame_count);

private:
    static aaudio_data_callback_result_t data_callback(
        AAudioStream* stream,
        void*         user_data,
        void*         audio_data,
        int32_t       num_frames);

    static void error_callback(
        AAudioStream* stream,
        void*         user_data,
        aaudio_result_t error);

    void open_stream();
    void close_stream();

    AAudioStream*          stream_   = nullptr;
    std::atomic<bool>      active_   = false;

    // Lock-free ring buffer (power-of-two samples for easy masking).
    static constexpr size_t RING_SAMPLES = RING_FRAMES * CHANNEL_COUNT;
    std::vector<int16_t>   ring_;
    std::atomic<size_t>    write_pos_ = 0;
    std::atomic<size_t>    read_pos_  = 0;
};
