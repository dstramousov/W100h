#include "app/application.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio/audio_system.hpp"
#include "core/command_line.hpp"
#include "core/config.hpp"
#include "core/version.hpp"
#include "library/music_library.hpp"
#include "render/renderer.hpp"
#include "ui/player_view.hpp"

namespace w100h::app {
namespace {

constexpr std::string_view kCurrentTrackId = "current";

enum class PlaybackState {
    stopped,
    playing,
    paused,
    load_error,
};

struct StartupLibrary {
    std::vector<library::MusicTrack> tracks;
    std::optional<std::size_t> current_index;
    bool autoplay = false;
};

class SdlGuard final {
public:
    explicit SdlGuard(bool enable_audio) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error{"SDL_Init failed: " + std::string{SDL_GetError()}};
        }
        if (enable_audio && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Audio subsystem unavailable; continuing without sound: %s", SDL_GetError());
            return;
        }
        audio_available_ = enable_audio;
    }

    ~SdlGuard() { SDL_Quit(); }
    SdlGuard(const SdlGuard&) = delete;
    SdlGuard& operator=(const SdlGuard&) = delete;

    /** @brief Returns whether the SDL audio subsystem initialized successfully. */
    [[nodiscard]] bool audio_available() const noexcept { return audio_available_; }

private:
    bool audio_available_ = false;
};

[[nodiscard]] audio::AudioMixSettings make_audio_mix_settings(const core::AudioConfig& config) {
    return audio::AudioMixSettings{
        .master_volume = config.master_volume,
        .music_enabled = config.music_enabled,
        .music_volume = config.music_volume,
    };
}

[[nodiscard]] StartupLibrary build_startup_library(const core::CommandLineOptions& options) {
    StartupLibrary result;
    if (!options.input_path.has_value()) {
        result.tracks = library::scan_music_directory(library::default_music_directory());
        if (!result.tracks.empty()) {
            result.current_index = 0;
        }
        return result;
    }

    const std::filesystem::path input = *options.input_path;
    std::error_code error;
    if (!std::filesystem::exists(input, error)) {
        if (error) {
            throw std::runtime_error{"cannot inspect input path '" + input.string() +
                                     "': " + error.message()};
        }
        throw std::runtime_error{"input path does not exist: " + input.string()};
    }

    if (std::filesystem::is_directory(input, error) && !error) {
        result.tracks = library::scan_music_directory(input);
        if (!result.tracks.empty()) {
            result.current_index = 0;
        }
        return result;
    }

    error.clear();
    if (!std::filesystem::is_regular_file(input, error) || error) {
        throw std::runtime_error{"input path is neither a regular file nor a directory: " +
                                 input.string()};
    }
    if (!library::is_supported_music_file(input)) {
        throw std::runtime_error{"unsupported music file: " + input.string()};
    }

    std::filesystem::path parent = input.parent_path();
    if (parent.empty()) {
        parent = std::filesystem::current_path();
    }
    result.tracks = library::scan_music_directory(parent);
    result.current_index = library::find_track_index(result.tracks, input);
    if (!result.current_index.has_value()) {
        throw std::runtime_error{"selected music file disappeared while scanning: " + input.string()};
    }
    result.autoplay = true;
    return result;
}

[[nodiscard]] std::string_view playback_status(
    PlaybackState state,
    bool audio_available,
    bool has_tracks) noexcept {
    if (!has_tracks) {
        return "EMPTY LIBRARY";
    }
    if (!audio_available) {
        return "AUDIO UNAVAILABLE";
    }
    switch (state) {
        case PlaybackState::playing: return "PLAYING";
        case PlaybackState::paused: return "PAUSED";
        case PlaybackState::load_error: return "LOAD ERROR";
        case PlaybackState::stopped: return "STOPPED";
    }
    return "STOPPED";
}

}  // namespace

