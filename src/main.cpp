#include <iostream>
#include <thread>

#include "emulator/gameboy.h"
#include "sdl/display.h"

int main(int argc, char** argv)
{
    GameBoy game_boy(argv[1], argv[2]);

    auto display = Display();
    display.Initialize();

    game_boy.OnDraw([&](uint8_t* data)
    {
        display.Update(data);
    });

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
        }

        SDL_Delay(1);
    }

    game_thread.join();

    return 0;
}
