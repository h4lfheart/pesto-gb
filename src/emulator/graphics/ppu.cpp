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

    this->CheckLYC();

    switch (this->mode)
    {
    case PPUMode::MODE_OAM:
        if (this->dots >= DOTS_OAM)
        {
            this->dots = 0;
            this->ScanOAM();
            this->SetMode(PPUMode::MODE_DRAW);
        }
        break;

    case PPUMode::MODE_DRAW:
        {
            const int pixel_col = this->dots - 1;
            if (pixel_col >= 0 && pixel_col < SCREEN_WIDTH)
                this->bgp_snapshot[pixel_col] = this->memory->ReadIO(IO_ADDR_BGP);

            if (this->dots >= DOTS_DRAW)
            {
                this->dots = 0;
                this->RenderScanline();
                this->SetMode(PPUMode::MODE_HBLANK);

                uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);
                if (stat & STAT_HBLANK_INT)
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
        }

        break;

    case PPUMode::MODE_HBLANK:
        if (this->dots >= DOTS_HBLANK)
        {
            this->dots = 0;
            this->IncrementScanline();

            if (this->is_hmda_active)
            {
                for (int i = 0; i < 0x10; i++)
                    this->memory->WriteVRAM(this->hdma_dst + i, this->memory->Read8(this->hdma_src + i));

                this->hdma_src += 0x10;
                this->hdma_dst += 0x10;
                this->hdma_bytes_left -= 0x10;

                uint8_t* hdma5 = this->memory->PtrIO(IO_ADDR_HDMA5);
                if (this->hdma_bytes_left == 0)
                {
                    this->is_hmda_active = false;
                    *hdma5 = HDMA_TRANSFER_COMPLETE;
                }
                else
                {
                    *hdma5 = (this->hdma_bytes_left / 0x10) - 1;
                }
            }

            if (this->scanline > PPU_MAX_VISIBLE_SCANLINE)
            {
                this->SetMode(PPUMode::MODE_VBLANK);
                this->ready_for_draw = true;
                this->memory->SetInterruptFlag(INTERRUPT_VBLANK);

                uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);
                if (stat & STAT_VBLANK_INT)
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
            else
            {
                this->SetMode(PPUMode::MODE_OAM);

                uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);
                if (stat & STAT_OAM_INT)
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
        }
        break;

    case PPUMode::MODE_VBLANK:
        if (this->dots >= DOTS_TOTAL)
        {
            this->dots = 0;
            this->IncrementScanline();

            if (this->scanline > PPU_MAX_TOTAL_SCANLINE)
            {
                this->scanline = 0;
                this->window_line = 0;
                this->memory->WriteIO(IO_ADDR_LCDY, 0);
                this->SetMode(PPUMode::MODE_OAM);

                uint8_t stat = this->memory->ReadIO(IO_ADDR_STAT);
                if (stat & STAT_OAM_INT)
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
        }
        break;
    }
}


void PPU::AttachMemory(Memory* mem)
{
    this->memory = mem;
    mem->RegisterIOMemoryRegion(0x68, 0x6B, this, &PPU::PPURead, &PPU::PPUWrite);
    mem->RegisterIOMemoryRegion(0x51, 0x55, this, &PPU::PPURead, &PPU::PPUWrite);
}

uint8_t PPU::PPURead(uint8_t* io, uint16_t offset)
{
    if (offset == IO_ADDR_BCPD)
    {
        const uint8_t idx = io[IO_ADDR_BCPS] & PALETTE_ADDRESS_MASK;
        return this->background_palettes[idx];
    }

    if (offset == IO_ADDR_OCPD)
    {
        const uint8_t idx = io[IO_ADDR_OCPS] & PALETTE_ADDRESS_MASK;
        return this->object_palettes[idx];
    }

    if (offset == IO_ADDR_HDMA5)
    {
        return this->is_hmda_active ? (this->hdma_bytes_left / 0x10) - 1 : HDMA_TRANSFER_COMPLETE;
    }

    return io[offset];
}

