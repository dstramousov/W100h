#include "audio/ay_chip.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace {

constexpr int kSampleRate = 48'000;
constexpr std::size_t kFrames = 4096;
constexpr std::size_t kSamples = kFrames * 2;

void configure_test_tone(w100h::audio::AyChip& chip) {
    constexpr int tone_period = 252;
    chip.write_register(2, static_cast<std::uint8_t>(tone_period & 0xFF));
    chip.write_register(3, static_cast<std::uint8_t>((tone_period >> 8) & 0x0F));
    chip.write_register(7, 0x3D);
    chip.write_register(9, 10);
}

}  // namespace

int main() {
    std::array<float, kSamples> silence{};
    w100h::audio::AyChip silent_chip{kSampleRate};
    silent_chip.render(silence);
    assert(std::all_of(silence.begin(), silence.end(), [](float sample) {
        return std::isfinite(sample) && std::abs(sample) < 1.0e-7F;
    }));

    w100h::audio::AyChip first{kSampleRate};
    w100h::audio::AyChip second{kSampleRate};
    configure_test_tone(first);
    configure_test_tone(second);
    assert(first.read_register(2) == static_cast<std::uint8_t>(252));
    assert(first.read_register(3) == 0);
    assert(first.read_register(7) == 0x3D);
    assert(first.read_register(9) == 10);

    std::array<float, kSamples> first_pcm{};
    std::array<float, kSamples> second_pcm{};
    first.render(first_pcm);
    second.render(second_pcm);

    assert(first_pcm == second_pcm);
    assert(std::any_of(first_pcm.begin(), first_pcm.end(), [](float sample) {
        return std::abs(sample) > 1.0e-4F;
    }));
    assert(std::all_of(first_pcm.begin(), first_pcm.end(), [](float sample) {
        return std::isfinite(sample) && sample >= -1.0F && sample <= 1.0F;
    }));

    bool bad_register_rejected = false;
    try {
        first.write_register(14, 0);
    } catch (const std::out_of_range&) {
        bad_register_rejected = true;
    }
    assert(bad_register_rejected);

    first.reset();
    first_pcm.fill(1.0F);
    first.render(first_pcm);
    assert(std::all_of(first_pcm.begin(), first_pcm.end(), [](float sample) {
        return std::isfinite(sample) && std::abs(sample) < 1.0e-7F;
    }));

    return 0;
}
