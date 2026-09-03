#include "ui/player_view.hpp"

#include <array>

#include "ui/pixel_font.hpp"

namespace w100h::ui {
namespace {

constexpr SDL_Color kBackground{10, 10, 18, 255};
constexpr SDL_Color kPanel{21, 22, 36, 255};
constexpr SDL_Color kBorder{92, 93, 112, 255};
constexpr SDL_Color kText{235, 236, 232, 255};
constexpr SDL_Color kDim{145, 147, 158, 255};
constexpr SDL_Color kAccent{255, 224, 75, 255};

void fill_rect(SDL_Renderer* renderer, float x, float y, float width, float height, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_FRect rect{x, y, width, height};
    SDL_RenderFillRect(renderer, &rect);
}

void draw_rect(SDL_Renderer* renderer, float x, float y, float width, float height, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    const SDL_FRect rect{x, y, width, height};
    SDL_RenderRect(renderer, &rect);
}

void draw_spectrum_mark(SDL_Renderer* renderer) {
    constexpr std::array<SDL_Color, 4> colors{
        SDL_Color{223, 62, 87, 255}, SDL_Color{242, 193, 63, 255},
        SDL_Color{80, 190, 110, 255}, SDL_Color{77, 142, 223, 255}};

    for (std::size_t index = 0; index < colors.size(); ++index) {
        const float base_x = 265.0F + static_cast<float>(index) * 8.0F;
        for (int step = 0; step < 12; ++step) {
            fill_rect(renderer, base_x + static_cast<float>(step), 10.0F + static_cast<float>(step),
                      7.0F, 2.0F, colors[index]);
        }
    }
}

}  // namespace

void draw_player_view(SDL_Renderer* renderer, std::string_view track_name, bool audio_available) {
    if (renderer == nullptr) {
        return;
    }

    fill_rect(renderer, 0.0F, 0.0F, 320.0F, 160.0F, kBackground);
    draw_rect(renderer, 4.0F, 4.0F, 312.0F, 152.0F, kBorder);
    fill_rect(renderer, 10.0F, 10.0F, 300.0F, 28.0F, kPanel);
    draw_text(renderer, 16, 16, "W100H", kAccent, 2);
    draw_text(renderer, 92, 20, "CHIP MUSIC PLAYER", kDim);
    draw_spectrum_mark(renderer);

    fill_rect(renderer, 10.0F, 45.0F, 300.0F, 55.0F, kPanel);
    draw_text(renderer, 16, 53, track_name, kText);
    draw_text(renderer, 16, 68, "PT3 / AY-3-8910 / 50 HZ", kDim);
    draw_text(renderer, 16, 83, audio_available ? "AUDIO ONLINE" : "AUDIO UNAVAILABLE",
              audio_available ? kAccent : kDim);

    fill_rect(renderer, 10.0F, 108.0F, 300.0F, 35.0F, kPanel);
    draw_text(renderer, 18, 118, "|<<", kText);
    draw_text(renderer, 72, 118, ">", kAccent, 2);
    draw_text(renderer, 120, 118, "||", kText);
    draw_text(renderer, 173, 118, "[]", kText);
    draw_text(renderer, 229, 118, ">>|", kText);
    draw_text(renderer, 16, 146, "BOOTSTRAP BUILD - ESC QUITS", kDim);
}

}  // namespace w100h::ui
