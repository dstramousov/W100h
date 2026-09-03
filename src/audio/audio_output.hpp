#pragma once

#include <SDL3/SDL_audio.h>

#include <array>
#include <span>

namespace w100h::audio {

/**
 * @brief Owns one SDL3 playback stream and requests PCM from a realtime-safe renderer callback.
 */
class AudioOutput final {
public:
    /**
     * @brief Function pointer used to render interleaved stereo float PCM.
     *
     * @param userdata Opaque owner pointer supplied to the constructor.
     * @param output Destination sample buffer in L,R,L,R order.
     */
    using RenderCallback = void (*)(void* userdata, std::span<float> output);

    /**
     * @brief Opens and starts the default SDL3 playback device.
     *
     * @param sample_rate Application-side audio sample rate in Hz.
     * @param render_callback Realtime renderer invoked from SDL's audio thread.
     * @param userdata Opaque pointer forwarded to render_callback.
     * @throws std::invalid_argument If arguments are invalid.
     * @throws std::runtime_error If SDL cannot open or resume the playback stream.
     */
    AudioOutput(int sample_rate, RenderCallback render_callback, void* userdata);

    /**
     * @brief Stops playback and releases the SDL audio stream and logical device.
     */
    ~AudioOutput();

    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;
    AudioOutput(AudioOutput&&) = delete;
    AudioOutput& operator=(AudioOutput&&) = delete;

    /**
     * @brief Locks the SDL audio stream so its callback cannot run concurrently.
     *
     * @throws std::runtime_error If SDL cannot lock the stream.
     */
    void lock();

    /**
     * @brief Unlocks a stream previously locked by the current thread.
     */
    void unlock() noexcept;

private:
    static constexpr int kChannelCount = 2;
    static constexpr int kChunkFrames = 1024;
    static constexpr int kChunkSamples = kChunkFrames * kChannelCount;

    static void SDLCALL stream_callback(void* userdata, SDL_AudioStream* stream,
                                        int additional_amount, int total_amount);
    void feed(SDL_AudioStream* stream, int additional_amount) noexcept;

    SDL_AudioStream* stream_ = nullptr;
    RenderCallback render_callback_ = nullptr;
    void* userdata_ = nullptr;
    std::array<float, kChunkSamples> buffer_{};
};

}  // namespace w100h::audio
