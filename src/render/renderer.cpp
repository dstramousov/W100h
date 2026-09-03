#include "render/renderer.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace w100h::render {
namespace {

[[nodiscard]] SDL_FRect letterbox_destination(int output_width, int output_height) {
    const float scale_x = static_cast<float>(output_width) / static_cast<float>(kLogicalWidth);
    const float scale_y = static_cast<float>(output_height) / static_cast<float>(kLogicalHeight);
    const float scale = std::min(scale_x, scale_y);
    const float width = static_cast<float>(kLogicalWidth) * scale;
    const float height = static_cast<float>(kLogicalHeight) * scale;
    return SDL_FRect{(static_cast<float>(output_width) - width) / 2.0F,
                     (static_cast<float>(output_height) - height) / 2.0F, width, height};
}

}  // namespace

void Renderer::WindowDeleter::operator()(SDL_Window* window) const noexcept {
    SDL_DestroyWindow(window);
}

void Renderer::RendererDeleter::operator()(SDL_Renderer* renderer) const noexcept {
    SDL_DestroyRenderer(renderer);
}

void Renderer::TextureDeleter::operator()(SDL_Texture* texture) const noexcept {
    SDL_DestroyTexture(texture);
}

Renderer::Renderer(int scale, bool vsync) {
    SDL_Window* raw_window = nullptr;
    SDL_Renderer* raw_renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("W100h", kLogicalWidth * scale, kLogicalHeight * scale, 0,
                                     &raw_window, &raw_renderer)) {
        throw std::runtime_error{"SDL_CreateWindowAndRenderer failed: " +
                                 std::string{SDL_GetError()}};
    }
    window_.reset(raw_window);
    renderer_.reset(raw_renderer);

    SDL_Texture* raw_framebuffer =
        SDL_CreateTexture(renderer_.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                          kLogicalWidth, kLogicalHeight);
    if (raw_framebuffer == nullptr) {
        throw std::runtime_error{"SDL_CreateTexture(framebuffer) failed: " +
                                 std::string{SDL_GetError()}};
    }
    framebuffer_.reset(raw_framebuffer);
    if (!SDL_SetTextureScaleMode(framebuffer_.get(), SDL_SCALEMODE_NEAREST)) {
        throw std::runtime_error{"SDL_SetTextureScaleMode failed: " +
                                 std::string{SDL_GetError()}};
    }

    if (!set_vsync(vsync)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not configure VSync: %s", SDL_GetError());
    }
}

SDL_Renderer* Renderer::native_renderer() noexcept {
    return renderer_.get();
}

void Renderer::set_scale(int scale) {
    if (!SDL_SetWindowSize(window_.get(), kLogicalWidth * scale, kLogicalHeight * scale)) {
        throw std::runtime_error{"SDL_SetWindowSize failed: " + std::string{SDL_GetError()}};
    }
}

bool Renderer::set_vsync(bool enabled) noexcept {
    return SDL_SetRenderVSync(renderer_.get(), enabled ? 1 : 0);
}

void Renderer::begin_frame() {
    if (!SDL_SetRenderTarget(renderer_.get(), framebuffer_.get())) {
        throw std::runtime_error{"SDL_SetRenderTarget(framebuffer) failed: " +
                                 std::string{SDL_GetError()}};
    }
    if (!SDL_SetRenderScale(renderer_.get(), 1.0F, 1.0F)) {
        throw std::runtime_error{"SDL_SetRenderScale failed: " + std::string{SDL_GetError()}};
    }
    SDL_SetRenderDrawColor(renderer_.get(), 10, 10, 18, 255);
    SDL_RenderClear(renderer_.get());
}

void Renderer::present() {
    if (!SDL_SetRenderTarget(renderer_.get(), nullptr)) {
        throw std::runtime_error{"SDL_SetRenderTarget(window) failed: " +
                                 std::string{SDL_GetError()}};
    }
    if (!SDL_SetRenderScale(renderer_.get(), 1.0F, 1.0F)) {
        throw std::runtime_error{"SDL_SetRenderScale failed: " + std::string{SDL_GetError()}};
    }

    SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
    SDL_RenderClear(renderer_.get());

    int output_width = 0;
    int output_height = 0;
    if (!SDL_GetRenderOutputSize(renderer_.get(), &output_width, &output_height)) {
        throw std::runtime_error{"SDL_GetRenderOutputSize failed: " +
                                 std::string{SDL_GetError()}};
    }

    const SDL_FRect source{0.0F, 0.0F, static_cast<float>(kLogicalWidth),
                           static_cast<float>(kLogicalHeight)};
    const SDL_FRect destination = letterbox_destination(output_width, output_height);
    if (!SDL_RenderTexture(renderer_.get(), framebuffer_.get(), &source, &destination)) {
        throw std::runtime_error{"SDL_RenderTexture(framebuffer) failed: " +
                                 std::string{SDL_GetError()}};
    }
    SDL_RenderPresent(renderer_.get());
}

}  // namespace w100h::render
