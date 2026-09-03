#include "app/application.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "audio/audio_system.hpp"
#include "core/command_line.hpp"
#include "core/config.hpp"
#include "core/version.hpp"
#include "render/renderer.hpp"
#include "ui/player_view.hpp"

namespace w100h::app {
namespace {

constexpr const char* kBundledTrackPath = "assets/audio/music/Pator - August Melancholy.pt3";
constexpr const char* kBundledTrackName = "PATOR - AUGUST MELANCHOLY.PT3";

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

}  // namespace

int Application::run(int argc, char* argv[]) {
    const core::CommandLineOptions options = core::parse_command_line(argc, argv);
    if (options.show_help) {
        std::cout << "W100h v" << core::kVersion << '\n'
                  << "Usage: w100h [--scale 1..8]\n";
        return 0;
    }

    const core::AppConfig config = core::load_config(core::kDefaultConfigPath);
    const int startup_scale = options.scale_override.value_or(config.window.scale);
    const SdlGuard sdl_guard{config.audio.enabled};

    SDL_SetAppMetadata("W100h", core::kVersion.data(), "W100h");
    render::Renderer renderer{startup_scale, config.window.vsync};

    std::unique_ptr<audio::AudioSystem> audio_system;
    bool audio_playing = false;
    if (sdl_guard.audio_available()) {
        try {
            audio_system = std::make_unique<audio::AudioSystem>(make_audio_mix_settings(config.audio));
            audio_system->load_music("demo", kBundledTrackPath);
            audio_playing = audio_system->play_music("demo");
            if (!audio_playing) {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Bundled PT3 music is not loaded");
            }
        } catch (const std::exception& error) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Audio output unavailable; continuing without sound: %s", error.what());
            audio_system.reset();
        }
    }

    SDL_Log("W100h v%s: logical=%dx%d scale=%d window=%dx%d PT3=%s", core::kVersion.data(),
            render::kLogicalWidth, render::kLogicalHeight, startup_scale,
            render::kLogicalWidth * startup_scale, render::kLogicalHeight * startup_scale,
            audio_playing ? "playing" : "unavailable");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                       event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        renderer.begin_frame();
        ui::draw_player_view(renderer.native_renderer(), kBundledTrackName, audio_playing);
        renderer.present();
    }

    return 0;
}

}  // namespace w100h::app
