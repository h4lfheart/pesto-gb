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
#define IO_ADDR_SCY 0x42
#define IO_ADDR_SCX 0x43
#define IO_ADDR_LCDY 0x44
#define IO_ADDR_LYC 0x45
#define IO_ADDR_STAT 0x41
#define IO_ADDR_BGP 0x47
#define IO_ADDR_OBP0 0x48
#define IO_ADDR_OBP1 0x49
#define IO_ADDR_WY 0x4A
#define IO_ADDR_WX 0x4B

#define IO_ADDR_BCPS 0x68
#define IO_ADDR_BCPD 0x69
#define IO_ADDR_OCPS 0x6A
#define IO_ADDR_OCPD 0x6B

#define IO_ADDR_HDMA1 0x51
#define IO_ADDR_HDMA2 0x52
#define IO_ADDR_HDMA3 0x53
#define IO_ADDR_HDMA4 0x54
#define IO_ADDR_HDMA5 0x55

#define HDMA5_TYPE_MASK 0b10000000
#define HDMA_TRANSFER_COMPLETE 0xFF

#define PALETTE_ADDRESS_MASK 0b00111111
#define PALETTE_INCREMENT_MASK 0b10000000

#define LCDC_BG_ENABLE 0b00000001
#define LCDC_OBJ_ENABLE 0b00000010
#define LCDC_OBJ_SIZE 0b00000100
#define LCDC_BG_TILEMAP 0b00001000
#define LCDC_TILE_DATA 0b00010000
#define LCDC_WINDOW_ENABLE 0b00100000
#define LCDC_WINDOW_TILEMAP 0b01000000
#define LCDC_ENABLE 0b10000000

#define STAT_LYC 0b00000100
#define STAT_MODE_MASK 0b00000011
#define STAT_HBLANK_INT 0b00001000
#define STAT_VBLANK_INT 0b00010000
#define STAT_OAM_INT 0b00100000
#define STAT_LYC_INT 0b01000000

#define OBJ_DMG_PALETTE_MASK 0b00010000
#define OBJ_FLIP_X_MASK 0b00100000
#define OBJ_FLIP_Y_MASK 0b01000000
#define OBJ_PRIORITY_MASK 0b10000000
#define OBJ_BANK_MASK 0b00001000
#define OBJ_CGB_PALETTE_MASK 0b00000111

#define PALETTE_SIZE 8
#define COLOR_SIZE 2

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
    uint16_t color;
    uint8_t priority_idx;
};

class PPU
{
public:
    void Cycle();
    void AttachMemory(Memory* mem);

    uint16_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT] = {};
    bool ready_for_draw = false;

private:
    uint8_t PPURead(uint8_t* io, uint16_t offset);
    void PPUWrite(uint8_t* io, uint16_t offset, uint8_t value);

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

    uint8_t bgp_snapshot[SCREEN_WIDTH] = {};

    uint8_t background_palettes[64] = {};
    uint8_t object_palettes[64] = {};

    bool is_hmda_active = false;
    uint16_t hdma_src = 0;
    uint16_t hdma_dst = 0;
    uint16_t hdma_bytes_left = 0;

    uint16_t palette[4] = {
        0x6BDE,
        0x4F58,
        0x368F,
        0x1D67
    };
};