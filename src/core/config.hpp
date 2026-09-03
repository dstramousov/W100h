#pragma once

#include <filesystem>

namespace w100h::core {

inline constexpr int kMinScale = 1;
inline constexpr int kMaxScale = 8;
inline constexpr int kMinVolume = 0;
inline constexpr int kMaxVolume = 100;
inline constexpr const char* kDefaultConfigPath = "config/default.ini";

/** @brief Window-related runtime configuration. */
struct WindowConfig {
    int scale = 3;
    bool vsync = true;
};

/** @brief Music-playback runtime configuration. */
struct AudioConfig {
    bool enabled = true;
    int master_volume = 80;
    bool music_enabled = true;
    int music_volume = 60;
};

/** @brief Complete W100h application configuration. */
struct AppConfig {
    WindowConfig window;
    AudioConfig audio;
};

/**
 * @brief Validates a configuration object.
 * @param config Configuration to validate.
 * @throws std::runtime_error If a value is outside the supported range.
 */
void validate_config(const AppConfig& config);

/**
 * @brief Loads and validates an INI configuration file.
 * @param path Path to the configuration file.
 * @return Parsed configuration.
 * @throws std::runtime_error If the file cannot be read or contains invalid data.
 */
[[nodiscard]] AppConfig load_config(const std::filesystem::path& path);

/**
 * @brief Atomically saves an application configuration.
 * @param path Destination configuration file.
 * @param config Configuration to write.
 * @throws std::runtime_error If validation or file replacement fails.
 */
void save_config(const std::filesystem::path& path, const AppConfig& config);

}  // namespace w100h::core
