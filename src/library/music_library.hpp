#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace w100h::library {

/** @brief One supported music file discovered for playback. */
struct MusicTrack {
    std::filesystem::path path;
    std::string display_name;
};

/**
 * @brief Returns whether a path has a music-file extension supported by W100h.
 * @param path File path to inspect.
 * @return true for currently supported PT3-family files; false otherwise.
 */
[[nodiscard]] bool is_supported_music_file(const std::filesystem::path& path);

/**
 * @brief Resolves the default per-user music directory as ~/Music.
 * @return Path to the current user's Music directory.
 * @throws std::runtime_error If the HOME environment variable is unavailable.
 */
[[nodiscard]] std::filesystem::path default_music_directory();

/**
 * @brief Recursively scans a directory for supported music files.
 *
 * Missing directories are treated as empty libraries. Existing non-directory
 * paths are rejected. Directory symlinks are not followed.
 *
 * @param root Directory to scan.
 * @return Deterministically sorted list of supported tracks.
 * @throws std::runtime_error If an existing root is not a directory or cannot be scanned.
 */
[[nodiscard]] std::vector<MusicTrack> scan_music_directory(const std::filesystem::path& root);

/**
 * @brief Finds one existing music file in a discovered track list.
 * @param tracks Track list to search.
 * @param path Existing music file path to locate.
 * @return Zero-based track index, or std::nullopt when not present.
 */
[[nodiscard]] std::optional<std::size_t> find_track_index(
    std::span<const MusicTrack> tracks,
    const std::filesystem::path& path);

}  // namespace w100h::library
