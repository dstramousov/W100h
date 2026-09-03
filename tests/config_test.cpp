#include "core/config.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {

class TempFile final {
public:
    explicit TempFile(std::filesystem::path path) : path_{std::move(path)} {}
    ~TempFile() {
        std::error_code error;
        std::filesystem::remove(path_, error);
        std::filesystem::remove(path_.string() + ".tmp", error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

}  // namespace

int main() {
    namespace core = w100h::core;

    core::AppConfig defaults;
    assert(defaults.window.scale == 3);
    assert(defaults.window.vsync);
    assert(defaults.audio.enabled);
    assert(defaults.audio.master_volume == 80);
    assert(defaults.audio.music_enabled);
    assert(defaults.audio.music_volume == 60);

    TempFile round_trip{"w100h_config_test.ini"};
    defaults.window.scale = 4;
    defaults.window.vsync = false;
    defaults.audio.master_volume = 65;
    defaults.audio.music_enabled = false;
    defaults.audio.music_volume = 25;
    core::save_config(round_trip.path(), defaults);

    const core::AppConfig loaded = core::load_config(round_trip.path());
    assert(loaded.window.scale == 4);
    assert(!loaded.window.vsync);
    assert(loaded.audio.master_volume == 65);
    assert(!loaded.audio.music_enabled);
    assert(loaded.audio.music_volume == 25);

    TempFile invalid{"w100h_config_invalid_test.ini"};
    {
        std::ofstream output{invalid.path()};
        output << "[audio]\n"
               << "master_volume=101\n";
    }
    bool invalid_rejected = false;
    try {
        [[maybe_unused]] const auto ignored = core::load_config(invalid.path());
    } catch (const std::runtime_error&) {
        invalid_rejected = true;
    }
    assert(invalid_rejected);

    return 0;
}
