#pragma once
#include <functional>

#include "audio/apu.h"
#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/cartridge.h"
#include "graphics/ppu.h"
#include "io/input.h"
#include "timer/timer.h"

typedef std::function<void(uint8_t data[SCREEN_WIDTH * SCREEN_HEIGHT])> DrawFunction;
typedef std::function<void(float left, float right)> AudioFunction;

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
    void OnAudio(AudioFunction onAudio);

    DrawFunction OnDrawFunction;
    AudioFunction OnAudioFunction;

private:
    Cpu* cpu;
    Memory* memory;
    Cartridge* cartridge;
    PPU* ppu;
    Input* input;
    Timer* timer;
    APU* apu;

    bool is_running = false;
};