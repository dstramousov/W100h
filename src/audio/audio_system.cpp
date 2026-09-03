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
    return true;
}

void AudioSystem::stop_music() {
    StreamLock lock{*output_};
    music_player_.stop(music_primary_, music_secondary_);
}

void AudioSystem::set_mix_settings(const AudioMixSettings& settings) {
    validate_mix_settings(settings);
    StreamLock lock{*output_};
    master_gain_ = volume_to_gain(settings.master_volume);
    music_gain_ = volume_to_gain(settings.music_volume);
    music_enabled_ = settings.music_enabled;
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
    if (music_enabled_) {
        music_player_.render(music_primary_, music_secondary_, music);
    } else {
        std::fill(music.begin(), music.end(), 0.0F);
    }

    const float output_gain = master_gain_ * music_gain_ * kOutputHeadroom;
    for (std::size_t index = 0; index < sample_count; ++index) {
        output[index] = std::clamp(music[index] * output_gain, -1.0F, 1.0F);
    }
}

}  // namespace w100h::audio
