#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace w100h::ui {

/** @brief Playback state used only for rendering the player front panel. */
enum class PlaybackVisualState {
    stopped,
    playing,
    paused,
    load_error,
};

/** @brief Transport action selected by a mouse click on the front panel. */
enum class PlayerAction {
    none,
    previous,
    toggle_play_pause,
    stop,
    next,
};

/** @brief Dynamic data required to render one W100h player frame. */
struct PlayerViewModel {
    std::string_view track_name;
    std::string_view playback_status;
    std::size_t current_track_number = 0;
    std::size_t library_size = 0;
    bool audio_available = false;
    int master_volume = 80;
    float reel_phase = 0.0F;
    int elapsed_seconds = 0;
    std::array<std::uint8_t, 6> ay_channel_levels{};
    std::uint8_t ay_chip_count = 0;
    bool ay_noise_active = false;
    bool ay_envelope_active = false;
    PlaybackVisualState playback_state = PlaybackVisualState::stopped;
};

/** @brief Owns the raster skin resources and renders the live W100h controls over them. */
class PlayerView final {
public:
    /**
     * @brief Creates the view and loads the required front-panel skin assets.
     * @param renderer SDL renderer used to create skin textures.
     * @throws std::runtime_error If a required skin asset cannot be loaded.
     */
    explicit PlayerView(SDL_Renderer* renderer);

    ~PlayerView();
    PlayerView(const PlayerView&) = delete;
    PlayerView& operator=(const PlayerView&) = delete;
    PlayerView(PlayerView&&) = delete;
    PlayerView& operator=(PlayerView&&) = delete;

    /**
     * @brief Draws the static skin and all live W100h controls into the active framebuffer.
     * @param renderer Renderer currently targeting the logical framebuffer.
     * @param model Current playback and library state.
     */
    void draw(SDL_Renderer* renderer, const PlayerViewModel& model) const;

    /**
     * @brief Maps a logical front-panel position to a transport action.
     * @param x Logical X coordinate.
     * @param y Logical Y coordinate.
     * @return Selected action, or PlayerAction::none outside transport buttons.
     */
    [[nodiscard]] static PlayerAction hit_test_transport(float x, float y) noexcept;

    /**
     * @brief Returns whether a logical position lies over the volume knob.
     * @param x Logical X coordinate.
     * @param y Logical Y coordinate.
     * @return true when the point is inside the volume interaction area.
     */
    [[nodiscard]] static bool hit_test_volume(float x, float y) noexcept;

private:
    struct TextureDeleter {
        void operator()(SDL_Texture* texture) const noexcept;
    };

    std::unique_ptr<SDL_Texture, TextureDeleter> skin_;
    std::unique_ptr<SDL_Texture, TextureDeleter> reel_frames_;
};

}  // namespace w100h::ui
