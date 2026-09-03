#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <string_view>

namespace w100h::ui {

/**
 * @brief Draws the bootstrap W100h player shell with live library/playback state.
 * @param renderer Renderer currently targeting the logical framebuffer.
 * @param track_name Display name for the selected track, or an empty-library label.
 * @param playback_status Human-readable playback state.
 * @param library_size Number of supported tracks in the active library.
 * @param audio_available Whether the SDL audio path initialized successfully.
 */
void draw_player_view(
    SDL_Renderer* renderer,
    std::string_view track_name,
    std::string_view playback_status,
    std::size_t library_size,
    bool audio_available);

}  // namespace w100h::ui
