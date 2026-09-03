#pragma once

#include <SDL3/SDL.h>

#include <memory>

namespace w100h::render {

inline constexpr int kLogicalWidth = 320;
inline constexpr int kLogicalHeight = 160;

/** @brief Owns the pixel-native W100h SDL window, renderer, and logical framebuffer. */
class Renderer final {
public:
    /**
     * @brief Creates the W100h window and 320x160 nearest-neighbor framebuffer.
     * @param scale Integer window scale relative to the logical framebuffer.
     * @param vsync Whether VSync should be enabled initially.
     * @throws std::runtime_error If SDL resource creation fails.
     */
    Renderer(int scale, bool vsync);

    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    /** @brief Returns the owned SDL renderer for pixel UI drawing. */
    [[nodiscard]] SDL_Renderer* native_renderer() noexcept;

    /**
     * @brief Changes the integer window scale.
     * @param scale New integer scale relative to 320x160.
     * @throws std::runtime_error If SDL rejects the requested size.
     */
    void set_scale(int scale);

    /**
     * @brief Configures renderer VSync.
     * @param enabled Desired VSync state.
     * @return true when SDL accepted the setting.
     */
    [[nodiscard]] bool set_vsync(bool enabled) noexcept;

    /** @brief Selects and clears the logical framebuffer for a new frame. */
    void begin_frame();

    /** @brief Presents the logical framebuffer with nearest-neighbor scaling. */
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
