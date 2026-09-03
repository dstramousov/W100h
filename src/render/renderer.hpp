#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <optional>

namespace w100h::render {

inline constexpr int kLogicalWidth = 960;
inline constexpr int kLogicalHeight = 480;

/** @brief Owns the W100h SDL window, renderer, and skin-native framebuffer. */
class Renderer final {
public:
    /**
     * @brief Creates the W100h window and 960x480 skin-native framebuffer.
     * @param scale Integer window scale relative to the native skin resolution.
     * @param vsync Whether VSync should be enabled initially.
     * @throws std::runtime_error If SDL resource creation fails.
     */
    Renderer(int scale, bool vsync);

    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    /** @brief Returns the owned SDL renderer for UI drawing. */
    [[nodiscard]] SDL_Renderer* native_renderer() noexcept;

    /**
     * @brief Changes the integer window scale.
     * @param scale New integer scale relative to 960x480.
     * @throws std::runtime_error If SDL rejects the requested size.
     */
    void set_scale(int scale);

    /**
     * @brief Configures renderer VSync.
     * @param enabled Desired VSync state.
     * @return true when SDL accepted the setting.
     */
    [[nodiscard]] bool set_vsync(bool enabled) noexcept;

    /**
     * @brief Converts window-space mouse coordinates to logical framebuffer coordinates.
     * @param x Window-space X coordinate.
     * @param y Window-space Y coordinate.
     * @return Logical point, or std::nullopt when the position lies in letterboxing.
     */
    [[nodiscard]] std::optional<SDL_FPoint> window_to_logical(float x, float y) const noexcept;

    /** @brief Selects and clears the logical framebuffer for a new frame. */
    void begin_frame();

    /** @brief Presents the skin-native framebuffer to the window. */
    void present();

private:
    struct WindowDeleter {
        void operator()(SDL_Window* window) const noexcept;
    };
    struct RendererDeleter {
        void operator()(SDL_Renderer* renderer) const noexcept;
    };
    struct TextureDeleter {
        void operator()(SDL_Texture* texture) const noexcept;
    };

    std::unique_ptr<SDL_Window, WindowDeleter> window_;
    std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
    std::unique_ptr<SDL_Texture, TextureDeleter> framebuffer_;
};

}  // namespace w100h::render
