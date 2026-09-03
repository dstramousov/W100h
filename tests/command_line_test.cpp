#include "core/command_line.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

w100h::core::CommandLineOptions parse(std::vector<std::string> values) {
    std::vector<char*> argv;
    argv.reserve(values.size());
    for (std::string& value : values) {
        argv.push_back(value.data());
    }
    return w100h::core::parse_command_line(static_cast<int>(argv.size()), argv.data());
}

}  // namespace

int main() {
    {
        const auto options = parse({"w100h"});
        assert(!options.scale_override.has_value());
        assert(!options.input_path.has_value());
        assert(!options.show_help);
    }
    {
        const auto options = parse({"w100h", "--scale", "4", "/tmp/music.pt3"});
        assert(options.scale_override == 4);
        assert(options.input_path.has_value());
        assert(options.input_path->string() == "/tmp/music.pt3");
    }
    {
        const auto options = parse({"w100h", "/tmp/Music"});
        assert(options.input_path.has_value());
        assert(options.input_path->string() == "/tmp/Music");
    }
    {
        bool rejected = false;
        try {
            [[maybe_unused]] const auto ignored =
                parse({"w100h", "/tmp/one.pt3", "/tmp/two.pt3"});
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        assert(rejected);
    }
    return 0;
}
