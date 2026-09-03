#pragma once

#include <array>
#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "audio/ay_chip.hpp"
#include "audio/pt3_music.hpp"
#include "audio/pt3_player.hpp"

namespace w100h::audio {

class AudioOutput;

/**
 * @brief User-facing gain and enable state for W100h music playback.
 */
struct AudioMixSettings {
    int master_volume = 80;
    bool music_enabled = true;
    int music_volume = 60;
};

/** @brief Lock-free front-panel snapshot of the currently sounding AY registers. */
struct AyTelemetrySnapshot {
    std::array<std::uint8_t, 6> channel_levels{};
    std::uint8_t chip_count = 0;
    bool noise_active = false;
    bool envelope_active = false;
};

/**
 * @brief Owns PT3/TurboSound synthesis, music resources, and SDL playback.
 *
 * The music path intentionally keeps the proven Mode256 design: up to two
 * virtual AY-3-8910 chips driven by the PT3 sequencer at 50 Hz and rendered
 * to 48 kHz stereo PCM through Ayumi. Game-oriented AYFX/SFX voices are not
 * part of W100h.
 */
class AudioSystem final {
public:
    /**
     * @brief Creates the music AY chips and starts SDL3 audio playback.
     *
     * @param settings Initial master and music bus settings.
     * @throws std::invalid_argument If a volume is outside 0..100.
     * @throws std::runtime_error If the SDL playback device cannot be opened.
     */
    explicit AudioSystem(const AudioMixSettings& settings);

    /** @brief Stops audio playback and releases owned audio resources. */
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) = delete;
    AudioSystem& operator=(AudioSystem&&) = delete;

    /**
     * @brief Loads or replaces one logical PT3 music resource.
     *
     * @param id Stable logical identifier used by play_music().
     * @param path Path to a PT3 or 02TS TurboSound file.
     * @throws std::invalid_argument If id is empty.
     * @throws std::runtime_error If the file cannot be loaded or validated.
     */
    void load_music(std::string id, const std::filesystem::path& path);

    /**
     * @brief Starts loaded PT3 music from the beginning.
     *
     * @param id Logical identifier previously passed to load_music().
     * @return true if the resource exists; false otherwise.
     */
    [[nodiscard]] bool play_music(std::string_view id);

    /** @brief Stops current music and silences both music AY chips. */
    void stop_music();

    /**
     * @brief Pauses or resumes the current music timeline without restarting it.
     * @param paused true to freeze playback; false to continue from the same position.
     */
    void set_music_paused(bool paused);

    /**
     * @brief Applies new master and music bus settings at runtime.
     *
     * @param settings New audio mix settings.
     * @throws std::invalid_argument If a volume is outside 0..100.
     * @throws std::runtime_error If the SDL stream cannot be synchronized.
     */
    void set_mix_settings(const AudioMixSettings& settings);

    /**
     * @brief Returns the latest lock-free AY front-panel telemetry snapshot.
     *
     * The audio callback publishes programmed channel/envelope levels after synthesis;
     * callers never lock or touch realtime decoder state.
     */
    [[nodiscard]] AyTelemetrySnapshot telemetry_snapshot() const noexcept;

private:
    class StreamLock final {
    public:
        explicit StreamLock(AudioOutput& output);
        ~StreamLock();

        StreamLock(const StreamLock&) = delete;
        StreamLock& operator=(const StreamLock&) = delete;

    private:
        AudioOutput& output_;
    };

    static constexpr int kSampleRate = 48'000;
    static constexpr int kChannelCount = 2;
    static constexpr int kChunkFrames = 1024;
    static constexpr int kChunkSamples = kChunkFrames * kChannelCount;

    static void render_callback(void* userdata, std::span<float> output);
    static void validate_mix_settings(const AudioMixSettings& settings);
    void render(std::span<float> output);
    void publish_telemetry() noexcept;
    void clear_telemetry() noexcept;

    AyChip music_primary_{kSampleRate};
    AyChip music_secondary_{kSampleRate};
    Pt3Player music_player_{kSampleRate};
    std::map<std::string, Pt3Music, std::less<>> music_catalog_;
    float master_gain_ = 0.8F;
    float music_gain_ = 0.6F;
    bool music_enabled_ = true;
    bool music_paused_ = false;
    std::array<float, kChunkSamples> music_buffer_{};
    std::array<std::atomic<std::uint8_t>, 6> meter_levels_{};
    std::atomic<std::uint8_t> meter_chip_count_{0};
    std::atomic<bool> meter_noise_active_{false};
    std::atomic<bool> meter_envelope_active_{false};
    std::unique_ptr<AudioOutput> output_;
};

}  // namespace w100h::audio
