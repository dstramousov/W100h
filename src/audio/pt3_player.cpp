#include "audio/pt3_player.hpp"

extern "C" {
#include <pt3player.h>
#include "audio/pt3player_bridge.h"
}

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
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

[[nodiscard]] bool valid_decoder_chip_count(int decoder_chips, std::size_t expected) {
    return decoder_chips > 0 && decoder_chips <= 2 &&
           static_cast<std::size_t>(decoder_chips) == expected;
}

}  // namespace

Pt3Player::Pt3Player(int sample_rate) {
    if (sample_rate <= 0 || (sample_rate % kFrameRate) != 0) {
        throw std::invalid_argument{"PT3 sample rate must be positive and divisible by 50"};
    }
    frames_per_tick_ = sample_rate / kFrameRate;
    frames_until_tick_ = frames_per_tick_;
    seek_buffer_.resize(static_cast<std::size_t>(frames_per_tick_) * kStereoChannels);
}

int Pt3Player::setup_decoder(Pt3Music& music) {
    ScopedDecoderStdoutSilencer silence_upstream_diagnostics;
    return func_setup_music(music.data(), static_cast<int>(music.payload_size()), 0, 0);
}

Pt3Player::TimelineAnalysis Pt3Player::analyze_timeline(std::size_t decoder_chips) {
    const int position_count = w100h_pt3_position_count(0);
    const int loop_position = w100h_pt3_loop_position(0);
    int previous_position = w100h_pt3_current_position(0);
    int previous_address = w100h_pt3_pattern_address(0, 0);

    if (position_count <= 0 || loop_position < 0 || loop_position >= position_count ||
        previous_position < 0) {
        return {};
    }

    std::uint32_t loop_tick = loop_position == previous_position
                                  ? 0U
                                  : std::numeric_limits<std::uint32_t>::max();

    for (std::uint32_t tick_count = 1; tick_count <= kMaxTimelineAnalysisTicks; ++tick_count) {
        func_play_tick(0);
        if (decoder_chips == 2) {
            func_play_tick(1);
        }

        const int current_position = w100h_pt3_current_position(0);
        const int current_address = w100h_pt3_pattern_address(0, 0);
        if (current_position < 0) {
            return {};
        }

        if (loop_tick == std::numeric_limits<std::uint32_t>::max() &&
            current_position == loop_position && previous_position != loop_position) {
            loop_tick = tick_count;
        }

        bool wrapped = false;
        if (previous_position == position_count - 1) {
            if (loop_position != position_count - 1) {
                wrapped = current_position == loop_position;
            } else if (current_position == loop_position && previous_address >= 0 &&
                       current_address >= 0) {
                // A module can legally loop its final position back to itself. In that
                // case CurrentPosition does not change, but channel A's pattern cursor
                // jumps back to the beginning of the same pattern.
                wrapped = current_address < previous_address;
            }
        }

        if (wrapped) {
            if (loop_tick == std::numeric_limits<std::uint32_t>::max()) {
                loop_tick = 0;
            }
            return TimelineAnalysis{.duration_ticks = tick_count, .loop_tick = loop_tick};
        }

        previous_position = current_position;
        previous_address = current_address;
    }

    return {};
}

void Pt3Player::start(Pt3Music& music, AyChip& primary, AyChip& secondary) {
    int decoder_chips = setup_decoder(music);
    if (!valid_decoder_chip_count(decoder_chips, music.chip_count())) {
        throw std::runtime_error{"PT3 decoder rejected the music resource"};
    }

    const TimelineAnalysis timeline = analyze_timeline(static_cast<std::size_t>(decoder_chips));

    // Timeline analysis advances the legacy global decoder. Reset it once more so
    // audible playback starts at the real beginning.
    decoder_chips = setup_decoder(music);
    if (!valid_decoder_chip_count(decoder_chips, music.chip_count())) {
        throw std::runtime_error{"PT3 decoder rejected the music resource after timeline analysis"};
    }

    primary.reset();
    secondary.reset();
    current_music_ = &music;
    chip_count_ = static_cast<std::size_t>(decoder_chips);
    active_ = true;
    current_tick_ = 0;
    duration_ticks_ = timeline.duration_ticks;
    loop_tick_ = timeline.duration_ticks == 0
                     ? 0U
                     : std::min(timeline.loop_tick, timeline.duration_ticks - 1U);

    // Apply the first PT3 tick immediately. The next tick occurs 20 ms later.
    tick(primary, secondary);
    frames_until_tick_ = frames_per_tick_;
}

void Pt3Player::stop(AyChip& primary, AyChip& secondary) {
    active_ = false;
    chip_count_ = 0;
    current_tick_ = 0;
    frames_until_tick_ = frames_per_tick_;
    primary.reset();
    secondary.reset();
}

bool Pt3Player::seek_to_tick(std::uint32_t target_tick, AyChip& primary, AyChip& secondary) {
    if (!active_ || current_music_ == nullptr || duration_ticks_ == 0 ||
        target_tick >= duration_ticks_) {
        return false;
    }

    const int decoder_chips = setup_decoder(*current_music_);
    if (!valid_decoder_chip_count(decoder_chips, current_music_->chip_count())) {
        return false;
    }

    chip_count_ = static_cast<std::size_t>(decoder_chips);
    primary.reset();
    secondary.reset();
    current_tick_ = 0;
    tick(primary, secondary);
    frames_until_tick_ = frames_per_tick_;

    for (std::uint32_t tick_index = 0; tick_index < target_tick; ++tick_index) {
        advance_synthesis_tick(primary, secondary);
        tick(primary, secondary);
        ++current_tick_;
    }
    return true;
}

bool Pt3Player::seek_relative_seconds(int seconds, AyChip& primary, AyChip& secondary) {
    if (!active_ || duration_ticks_ == 0 || seconds == 0) {
        return false;
    }

    const std::int64_t delta_ticks = static_cast<std::int64_t>(seconds) * kFrameRate;
    const std::int64_t current = static_cast<std::int64_t>(current_tick_);
    const std::int64_t maximum = static_cast<std::int64_t>(duration_ticks_ - 1U);
    const std::uint32_t target = static_cast<std::uint32_t>(
        std::clamp(current + delta_ticks, std::int64_t{0}, maximum));
    if (target == current_tick_) {
        return false;
    }
    return seek_to_tick(target, primary, secondary);
}

void Pt3Player::advance_synthesis_tick(AyChip& primary, AyChip& secondary) {
    std::span<float> scratch{seek_buffer_.data(), seek_buffer_.size()};
    primary.render(scratch);
    if (chip_count_ == 2) {
        secondary.render(scratch);
    }
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

        std::span<float> primary_output = output.subspan(sample_offset, segment_samples);
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
            advance_timeline_tick();
            frames_until_tick_ = frames_per_tick_;
        }
    }
}

void Pt3Player::advance_timeline_tick() noexcept {
    if (duration_ticks_ == 0) {
        ++current_tick_;
        return;
    }

    ++current_tick_;
    if (current_tick_ >= duration_ticks_) {
        current_tick_ = loop_tick_;
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
