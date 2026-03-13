#include <iostream>
#include <thread>
#include <filesystem>

#include "gameboy.h"
#include "sdl/display.h"
#include "sdl/audio.h"

#include "argparse/argparse.hpp"

int main(int argc, char** argv)
{
    argparse::ArgumentParser arguments("PestoGB");

    arguments.add_argument("rom")
        .help("The path for your gameboy or gameboy color rom file.")
        .required();

    arguments.add_argument("--dmg-bootrom")
        .help("The path for your gameboy boot rom file.")
        .required();

    arguments.add_argument("--cgb-bootrom")
        .help("The path for your gameboy color boot rom file.");

    try {
        arguments.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << arguments;

        printf("Press any key to exit...\n");
        std::cin.get();
        return 1;
    }

    auto rom_path = arguments.get<std::string>("rom");

    GameBoy game_boy(rom_path);

    if (auto cgb_boot_path = arguments.present<std::string>("--cgb-bootrom");
        cgb_boot_path.has_value() &&game_boy.IsCGBGame())
    {
        game_boy.LoadBootRom(*cgb_boot_path);
    }
    else
    {
        auto dmg_boot_path = arguments.get<std::string>("--dmg-bootrom");
        game_boy.LoadBootRom(dmg_boot_path);
    }

    std::filesystem::path path(rom_path);

    auto display = Display();
    if (!display.Initialize(path.filename().string())) {
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

    std::string save_path = (path.parent_path() / (path.stem().string() + ".sav")).string();
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
                case SDLK_TAB:
                    game_boy.SetSpeedup(true);
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
                case SDLK_TAB:
                    game_boy.SetSpeedup(false);
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