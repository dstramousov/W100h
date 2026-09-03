#include "library/music_library.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TempDirectory final {
public:
    TempDirectory() {
        path_ = std::filesystem::temp_directory_path() / "w100h_music_library_test";
        std::error_code error;
        std::filesystem::remove_all(path_, error);
        std::filesystem::create_directories(path_ / "nested");
    }

    ~TempDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

void touch(const std::filesystem::path& path) {
    std::ofstream output{path};
    output << "test";
}

}  // namespace

int main() {
    namespace library = w100h::library;

    assert(library::is_supported_music_file("track.pt3"));
    assert(library::is_supported_music_file("track.PT3"));
    assert(!library::is_supported_music_file("track.s3m"));
    assert(!library::is_supported_music_file("track.txt"));

    TempDirectory directory;
    touch(directory.path() / "B.pt3");
    touch(directory.path() / "a.PT3");
    touch(directory.path() / "ignored.txt");
    touch(directory.path() / "nested" / "c.pt3");

    const std::vector<library::MusicTrack> tracks =
        library::scan_music_directory(directory.path());
    assert(tracks.size() == 3);
    assert(tracks[0].display_name == "a.PT3");
    assert(tracks[1].display_name == "B.pt3");
    assert(tracks[2].display_name == "c.pt3");

    const auto selected = library::find_track_index(tracks, directory.path() / "B.pt3");
    assert(selected.has_value());
    assert(*selected == 1);

    const auto missing = library::find_track_index(tracks, directory.path() / "missing.pt3");
    assert(!missing.has_value());

    const auto empty = library::scan_music_directory(directory.path() / "does-not-exist");
    assert(empty.empty());

    return 0;
}