void PPU::PPUWrite(uint8_t* io, uint16_t offset, uint8_t value)
{
    io[offset] = value;

    if (offset == IO_ADDR_BCPD)
    {
        const uint8_t idx = io[IO_ADDR_BCPS] & PALETTE_ADDRESS_MASK;
        this->background_palettes[idx] = value;

        if (io[IO_ADDR_BCPS] & PALETTE_INCREMENT_MASK)
            io[IO_ADDR_BCPS] = (io[IO_ADDR_BCPS] & PALETTE_INCREMENT_MASK) | ((idx + 1) & PALETTE_ADDRESS_MASK);
    }

    if (offset == IO_ADDR_OCPD)
    {
        const uint8_t idx = io[IO_ADDR_OCPS] & PALETTE_ADDRESS_MASK;
        this->object_palettes[idx] = value;

        if (io[IO_ADDR_OCPS] & PALETTE_INCREMENT_MASK)
            io[IO_ADDR_OCPS] = (io[IO_ADDR_OCPS] & PALETTE_INCREMENT_MASK) | ((idx + 1) & PALETTE_ADDRESS_MASK);
    }

    if (offset == IO_ADDR_HDMA5)
    {
        const uint16_t src = (io[IO_ADDR_HDMA1] << 8 | io[IO_ADDR_HDMA2]) & 0xFFF0;
        const uint16_t dst = (io[IO_ADDR_HDMA3] << 8 | io[IO_ADDR_HDMA4]) & 0x1FF0;
        const uint16_t len = ((value & 0x7F) + 1) * 0x10;

        if ((value & HDMA5_TYPE_MASK) == 0)
        {
            // general purpose dma
            for (uint16_t i = 0; i < len; i++)
                this->memory->WriteVRAM(dst + i, this->memory->Read8(src + i));

            io[IO_ADDR_HDMA5] = HDMA_TRANSFER_COMPLETE;
        }
        else
        {
            // hblank dma
            this->hdma_src = src;
            this->hdma_dst = dst;
            this->hdma_bytes_left = len;
            this->is_hmda_active = true;
            io[IO_ADDR_HDMA5] = (len / 0x10) - 1;
        }
    }

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

void PPU::RenderScanline()
{
    memset(this->scanline_pixels, 0, sizeof(PPUPixel) * SCREEN_WIDTH);

    this->RenderBackground();
    this->RenderWindow();
    this->RenderObjects();

    for (uint8_t pixel_idx = 0; pixel_idx < SCREEN_WIDTH; pixel_idx++)
        this->framebuffer[this->scanline * SCREEN_WIDTH + pixel_idx] = this->scanline_pixels[pixel_idx].color;
}

void PPU::RenderBackground()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);

    if ((lcdc & LCDC_BG_ENABLE) == 0)
        return;

    const uint8_t scy = this->memory->ReadIO(IO_ADDR_SCY);
    const uint8_t scx = this->memory->ReadIO(IO_ADDR_SCX);

    const uint16_t tile_map_base = (lcdc & LCDC_BG_TILEMAP) ? 0x1C00 : 0x1800;
    const uint16_t tile_data_base = (lcdc & LCDC_TILE_DATA) ? 0x0000 : 0x1000;

    const uint8_t py = this->scanline + scy;
    const uint8_t tile_y = py / 8;
    const uint8_t pixel_y = py % 8;

    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
        const uint8_t px = x + scx;
        const uint8_t tile_x = px / 8;
        const uint8_t pixel_x = px % 8;

        const uint16_t tile_map_offset = (tile_map_base) + (tile_y * 32) + tile_x;
        const uint8_t tile_index = this->memory->ReadVRAMBank(false, tile_map_offset);
        const uint8_t tile_attr  = this->memory->ReadVRAMBank(true,  tile_map_offset);

        const bool use_extra_vram = tile_attr & OBJ_BANK_MASK;
        const bool flip_x = tile_attr & OBJ_FLIP_X_MASK;
        const bool flip_y = tile_attr & OBJ_FLIP_Y_MASK;
        const uint8_t cgb_palette = tile_attr & OBJ_CGB_PALETTE_MASK;

        const uint8_t real_pixel_y = flip_y ? (7 - pixel_y) : pixel_y;
        const uint8_t real_pixel_x = flip_x ? pixel_x : (7 - pixel_x);

        const uint16_t tile_addr = (tile_data_base) + (lcdc & LCDC_TILE_DATA ? tile_index : static_cast<int8_t>(tile_index)) * 16;

        const uint8_t tile_data_1 = this->memory->ReadVRAMBank(use_extra_vram, tile_addr + real_pixel_y * 2);
        const uint8_t tile_data_2 = this->memory->ReadVRAMBank(use_extra_vram, tile_addr + real_pixel_y * 2 + 1);

        const uint8_t color_idx = ((tile_data_2 >> real_pixel_x) & 1) << 1 | ((tile_data_1 >> real_pixel_x) & 1);

        uint16_t rgb555;
        if (this->memory->IsCGB())
        {
            const uint8_t byte_idx = (cgb_palette * 8) + (color_idx * 2);
            rgb555 = (background_palettes[byte_idx + 1] << 8) | background_palettes[byte_idx];
        }
        else
        {
            const uint8_t palette_idx = (bgp_snapshot[x] >> (color_idx << 1)) & 0b11;
            rgb555 = palette[palette_idx];
        }

        this->scanline_pixels[x] = PPUPixel{rgb555, color_idx };
    }
}

