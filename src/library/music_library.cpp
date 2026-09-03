#include "library/music_library.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>

namespace w100h::library {
namespace {

[[nodiscard]] std::string ascii_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

[[nodiscard]] std::filesystem::path normalized_existing_path(const std::filesystem::path& path) {
    std::error_code error;
    const std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
    if (!error) {
        return canonical;
    }
    error.clear();
    const std::filesystem::path absolute = std::filesystem::absolute(path, error);
    return error ? path.lexically_normal() : absolute.lexically_normal();
}

}  // namespace

bool is_supported_music_file(const std::filesystem::path& path) {
    return ascii_lower(path.extension().string()) == ".pt3";
}

std::filesystem::path default_music_directory() {
    const char* home = std::getenv("HOME");
    if (home == nullptr || *home == '\0') {
        throw std::runtime_error{"HOME is not set; cannot resolve ~/Music"};
    }
    return std::filesystem::path{home} / "Music";
}

std::vector<MusicTrack> scan_music_directory(const std::filesystem::path& root) {
    std::error_code error;
    if (!std::filesystem::exists(root, error)) {
        if (error) {
            throw std::runtime_error{"cannot inspect music directory '" + root.string() +
                                     "': " + error.message()};
        }
        return {};
    }
    if (!std::filesystem::is_directory(root, error) || error) {
        throw std::runtime_error{"music library path is not a directory: " + root.string()};
    }

    std::vector<MusicTrack> tracks;
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::recursive_directory_iterator iterator{root, options, error};
    const std::filesystem::recursive_directory_iterator end;
    if (error) {
        throw std::runtime_error{"cannot scan music directory '" + root.string() +
                                 "': " + error.message()};
    }

    while (iterator != end) {
        const std::filesystem::directory_entry& entry = *iterator;
        std::error_code status_error;
        if (entry.is_regular_file(status_error) && !status_error &&
            is_supported_music_file(entry.path())) {
            tracks.push_back(MusicTrack{
                .path = normalized_existing_path(entry.path()),
                .display_name = entry.path().filename().string(),
            });
        }

        iterator.increment(error);
        if (error) {
            error.clear();
        }
    }

    std::sort(tracks.begin(), tracks.end(), [](const MusicTrack& left, const MusicTrack& right) {
        const std::string left_key = ascii_lower(left.path.generic_string());
        const std::string right_key = ascii_lower(right.path.generic_string());
        return left_key < right_key;
    });
    return tracks;
}

std::optional<std::size_t> find_track_index(
    std::span<const MusicTrack> tracks,
    const std::filesystem::path& path) {
    const std::filesystem::path normalized = normalized_existing_path(path);
    for (std::size_t index = 0; index < tracks.size(); ++index) {
        if (tracks[index].path == normalized) {
            return index;
        }
    }
    return std::nullopt;
}

}  // namespace w100h::library
