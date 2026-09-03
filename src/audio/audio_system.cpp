#include "audio/audio_system.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

#include "audio/audio_output.hpp"

namespace w100h::audio {
namespace {

constexpr float kOutputHeadroom = 0.75F;

[[nodiscard]] float volume_to_gain(int volume) {
    return static_cast<float>(volume) / 100.0F;
}

}  // namespace

AudioSystem::StreamLock::StreamLock(AudioOutput& output) : output_{output} { output_.lock(); }

AudioSystem::StreamLock::~StreamLock() { output_.unlock(); }

AudioSystem::AudioSystem(const AudioMixSettings& settings) {
    validate_mix_settings(settings);
    master_gain_ = volume_to_gain(settings.master_volume);
    music_gain_ = volume_to_gain(settings.music_volume);
    music_enabled_ = settings.music_enabled;
    output_ = std::make_unique<AudioOutput>(kSampleRate, &AudioSystem::render_callback, this);
}

AudioSystem::~AudioSystem() = default;

void AudioSystem::load_music(std::string id, const std::filesystem::path& path) {
    if (id.empty()) {
        throw std::invalid_argument{"audio music id must not be empty"};
    }

    Pt3Music music = Pt3Music::load(path);
    StreamLock lock{*output_};
    music_catalog_.insert_or_assign(std::move(id), std::move(music));
}

bool AudioSystem::play_music(std::string_view id) {
    StreamLock lock{*output_};
    const auto found = music_catalog_.find(id);
    if (found == music_catalog_.end()) {
        return false;
    }

    music_player_.start(found->second, music_primary_, music_secondary_);
    music_paused_ = false;
    return true;
}

void AudioSystem::stop_music() {
    StreamLock lock{*output_};
    music_player_.stop(music_primary_, music_secondary_);
    music_paused_ = false;
    clear_telemetry();
}

void AudioSystem::set_music_paused(bool paused) {
    StreamLock lock{*output_};
    music_paused_ = paused;
    if (music_paused_) {
        clear_telemetry();
    }
}

void AudioSystem::set_mix_settings(const AudioMixSettings& settings) {
    validate_mix_settings(settings);
    StreamLock lock{*output_};
    master_gain_ = volume_to_gain(settings.master_volume);
    music_gain_ = volume_to_gain(settings.music_volume);
    music_enabled_ = settings.music_enabled;
    if (!music_enabled_) {
        clear_telemetry();
    }
}

AyTelemetrySnapshot AudioSystem::telemetry_snapshot() const noexcept {
    AyTelemetrySnapshot snapshot;
    for (std::size_t index = 0; index < snapshot.channel_levels.size(); ++index) {
        snapshot.channel_levels[index] = meter_levels_[index].load(std::memory_order_relaxed);
    }
    snapshot.chip_count = meter_chip_count_.load(std::memory_order_relaxed);
    snapshot.noise_active = meter_noise_active_.load(std::memory_order_relaxed);
    snapshot.envelope_active = meter_envelope_active_.load(std::memory_order_relaxed);
    return snapshot;
}

void AudioSystem::render_callback(void* userdata, std::span<float> output) {
    static_cast<AudioSystem*>(userdata)->render(output);
}

void AudioSystem::validate_mix_settings(const AudioMixSettings& settings) {
    const auto valid_volume = [](int volume) { return volume >= 0 && volume <= 100; };
    if (!valid_volume(settings.master_volume) || !valid_volume(settings.music_volume)) {
        throw std::invalid_argument{"audio volumes must be between 0 and 100"};
    }
}

void AudioSystem::render(std::span<float> output) {
    const std::size_t sample_count = output.size();
    std::span<float> music{music_buffer_.data(), sample_count};
    if (music_enabled_ && !music_paused_) {
        music_player_.render(music_primary_, music_secondary_, music);
        publish_telemetry();
    } else {
        std::fill(music.begin(), music.end(), 0.0F);
        clear_telemetry();
    }

    const float output_gain = master_gain_ * music_gain_ * kOutputHeadroom;
    for (std::size_t index = 0; index < sample_count; ++index) {
        output[index] = std::clamp(music[index] * output_gain, -1.0F, 1.0F);
    }
}

void AudioSystem::publish_telemetry() noexcept {
    const std::size_t chips = music_player_.active() ? music_player_.chip_count() : 0;
    bool any_noise = false;
    bool any_envelope = false;

    const auto publish_chip = [&](const AyChip& chip, std::size_t chip_index) {
        for (std::uint8_t channel = 0; channel < 3; ++channel) {
            const std::size_t meter_index = chip_index * 3 + channel;
            const std::uint8_t level = chip.channel_visual_level(channel);
            meter_levels_[meter_index].store(level, std::memory_order_relaxed);
            const bool audible = level > 0;
            any_noise = any_noise || (audible && chip.channel_noise_enabled(channel));
            any_envelope = any_envelope || (audible && chip.channel_envelope_enabled(channel));
        }
    };

    if (chips >= 1) {
        publish_chip(music_primary_, 0);
    } else {
        for (std::size_t index = 0; index < 3; ++index) {
            meter_levels_[index].store(0, std::memory_order_relaxed);
        }
    }

    if (chips >= 2) {
        publish_chip(music_secondary_, 1);
    } else {
        for (std::size_t index = 3; index < 6; ++index) {
            meter_levels_[index].store(0, std::memory_order_relaxed);
        }
    }

    meter_chip_count_.store(static_cast<std::uint8_t>(chips), std::memory_order_relaxed);
    meter_noise_active_.store(any_noise, std::memory_order_relaxed);
    meter_envelope_active_.store(any_envelope, std::memory_order_relaxed);
}

void AudioSystem::clear_telemetry() noexcept {
    for (auto& level : meter_levels_) {
        level.store(0, std::memory_order_relaxed);
    }
    meter_chip_count_.store(0, std::memory_order_relaxed);
    meter_noise_active_.store(false, std::memory_order_relaxed);
    meter_envelope_active_.store(false, std::memory_order_relaxed);
}

}  // namespace w100h::audio