void PPU::RenderWindow()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if (!(lcdc & LCDC_WINDOW_ENABLE) || !(lcdc & LCDC_BG_ENABLE))
        return;

    const uint8_t wy = this->memory->ReadIO(IO_ADDR_WY);
    const uint8_t wx = this->memory->ReadIO(IO_ADDR_WX);

    if (this->scanline < wy)
        return;

    const int start_x = wx >= WX_OFFSET ? wx - WX_OFFSET : 0;
    if (start_x >= SCREEN_WIDTH)
        return;

    const uint16_t tile_map_base = (lcdc & LCDC_WINDOW_TILEMAP) ? 0x1C00 : 0x1800;
    const uint16_t tile_data_base = (lcdc & LCDC_TILE_DATA) ? 0x0000 : 0x1000;

    const uint8_t py = this->window_line;
    const uint8_t tile_y = py / 8;
    const uint8_t pixel_y = py % 8;

    for (int x = start_x; x < SCREEN_WIDTH; x++)
    {
        const uint8_t px = x - start_x;
        const uint8_t tile_x = px / 8;
        const uint8_t pixel_x = px % 8;


        const uint16_t tile_map_offset = (tile_map_base) + (tile_y * 32) + tile_x;
        const uint8_t tile_index = this->memory->ReadVRAMBank(false, tile_map_offset);
        const uint8_t tile_attr  = this->memory->ReadVRAMBank(true,  tile_map_offset);

        const bool use_extra_vram = tile_attr & OBJ_BANK_MASK;
        const bool flip_x = tile_attr & OBJ_FLIP_X_MASK;
        const bool flip_y = tile_attr & OBJ_FLIP_Y_MASK;
        const uint8_t cgb_palette = tile_attr & OBJ_CGB_PALETTE_MASK;

        const uint8_t real_pixel_y = flip_y ? (7 - pixel_y) : pixel_y;
        const uint8_t real_pixel_x = flip_x ? pixel_x : (7 - pixel_x);

        const uint16_t tile_addr = (tile_data_base) + (lcdc & LCDC_TILE_DATA ? tile_index : static_cast<int8_t>(tile_index)) * 16;

        const uint8_t tile_data_1 = this->memory->ReadVRAMBank(use_extra_vram, tile_addr + real_pixel_y * 2);
        const uint8_t tile_data_2 = this->memory->ReadVRAMBank(use_extra_vram, tile_addr + real_pixel_y * 2 + 1);

        const uint8_t color_idx = ((tile_data_2 >> real_pixel_x) & 1) << 1 | ((tile_data_1 >> real_pixel_x) & 1);

        uint16_t rgb555;
        if (this->memory->IsCGB())
        {
            const uint8_t byte_idx = cgb_palette * PALETTE_SIZE + color_idx * COLOR_SIZE;
            rgb555 = (background_palettes[byte_idx + 1] << 8) | background_palettes[byte_idx];
        }
        else
        {
            const uint8_t palette_idx = (bgp_snapshot[x] >> (color_idx << 1)) & 0b11;
            rgb555 = palette[palette_idx];
        }

        this->scanline_pixels[x] = PPUPixel{rgb555, color_idx };
    }

    this->window_line++;

}

