#include "display.h"
#include <cstring>
#include <string>

static constexpr uint32_t FPS_WINDOW_MS = 1000;

Display::Display() : window(nullptr), renderer(nullptr), texture(nullptr) {}

Display::~Display() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Display::Initialize(std::string rom_name) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }

    this->rom_name = rom_name;

    char name_buf[64];
    snprintf(name_buf, 64, "Pesto GB - %s", rom_name.c_str());

    window = SDL_CreateWindow(
        name_buf,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH * SCALE,
        HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        return false;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );

    if (!texture) {
        return false;
    }

    fps_window_start = SDL_GetTicks();

    Clear();

    return true;
}

void Display::UpdateFPS() {
    fps_frame_count++;

    uint32_t now = SDL_GetTicks();
    uint32_t elapsed = now - fps_window_start;

    if (elapsed >= FPS_WINDOW_MS) {
        float fps = fps_frame_count / (elapsed / 1000.0f);

        char title_buf[64];
        snprintf(title_buf, 64, "Pesto GB - %s [%.0f FPS]", rom_name.c_str(), fps);
        SDL_SetWindowTitle(window, title_buf);

        fps_frame_count = 0;
        fps_window_start = now;
    }
}

void Display::Update(uint16_t* pixels) {
    uint32_t framebuffer[WIDTH * HEIGHT] = {};

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        uint16_t color = pixels[i];
        uint8_t r = (color & 0x1F) << 3;
        uint8_t g = ((color >> 5) & 0x1F) << 3;
        uint8_t b = ((color >> 10) & 0x1F) << 3;
        framebuffer[i] = 0xFF << 24 | (r << 16) | (g << 8) | b;
    }

    SDL_UpdateTexture(texture, nullptr, framebuffer, WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);

    // pixel grid effect
    SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x10, 0x18);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int x = 1; x < WIDTH; x++) {
        SDL_Rect line = { x * SCALE - 1, 0, 1, HEIGHT * SCALE };
        SDL_RenderFillRect(renderer, &line);
    }

    for (int y = 1; y < HEIGHT; y++) {
        SDL_Rect line = { 0, y * SCALE - 1, WIDTH * SCALE, 1 };
        SDL_RenderFillRect(renderer, &line);
    }

    SDL_RenderPresent(renderer);

    UpdateFPS();
}

void Display::Clear() {
    SDL_SetRenderDrawColor(renderer, 0xCC, 0xCC, 0xCC, 0xFF);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}