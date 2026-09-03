#include "core/command_line.hpp"

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "core/config.hpp"

namespace w100h::core {
namespace {

[[nodiscard]] bool parse_int(std::string_view value, int& output) {
    const auto result = std::from_chars(value.data(), value.data() + value.size(), output);
    return result.ec == std::errc{} && result.ptr == value.data() + value.size();
}

}  // namespace

CommandLineOptions parse_command_line(int argc, char* argv[]) {
    CommandLineOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument{argv[index]};
        if (argument == "--scale") {
            if (index + 1 >= argc) {
                throw std::runtime_error{"--scale requires a value"};
            }
            int scale = 0;
            const std::string_view value{argv[++index]};
            if (!parse_int(value, scale) || scale < kMinScale || scale > kMaxScale) {
                throw std::runtime_error{"--scale must be an integer between 1 and 8"};
            }
            options.scale_override = scale;
        } else if (argument == "--help" || argument == "-h") {
            options.show_help = true;
        } else if (argument.starts_with('-')) {
            throw std::runtime_error{"unknown argument: " + std::string{argument}};
        } else if (options.input_path.has_value()) {
            throw std::runtime_error{"only one music file or directory may be supplied"};
        } else {
            options.input_path = std::filesystem::path{argument};
        }
    }
    return options;
}

}  // namespace w100h::core
