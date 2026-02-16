#pragma once
#include "../memory/memory.h"
#include <vector>

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144

#define PPU_MAX_VISIBLE_SCANLINE 143
#define PPU_MAX_TOTAL_SCANLINE 153

#define DOTS_TOTAL 456
#define DOTS_OAM 80
#define DOTS_DRAW 172
#define DOTS_HBLANK 204

#define IO_ADDR_LCDC 0x40
#define IO_ADDR_SCY  0x42
#define IO_ADDR_SCX  0x43
#define IO_ADDR_LCDY 0x44
#define IO_ADDR_LYC 0x45
#define IO_ADDR_STAT 0x41
#define IO_ADDR_BGP  0x47
#define IO_ADDR_OBP0 0x48
#define IO_ADDR_OBP1 0x49
#define IO_ADDR_WY 0x4A
#define IO_ADDR_WX 0x4B

#define LCDC_BG_ENABLE      0b00000001
#define LCDC_OBJ_ENABLE     0b00000010
#define LCDC_OBJ_SIZE       0b00000100
#define LCDC_BG_TILEMAP     0b00001000
#define LCDC_TILE_DATA      0b00010000
#define LCDC_WINDOW_ENABLE  0b00100000
#define LCDC_WINDOW_TILEMAP 0b01000000
#define LCDC_ENABLE         0b10000000

#define STAT_LYC 0b00000100
#define STAT_MODE_MASK 0b00000011
#define STAT_HBLANK_INT 0b00001000
#define STAT_VBLANK_INT 0b00010000
#define STAT_OAM_INT 0b00100000
#define STAT_LYC_INT 0b01000000

#define OBJ_PALETTE    0b00010000
#define OBJ_FLIP_X     0b00100000
#define OBJ_FLIP_Y     0b01000000
#define OBJ_PRIORITY   0b10000000

#define WX_OFFSET 7

#define OAM_STRUCT_SIZE 4
#define OAM_MAX_COUNT 40

enum class PPUMode
{
    MODE_OAM = 2,
    MODE_DRAW = 3,
    MODE_HBLANK = 0,
    MODE_VBLANK = 1
};

struct OAMObject
{
    uint8_t y;
    uint8_t x;
    uint8_t tile_index;
    uint8_t attributes;
    uint8_t oam_index;
};

struct PPUPixel
{
    uint8_t color_idx;
    uint8_t priority_idx;
};

class PPU
{
public:
    void Cycle();
    void AttachMemory(Memory* mem);

    uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT] = {};
    bool ready_for_draw = false;

private:
    void SetMode(PPUMode new_mode);
    void IncrementScanline();

    void CheckLYC() const;
    void RenderScanline();
    void RenderBackground();
    void RenderWindow();

    void ScanOAM();
    void RenderObjects();

    Memory* memory = nullptr;

    PPUMode mode = PPUMode::MODE_OAM;
    uint16_t dots = 0;
    uint8_t scanline = 0;
    uint8_t window_line = 0;

    PPUPixel scanline_pixels[SCREEN_WIDTH] = {};
    std::vector<OAMObject> objects;
};