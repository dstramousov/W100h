#include "ui/pixel_font.hpp"

#include <array>
#include <cctype>
#include <cstdint>

namespace w100h::ui {
namespace {

using Glyph = std::array<std::uint8_t, 7>;

constexpr Glyph kBlank{};

[[nodiscard]] constexpr Glyph glyph_for(char character) noexcept {
    switch (character) {
        case 'A': return {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
        case 'B': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110};
        case 'C': return {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111};
        case 'D': return {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110};
        case 'E': return {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111};
        case 'F': return {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000};
        case 'G': return {0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111};
        case 'H': return {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
        case 'I': return {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111};
        case 'J': return {0b00111, 0b00010, 0b00010, 0b00010, 0b10010, 0b10010, 0b01100};
        case 'K': return {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001};
        case 'L': return {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
        case 'M': return {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001};
        case 'N': return {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001};
        case 'O': return {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
        case 'P': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000};
        case 'Q': return {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101};
        case 'R': return {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001};
        case 'S': return {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110};
        case 'T': return {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
        case 'U': return {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
        case 'V': return {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100};
        case 'W': return {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001};
        case 'X': return {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001};
        case 'Y': return {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100};
        case 'Z': return {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111};
        case '0': return {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110};
        case '1': return {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
        case '2': return {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111};
        case '3': return {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110};
        case '4': return {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010};
        case '5': return {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110};
        case '6': return {0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110};
        case '7': return {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000};
        case '8': return {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110};
        case '9': return {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110};
        case '<': return {0b00010, 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, 0b00010};
        case '>': return {0b01000, 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, 0b01000};
        case '-': return {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000};
        case ':': return {0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000};
        case '.': return {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100};
        case ' ': return kBlank;
        default: return kBlank;
    }
}

[[nodiscard]] constexpr char normalized_character(char character) noexcept {
    if (character >= 'a' && character <= 'z') {
        return static_cast<char>(character - ('a' - 'A'));
    }
    return character;
}

}  // namespace

void draw_text(
    SDL_Renderer* renderer,
    int x,
    int y,
    std::string_view text,
    SDL_Color color,
    int pixel_size) {
    if (renderer == nullptr || pixel_size <= 0) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    int cursor_x = x;
    for (const char raw_character : text) {
        const Glyph glyph = glyph_for(normalized_character(raw_character));
        for (std::size_t row = 0; row < glyph.size(); ++row) {
            for (int column = 0; column < 5; ++column) {
                const std::uint8_t mask = static_cast<std::uint8_t>(1U << (4 - column));
                if ((glyph[row] & mask) == 0U) {
                    continue;
                }

                const SDL_FRect pixel{
                    static_cast<float>(cursor_x + column * pixel_size),
                    static_cast<float>(y + static_cast<int>(row) * pixel_size),
                    static_cast<float>(pixel_size),
                    static_cast<float>(pixel_size)};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
        cursor_x += 6 * pixel_size;
    }
}

int text_width(std::string_view text, int pixel_size) noexcept {
    if (text.empty() || pixel_size <= 0) {
        return 0;
    }
    return static_cast<int>(text.size()) * 6 * pixel_size - pixel_size;
}

}  // namespace w100h::ui
