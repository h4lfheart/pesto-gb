#pragma once
#include <functional>

#include "audio/apu.h"
#include "cpu/cpu.h"
#include "memory/memory.h"
#include "memory/cartridge.h"
#include "graphics/ppu.h"
#include "io/input.h"
#include "timer/timer.h"

#define GB_COLOR(r, g, b) ((uint16_t)((((b) * 31 / 255) & 0x1F) << 10 | (((g) * 31 / 255) & 0x1F) << 5 | (((r) * 31 / 255) & 0x1F)))

typedef std::function<void(uint16_t data[SCREEN_WIDTH * SCREEN_HEIGHT])> DrawFunction;
typedef std::function<void(float left, float right)> AudioFunction;

struct GameBoySettings
{
    uint16_t* framebuffer = nullptr;
    std::array<uint16_t, 4> palette = {
        0x6BDE,
        0x4F58,
        0x368F,
        0x1D67
    };
};

class GameBoy
{
public:
    GameBoy(const std::string& rom_path, GameBoySettings settings = {});

    void Run();
    void Stop();
    bool IsRunning();
    bool IsCGBGame();

    void LoadBootRom(const std::string& boot_rom_path);

    void PressButton(InputButton button);
    void ReleaseButton(InputButton button);

    void OnDraw(DrawFunction onDraw);
    void OnAudio(AudioFunction onAudio);

    void ReadSave(const char* path);
    void WriteSave(const char* path);

    void SetSpeedup(bool speedup);
    PPU* ppu;
    APU* apu;

    DrawFunction OnDrawFunction;
    AudioFunction OnAudioFunction;

private:
    CPU* cpu;
    Memory* memory;
    Cartridge* cartridge;
    Input* input;
    Timer* timer;

    bool is_running = false;
    bool is_speedup = false;
};
