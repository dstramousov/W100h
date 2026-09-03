#include "audio/audio_output.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace w100h::audio {

AudioOutput::AudioOutput(int sample_rate, RenderCallback render_callback, void* userdata)
    : render_callback_{render_callback}, userdata_{userdata} {
    if (sample_rate <= 0) {
        throw std::invalid_argument{"audio sample rate must be positive"};
    }
    if (render_callback_ == nullptr) {
        throw std::invalid_argument{"audio render callback must not be null"};
    }

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = kChannelCount;
    spec.freq = sample_rate;

    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
                                        &AudioOutput::stream_callback, this);
    if (stream_ == nullptr) {
        throw std::runtime_error{"SDL_OpenAudioDeviceStream failed: " +
                                 std::string{SDL_GetError()}};
    }

    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        const std::string error = SDL_GetError();
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        throw std::runtime_error{"SDL_ResumeAudioStreamDevice failed: " + error};
    }
}

AudioOutput::~AudioOutput() {
    if (stream_ != nullptr) {
        SDL_DestroyAudioStream(stream_);
    }
}

void AudioOutput::lock() {
    if (!SDL_LockAudioStream(stream_)) {
        throw std::runtime_error{"SDL_LockAudioStream failed: " + std::string{SDL_GetError()}};
    }
}

void AudioOutput::unlock() noexcept {
    if (!SDL_UnlockAudioStream(stream_)) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL_UnlockAudioStream failed: %s", SDL_GetError());
    }
}

void SDLCALL AudioOutput::stream_callback(void* userdata, SDL_AudioStream* stream,
                                          int additional_amount, int /*total_amount*/) {
    auto* output = static_cast<AudioOutput*>(userdata);
    output->feed(stream, additional_amount);
}

void AudioOutput::feed(SDL_AudioStream* stream, int additional_amount) noexcept {
    constexpr int bytes_per_frame = static_cast<int>(sizeof(float)) * kChannelCount;

    while (additional_amount > 0) {
        const int requested_frames = (additional_amount + bytes_per_frame - 1) / bytes_per_frame;
        const int chunk_frames = std::min(requested_frames, kChunkFrames);
        const int chunk_samples = chunk_frames * kChannelCount;
        const int chunk_bytes = chunk_frames * bytes_per_frame;

        std::span<float> chunk{buffer_.data(), static_cast<std::size_t>(chunk_samples)};
        render_callback_(userdata_, chunk);
        if (!SDL_PutAudioStreamData(stream, chunk.data(), chunk_bytes)) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL_PutAudioStreamData failed: %s", SDL_GetError());
            return;
        }
        additional_amount -= chunk_bytes;
    }
}

}  // namespace w100h::audio
