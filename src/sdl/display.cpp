#include "display.h"
#include <cstring>

Display::Display() : window(nullptr), renderer(nullptr), texture(nullptr) {}

Display::~Display() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Display::Initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }

    window = SDL_CreateWindow(
        "Pesto GB",
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

    Clear();

    return true;
}

void Display::Update(uint8_t* pixels) {
    uint32_t framebuffer[WIDTH * HEIGHT] = {};

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        framebuffer[i] = palette[pixels[i]];
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
}

void Display::Clear() {
    SDL_SetRenderDrawColor(renderer, 0xCC, 0xCC, 0xCC, 0xFF);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}