void PPU::ScanOAM()
{
    this->objects.clear();

    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t object_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

    for (uint8_t oam_idx = 0; oam_idx < OAM_MAX_COUNT; oam_idx++)
    {
        const uint16_t oam_addr = OAM_BEGIN + oam_idx * OAM_STRUCT_SIZE;

        OAMObject object = {
            .y = this->memory->Read8(oam_addr),
            .x = this->memory->Read8(oam_addr + 1),
            .tile_index = this->memory->Read8(oam_addr + 2),
            .attributes = this->memory->Read8(oam_addr + 3),
            .oam_index = oam_idx
        };

        const int object_y = object.y - 16;
        const int object_line = this->scanline - object_y;

        if (object_line >= 0 && object_line < object_height)
        {
            this->objects.push_back(object);
        }
    }

    std::ranges::sort(this->objects, [](const OAMObject& a, const OAMObject& b) {
       if (a.x != b.x)
           return a.x < b.x;
       return a.oam_index < b.oam_index;
   });

    if (this->objects.size() > 10)
        this->objects.resize(10);
}

void PPU::RenderObjects()
{
    const uint8_t lcdc = this->memory->ReadIO(IO_ADDR_LCDC);
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t object_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;
    for (int8_t obj_idx = this->objects.size() - 1; obj_idx >= 0; obj_idx--)
    {
        const auto& object = this->objects[obj_idx];

        const int object_x = object.x - 8;
        const int object_y = object.y - 16;
        const int object_line = this->scanline - object_y;

        const bool flip_y = object.attributes & OBJ_FLIP_Y_MASK;
        const bool flip_x = object.attributes & OBJ_FLIP_X_MASK;
        const bool has_priority = object.attributes & OBJ_PRIORITY_MASK;
        const bool use_extra_vram = object.attributes & OBJ_BANK_MASK;
        const uint8_t cgb_palette = object.attributes & OBJ_CGB_PALETTE_MASK;
        const uint8_t palette_addr = (object.attributes & OBJ_DMG_PALETTE_MASK) ? IO_ADDR_OBP1 : IO_ADDR_OBP0;
        const uint8_t obj_palette = this->memory->ReadIO(palette_addr);

        uint16_t tile_index = object.tile_index;
        uint8_t tile_line = flip_y ? (object_height - object_line - 1) : object_line;

        if (object_height == 16)
        {
            tile_index = (tile_index & 0b11111110) | (tile_line >> 3);
            tile_line = tile_line & 0b111;
        }

        const uint8_t tile_data_1 = this->memory->ReadVRAMBank(use_extra_vram, tile_index * 16 + tile_line * 2);
        const uint8_t tile_data_2 = this->memory->ReadVRAMBank(use_extra_vram, tile_index * 16 + tile_line * 2 + 1);

        for (int px = 0; px < 8; px++)
        {
            const int screen_x = object_x + px;

            if (screen_x < 0 || screen_x >= SCREEN_WIDTH)
                continue;

            const uint8_t bit = flip_x ? px : (7 - px);
            const uint8_t color_idx = ((tile_data_2 >> bit) & 1) << 1 | ((tile_data_1 >> bit) & 1);

            if (color_idx == 0) // transparent
                continue;

            // TODO add proper cgb priority
            if (has_priority && this->scanline_pixels[screen_x].priority_idx != 0)
                continue;

            uint16_t rgb555;
            if (this->memory->IsCGB())
            {
                const uint8_t byte_idx = cgb_palette * PALETTE_SIZE + color_idx * COLOR_SIZE;
                rgb555 = (object_palettes[byte_idx + 1] << 8) | object_palettes[byte_idx];
            }
            else
            {
                const uint8_t palette_idx = (obj_palette >> (color_idx << 1)) & 0b11;
                rgb555 = palette[palette_idx];
            }

            this->scanline_pixels[screen_x] = PPUPixel{rgb555, color_idx };
        }
    }
}