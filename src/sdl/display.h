#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

class Display {
public:
    static constexpr int WIDTH = 160;
    static constexpr int HEIGHT = 144;
    static constexpr int SCALE = 4;

    Display();
    ~Display();

    bool Initialize();
    void Update(uint8_t* pixels);
    void Clear();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    uint32_t palette[4] = {
        0xFFF1F7D2,
        0xFFC5D39B,
        0xFF7FA06B,
        0xFF3E5A3C
    };
};