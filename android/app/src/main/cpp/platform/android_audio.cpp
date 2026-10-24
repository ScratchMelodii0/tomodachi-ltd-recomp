/**
 * android_audio.cpp — AAudio backend implementation.
 *
 * Design:
 *  - suyu's audio output thread calls write() with 48 kHz stereo s16 frames.
 *  - AAudio's callback thread calls data_callback(), which drains the ring.
 *  - If the ring underruns, data_callback() outputs silence.
 *  - If the ring overruns, write() drops the oldest data.
 */

#include "android_audio.h"
#include <android/log.h>
#include <cstring>
#include <algorithm>

#define LOG_TAG "AndroidAudio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidAudio::AndroidAudio() : ring_(RING_SAMPLES, 0) {}

AndroidAudio::~AndroidAudio() {
    stop();
}

void AndroidAudio::open_stream() {
    AAudioStreamBuilder* builder = nullptr;
    AAudio_createStreamBuilder(&builder);

    AAudioStreamBuilder_setDirection(builder,       AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder,     AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setFormat(builder,          AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSampleRate(builder,      SAMPLE_RATE);
    AAudioStreamBuilder_setChannelCount(builder,    CHANNEL_COUNT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder,    data_callback, this);
    AAudioStreamBuilder_setErrorCallback(builder,   error_callback, this);

    const aaudio_result_t res = AAudioStreamBuilder_openStream(builder, &stream_);
    AAudioStreamBuilder_delete(builder);

    if (res != AAUDIO_OK) {
        LOGE("AAudioStreamBuilder_openStream failed: %s", AAudio_convertResultToText(res));
        stream_ = nullptr;
    } else {
        LOGI("AAudio stream opened: rate=%d ch=%d burst=%d",
             AAudioStream_getSampleRate(stream_),
             AAudioStream_getChannelCount(stream_),
             AAudioStream_getFramesPerBurst(stream_));
    }
}

void AndroidAudio::close_stream() {
    if (stream_) {
        AAudioStream_close(stream_);
        stream_ = nullptr;
    }
}

void AndroidAudio::start() {
    if (active_.load()) return;
    open_stream();
    if (stream_) {
        write_pos_.store(0);
        read_pos_.store(0);
        AAudioStream_requestStart(stream_);
        active_.store(true);
        LOGI("audio started");
    }
}

void AndroidAudio::pause() {
    if (!active_.load()) return;
    if (stream_) AAudioStream_requestPause(stream_);
    LOGI("audio paused");
}

void AndroidAudio::resume() {
    if (!active_.load()) return;
    if (stream_) AAudioStream_requestStart(stream_);
    LOGI("audio resumed");
}

void AndroidAudio::stop() {
    if (!active_.load()) return;
    active_.store(false);
    if (stream_) {
        AAudioStream_requestStop(stream_);
        close_stream();
    }
    LOGI("audio stopped");
}

size_t AndroidAudio::write(const int16_t* frames, size_t frame_count) {
    const size_t samples = frame_count * CHANNEL_COUNT;
    size_t written = 0;

    for (size_t i = 0; i < samples; ++i) {
        const size_t wp = write_pos_.load(std::memory_order_relaxed);
        const size_t rp = read_pos_.load(std::memory_order_acquire);
        const size_t used = (wp - rp) & (RING_SAMPLES - 1);
        if (used >= RING_SAMPLES - 1) break;   // ring full: drop remaining

        ring_[wp & (RING_SAMPLES - 1)] = frames[i];
        write_pos_.store((wp + 1) & (RING_SAMPLES - 1), std::memory_order_release);
        ++written;
    }
    return written / CHANNEL_COUNT;
}

// ── AAudio callbacks ──────────────────────────────────────────────────────────

aaudio_data_callback_result_t AndroidAudio::data_callback(
    AAudioStream* /*stream*/,
    void*         user_data,
    void*         audio_data,
    int32_t       num_frames)
{
    auto* self    = static_cast<AndroidAudio*>(user_data);
    auto* out     = static_cast<int16_t*>(audio_data);
    const size_t  samples = static_cast<size_t>(num_frames) * CHANNEL_COUNT;

    for (size_t i = 0; i < samples; ++i) {
        const size_t rp = self->read_pos_.load(std::memory_order_relaxed);
        const size_t wp = self->write_pos_.load(std::memory_order_acquire);
        if (rp == wp) {
            // Underrun: silence.
            out[i] = 0;
        } else {
            out[i] = self->ring_[rp & (RING_SAMPLES - 1)];
            self->read_pos_.store((rp + 1) & (RING_SAMPLES - 1), std::memory_order_release);
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AndroidAudio::error_callback(
    AAudioStream* /*stream*/,
    void*         user_data,
    aaudio_result_t error)
{
    LOGE("AAudio error: %s", AAudio_convertResultToText(error));
    // Attempt stream recovery.
    auto* self = static_cast<AndroidAudio*>(user_data);
    self->close_stream();
    self->open_stream();
    if (self->stream_) AAudioStream_requestStart(self->stream_);
}
