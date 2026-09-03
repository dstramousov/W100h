#include "core/config.hpp"

#include <charconv>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace w100h::core {
namespace {

[[nodiscard]] std::string trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return std::string{value.substr(first, last - first + 1)};
}

[[nodiscard]] bool parse_bool(std::string_view value, bool& output) {
    if (value == "true") {
        output = true;
        return true;
    }
    if (value == "false") {
        output = false;
        return true;
    }
    return false;
}

[[nodiscard]] bool parse_int(std::string_view value, int& output) {
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, output);
    return result.ec == std::errc{} && result.ptr == end;
}

}  // namespace

void validate_config(const AppConfig& config) {
    if (config.window.scale < kMinScale || config.window.scale > kMaxScale) {
        throw std::runtime_error{"window.scale must be between 1 and 8"};
    }
    const auto valid_volume = [](int volume) {
        return volume >= kMinVolume && volume <= kMaxVolume;
    };
    if (!valid_volume(config.audio.master_volume)) {
        throw std::runtime_error{"audio.master_volume must be between 0 and 100"};
    }
    if (!valid_volume(config.audio.music_volume)) {
        throw std::runtime_error{"audio.music_volume must be between 0 and 100"};
    }
}

AppConfig load_config(const std::filesystem::path& path) {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{"cannot open config file: " + path.string()};
    }

    AppConfig config;
    std::string section;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;
        const std::string stripped = trim(line);
        if (stripped.empty() || stripped.starts_with('#') || stripped.starts_with(';')) {
            continue;
        }
        if (stripped.front() == '[' && stripped.back() == ']') {
            section = trim(std::string_view{stripped}.substr(1, stripped.size() - 2));
            continue;
        }

        const auto separator = stripped.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error{"invalid config line " + std::to_string(line_number) +
                                     ": expected key=value"};
        }

        const std::string key = trim(std::string_view{stripped}.substr(0, separator));
        const std::string value = trim(std::string_view{stripped}.substr(separator + 1));
        bool bool_value = false;
        int int_value = 0;

        if (section == "window" && key == "scale") {
            if (!parse_int(value, int_value)) {
                throw std::runtime_error{"invalid integer for window.scale"};
            }
            config.window.scale = int_value;
        } else if (section == "window" && key == "vsync") {
            if (!parse_bool(value, bool_value)) {
                throw std::runtime_error{"invalid boolean for window.vsync"};
            }
            config.window.vsync = bool_value;
        } else if (section == "audio" && key == "enabled") {
            if (!parse_bool(value, bool_value)) {
                throw std::runtime_error{"invalid boolean for audio.enabled"};
            }
            config.audio.enabled = bool_value;
        } else if (section == "audio" && key == "master_volume") {
            if (!parse_int(value, int_value)) {
                throw std::runtime_error{"invalid integer for audio.master_volume"};
            }
            config.audio.master_volume = int_value;
        } else if (section == "audio" && key == "music_enabled") {
            if (!parse_bool(value, bool_value)) {
                throw std::runtime_error{"invalid boolean for audio.music_enabled"};
            }
            config.audio.music_enabled = bool_value;
        } else if (section == "audio" && key == "music_volume") {
            if (!parse_int(value, int_value)) {
                throw std::runtime_error{"invalid integer for audio.music_volume"};
            }
            config.audio.music_volume = int_value;
        }
    }

    if (!input.eof()) {
        throw std::runtime_error{"failed while reading config file: " + path.string()};
    }

    validate_config(config);
    return config;
}

void save_config(const std::filesystem::path& path, const AppConfig& config) {
    validate_config(config);
    const std::filesystem::path temporary = path.string() + ".tmp";
    {
        std::ofstream output{temporary, std::ios::trunc};
        if (!output) {
            throw std::runtime_error{"cannot write temporary config file: " + temporary.string()};
        }

        output << "[window]\n"
               << "scale=" << config.window.scale << '\n'
               << "vsync=" << (config.window.vsync ? "true" : "false") << "\n\n"
               << "[audio]\n"
               << "enabled=" << (config.audio.enabled ? "true" : "false") << '\n'
               << "master_volume=" << config.audio.master_volume << '\n'
               << "music_enabled=" << (config.audio.music_enabled ? "true" : "false") << '\n'
               << "music_volume=" << config.audio.music_volume << '\n';
        output.flush();
        if (!output) {
            throw std::runtime_error{"failed while writing config file: " + temporary.string()};
        }
    }

    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (!error) {
        return;
    }

    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(temporary);
        throw std::runtime_error{"cannot replace config file: " + error.message()};
    }
}

}  // namespace w100h::core
