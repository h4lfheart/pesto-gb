#pragma once
#include <functional>

#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/cartridge.h"
#include "graphics/ppu.h"
#include "io/input.h"
#include "timer/timer.h"

typedef std::function<void(uint8_t data[SCREEN_WIDTH * SCREEN_HEIGHT])> DrawFunction;

class GameBoy
{
public:
    GameBoy(char* boot_rom_path, char* rom_path);

    void Run();
    void Stop();
    bool IsRunning();

    void PressButton(InputButton button);
    void ReleaseButton(InputButton button);

    void OnDraw(DrawFunction onDraw);

    DrawFunction OnDrawFunction;

private:
    Cpu* cpu;
    Memory* memory;
    Cartridge* cartridge;
    PPU* ppu;
    Input* input;
    Timer* timer;

    bool is_running = false;
};