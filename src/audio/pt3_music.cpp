#include "audio/pt3_music.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace w100h::audio {
namespace {

constexpr std::size_t kPt3HeaderSize = 202;
constexpr std::size_t kPt3MaximumSize = 65'536;
constexpr std::size_t kPt3ModeField = 98;
constexpr std::size_t kPt3PatternsOffsetField = 103;
constexpr std::size_t kPt3SamplesOffsetField = 105;
constexpr std::size_t kPt3OrnamentsOffsetField = 169;
constexpr std::size_t kPt3PositionsOffset = 201;
constexpr std::size_t kPt3LoopPositionField = 102;
constexpr std::size_t kPt3SampleCount = 32;
constexpr std::size_t kPt3OrnamentCount = 16;
constexpr std::size_t kTurboSoundFooterSize = 16;

[[nodiscard]] std::uint16_t read_u16_le(std::span<const std::uint8_t> data,
                                        std::size_t offset) {
    if (offset + 2 > data.size()) {
        throw std::runtime_error{"PT3 word extends past end of file"};
    }
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(data[offset]) |
        (static_cast<std::uint16_t>(data[offset + 1]) << 8U));
}

[[nodiscard]] bool starts_with_pt3_signature(std::span<const std::uint8_t> data) {
    constexpr std::string_view signature = "ProTracker 3.";
    return data.size() >= signature.size() &&
           std::equal(signature.begin(), signature.end(), data.begin());
}

void validate_module(std::span<const std::uint8_t> data) {
    if (data.size() < kPt3HeaderSize || data.size() > kPt3MaximumSize) {
        throw std::runtime_error{"PT3 module size is outside the supported range"};
    }
    if (!starts_with_pt3_signature(data)) {
        throw std::runtime_error{"PT3 module signature is missing"};
    }
    if (data[kPt3ModeField] != 0x20) {
        throw std::runtime_error{
            "embedded TurboSound PT3 mode is unsupported; use an 02TS container"};
    }

    const std::uint16_t patterns_offset = read_u16_le(data, kPt3PatternsOffsetField);
    if (patterns_offset < kPt3HeaderSize || patterns_offset >= data.size()) {
        throw std::runtime_error{"PT3 patterns table offset is invalid"};
    }

    std::size_t positions_count = 0;
    for (std::size_t cursor = kPt3PositionsOffset; cursor < data.size(); ++cursor) {
        const std::uint8_t value = data[cursor];
        if (value == 0xFF) {
            break;
        }
        if ((value % 3U) != 0U) {
            throw std::runtime_error{"PT3 position does not reference a channel triplet"};
        }
        ++positions_count;
        if (positions_count > 255) {
            throw std::runtime_error{"PT3 position list is too long"};
        }
    }
    if (positions_count == 0 ||
        kPt3PositionsOffset + positions_count >= data.size() ||
        data[kPt3PositionsOffset + positions_count] != 0xFF) {
        throw std::runtime_error{"PT3 position list has no terminator"};
    }
    if (data[kPt3LoopPositionField] >= positions_count) {
        throw std::runtime_error{"PT3 loop position is outside the position list"};
    }

    for (std::size_t index = 0; index < kPt3SampleCount; ++index) {
        const std::uint16_t offset = read_u16_le(data, kPt3SamplesOffsetField + index * 2);
        if (offset != 0 && offset >= data.size()) {
            throw std::runtime_error{"PT3 sample offset is outside the module"};
        }
    }
    for (std::size_t index = 0; index < kPt3OrnamentCount; ++index) {
        const std::uint16_t offset = read_u16_le(data, kPt3OrnamentsOffsetField + index * 2);
        if (offset != 0 && offset >= data.size()) {
            throw std::runtime_error{"PT3 ornament offset is outside the module"};
        }
    }
}

[[nodiscard]] bool matches(std::span<const std::uint8_t> data, std::size_t offset,
                           std::string_view value) {
    return offset + value.size() <= data.size() &&
           std::equal(value.begin(), value.end(), data.begin() +
                                                   static_cast<std::ptrdiff_t>(offset));
}

}  // namespace

Pt3Music::Pt3Music(std::vector<std::uint8_t> storage, std::size_t payload_size,
                   std::size_t chip_count)
    : storage_{std::move(storage)}, payload_size_{payload_size}, chip_count_{chip_count} {}

Pt3Music Pt3Music::load(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary | std::ios::ate};
    if (!input) {
        throw std::runtime_error{"Could not open PT3 music file: " + path.string()};
    }

    const std::streampos end_position = input.tellg();
    if (end_position <= 0 ||
        static_cast<std::uintmax_t>(end_position) > kPt3MaximumSize ||
        static_cast<std::uintmax_t>(end_position) >
            static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error{"PT3 music file size is outside the supported range: " +
                                 path.string()};
    }

    std::vector<std::uint8_t> data(static_cast<std::size_t>(end_position));
    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!input || input.gcount() != static_cast<std::streamsize>(data.size())) {
        throw std::runtime_error{"Could not read PT3 music file: " + path.string()};
    }
    return parse(data);
}

Pt3Music Pt3Music::parse(std::span<const std::uint8_t> data) {
    if (data.empty()) {
        throw std::runtime_error{"PT3 music file is empty"};
    }
    if (data.size() > kPt3MaximumSize) {
        throw std::runtime_error{"PT3 music file exceeds the decoder 64 KiB limit"};
    }

    std::size_t chip_count = 1;
    if (data.size() >= kTurboSoundFooterSize &&
        matches(data, data.size() - 4, "02TS")) {
        const std::size_t footer = data.size() - kTurboSoundFooterSize;
        if (!matches(data, footer, "PT3!") || !matches(data, footer + 6, "PT3!")) {
            throw std::runtime_error{"TurboSound footer contains invalid PT3 tags"};
        }

        const std::size_t first_size = read_u16_le(data, footer + 4);
        const std::size_t second_size = read_u16_le(data, footer + 10);
        if (first_size < kPt3HeaderSize || second_size < kPt3HeaderSize ||
            first_size + second_size + kTurboSoundFooterSize != data.size()) {
            throw std::runtime_error{"TurboSound module sizes do not match the file"};
        }

        validate_module(data.first(first_size));
        validate_module(data.subspan(first_size, second_size));
        chip_count = 2;
    } else if (data.size() >= 4 && matches(data, data.size() - 4, "03TS")) {
        throw std::runtime_error{"3-chip TurboSound PT3 is not supported by W100h"};
    } else {
        validate_module(data);
    }

    std::vector<std::uint8_t> storage{data.begin(), data.end()};
    storage.push_back(0);
    return Pt3Music{std::move(storage), data.size(), chip_count};
}

}  // namespace w100h::audio
