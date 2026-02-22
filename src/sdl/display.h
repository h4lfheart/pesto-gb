#pragma once
#include <string>

#include <SDL2/SDL.h>
#include <cstdint>

class Display {
public:
    static constexpr int WIDTH = 160;
    static constexpr int HEIGHT = 144;
    static constexpr int SCALE = 4;

    Display();
    ~Display();

    bool Initialize(std::string rom_name);
    void Update(uint16_t* pixels);
    void Clear();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
};
