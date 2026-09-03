#pragma once

#include <SDL3/SDL.h>

#include <string_view>

namespace w100h::ui {

/**
 * @brief Draws the initial single-window W100h player shell.
 * @param renderer Renderer currently targeting the logical framebuffer.
 * @param track_name Display name for the current track.
 * @param audio_available Whether the SDL audio path initialized successfully.
 */
void draw_player_view(SDL_Renderer* renderer, std::string_view track_name, bool audio_available);

}  // namespace w100h::ui
