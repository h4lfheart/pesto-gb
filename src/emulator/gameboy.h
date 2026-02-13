#pragma once
#include <functional>

#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/cartridge.h"
#include "graphics/ppu.h"
#include "../sdl/display.h"

typedef std::function<void(uint8_t data[SCREEN_WIDTH * SCREEN_HEIGHT])> DrawFunction;

class GameBoy
{
public:
    GameBoy(char* boot_rom_path, char* rom_path);
    ~GameBoy();

    void Run();
    void Stop();
    bool IsRunning();

    void OnDraw(DrawFunction onDraw);

    DrawFunction OnDrawFunction;

private:
    Cpu* cpu;
    Memory* memory;
    Cartridge* cartridge;
    PPU* ppu;

    bool is_running = false;
};