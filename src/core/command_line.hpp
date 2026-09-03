#pragma once

#include <optional>

namespace w100h::core {

/** @brief Parsed W100h command-line options. */
struct CommandLineOptions {
    std::optional<int> scale_override;
    bool show_help = false;
};

/**
 * @brief Parses W100h command-line options.
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @return Parsed options.
 * @throws std::runtime_error If an option is unknown or malformed.
 */
[[nodiscard]] CommandLineOptions parse_command_line(int argc, char* argv[]);

}  // namespace w100h::core
