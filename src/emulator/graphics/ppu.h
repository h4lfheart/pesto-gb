#pragma once
#include "../memory/memory.h"

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144

#define PPU_MAX_VISIBLE_SCANLINE 143 // 0..143 = 144 total
#define PPU_MAX_TOTAL_SCANLINE 153 // 0..153 = 153

#define DOTS_OAM 80
#define DOTS_DRAW 289
#define DOTS_HBLANK 87
#define DOTS_VBLANK (DOTS_OAM + DOTS_DRAW + DOTS_HBLANK)

#define IO_ADDR_LCDC 0x40
#define IO_ADDR_SCY  0x42
#define IO_ADDR_SCX  0x43
#define IO_ADDR_LCDY 0x44
#define IO_ADDR_STAT 0x45
#define IO_ADDR_BGP  0x47

#define LCDC_BG_ENABLE 0b00000001
#define LCDC_BG_TILEMAP 0b00001000
#define LCDC_TILE_DATA 0b00010000
#define LCDC_ENABLE 0b10000000

#define STAT_MODE_MASK 0b00000011

enum class PPUMode
{
    MODE_NONE = -1,
    MODE_OAM = 2,
    MODE_DRAW = 3,
    MODE_HBLANK = 0,
    MODE_VBLANK = 1
};

class PPU {
public:
    void Cycle();
    void AttachMemory(Memory* mem);

    uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT] = {};

    bool ready_for_draw = false;

private:
    void DrawScanline();

    Memory* memory = nullptr;

    PPUMode mode = PPUMode::MODE_NONE;

    uint16_t dots = 0;
    uint8_t scanline = 0;
};