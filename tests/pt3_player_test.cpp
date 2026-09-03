#include "audio/ay_chip.hpp"
#include "audio/pt3_music.hpp"
#include "audio/pt3_player.hpp"
#include "pt3player_test_api.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <stdexcept>

int main() {
    constexpr int kSampleRate = 48'000;
    constexpr std::size_t kFramesPerTick = 960;
    constexpr std::size_t kSamples = kFramesPerTick * 2;

    auto music = w100h::audio::Pt3Music::load(
        "assets/audio/music/Pator - August Melancholy.pt3");
    w100h::audio::AyChip primary{kSampleRate};
    w100h::audio::AyChip secondary{kSampleRate};
    w100h::audio::Pt3Player player{kSampleRate};

    player.start(music, primary, secondary);
    assert(player.active());
    assert(player.chip_count() == 1);
    assert(pt3_stub_tick_count(0) == 1);
    assert(pt3_stub_tick_count(1) == 0);
    assert(primary.read_register(8) == 15);
    assert(secondary.read_register(8) == 0);
    assert(primary.read_register(13) == 9);

    std::array<float, kSamples> pcm{};
    player.render(primary, secondary, pcm);
    assert(pt3_stub_tick_count(0) == 2);
    assert(primary.read_register(13) == 9);
    assert(std::all_of(pcm.begin(), pcm.end(), [](float sample) {
        return std::isfinite(sample) && sample >= -1.0F && sample <= 1.0F;
    }));
    assert(std::any_of(pcm.begin(), pcm.end(), [](float sample) {
        return std::abs(sample) > 1.0e-5F;
    }));

    player.stop(primary, secondary);
    assert(!player.active());
    assert(player.chip_count() == 0);
    assert(primary.read_register(8) == 0);
    assert(secondary.read_register(8) == 0);

    bool bad_rate_rejected = false;
    try {
        w100h::audio::Pt3Player invalid{48'001};
    } catch (const std::invalid_argument&) {
        bad_rate_rejected = true;
    }
    assert(bad_rate_rejected);

    bool odd_buffer_rejected = false;
    try {
        std::array<float, 3> odd{};
        player.render(primary, secondary, odd);
    } catch (const std::invalid_argument&) {
        odd_buffer_rejected = true;
    }
    assert(odd_buffer_rejected);

    return 0;
}
