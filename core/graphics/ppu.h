#pragma once
#include <array>

#include "../memory/memory.h"

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144

#define PPU_MAX_VISIBLE_SCANLINE 143
#define PPU_MAX_TOTAL_SCANLINE 153

#define DOTS_TOTAL 456
#define DOTS_OAM 80
#define DOTS_DRAW 172
#define DOTS_HBLANK 204

#define IO_ADDR_LCDC 0x40
#define IO_ADDR_STAT 0x41
#define IO_ADDR_SCY 0x42
#define IO_ADDR_SCX 0x43
#define IO_ADDR_LCDY 0x44
#define IO_ADDR_LYC 0x45
#define IO_ADDR_BGP 0x47
#define IO_ADDR_OBP0 0x48
#define IO_ADDR_OBP1 0x49
#define IO_ADDR_WY 0x4A
#define IO_ADDR_WX 0x4B
#define IO_ADDR_OPRI 0x6C

#define IO_ADDR_BCPS 0x68
#define IO_ADDR_BCPD 0x69
#define IO_ADDR_OCPS 0x6A
#define IO_ADDR_OCPD 0x6B

#define IO_ADDR_HDMA1 0x51
#define IO_ADDR_HDMA2 0x52
#define IO_ADDR_HDMA3 0x53
#define IO_ADDR_HDMA4 0x54
#define IO_ADDR_HDMA5 0x55

#define OPRI_DMG_MODE_MASK 0b00000001

#define HDMA5_TYPE_MASK 0b10000000
#define HDMA_TRANSFER_COMPLETE 0xFF
#define HDMA_BLOCK_SIZE 0x10

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

#define TILE_SIZE_BYTES 16
#define TILE_PIXEL_SIZE 8
#define OAM_MAX_SPRITES 10

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

struct BGPEvent
{
    uint8_t dot;
    uint8_t bgp;
};

class PPU
{
public:
    PPU(uint16_t* framebuffer, std::array<uint16_t, 4> palette);
    void Cycle(uint8_t cycles);
    void AttachMemory(Memory* mem);

    uint16_t* framebuffer = nullptr;
    bool ready_for_draw = false;

    bool use_cgb_rendering = false;

private:
    uint8_t PPURead(uint8_t* io, uint16_t offset);
    void PPUWrite(uint8_t* io, uint16_t offset, uint8_t value);

    void SetMode(PPUMode new_mode);
    void IncrementScanline();

    void CheckLYC() const;
    void RenderScanline();
    void ScanOAM();

    void RenderBackground(uint16_t* row);
    void RenderWindow(uint16_t* row);
    void RenderObjects(uint16_t* row);

    void RebuildBGPaletteCache(uint8_t idx);
    void RebuildOBJPaletteCache(uint8_t idx);

    Memory* memory = nullptr;

    PPUMode mode = PPUMode::MODE_OAM;
    uint16_t dots = 0;
    uint8_t scanline = 0;
    uint8_t window_line = 0;

    uint8_t scanline_priority[SCREEN_WIDTH] = {};
    uint8_t scanline_bg_priority[SCREEN_WIDTH] = {};

    BGPEvent bgp_events[SCREEN_WIDTH] = {};
    uint8_t bgp_event_count = 0;
    uint8_t scanline_bgp[SCREEN_WIDTH] = {};

    OAMObject objects[OAM_MAX_SPRITES] = {};
    uint8_t object_count = 0;

    uint8_t background_palettes[64] = {};
    uint8_t object_palettes[64] = {};

    uint16_t bg_palette_cache[8][4] = {};
    uint16_t obj_palette_cache[8][4] = {};

    bool is_hdma_active = false;
    uint16_t hdma_src = 0;
    uint16_t hdma_dst = 0;
    uint16_t hdma_bytes_left = 0;

    std::array<uint16_t, 4> dmg_palette = {};

    uint8_t* LCDC = nullptr;
    uint8_t* STAT = nullptr;
    uint8_t* SCY = nullptr;
    uint8_t* SCX = nullptr;
    uint8_t* LY = nullptr;
    uint8_t* LYC = nullptr;
    uint8_t* BGP = nullptr;
    uint8_t* OBP0 = nullptr;
    uint8_t* OBP1 = nullptr;
    uint8_t* WY = nullptr;
    uint8_t* WX = nullptr;
    uint8_t* OPRI = nullptr;
    uint8_t* HDMA5 = nullptr;
};
