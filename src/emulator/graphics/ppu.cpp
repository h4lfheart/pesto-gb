#include "ppu.h"
#include <cstring>
#include <algorithm>

#include "../cpu/cpu.h"

void PPU::Cycle()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);

    if ((lcdc & LCDC_ENABLE) == 0)
    {
        this->mode = PPUMode::MODE_HBLANK;
        this->scanline = 0;
        this->dots = 0;
        this->window_line = 0;
        this->memory->WriteIO(IO_ADDR_LCDY, 0);
        return;
    }

    this->dots++;

    uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);
    switch (this->mode)
    {
    case PPUMode::MODE_OAM:
        if (this->dots >= DOTS_OAM)
        {
            this->dots = 0;
            this->SetMode(PPUMode::MODE_DRAW);
            this->ScanOAM();
        }
        break;

    case PPUMode::MODE_DRAW:
        if (this->dots >= DOTS_DRAW)
        {
            this->dots = 0;
            this->SetMode(PPUMode::MODE_HBLANK);
            this->RenderScanline();

            if (stat & STAT_HBLANK_INT)
            {
                this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
        }
        break;

    case PPUMode::MODE_HBLANK:
        if (this->dots >= DOTS_HBLANK)
        {
            this->dots = 0;
            this->IncrementScanline();
            this->CheckLYC();

            if (this->scanline > PPU_MAX_VISIBLE_SCANLINE)
            {
                this->SetMode(PPUMode::MODE_VBLANK);

                this->ready_for_draw = true;

                if (stat & STAT_VBLANK_INT)
                {
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
                }

                this->memory->SetInterruptFlag(INTERRUPT_VBLANK);
            }
            else
            {
                this->SetMode(PPUMode::MODE_OAM);

                if (stat & STAT_OAM_INT)
                {
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
                }
            }
        }
        break;

    case PPUMode::MODE_VBLANK:
        if (this->dots >= DOTS_TOTAL)
        {
            this->dots = 0;
            this->IncrementScanline();
            this->CheckLYC();

            if (this->scanline > PPU_MAX_TOTAL_SCANLINE)
            {
                this->scanline = 0;
                this->window_line = 0;
                this->memory->WriteIO(IO_ADDR_LCDY, 0);
                this->CheckLYC();
                this->SetMode(PPUMode::MODE_OAM);
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

void PPU::CheckLYC() const
{
    const uint8_t ly = this->memory->ReadIO(IO_ADDR_LCDY);
    const uint8_t lyc = this->memory->ReadIO(IO_ADDR_LYC);
    uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);

    bool prev_lyc_flag = (stat & STAT_LYC) != 0;
    bool ly_match = (ly == lyc);

    if (ly_match)
        stat |= STAT_LYC;
    else
        stat &= ~STAT_LYC;

    if (ly_match && !prev_lyc_flag && (stat & STAT_LYC_INT))
        this->memory->SetInterruptFlag(INTERRUPT_STAT);

    this->memory->WriteIO(IO_ADDR_STAT, stat);
}

void PPU::ScanOAM()
{
    this->sprites.clear();

    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t sprite_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    for (uint8_t oam_idx = 0; oam_idx < 40; oam_idx++)
    {
        const uint16_t oam_addr = OAM_BEGIN + oam_idx * 4;

        OAMEntry sprite = {
            .y = this->memory->Read8(oam_addr),
            .x = this->memory->Read8(oam_addr + 1),
            .tile_index = this->memory->Read8(oam_addr + 2),
            .attributes = this->memory->Read8(oam_addr + 3),
            .oam_index = oam_idx
        };

        const int sprite_y = sprite.y - 16;
        const int sprite_line = this->scanline - sprite_y;

        if (sprite_line >= 0 && sprite_line < sprite_height)
        {
            this->sprites.push_back(sprite);
            if (this->sprites.size() >= 10)
                break;
        }
    }

    SortSprites();
}

void PPU::RenderScanline()
{
    memset(this->scanline_buffer, 0, SCREEN_WIDTH);
    memset(this->priority_buffer, 0, SCREEN_WIDTH);

    this->RenderBackground();
    this->RenderWindow();
    this->RenderSprites();

    memcpy(&this->framebuffer[this->scanline * SCREEN_WIDTH], this->scanline_buffer, SCREEN_WIDTH);
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
    const bool use_unsigned_tile_data = (lcdc & LCDC_TILE_DATA) != 0;

    const uint8_t y = this->scanline + scy;
    const uint8_t tile_y = y / 8;
    const uint8_t pixel_y = y % 8;

    uint8_t last_tile_x = 0xFF;
    uint8_t tile_byte1 = 0;
    uint8_t tile_byte2 = 0;

    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
        const uint8_t px = x + scx;
        const uint8_t tile_x = px / 8;

        if (tile_x != last_tile_x)
        {
            const uint16_t tile_map_addr = tile_map_base + (tile_y * 32) + tile_x;
            const uint8_t tile_index = this->memory->Read8(tile_map_addr);

            const uint16_t tile_addr = use_unsigned_tile_data
                ? 0x8000 + (tile_index * 16)
                : 0x9000 + (static_cast<int8_t>(tile_index) * 16);

            tile_byte1 = this->memory->Read8(tile_addr + (pixel_y * 2));
            tile_byte2 = this->memory->Read8(tile_addr + (pixel_y * 2) + 1);

            last_tile_x = tile_x;
        }

        const uint8_t pixel_x = px % 8;
        const uint8_t bit = 7 - pixel_x;
        const uint8_t palette_idx = ((tile_byte2 >> bit) & 1) << 1 | ((tile_byte1 >> bit) & 1);
        const uint8_t color = (bgp >> (palette_idx << 1)) & 0b11;

        this->scanline_buffer[x] = color;
        this->priority_buffer[x] = palette_idx;
    }
}

void PPU::RenderSprites()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t sprite_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    for (const auto sprite : this->sprites)
    {
        const int sprite_x = sprite.x - 8;
        const int sprite_y = sprite.y - 16;
        const int sprite_line = this->scanline - sprite_y;

        const bool flip_y = sprite.attributes & OBJ_FLIP_Y;
        const bool flip_x = sprite.attributes & OBJ_FLIP_X;
        const bool priority = sprite.attributes & OBJ_PRIORITY;
        const uint8_t palette_addr = (sprite.attributes & OBJ_PALETTE) ? IO_ADDR_OBP1 : IO_ADDR_OBP0;
        const uint8_t palette = this->memory->ReadIO(palette_addr);

        const uint8_t flipped_line = flip_y ? (sprite_height - 1 - sprite_line) : sprite_line;

        uint8_t tile_index = sprite.tile_index;
        uint8_t tile_line = flipped_line;


        if (sprite_height == 16)
        {
            tile_index = (tile_index & 0xFE) | (flipped_line >> 3);
            tile_line = flipped_line & 0x07;
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

            if (priority && this->priority_buffer[screen_x] != 0)
                continue;

            const uint8_t color = (palette >> (palette_idx << 1)) & 0b11;
            this->scanline_buffer[screen_x] = color;
        }
    }
}

void PPU::RenderWindow()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if (!(lcdc & LCDC_WINDOW_ENABLE) || !(lcdc & LCDC_BG_ENABLE))
        return;

    const uint8_t wy = this->memory->ReadIO(IO_ADDR_WY);
    const uint8_t wx = this->memory->ReadIO(IO_ADDR_WX);

    if (this->scanline < wy || wx > 166)
        return;

    const uint16_t tile_map_base = (lcdc & LCDC_WINDOW_TILEMAP) ? 0x9C00 : 0x9800;
    const bool use_unsigned_tile_data = (lcdc & LCDC_TILE_DATA) != 0;
    const uint8_t bgp = this->memory->ReadIO(IO_ADDR_BGP);

    const uint8_t tile_y = this->window_line / 8;
    const uint8_t pixel_y = this->window_line % 8;

    const int start_x = (wx >= 7) ? (wx - 7) : 0;

    uint8_t last_tile_x = 0xFF;
    uint8_t tile_byte1 = 0;
    uint8_t tile_byte2 = 0;

    for (int x = start_x; x < SCREEN_WIDTH; x++)
    {
        const int win_x = x - (wx - 7);
        if (win_x < 0)
            continue;

        const uint8_t tile_x = win_x / 8;

        if (tile_x != last_tile_x)
        {
            const uint16_t tile_map_addr = tile_map_base + (tile_y * 32) + tile_x;
            const uint8_t tile_index = this->memory->Read8(tile_map_addr);

            const uint16_t tile_addr = use_unsigned_tile_data
                ? 0x8000 + (tile_index * 16)
                : 0x9000 + (static_cast<int8_t>(tile_index) * 16);

            tile_byte1 = this->memory->Read8(tile_addr + (pixel_y * 2));
            tile_byte2 = this->memory->Read8(tile_addr + (pixel_y * 2) + 1);

            last_tile_x = tile_x;
        }

        const uint8_t bit = 7 - (win_x % 8);
        const uint8_t palette_idx = ((tile_byte2 >> bit) & 1) << 1 | ((tile_byte1 >> bit) & 1);
        const uint8_t color = (bgp >> (palette_idx << 1)) & 0b11;

        this->scanline_buffer[x] = color;
        this->priority_buffer[x] = palette_idx;
    }

    this->window_line++;
}

void PPU::SortSprites()
{
    for (size_t i = 1; i < this->sprites.size(); i++)
    {
        const OAMEntry key = this->sprites[i];
        int j = static_cast<int>(i) - 1;

        while (j >= 0)
        {
            bool should_swap = false;
            if (this->sprites[j].x != key.x)
                should_swap = this->sprites[j].x < key.x;
            else
                should_swap = this->sprites[j].oam_index < key.oam_index;

            if (!should_swap)
                break;

            this->sprites[j + 1] = this->sprites[j];
            j--;
        }
        this->sprites[j + 1] = key;
    }
}