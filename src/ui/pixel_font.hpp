#pragma once

#include <SDL3/SDL.h>

#include <string_view>

namespace w100h::ui {

/**
 * @brief Draws uppercase pixel text using the built-in 5x7 font.
 *
 * Unsupported characters are rendered as spaces.
 *
 * @param renderer Renderer that receives the glyph pixels. Must not be null.
 * @param x Left coordinate in logical render units.
 * @param y Top coordinate in logical render units.
 * @param text Text to render.
 * @param color Glyph color.
 * @param pixel_size Size of one font pixel in logical render units. Must be positive.
 */
void draw_text(
    SDL_Renderer* renderer,
    int x,
    int y,
    std::string_view text,
    SDL_Color color,
    int pixel_size = 1);

/**
 * @brief Returns the logical width of text rendered with the built-in 5x7 font.
 *
 * @param text Text to measure.
 * @param pixel_size Size of one font pixel in logical render units. Must be positive.
 * @return Width in logical render units.
 */
[[nodiscard]] int text_width(std::string_view text, int pixel_size = 1) noexcept;

}  // namespace w100h::ui
