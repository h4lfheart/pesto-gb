#include "ppu.h"
#include <cstring>

#include "../cpu/cpu.h"


void PPU::Cycle()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);

    if ((lcdc & LCDC_ENABLE) == 0)
    {
        this->mode = PPUMode::MODE_HBLANK;
        this->scanline = 0;
        this->dots = 0;
        this->memory->WriteIO(IO_ADDR_LCDY, 0);
        return;
    }

    this->dots++;

    switch (this->mode)
    {
    case PPUMode::MODE_OAM:
        if (this->dots >= DOTS_OAM)
        {
            this->dots = 0;
            SetMode(PPUMode::MODE_DRAW);
            ScanOAM();
        }
        break;

    case PPUMode::MODE_DRAW:
        if (this->dots >= DOTS_DRAW)
        {
            this->dots = 0;
            SetMode(PPUMode::MODE_HBLANK);
            RenderScanline();
        }
        break;

    case PPUMode::MODE_HBLANK:
        if (this->dots >= DOTS_HBLANK)
        {
            this->dots = 0;
            IncrementScanline();

            if (this->scanline > PPU_MAX_VISIBLE_SCANLINE)
            {
                SetMode(PPUMode::MODE_VBLANK);
                this->ready_for_draw = true;

                const uint8_t interrupt_flag = this->memory->ReadIO(IO_ADDR_INTERRUPT_FLAG);
                this->memory->WriteIO(IO_ADDR_INTERRUPT_FLAG, interrupt_flag | INTERRUPT_VBLANK);
            }
            else
            {
                SetMode(PPUMode::MODE_OAM);
            }
        }
        break;

    case PPUMode::MODE_VBLANK:
        if (this->dots >= DOTS_TOTAL)
        {
            this->dots = 0;
            IncrementScanline();

            if (this->scanline > PPU_MAX_TOTAL_SCANLINE)
            {
                this->scanline = 0;
                this->memory->WriteIO(IO_ADDR_LCDY, 0);
                SetMode(PPUMode::MODE_OAM);
            }
        }
        break;
    }
}

void PPU::AttachMemory(Memory* mem)
{
    this->memory = mem;
}

void PPU::SetMode(PPUMode new_mode)
{
    this->mode = new_mode;

    uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);
    stat &= ~STAT_MODE_MASK;
    stat |= static_cast<uint8_t>(new_mode);
    this->memory->WriteIO(IO_ADDR_STAT, stat);
}

void PPU::IncrementScanline()
{
    this->scanline++;
    this->memory->WriteIO(IO_ADDR_LCDY, this->scanline);
}

void PPU::ScanOAM()
{
    this->sprites.clear();

    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t sprite_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    for (int oam_idx = 0; oam_idx < 40; oam_idx++)
    {
        const uint16_t oam_addr = ADDR_OAM_START + oam_idx * 4;

        OAMEntry sprite;
        sprite.y = this->memory->Read8(oam_addr);
        sprite.x = this->memory->Read8(oam_addr + 1);
        sprite.tile_index = this->memory->Read8(oam_addr + 2);
        sprite.attributes = this->memory->Read8(oam_addr + 3);

        const int sprite_y = sprite.y - 16;
        const int sprite_line = this->scanline - sprite_y;

        if (sprite_line >= 0 && sprite_line < sprite_height)
        {
            this->sprites.push_back(sprite);
            if (this->sprites.size() >= 10)
                break;
        }
    }
}

void PPU::RenderScanline()
{
    memset(scanline_buffer, 0, SCREEN_WIDTH);
    memset(priority_buffer, 0, SCREEN_WIDTH);

    RenderBackground();
    RenderSprites();

    memcpy(&framebuffer[scanline * SCREEN_WIDTH], scanline_buffer, SCREEN_WIDTH);
}

void PPU::RenderBackground()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);

    if ((lcdc & LCDC_BG_ENABLE) == 0)
        return;

    const uint8_t scy = this->memory->ReadIO(IO_ADDR_SCY);
    const uint8_t scx = this->memory->ReadIO(IO_ADDR_SCX);
    const uint8_t bgp = this->memory->ReadIO(IO_ADDR_BGP);

    const uint16_t tile_map_base = (lcdc & LCDC_BG_TILEMAP) ? 0x9C00 : 0x9800;
    const uint16_t tile_data_base = (lcdc & LCDC_TILE_DATA) ? 0x8000 : 0x8800;
    const bool is_signed = (lcdc & LCDC_TILE_DATA) == 0;

    const uint8_t y = this->scanline + scy;

    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
        const uint8_t px = x + scx;

        const uint8_t tile_x = px / 8;
        const uint8_t tile_y = y / 8;

        const uint16_t tile_map_addr = tile_map_base + (tile_y * 32) + tile_x;
        const uint8_t tile_index = this->memory->Read8(tile_map_addr);

        const uint16_t tile_addr = is_signed
            ? tile_data_base + (static_cast<int8_t>(tile_index) * 16)
            : tile_data_base + (tile_index * 16);

        const uint8_t pixel_y = y % 8;
        const uint8_t pixel_x = px % 8;

        const uint8_t byte1 = this->memory->Read8(tile_addr + (pixel_y * 2));
        const uint8_t byte2 = this->memory->Read8(tile_addr + (pixel_y * 2) + 1);

        const uint8_t bit = 7 - pixel_x;
        const uint8_t palette_idx = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);
        const uint8_t color = (bgp >> (palette_idx << 1)) & 0b11;

        scanline_buffer[x] = color;
        priority_buffer[x] = palette_idx;
    }
}

void PPU::RenderSprites()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t sprite_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    for (int s = this->sprites.size() - 1; s >= 0; s--)
    {
        const OAMEntry& sprite = this->sprites[s];

        const int sprite_x = sprite.x - 8;
        const int sprite_y = sprite.y - 16;
        const int sprite_line = this->scanline - sprite_y;

        const bool flip_y = sprite.attributes & OBJ_FLIP_Y;
        const bool flip_x = sprite.attributes & OBJ_FLIP_X;
        const bool priority = sprite.attributes & OBJ_PRIORITY;
        const uint8_t palette_addr = (sprite.attributes & OBJ_PALETTE) ? IO_ADDR_OBP1 : IO_ADDR_OBP0;
        const uint8_t palette = this->memory->ReadIO(palette_addr);

        uint8_t tile_line = flip_y ? (sprite_height - 1 - sprite_line) : sprite_line;
        uint8_t tile_index = sprite.tile_index;

        if (sprite_height == 16)
        {
            if (tile_line >= 8)
            {
                tile_index |= 0x01;
                tile_line -= 8;
            }
            else
            {
                tile_index &= 0xFE;
            }
        }

        const uint16_t tile_addr = 0x8000 + (tile_index * 16);
        const uint8_t byte1 = this->memory->Read8(tile_addr + (tile_line * 2));
        const uint8_t byte2 = this->memory->Read8(tile_addr + (tile_line * 2) + 1);

        for (int px = 0; px < 8; px++)
        {
            const int screen_x = sprite_x + px;

            if (screen_x < 0 || screen_x >= SCREEN_WIDTH)
                continue;

            const uint8_t bit = flip_x ? px : (7 - px);
            const uint8_t palette_idx = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

            if (palette_idx == 0)
                continue;

            if (priority && priority_buffer[screen_x] != 0)
                continue;

            const uint8_t color = (palette >> (palette_idx << 1)) & 0b11;
            scanline_buffer[screen_x] = color;
        }
    }
}