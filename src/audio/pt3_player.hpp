#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace w100h::audio {

class AyChip;
class Pt3Music;

/**
 * @brief Drives one PT3 or 02TS TurboSound resource at the Spectrum 50 Hz music rate.
 *
 * The PT3 decoder only updates AY register state. AyChip remains responsible for
 * continuous PCM synthesis at the SDL sample rate.
 */
class Pt3Player final {
public:
    /**
     * @brief Creates a PT3 sequencer for the requested PCM sample rate.
     *
     * @param sample_rate PCM sample rate. It must be divisible by 50.
     * @throws std::invalid_argument If the sample rate cannot represent exact 50 Hz ticks.
     */
    explicit Pt3Player(int sample_rate);

    /**
     * @brief Starts a validated PT3 resource from its first position.
     *
     * Playback loops automatically through the PT3 module's embedded loop position.
     * A 02TS resource uses both supplied AY chips; a normal PT3 uses only primary.
     *
     * @throws std::runtime_error If the upstream decoder rejects the resource.
     */
    void start(Pt3Music& music, AyChip& primary, AyChip& secondary);

    /**
     * @brief Stops playback and resets both music AY chips to silence.
     */
    void stop(AyChip& primary, AyChip& secondary);

    /**
     * @brief Renders mixed stereo PCM while advancing PT3 state at exact 50 Hz boundaries.
     *
     * For 02TS music the two AY outputs are averaged so music-bus loudness stays
     * comparable with a normal one-chip PT3.
     */
    void render(AyChip& primary, AyChip& secondary, std::span<float> output);

    /**
     * @brief Returns whether a PT3 resource is currently playing.
     */
    [[nodiscard]] bool active() const noexcept { return active_; }

    /**
     * @brief Returns the number of music AY chips used by the current resource.
     */
    [[nodiscard]] std::size_t chip_count() const noexcept { return chip_count_; }

private:
    static constexpr int kFrameRate = 50;
    static constexpr int kRenderChunkFrames = 1024;
    static constexpr int kStereoChannels = 2;
    static constexpr int kRenderChunkSamples = kRenderChunkFrames * kStereoChannels;

    void tick(AyChip& primary, AyChip& secondary);
    static void apply_registers(int decoder_channel, AyChip& chip);

    int frames_per_tick_ = 0;
    int frames_until_tick_ = 0;
    std::size_t chip_count_ = 0;
    bool active_ = false;
    std::array<float, kRenderChunkSamples> secondary_buffer_{};
};

}  // namespace w100h::audio
