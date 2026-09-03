#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

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
     * @brief Seeks relative to the current PT3 timeline position.
     *
     * Seeking is reconstructed from the beginning while advancing AY synthesis so
     * envelope/oscillator state stays coherent. The target is clamped to the first
     * complete pass of the module.
     *
     * @param seconds Signed seek distance in seconds.
     * @return true when a seek was performed; false if no active/known timeline exists.
     */
    [[nodiscard]] bool seek_relative_seconds(int seconds, AyChip& primary, AyChip& secondary);

    /**
     * @brief Renders mixed stereo PCM while advancing PT3 state at exact 50 Hz boundaries.
     *
     * For 02TS music the two AY outputs are averaged so music-bus loudness stays
     * comparable with a normal one-chip PT3.
     */
    void render(AyChip& primary, AyChip& secondary, std::span<float> output);

    /** @brief Returns whether a PT3 resource is currently playing. */
    [[nodiscard]] bool active() const noexcept { return active_; }

    /** @brief Returns the number of music AY chips used by the current resource. */
    [[nodiscard]] std::size_t chip_count() const noexcept { return chip_count_; }

    /** @brief Returns the current 50 Hz timeline tick within the first-pass timeline. */
    [[nodiscard]] std::uint32_t current_tick() const noexcept { return current_tick_; }

    /** @brief Returns first-pass duration in 50 Hz ticks, or zero when unknown. */
    [[nodiscard]] std::uint32_t duration_ticks() const noexcept { return duration_ticks_; }

private:
    struct TimelineAnalysis {
        std::uint32_t duration_ticks = 0;
        std::uint32_t loop_tick = 0;
    };

    static constexpr int kFrameRate = 50;
    static constexpr int kRenderChunkFrames = 1024;
    static constexpr int kStereoChannels = 2;
    static constexpr int kRenderChunkSamples = kRenderChunkFrames * kStereoChannels;
    static constexpr std::uint32_t kMaxTimelineAnalysisTicks = 30U * 60U * kFrameRate;

    [[nodiscard]] int setup_decoder(Pt3Music& music);
    [[nodiscard]] TimelineAnalysis analyze_timeline(std::size_t decoder_chips);
    [[nodiscard]] bool seek_to_tick(std::uint32_t target_tick, AyChip& primary,
                                    AyChip& secondary);
    void advance_synthesis_tick(AyChip& primary, AyChip& secondary);
    void advance_timeline_tick() noexcept;
    void tick(AyChip& primary, AyChip& secondary);
    static void apply_registers(int decoder_channel, AyChip& chip);

    int frames_per_tick_ = 0;
    int frames_until_tick_ = 0;
    std::size_t chip_count_ = 0;
    bool active_ = false;
    Pt3Music* current_music_ = nullptr;
    std::uint32_t current_tick_ = 0;
    std::uint32_t duration_ticks_ = 0;
    std::uint32_t loop_tick_ = 0;
    std::array<float, kRenderChunkSamples> secondary_buffer_{};
    std::vector<float> seek_buffer_;
};

}  // namespace w100h::audio
