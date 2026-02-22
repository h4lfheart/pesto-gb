#include <iostream>
#include <thread>

#include "emulator/gameboy.h"
#include "sdl/display.h"
#include "sdl/audio.h"

int main(int argc, char** argv)
{
    GameBoy game_boy(argv[1], argv[2]);

    auto display = Display();
    if (!display.Initialize()) {
        fprintf(stderr, "Failed to initialize display");
        return 1;
    }

    game_boy.OnDraw([&](uint16_t* data)
    {
        display.Update(data);
    });

    auto audio = Audio();
    if (!audio.Initialize()) {
        fprintf(stderr, "Failed to initialize audio");
        return 1;
    }

    game_boy.OnAudio([&](float left, float right)
    {
        audio.PushSample(left, right);
    });

    const auto rom_path = std::string(argv[2]);
    const auto save_path = rom_path.substr(0, rom_path.find_last_of('.')) + ".sav";
    game_boy.ReadSave(save_path.c_str());

    std::thread game_thread([&]{
        game_boy.Run();
    });

    while (game_boy.IsRunning())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game_boy.Stop();
                break;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch(event.key.keysym.sym)
                {
                case SDLK_UP:
                    game_boy.PressButton(InputButton::BUTTON_UP);
                    break;
                case SDLK_DOWN:
                    game_boy.PressButton(InputButton::BUTTON_DOWN);
                    break;
                case SDLK_LEFT:
                    game_boy.PressButton(InputButton::BUTTON_LEFT);
                    break;
                case SDLK_RIGHT:
                    game_boy.PressButton(InputButton::BUTTON_RIGHT);
                    break;
                case SDLK_x:
                    game_boy.PressButton(InputButton::BUTTON_A);
                    break;
                case SDLK_z:
                    game_boy.PressButton(InputButton::BUTTON_B);
                    break;
                case SDLK_RETURN:
                    game_boy.PressButton(InputButton::BUTTON_START);
                    break;
                case SDLK_RSHIFT:
                    game_boy.PressButton(InputButton::BUTTON_SELECT);
                    break;
                }
            }

            if (event.type == SDL_KEYUP)
            {
                switch(event.key.keysym.sym)
                {
                case SDLK_UP:
                    game_boy.ReleaseButton(InputButton::BUTTON_UP);
                    break;
                case SDLK_DOWN:
                    game_boy.ReleaseButton(InputButton::BUTTON_DOWN);
                    break;
                case SDLK_LEFT:
                    game_boy.ReleaseButton(InputButton::BUTTON_LEFT);
                    break;
                case SDLK_RIGHT:
                    game_boy.ReleaseButton(InputButton::BUTTON_RIGHT);
                    break;
                case SDLK_x:
                    game_boy.ReleaseButton(InputButton::BUTTON_A);
                    break;
                case SDLK_z:
                    game_boy.ReleaseButton(InputButton::BUTTON_B);
                    break;
                case SDLK_RETURN:
                    game_boy.ReleaseButton(InputButton::BUTTON_START);
                    break;
                case SDLK_RSHIFT:
                    game_boy.ReleaseButton(InputButton::BUTTON_SELECT);
                    break;
                }
            }
        }
    }

    game_boy.WriteSave(save_path.c_str());
    game_thread.join();
    audio.Close();

    return 0;
}