int Application::run(int argc, char* argv[]) {
    const core::CommandLineOptions options = core::parse_command_line(argc, argv);
    if (options.show_help) {
        std::cout << "W100h v" << core::kVersion << '\n'
                  << "Usage: w100h [--scale 1..8] [music-file.pt3 | directory]\n"
                  << "Without a path W100h scans ~/Music and stays stopped.\n";
        return 0;
    }

    const core::AppConfig config = core::load_config(core::kDefaultConfigPath);
    const StartupLibrary startup = build_startup_library(options);
    const int startup_scale = options.scale_override.value_or(config.window.scale);
    const SdlGuard sdl_guard{config.audio.enabled};

    SDL_SetAppMetadata("W100h", core::kVersion.data(), "W100h");
    render::Renderer renderer{startup_scale, config.window.vsync};

    std::unique_ptr<audio::AudioSystem> audio_system;
    if (sdl_guard.audio_available()) {
        try {
            audio_system = std::make_unique<audio::AudioSystem>(make_audio_mix_settings(config.audio));
        } catch (const std::exception& error) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Audio output unavailable; continuing without sound: %s", error.what());
        }
    }

    std::vector<library::MusicTrack> tracks = startup.tracks;
    std::optional<std::size_t> current_index = startup.current_index;
    PlaybackState playback_state = PlaybackState::stopped;

    const auto play_current = [&]() {
        if (!audio_system || !current_index.has_value() || *current_index >= tracks.size()) {
            return;
        }
        try {
            audio_system->load_music(std::string{kCurrentTrackId}, tracks[*current_index].path);
            if (!audio_system->play_music(kCurrentTrackId)) {
                playback_state = PlaybackState::load_error;
                return;
            }
            playback_state = PlaybackState::playing;
        } catch (const std::exception& error) {
            playback_state = PlaybackState::load_error;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot play '%s': %s",
                        tracks[*current_index].path.string().c_str(), error.what());
        }
    };

    const auto stop_playback = [&]() {
        if (audio_system) {
            audio_system->stop_music();
        }
        playback_state = PlaybackState::stopped;
    };

    const auto toggle_play_pause = [&]() {
        if (!audio_system || !current_index.has_value()) {
            return;
        }
        if (playback_state == PlaybackState::playing) {
            audio_system->set_music_paused(true);
            playback_state = PlaybackState::paused;
        } else if (playback_state == PlaybackState::paused) {
            audio_system->set_music_paused(false);
            playback_state = PlaybackState::playing;
        } else {
            play_current();
        }
    };

    const auto move_track = [&](int direction) {
        if (tracks.empty()) {
            return;
        }
        const std::size_t count = tracks.size();
        const std::size_t current = current_index.value_or(0);
        current_index = direction < 0 ? (current + count - 1) % count : (current + 1) % count;
        play_current();
    };

    if (startup.autoplay) {
        play_current();
    }

    SDL_Log("W100h v%s: logical=%dx%d scale=%d window=%dx%d tracks=%zu startup=%s",
            core::kVersion.data(), render::kLogicalWidth, render::kLogicalHeight, startup_scale,
            render::kLogicalWidth * startup_scale, render::kLogicalHeight * startup_scale,
            tracks.size(), startup.autoplay ? "autoplay-selected-file" : "stopped");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                continue;
            }
            if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
                continue;
            }

            switch (event.key.key) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_SPACE:
                    toggle_play_pause();
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    play_current();
                    break;
                case SDLK_LEFT:
                    move_track(-1);
                    break;
                case SDLK_RIGHT:
                    move_track(1);
                    break;
                case SDLK_S:
                    stop_playback();
                    break;
                default:
                    break;
            }
        }

        const bool audio_available = audio_system != nullptr;
        const std::string_view track_name =
            current_index.has_value() && *current_index < tracks.size()
                ? std::string_view{tracks[*current_index].display_name}
                : std::string_view{"NO PT3 FILES"};

        renderer.begin_frame();
        ui::draw_player_view(renderer.native_renderer(), track_name,
                             playback_status(playback_state, audio_available, !tracks.empty()),
                             tracks.size(), audio_available);
        renderer.present();
    }

    return 0;
}

}  // namespace w100h::app
