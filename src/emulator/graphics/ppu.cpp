#include "ppu.h"

#include <cstring>

#include "../cpu/cpu.h"

void PPU::Cycle()
{
    if (this->dots--)
        return;

    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if (lcdc & LCDC_ENABLE)
    {
        const uint8_t status = this->memory->ReadIO(IO_ADDR_STAT);
        const uint8_t cur_scanline = this->scanline;
        const PPUMode cur_mode = this->mode;
        switch (cur_mode)
        {
        case PPUMode::MODE_NONE:
            this->mode = PPUMode::MODE_OAM;
            this->dots = DOTS_OAM;
            break;
        case PPUMode::MODE_OAM:
            this->mode = PPUMode::MODE_DRAW;
            this->dots = DOTS_DRAW;
            DrawScanline();
            break;
        case PPUMode::MODE_DRAW:
            this->mode = PPUMode::MODE_HBLANK;
            this->dots = DOTS_HBLANK;
            break;
        case PPUMode::MODE_HBLANK:
            this->scanline++;

            if (this->scanline > PPU_MAX_VISIBLE_SCANLINE)
            {
                this->mode = PPUMode::MODE_VBLANK;
                this->dots = DOTS_VBLANK;

                this->ready_for_draw = true;

                // TODO memory pointers
                const uint8_t interrupt_flag = this->memory->ReadIO(IO_ADDR_INTERRUPT_FLAG);
                this->memory->WriteIO(IO_ADDR_INTERRUPT_FLAG, interrupt_flag | INTERRUPT_VBLANK);
            }
            else
            {
                this->mode = PPUMode::MODE_OAM;
                this->dots = DOTS_OAM;
            }
            break;
        case PPUMode::MODE_VBLANK:
            this->dots = DOTS_VBLANK;
            this->scanline++;

            if (this->scanline > PPU_MAX_TOTAL_SCANLINE)
            {
                this->scanline = 0;
                this->mode = PPUMode::MODE_OAM;
            }
            break;
        }

        // set scanline
        this->memory->WriteIO(IO_ADDR_LCDY, this->scanline);

        // set mode
        uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);
        stat &= ~STAT_MODE_MASK;
        stat |= static_cast<uint8_t>(this->mode);
        this->memory->WriteIO(IO_ADDR_STAT, stat);
    }
    else
    {
        this->mode = PPUMode::MODE_NONE;
        this->scanline = 0;
        this->dots = 0;
        this->memory->WriteIO(IO_ADDR_LCDY, 0);
    }
}

void PPU::AttachMemory(Memory* mem)
{
    this->memory = mem;
}

void PPU::DrawScanline()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);

    if ((lcdc & LCDC_BG_ENABLE) == 0)
    {
        memset(framebuffer, 0, SCREEN_HEIGHT * SCREEN_WIDTH);
        return;
    }

    const uint8_t scy = this->memory->ReadIO(IO_ADDR_SCY);
    const uint8_t scx = this->memory->ReadIO(IO_ADDR_SCX);

    const uint16_t tile_map_base = (lcdc & LCDC_BG_TILEMAP) ? 0x9C00 : 0x9800;
    const uint16_t tile_data_base = (lcdc & LCDC_TILE_DATA) ? 0x8000 : 0x8800;
    const bool is_signed = (lcdc & LCDC_TILE_DATA) == 0;

    const uint8_t y = scanline + scy;
    for (int x = 0; x < 160; x++)
    {
        const uint8_t px = x + scx;

        const uint8_t tile_x = px / 8;
        const uint8_t tile_y = y / 8;

        const uint16_t tile_map_addr = tile_map_base + (tile_y * 32) + tile_x;
        const uint8_t tile_index = this->memory->Read8(tile_map_addr);

        uint16_t tile_addr = is_signed ? tile_data_base + (static_cast<int8_t>(tile_index) * 16) : tile_data_base + (tile_index * 16);
        const uint8_t pixel_y = y % 8;
        const uint8_t pixel_x = px % 8;

        const uint8_t byte1 = this->memory->Read8(tile_addr + (pixel_y * 2));
        const uint8_t byte2 = this->memory->Read8(tile_addr + (pixel_y * 2) + 1);

        const uint8_t bit = 7 - pixel_x;
        const uint8_t color_id = ((byte2 >> bit) & 1) << 1 | ((byte1 >> bit) & 1);

        const uint8_t bgp = this->memory->ReadIO(IO_ADDR_BGP);
        const uint8_t color = (bgp >> (color_id << 1)) & 0b11;

        framebuffer[scanline * 160 + x] = color;
    }
}
