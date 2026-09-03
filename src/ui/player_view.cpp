#include "ui/player_view.hpp"

#include <SDL3/SDL_filesystem.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "ui/pixel_font.hpp"

namespace w100h::ui {
namespace {

constexpr SDL_Color kText{205, 205, 198, 255};
constexpr SDL_Color kDim{126, 126, 120, 255};
constexpr SDL_Color kDisplay{67, 214, 234, 255};
constexpr SDL_Color kPlay{83, 238, 70, 255};
constexpr SDL_Color kPause{238, 190, 65, 255};
constexpr SDL_Color kError{228, 76, 65, 255};
constexpr SDL_Color kMetalLight{213, 207, 190, 255};
constexpr SDL_Color kVolumeGlow{76, 255, 108, 255};
constexpr SDL_Color kMeterGreen{64, 205, 86, 255};
constexpr SDL_Color kMeterYellow{224, 212, 63, 255};
constexpr SDL_Color kMeterOrange{238, 145, 47, 255};
constexpr SDL_Color kMeterRed{228, 67, 49, 255};
constexpr SDL_Color kMeterOff{20, 31, 27, 255};
constexpr SDL_Color kMeterFrame{50, 65, 67, 255};

constexpr SDL_FRect kPreviousButton{86.0F, 400.0F, 168.0F, 46.0F};
constexpr SDL_FRect kPlayPauseButton{269.0F, 400.0F, 173.0F, 46.0F};
constexpr SDL_FRect kStopButton{456.0F, 400.0F, 167.0F, 46.0F};
constexpr SDL_FRect kNextButton{635.0F, 400.0F, 168.0F, 46.0F};
constexpr SDL_FPoint kVolumeCenter{833.0F, 96.0F};
constexpr float kVolumeRadius = 51.0F;
constexpr float kPi = 3.14159265358979323846F;
constexpr int kReelFrameCount = 12;
constexpr int kReelFrameSize = 104;

[[nodiscard]] bool contains(const SDL_FRect& rect, float x, float y) noexcept {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

void set_color(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fill_rect(SDL_Renderer* renderer, const SDL_FRect& rect, SDL_Color color) {
    set_color(renderer, color);
    SDL_RenderFillRect(renderer, &rect);
}

void draw_line(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, SDL_Color color) {
    set_color(renderer, color);
    SDL_RenderLine(renderer, x1, y1, x2, y2);
}

void draw_centered_text(
    SDL_Renderer* renderer,
    const SDL_FRect& rect,
    int y,
    std::string_view text,
    SDL_Color color,
    int pixel_size) {
    const int width = text_width(text, pixel_size);
    const int x = static_cast<int>(rect.x + (rect.w - static_cast<float>(width)) / 2.0F);
    draw_text(renderer, x, y, text, color, pixel_size);
}

[[nodiscard]] std::filesystem::path asset_path(std::string_view filename) {
    const char* base_path = SDL_GetBasePath();
    if (base_path == nullptr) {
        throw std::runtime_error{"SDL_GetBasePath failed: " + std::string{SDL_GetError()}};
    }
    return std::filesystem::path{base_path} / "assets" / "ui" / filename;
}

[[nodiscard]] SDL_Texture* load_required_bmp(SDL_Renderer* renderer, std::string_view filename) {
    const std::filesystem::path path = asset_path(filename);
    SDL_Surface* surface = SDL_LoadBMP(path.string().c_str());
    if (surface == nullptr) {
        throw std::runtime_error{"cannot load UI asset '" + path.string() + "': " +
                                 std::string{SDL_GetError()}};
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr) {
        throw std::runtime_error{"cannot create UI texture '" + path.string() + "': " +
                                 std::string{SDL_GetError()}};
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    return texture;
}

[[nodiscard]] std::string clipped_track_name(std::string_view name) {
    constexpr std::size_t kMaxCharacters = 48;
    if (name.size() <= kMaxCharacters) {
        return std::string{name};
    }
    std::string result{name.substr(0, kMaxCharacters - 3)};
    result += "...";
    return result;
}

[[nodiscard]] std::string format_elapsed(int elapsed_seconds) {
    const int clamped = std::max(elapsed_seconds, 0);
    const int minutes = clamped / 60;
    const int seconds = clamped % 60;
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    return buffer;
}

void draw_reels(SDL_Renderer* renderer, SDL_Texture* frames, float reel_phase) {
    if (frames == nullptr) {
        return;
    }
    const float normalized = std::fmod(std::max(reel_phase, 0.0F), 2.0F * kPi) / (2.0F * kPi);
    const int frame_index = std::clamp(
        static_cast<int>(normalized * static_cast<float>(kReelFrameCount)), 0,
        kReelFrameCount - 1);
    const int second_frame = (frame_index + 2) % kReelFrameCount;

    const SDL_FRect left_source{static_cast<float>(frame_index * kReelFrameSize), 0.0F,
                               static_cast<float>(kReelFrameSize),
                               static_cast<float>(kReelFrameSize)};
    const SDL_FRect right_source{static_cast<float>(second_frame * kReelFrameSize),
                                static_cast<float>(kReelFrameSize),
                                static_cast<float>(kReelFrameSize),
                                static_cast<float>(kReelFrameSize)};
    // The reel strip now contains the photographed outer cassette engagement rings,
    // not a second synthetic spindle drawn on top of the real one.
    const SDL_FRect left_dest{226.0F, 168.0F, 104.0F, 104.0F};
    const SDL_FRect right_dest{474.0F, 168.0F, 104.0F, 104.0F};
    SDL_RenderTexture(renderer, frames, &left_source, &left_dest);
    SDL_RenderTexture(renderer, frames, &right_source, &right_dest);
}

void draw_volume_pointer(SDL_Renderer* renderer, int volume) {
    const float normalized = static_cast<float>(std::clamp(volume, 0, 100)) / 100.0F;
    const float angle = (0.75F + normalized * 1.5F) * kPi;
    const float inner = 25.0F;
    const float outer = 39.0F;
    const float marker_radius = 40.0F;

    const float x1 = kVolumeCenter.x + std::cos(angle) * inner;
    const float y1 = kVolumeCenter.y + std::sin(angle) * inner;
    const float x2 = kVolumeCenter.x + std::cos(angle) * outer;
    const float y2 = kVolumeCenter.y + std::sin(angle) * outer;
    draw_line(renderer, x1, y1, x2, y2, kVolumeGlow);

    const float marker_x = kVolumeCenter.x + std::cos(angle) * marker_radius;
    const float marker_y = kVolumeCenter.y + std::sin(angle) * marker_radius;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    fill_rect(renderer, SDL_FRect{marker_x - 5.0F, marker_y - 5.0F, 10.0F, 10.0F},
              SDL_Color{kVolumeGlow.r, kVolumeGlow.g, kVolumeGlow.b, 38});
    fill_rect(renderer, SDL_FRect{marker_x - 3.0F, marker_y - 3.0F, 6.0F, 6.0F},
              SDL_Color{kVolumeGlow.r, kVolumeGlow.g, kVolumeGlow.b, 92});
    fill_rect(renderer, SDL_FRect{marker_x - 1.5F, marker_y - 1.5F, 3.0F, 3.0F}, kVolumeGlow);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    char value[16]{};
    std::snprintf(value, sizeof(value), "VOL %d", std::clamp(volume, 0, 100));
    const SDL_FRect value_area{801.0F, 145.0F, 66.0F, 14.0F};
    draw_centered_text(renderer, value_area, 146, value, kDim, 1);
}

[[nodiscard]] SDL_Color meter_color(int segment) {
    if (segment >= 8) {
        return kMeterRed;
    }
    if (segment >= 6) {
        return kMeterOrange;
    }
    if (segment >= 4) {
        return kMeterYellow;
    }
    return kMeterGreen;
}

void draw_meters(SDL_Renderer* renderer, const PlayerViewModel& model) {
    constexpr std::array<std::string_view, 6> labels{"1A", "1B", "1C", "2A", "2B", "2C"};
    constexpr std::array<float, 6> x_positions{767.0F, 789.0F, 811.0F,
                                                849.0F, 871.0F, 893.0F};
    constexpr int kSegments = 10;

    draw_line(renderer, 835.0F, 211.0F, 835.0F, 317.0F, kMeterFrame);
    for (std::size_t column = 0; column < labels.size(); ++column) {
        const bool chip_active = column < 3 ? model.ay_chip_count >= 1 : model.ay_chip_count >= 2;
        draw_text(renderer, static_cast<int>(x_positions[column] - 2.0F), 210, labels[column],
                  chip_active ? kText : kDim, 1);

        const int level = chip_active
                              ? std::clamp(static_cast<int>(model.ay_channel_levels[column]), 0, 15)
                              : 0;
        const int active_segments = (level * kSegments + 14) / 15;
        for (int segment = 0; segment < kSegments; ++segment) {
            const float y = 310.0F - static_cast<float>(segment) * 8.0F;
            const SDL_Color color =
                segment < active_segments ? meter_color(segment) : kMeterOff;
            fill_rect(renderer, SDL_FRect{x_positions[column], y, 10.0F, 5.0F}, color);
        }
    }

    draw_text(renderer, 773, 320, "N", model.ay_noise_active ? kText : kDim, 1);
    fill_rect(renderer, SDL_FRect{785.0F, 321.0F, 8.0F, 5.0F},
              model.ay_noise_active ? kMeterGreen : kMeterOff);
    draw_text(renderer, 877, 320, "E", model.ay_envelope_active ? kText : kDim, 1);
    fill_rect(renderer, SDL_FRect{889.0F, 321.0F, 8.0F, 5.0F},
              model.ay_envelope_active ? kMeterGreen : kMeterOff);
}

void draw_display(SDL_Renderer* renderer, const PlayerViewModel& model) {
    const SDL_Color state_color = model.playback_state == PlaybackVisualState::load_error
                                      ? kError
                                      : (model.playback_state == PlaybackVisualState::playing
                                             ? kDisplay
                                             : kDim);

    draw_text(renderer, 106, 356,
              model.playback_state == PlaybackVisualState::playing ? ">" : " ", state_color, 2);
    draw_text(renderer, 126, 356, clipped_track_name(model.track_name), kDisplay, 2);

    std::string track_text = "TRACK --/--";
    if (model.current_track_number > 0 && model.library_size > 0) {
        track_text = "TRACK " + std::to_string(model.current_track_number) + "/" +
                     std::to_string(model.library_size);
    }
    draw_text(renderer, 126, 375, track_text, kDisplay, 1);
    draw_text(renderer, 393, 375, model.playback_status, state_color, 1);
    draw_text(renderer, 654, 375, format_elapsed(model.elapsed_seconds), kDisplay, 1);
}

void draw_keycap_label(
    SDL_Renderer* renderer,
    const SDL_FRect& rect,
    std::string_view label) {
    draw_centered_text(renderer, rect, static_cast<int>(rect.y + 6.0F), label, kText, 1);
}

void draw_transport(SDL_Renderer* renderer, const PlayerViewModel& model) {
    draw_centered_text(renderer, kPreviousButton, 414, "<<", kText, 3);
    draw_centered_text(renderer, kNextButton, 414, ">>", kText, 3);
    draw_centered_text(renderer, kStopButton, 414, "[]",
                       model.playback_state == PlaybackVisualState::stopped ? kText : kDim, 3);

    const SDL_FRect play_symbol_area{303.0F, 400.0F, 55.0F, 46.0F};
    const SDL_FRect pause_symbol_area{359.0F, 400.0F, 55.0F, 46.0F};
    const SDL_Color play_color = model.playback_state == PlaybackVisualState::playing ? kPlay : kDim;
    const SDL_Color pause_color = model.playback_state == PlaybackVisualState::paused ? kPause : kDim;
    draw_centered_text(renderer, play_symbol_area, 414, ">", play_color, 3);
    draw_centered_text(renderer, pause_symbol_area, 414, "II", pause_color, 3);

    draw_keycap_label(renderer, SDL_FRect{143.0F, 458.0F, 57.0F, 20.0F}, "[LEFT]");
    draw_keycap_label(renderer, SDL_FRect{326.0F, 458.0F, 60.0F, 20.0F}, "[SPACE]");
    draw_keycap_label(renderer, SDL_FRect{521.0F, 458.0F, 31.0F, 20.0F}, "[S]");
    draw_keycap_label(renderer, SDL_FRect{691.0F, 458.0F, 59.0F, 20.0F}, "[RIGHT]");
    draw_text(renderer, 18, 468, "ESC QUIT", kDim, 1);
}

}  // namespace

void PlayerView::TextureDeleter::operator()(SDL_Texture* texture) const noexcept {
    SDL_DestroyTexture(texture);
}

PlayerView::PlayerView(SDL_Renderer* renderer) {
    if (renderer == nullptr) {
        throw std::runtime_error{"PlayerView requires a valid SDL renderer"};
    }
    skin_.reset(load_required_bmp(renderer, "player_skin.bmp"));
    reel_frames_.reset(load_required_bmp(renderer, "reel_frames.bmp"));
}

PlayerView::~PlayerView() = default;

void PlayerView::draw(SDL_Renderer* renderer, const PlayerViewModel& model) const {
    if (renderer == nullptr) {
        return;
    }

    const SDL_FRect destination{0.0F, 0.0F, 960.0F, 480.0F};
    SDL_RenderTexture(renderer, skin_.get(), nullptr, &destination);
    draw_reels(renderer, reel_frames_.get(), model.reel_phase);
    draw_volume_pointer(renderer, model.master_volume);
    draw_meters(renderer, model);
    draw_display(renderer, model);
    draw_transport(renderer, model);
}

PlayerAction PlayerView::hit_test_transport(float x, float y) noexcept {
    if (contains(kPreviousButton, x, y)) {
        return PlayerAction::previous;
    }
    if (contains(kPlayPauseButton, x, y)) {
        return PlayerAction::toggle_play_pause;
    }
    if (contains(kStopButton, x, y)) {
        return PlayerAction::stop;
    }
    if (contains(kNextButton, x, y)) {
        return PlayerAction::next;
    }
    return PlayerAction::none;
}

bool PlayerView::hit_test_volume(float x, float y) noexcept {
    const float dx = x - kVolumeCenter.x;
    const float dy = y - kVolumeCenter.y;
    return dx * dx + dy * dy <= kVolumeRadius * kVolumeRadius;
}

}  // namespace w100h::ui
