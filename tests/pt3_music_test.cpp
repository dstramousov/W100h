#include "audio/pt3_music.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

std::vector<std::uint8_t> make_minimal_pt3(
    std::string_view signature = "ProTracker 3.5") {
    constexpr std::size_t kSize = 204;
    std::vector<std::uint8_t> data(kSize, 0);
    for (std::size_t index = 0; index < signature.size(); ++index) {
        data[index] = static_cast<std::uint8_t>(signature[index]);
    }

    data[98] = 0x20;
    data[102] = 0;
    data[103] = 203;
    data[104] = 0;
    data[201] = 0;
    data[202] = 0xff;
    return data;
}

void append_u16_le(std::vector<std::uint8_t>& data, std::size_t value) {
    assert(value <= 0xffffU);
    data.push_back(static_cast<std::uint8_t>(value & 0xffU));
    data.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

std::vector<std::uint8_t> make_mixed_header_02ts() {
    const auto first = make_minimal_pt3("ProTracker 3.7");
    const auto second = make_minimal_pt3("Vortex Tracker II 1.0 module:");

    std::vector<std::uint8_t> data;
    data.reserve(first.size() + second.size() + 16U);
    data.insert(data.end(), first.begin(), first.end());
    data.insert(data.end(), second.begin(), second.end());
    data.insert(data.end(), {'P', 'T', '3', '!'});
    append_u16_le(data, first.size());
    data.insert(data.end(), {'P', 'T', '3', '!'});
    append_u16_le(data, second.size());
    data.insert(data.end(), {'0', '2', 'T', 'S'});
    return data;
}

template <class Function>
void expect_runtime_error(Function&& function) {
    bool rejected = false;
    try {
        function();
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    assert(rejected);
}

}  // namespace

int main() {
    const auto bundled = w100h::audio::Pt3Music::load(
        "assets/audio/music/Pator - August Melancholy.pt3");
    assert(bundled.chip_count() == 1);
    assert(bundled.payload_size() == 3905);

    auto single_data = make_minimal_pt3();
    const auto single = w100h::audio::Pt3Music::parse(single_data);
    assert(single.chip_count() == 1);
    assert(single.payload_size() == single_data.size());

    const auto vortex_data = make_minimal_pt3("Vortex Tracker II 1.0 module:");
    const auto vortex = w100h::audio::Pt3Music::parse(vortex_data);
    assert(vortex.chip_count() == 1);
    assert(vortex.payload_size() == vortex_data.size());

    const auto mixed_02ts_data = make_mixed_header_02ts();
    const auto mixed_02ts = w100h::audio::Pt3Music::parse(mixed_02ts_data);
    assert(mixed_02ts.chip_count() == 2);
    assert(mixed_02ts.payload_size() == mixed_02ts_data.size());

    expect_runtime_error([] { (void)w100h::audio::Pt3Music::parse({}); });

    auto bad_signature = make_minimal_pt3();
    bad_signature[0] = 'X';
    expect_runtime_error([&] { (void)w100h::audio::Pt3Music::parse(bad_signature); });

    auto bad_loop = make_minimal_pt3();
    bad_loop[102] = 1;
    expect_runtime_error([&] { (void)w100h::audio::Pt3Music::parse(bad_loop); });

    auto embedded_turbo = make_minimal_pt3();
    embedded_turbo[98] = 0x24;
    expect_runtime_error([&] { (void)w100h::audio::Pt3Music::parse(embedded_turbo); });

    auto bad_position = make_minimal_pt3();
    bad_position[201] = 1;
    expect_runtime_error([&] { (void)w100h::audio::Pt3Music::parse(bad_position); });

    auto three_chip = make_minimal_pt3();
    three_chip.insert(three_chip.end(), {'0', '3', 'T', 'S'});
    expect_runtime_error([&] { (void)w100h::audio::Pt3Music::parse(three_chip); });

    return 0;
}
