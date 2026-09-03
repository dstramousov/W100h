#include "audio/pt3_player.hpp"

extern "C" {
#include <pt3player.h>
}

#include <algorithm>
#include <cstdio>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif

#include "audio/ay_chip.hpp"
#include "audio/pt3_music.hpp"

namespace w100h::audio {
namespace {

class ScopedDecoderStdoutSilencer final {
public:
    ScopedDecoderStdoutSilencer() noexcept {
#if defined(__unix__) || defined(__APPLE__)
        std::fflush(stdout);
        saved_stdout_ = ::dup(STDOUT_FILENO);
        null_fd_ = ::open("/dev/null", O_WRONLY);
        if (saved_stdout_ >= 0 && null_fd_ >= 0) {
            (void)::dup2(null_fd_, STDOUT_FILENO);
        }
#endif
    }

    ~ScopedDecoderStdoutSilencer() {
#if defined(__unix__) || defined(__APPLE__)
        std::fflush(stdout);
        if (saved_stdout_ >= 0) {
            (void)::dup2(saved_stdout_, STDOUT_FILENO);
            ::close(saved_stdout_);
        }
        if (null_fd_ >= 0) {
            ::close(null_fd_);
        }
#endif
    }

    ScopedDecoderStdoutSilencer(const ScopedDecoderStdoutSilencer&) = delete;
    ScopedDecoderStdoutSilencer& operator=(const ScopedDecoderStdoutSilencer&) = delete;

private:
#if defined(__unix__) || defined(__APPLE__)
    int saved_stdout_ = -1;
    int null_fd_ = -1;
#endif
};

}  // namespace

Pt3Player::Pt3Player(int sample_rate) {
    if (sample_rate <= 0 || (sample_rate % kFrameRate) != 0) {
        throw std::invalid_argument{"PT3 sample rate must be positive and divisible by 50"};
    }
    frames_per_tick_ = sample_rate / kFrameRate;
    frames_until_tick_ = frames_per_tick_;
}

void Pt3Player::start(Pt3Music& music, AyChip& primary, AyChip& secondary) {
    int decoder_chips = 0;
    {
        ScopedDecoderStdoutSilencer silence_upstream_diagnostics;
        decoder_chips =
            func_setup_music(music.data(), static_cast<int>(music.payload_size()), 0, 0);
    }
    if (decoder_chips <= 0 || static_cast<std::size_t>(decoder_chips) != music.chip_count() ||
        decoder_chips > 2) {
        throw std::runtime_error{"PT3 decoder rejected the music resource"};
    }

    primary.reset();
    secondary.reset();
    chip_count_ = static_cast<std::size_t>(decoder_chips);
    active_ = true;

    // Apply the first PT3 tick immediately. The next tick occurs 20 ms later.
    tick(primary, secondary);
    frames_until_tick_ = frames_per_tick_;
}

void Pt3Player::stop(AyChip& primary, AyChip& secondary) {
    active_ = false;
    chip_count_ = 0;
    frames_until_tick_ = frames_per_tick_;
    primary.reset();
    secondary.reset();
}

void Pt3Player::render(AyChip& primary, AyChip& secondary, std::span<float> output) {
    if ((output.size() % kStereoChannels) != 0U) {
        throw std::invalid_argument{"PT3 render buffer must contain complete stereo frames"};
    }
    if (!active_) {
        std::fill(output.begin(), output.end(), 0.0F);
        return;
    }

    std::size_t output_frame = 0;
    const std::size_t total_frames = output.size() / kStereoChannels;

    while (output_frame < total_frames) {
        const std::size_t frames_left = total_frames - output_frame;
        const std::size_t segment_frames =
            std::min({frames_left, static_cast<std::size_t>(frames_until_tick_),
                      static_cast<std::size_t>(kRenderChunkFrames)});
        const std::size_t segment_samples = segment_frames * kStereoChannels;
        const std::size_t sample_offset = output_frame * kStereoChannels;

        std::span<float> primary_output =
            output.subspan(sample_offset, segment_samples);
        primary.render(primary_output);

        if (chip_count_ == 2) {
            std::span<float> secondary_output{secondary_buffer_.data(), segment_samples};
            secondary.render(secondary_output);
            for (std::size_t index = 0; index < segment_samples; ++index) {
                primary_output[index] =
                    (primary_output[index] + secondary_output[index]) * 0.5F;
            }
        }

        output_frame += segment_frames;
        frames_until_tick_ -= static_cast<int>(segment_frames);
        if (frames_until_tick_ == 0) {
            tick(primary, secondary);
            frames_until_tick_ = frames_per_tick_;
        }
    }
}

void Pt3Player::tick(AyChip& primary, AyChip& secondary) {
    func_play_tick(0);
    apply_registers(0, primary);
    if (chip_count_ == 2) {
        func_play_tick(1);
        apply_registers(1, secondary);
    }
}

void Pt3Player::apply_registers(int decoder_channel, AyChip& chip) {
    std::array<std::uint8_t, 14> registers{};
    func_getregs(registers.data(), decoder_channel);

    for (std::uint8_t index = 0; index < 13; ++index) {
        chip.write_register(index, registers[index]);
    }

    // PT3 uses 0xFF as "do not retrigger envelope shape on this tick".
    if (registers[13] != 0xFF) {
        chip.write_register(13, registers[13]);
    }
}

}  // namespace w100h::audio
