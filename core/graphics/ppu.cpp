#include "ppu.h"
#include <cstring>
#include <algorithm>
#include <array>

#include "../cpu/cpu.h"

static uint16_t tile_pixel_lut[256];
static uint16_t tile_pixel_lut_flipped[256];

PPU::PPU(uint16_t* framebuffer, std::array<uint16_t, 4> palette)
{
    this->framebuffer = framebuffer != nullptr
        ? framebuffer
        : static_cast<uint16_t*>(malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t)));

    this->dmg_palette = palette;

    // build tile pixel lut
    for (int b = 0; b < 256; b++)
    {
        uint16_t forward = 0, reverse = 0;
        for (int bit = 0; bit < 8; bit++)
        {
            const uint16_t v = (b >> (7 - bit)) & 1;
            forward |= v << (bit << 1);
            reverse |= v << ((7 - bit) << 1);
        }

        tile_pixel_lut[b] = forward;
        tile_pixel_lut_flipped[b] = reverse;
    }
}

void PPU::RebuildBGPaletteCache(uint8_t idx)
{
    const uint8_t pal = idx / PALETTE_SIZE;
    const uint8_t col = (idx % PALETTE_SIZE) / COLOR_SIZE;
    const uint8_t base = pal * PALETTE_SIZE + col * COLOR_SIZE;
    bg_palette_cache[pal][col] = (background_palettes[base + 1] << 8) | background_palettes[base];
}

void PPU::RebuildOBJPaletteCache(uint8_t idx)
{
    const uint8_t pal = idx / PALETTE_SIZE;
    const uint8_t col = (idx % PALETTE_SIZE) / COLOR_SIZE;
    const uint8_t base = pal * PALETTE_SIZE + col * COLOR_SIZE;
    obj_palette_cache[pal][col] = (object_palettes[base + 1] << 8) | object_palettes[base];
}

void PPU::Cycle(uint8_t cycles)
{
    const uint8_t lcdc = *LCDC;

    if ((lcdc & LCDC_ENABLE) == 0)
    {
        this->mode = PPUMode::MODE_HBLANK;
        this->scanline = 0;
        this->dots = 0;
        this->window_line = 0;
        *LY = 0;
        return;
    }

    this->dots += cycles;

    switch (this->mode)
    {
    case PPUMode::MODE_OAM:
        if (this->dots >= DOTS_OAM)
        {
            this->dots -= DOTS_OAM;
            this->ScanOAM();
            this->SetMode(PPUMode::MODE_DRAW);
            this->bgp_event_count = 0;
        }
        break;

    case PPUMode::MODE_DRAW:
        if (this->dots >= DOTS_DRAW)
        {
            this->dots -= DOTS_DRAW;
            this->RenderScanline();
            this->SetMode(PPUMode::MODE_HBLANK);
            if (*STAT & STAT_HBLANK_INT)
                this->memory->SetInterruptFlag(INTERRUPT_STAT);
        }
        break;

    case PPUMode::MODE_HBLANK:
        if (this->dots >= DOTS_HBLANK)
        {
            this->dots -= DOTS_HBLANK;
            this->IncrementScanline();

            if (this->is_hdma_active)
            {
                for (int i = 0; i < HDMA_BLOCK_SIZE; i++)
                    this->memory->WriteVRAM(this->hdma_dst + i, this->memory->Read8(this->hdma_src + i));

                this->hdma_src += HDMA_BLOCK_SIZE;
                this->hdma_dst += HDMA_BLOCK_SIZE;
                this->hdma_bytes_left -= HDMA_BLOCK_SIZE;

                if (this->hdma_bytes_left == 0)
                {
                    this->is_hdma_active = false;
                    *HDMA5 = HDMA_TRANSFER_COMPLETE;
                }
                else
                {
                    *HDMA5 = (this->hdma_bytes_left / HDMA_BLOCK_SIZE) - 1;
                }
            }

            if (this->scanline > PPU_MAX_VISIBLE_SCANLINE)
            {
                this->SetMode(PPUMode::MODE_VBLANK);
                this->ready_for_draw = true;
                this->memory->SetInterruptFlag(INTERRUPT_VBLANK);
                if (*STAT & STAT_VBLANK_INT)
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
            else
            {
                this->SetMode(PPUMode::MODE_OAM);
                if (*STAT & STAT_OAM_INT)
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
        }
        break;

    case PPUMode::MODE_VBLANK:
        if (this->dots >= DOTS_TOTAL)
        {
            this->dots -= DOTS_TOTAL;
            this->IncrementScanline();

            if (this->scanline > PPU_MAX_TOTAL_SCANLINE)
            {
                this->scanline = 0;
                this->window_line = 0;
                *LY = 0;
                CheckLYC();
                this->SetMode(PPUMode::MODE_OAM);
                if (*STAT & STAT_OAM_INT)
                    this->memory->SetInterruptFlag(INTERRUPT_STAT);
            }
        }
        break;
    }
}

void PPU::AttachMemory(Memory* mem)
{
    this->memory = mem;

    mem->RegisterIOHandler(0x47, 0x47, this, &PPU::PPURead, &PPU::PPUWrite);
    mem->RegisterIOHandler(0x68, 0x6B, this, &PPU::PPURead, &PPU::PPUWrite);
    mem->RegisterIOHandler(0x51, 0x55, this, &PPU::PPURead, &PPU::PPUWrite);

    LCDC = mem->PtrIO(IO_ADDR_LCDC);
    STAT = mem->PtrIO(IO_ADDR_STAT);
    SCY = mem->PtrIO(IO_ADDR_SCY);
    SCX = mem->PtrIO(IO_ADDR_SCX);
    LY = mem->PtrIO(IO_ADDR_LCDY);
    LYC = mem->PtrIO(IO_ADDR_LYC);
    BGP = mem->PtrIO(IO_ADDR_BGP);
    OBP0 = mem->PtrIO(IO_ADDR_OBP0);
    OBP1 = mem->PtrIO(IO_ADDR_OBP1);
    WY = mem->PtrIO(IO_ADDR_WY);
    WX = mem->PtrIO(IO_ADDR_WX);
    OPRI = mem->PtrIO(IO_ADDR_OPRI);
    HDMA5 = mem->PtrIO(IO_ADDR_HDMA5);

    use_cgb_rendering = mem->IsCGB();
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
        return this->is_hdma_active ? (this->hdma_bytes_left / HDMA_BLOCK_SIZE) - 1 : HDMA_TRANSFER_COMPLETE;

    return io[offset];
}

void PPU::PPUWrite(uint8_t* io, uint16_t offset, uint8_t value)
{
    io[offset] = value;

    if (offset == IO_ADDR_BGP && !use_cgb_rendering)
    {
        if (this->mode == PPUMode::MODE_DRAW && this->bgp_event_count < SCREEN_WIDTH)
        {
            const uint8_t dot = static_cast<uint8_t>(std::clamp<int>(this->dots - 7, 0, DOTS_DRAW - 1));
            this->bgp_events[this->bgp_event_count++] = {dot, value};
        }
    }

    if (offset == IO_ADDR_BCPD)
    {
        const uint8_t idx = io[IO_ADDR_BCPS] & PALETTE_ADDRESS_MASK;
        this->background_palettes[idx] = value;
        RebuildBGPaletteCache(idx);
        if (io[IO_ADDR_BCPS] & PALETTE_INCREMENT_MASK)
            io[IO_ADDR_BCPS] = (io[IO_ADDR_BCPS] & PALETTE_INCREMENT_MASK) | ((idx + 1) & PALETTE_ADDRESS_MASK);
    }

    if (offset == IO_ADDR_OCPD)
    {
        const uint8_t idx = io[IO_ADDR_OCPS] & PALETTE_ADDRESS_MASK;
        this->object_palettes[idx] = value;
        RebuildOBJPaletteCache(idx);
        if (io[IO_ADDR_OCPS] & PALETTE_INCREMENT_MASK)
            io[IO_ADDR_OCPS] = (io[IO_ADDR_OCPS] & PALETTE_INCREMENT_MASK) | ((idx + 1) & PALETTE_ADDRESS_MASK);
    }

    if (offset == IO_ADDR_HDMA5)
    {
        const uint16_t src = (io[IO_ADDR_HDMA1] << 8 | io[IO_ADDR_HDMA2]) & 0xFFF0;
        const uint16_t dst = (io[IO_ADDR_HDMA3] << 8 | io[IO_ADDR_HDMA4]) & 0x1FF0;
        const uint16_t len = ((value & 0x7F) + 1) * HDMA_BLOCK_SIZE;

        if ((value & HDMA5_TYPE_MASK) == 0)
        {
            if (this->is_hdma_active)
            {
                this->is_hdma_active = false;
                io[IO_ADDR_HDMA5] = HDMA_TRANSFER_COMPLETE;
                return;
            }
            for (uint16_t i = 0; i < len; i++)
                this->memory->WriteVRAM(dst + i, this->memory->Read8(src + i));
            io[IO_ADDR_HDMA5] = HDMA_TRANSFER_COMPLETE;
        }
        else
        {
            this->hdma_src = src;
            this->hdma_dst = dst;
            this->hdma_bytes_left = len;
            this->is_hdma_active = true;
            io[IO_ADDR_HDMA5] = (len / HDMA_BLOCK_SIZE) - 1;
        }
    }
}

void PPU::SetMode(PPUMode new_mode)
{
    this->mode = new_mode;
    *STAT = (*STAT & ~STAT_MODE_MASK) | static_cast<uint8_t>(new_mode);
}

void PPU::IncrementScanline()
{
    this->scanline++;
    *LY = this->scanline;
    CheckLYC();
}

void PPU::CheckLYC() const
{
    const uint8_t ly = *LY;
    const uint8_t lyc = *LYC;
    uint8_t stat = *STAT;

    const bool prev_lyc_flag = (stat & STAT_LYC) != 0;
    const bool ly_match = (ly == lyc);

    if (ly_match) stat |= STAT_LYC;
    else stat &= ~STAT_LYC;
    *STAT = stat;

    if (ly_match && !prev_lyc_flag && (stat & STAT_LYC_INT))
        memory->SetInterruptFlag(INTERRUPT_STAT);
}

void PPU::RenderScanline()
{
    uint16_t* row = this->framebuffer + this->scanline * SCREEN_WIDTH;

    memset(this->scanline_priority, 0, SCREEN_WIDTH);
    memset(this->scanline_bg_priority, 0, SCREEN_WIDTH);

    if (!use_cgb_rendering && this->bgp_event_count > 0)
    {
        memset(this->scanline_bgp, *BGP, SCREEN_WIDTH);
        for (int i = 0; i < this->bgp_event_count; i++)
        {
            const int from = std::clamp<int>(this->bgp_events[i].dot - 1, 0, SCREEN_WIDTH);
            const int to = (i + 1 < this->bgp_event_count)
                               ? std::clamp<int>(this->bgp_events[i + 1].dot - 1, 0, SCREEN_WIDTH)
                               : SCREEN_WIDTH;
            if (from < to)
                memset(this->scanline_bgp + from, this->bgp_events[i].bgp, to - from);
        }
    }

    RenderBackground(row);
    RenderWindow(row);
    RenderObjects(row);
}

void PPU::RenderBackground(uint16_t* row)
{
    const uint8_t lcdc = *LCDC;

    if ((lcdc & LCDC_BG_ENABLE) == 0 && !use_cgb_rendering)
        return;

    const uint8_t scy = *SCY;
    const uint8_t scx = *SCX;

    const uint16_t tile_map_base = (lcdc & LCDC_BG_TILEMAP) ? 0x1C00 : 0x1800;
    const uint16_t tile_data_base = (lcdc & LCDC_TILE_DATA) ? 0x0000 : 0x1000;
    const bool signed_tiles = !(lcdc & LCDC_TILE_DATA);

    const uint8_t py = this->scanline + scy;
    const uint8_t tile_y = py >> 3;
    const uint8_t pixel_y = py & 7;

    const uint8_t* vram0 = memory->VRAMPtr(false);
    const uint8_t* vram1 = memory->VRAMPtr(true);

    uint8_t tile_x = (scx >> 3) & 0x1F;
    uint8_t pixel_x = scx & 7;
    int screen_x = 0;

    if (use_cgb_rendering)
    {
        auto render_tile = [&](uint8_t tx, int sx, int pstart, int count)
        {
            const uint16_t map_off = tile_map_base + (tile_y * 32) + tx;
            const uint8_t tile_idx = vram0[map_off];
            const uint8_t tile_attr = vram1[map_off];

            const bool flip_x = tile_attr & OBJ_FLIP_X_MASK;
            const bool flip_y = tile_attr & OBJ_FLIP_Y_MASK;
            const bool has_bg_pri = tile_attr & OBJ_PRIORITY_MASK;
            const bool use_bank1 = tile_attr & OBJ_BANK_MASK;

            const uint8_t real_py = flip_y ? (7 - pixel_y) : pixel_y;
            const uint16_t tile_addr = tile_data_base +
                (signed_tiles ? (int16_t)(int8_t)tile_idx : (int16_t)tile_idx) * TILE_SIZE_BYTES;

            const uint8_t* vsrc = use_bank1 ? vram1 : vram0;
            const uint8_t td1 = vsrc[tile_addr + real_py * 2];
            const uint8_t td2 = vsrc[tile_addr + real_py * 2 + 1];

            const uint16_t* lut = flip_x ? tile_pixel_lut_flipped : tile_pixel_lut;
            const uint16_t* pal = bg_palette_cache[tile_attr & OBJ_CGB_PALETTE_MASK];

            uint16_t td = (lut[td1] | (lut[td2] << 1)) >> (pstart * 2);
            for (int i = 0; i < count; i++, td >>= 2)
            {
                const uint8_t ci = td & 3;
                row[sx + i] = pal[ci];
                scanline_priority[sx + i] = ci;
                scanline_bg_priority[sx + i] = has_bg_pri;
            }
        };

        if (pixel_x != 0)
        {
            const int count = std::min((int)(TILE_PIXEL_SIZE - pixel_x), SCREEN_WIDTH);
            render_tile(tile_x, screen_x, pixel_x, count);
            screen_x += count;
            tile_x = (tile_x + 1) & 0x1F;
        }

        while (screen_x <= SCREEN_WIDTH - TILE_PIXEL_SIZE)
        {
            render_tile(tile_x, screen_x, 0, TILE_PIXEL_SIZE);
            screen_x += TILE_PIXEL_SIZE;
            tile_x = (tile_x + 1) & 0x1F;
        }

        if (screen_x < SCREEN_WIDTH)
            render_tile(tile_x, screen_x, 0, SCREEN_WIDTH - screen_x);
    }
    else
    {
        const uint8_t current_bgp = *BGP;
        const bool use_bgp_snapshot = (this->bgp_event_count > 0);

        if (use_bgp_snapshot)
        {
            auto render_tile = [&](uint8_t tx, int sx, int pstart, int count)
            {
                const uint16_t map_off = tile_map_base + (tile_y * 32) + tx;
                const uint8_t tile_idx = vram0[map_off];
                const uint16_t tile_addr = tile_data_base +
                    (signed_tiles ? (int16_t)(int8_t)tile_idx : (int16_t)tile_idx) * TILE_SIZE_BYTES;

                const uint8_t td1 = vram0[tile_addr + pixel_y * 2];
                const uint8_t td2 = vram0[tile_addr + pixel_y * 2 + 1];
                uint16_t td = (tile_pixel_lut[td1] | (tile_pixel_lut[td2] << 1)) >> (pstart * 2);

                for (int i = 0; i < count; i++, td >>= 2)
                {
                    const uint8_t ci = td & 3;
                    row[sx + i] = dmg_palette[(scanline_bgp[sx + i] >> (ci << 1)) & 3];
                    scanline_priority[sx + i] = ci;
                }
            };

            if (pixel_x != 0)
            {
                const int count = std::min((int)(TILE_PIXEL_SIZE - pixel_x), SCREEN_WIDTH);
                render_tile(tile_x, screen_x, pixel_x, count);
                screen_x += count;
                tile_x = (tile_x + 1) & 0x1F;
            }

            while (screen_x <= SCREEN_WIDTH - TILE_PIXEL_SIZE)
            {
                render_tile(tile_x, screen_x, 0, TILE_PIXEL_SIZE);
                screen_x += TILE_PIXEL_SIZE;
                tile_x = (tile_x + 1) & 0x1F;
            }

            if (screen_x < SCREEN_WIDTH)
                render_tile(tile_x, screen_x, 0, SCREEN_WIDTH - screen_x);
        }
        else
        {
            auto render_tile = [&](uint8_t tx, int sx, int pstart, int count)
            {
                const uint16_t map_off = tile_map_base + (tile_y * 32) + tx;
                const uint8_t tile_idx = vram0[map_off];
                const uint16_t tile_addr = tile_data_base +
                    (signed_tiles ? (int16_t)(int8_t)tile_idx : (int16_t)tile_idx) * TILE_SIZE_BYTES;

                const uint8_t td1 = vram0[tile_addr + pixel_y * 2];
                const uint8_t td2 = vram0[tile_addr + pixel_y * 2 + 1];
                uint16_t td = (tile_pixel_lut[td1] | (tile_pixel_lut[td2] << 1)) >> (pstart * 2);

                for (int i = 0; i < count; i++, td >>= 2)
                {
                    const uint8_t ci = td & 3;
                    row[sx + i] = dmg_palette[(current_bgp >> (ci << 1)) & 3];
                    scanline_priority[sx + i] = ci;
                }
            };

            if (pixel_x != 0)
            {
                const int count = std::min((int)(TILE_PIXEL_SIZE - pixel_x), SCREEN_WIDTH);
                render_tile(tile_x, screen_x, pixel_x, count);
                screen_x += count;
                tile_x = (tile_x + 1) & 0x1F;
            }

            while (screen_x <= SCREEN_WIDTH - TILE_PIXEL_SIZE)
            {
                render_tile(tile_x, screen_x, 0, TILE_PIXEL_SIZE);
                screen_x += TILE_PIXEL_SIZE;
                tile_x = (tile_x + 1) & 0x1F;
            }

            if (screen_x < SCREEN_WIDTH)
                render_tile(tile_x, screen_x, 0, SCREEN_WIDTH - screen_x);
        }
    }
}

void PPU::RenderWindow(uint16_t* row)
{
    const uint8_t lcdc = *LCDC;
    if (!(lcdc & LCDC_WINDOW_ENABLE) || (!(lcdc & LCDC_BG_ENABLE) && !use_cgb_rendering))
        return;

    const uint8_t wy = *WY;
    const uint8_t wx = *WX;

    if (this->scanline < wy)
        return;

    const int start_x = wx >= WX_OFFSET ? wx - WX_OFFSET : 0;
    if (start_x >= SCREEN_WIDTH)
        return;

    const uint16_t tile_map_base = (lcdc & LCDC_WINDOW_TILEMAP) ? 0x1C00 : 0x1800;
    const uint16_t tile_data_base = (lcdc & LCDC_TILE_DATA) ? 0x0000 : 0x1000;
    const bool signed_tiles = !(lcdc & LCDC_TILE_DATA);

    const uint8_t* vram0 = memory->VRAMPtr(false);
    const uint8_t* vram1 = memory->VRAMPtr(true);

    const uint8_t py = this->window_line;
    const uint8_t tile_y = py >> 3;
    const uint8_t pixel_y = py & 7;

    int screen_x = start_x;
    uint8_t tile_x = 0;

    if (use_cgb_rendering)
    {
        auto render_tile = [&](uint8_t tx, int sx, int pstart, int count)
        {
            const uint16_t map_off = tile_map_base + (tile_y * 32) + tx;
            const uint8_t tile_idx = vram0[map_off];
            const uint8_t tile_attr = vram1[map_off];

            const bool use_bank1 = tile_attr & OBJ_BANK_MASK;
            const bool flip_x = tile_attr & OBJ_FLIP_X_MASK;
            const bool flip_y = tile_attr & OBJ_FLIP_Y_MASK;
            const bool has_bg_pri = tile_attr & OBJ_PRIORITY_MASK;

            const uint8_t real_py = flip_y ? (7 - pixel_y) : pixel_y;
            const uint16_t tile_addr = tile_data_base +
                (signed_tiles ? (int16_t)(int8_t)tile_idx : (int16_t)tile_idx) * TILE_SIZE_BYTES;

            const uint8_t* vsrc = use_bank1 ? vram1 : vram0;
            const uint8_t td1 = vsrc[tile_addr + real_py * 2];
            const uint8_t td2 = vsrc[tile_addr + real_py * 2 + 1];

            const uint16_t* lut = flip_x ? tile_pixel_lut_flipped : tile_pixel_lut;
            const uint16_t* pal = bg_palette_cache[tile_attr & OBJ_CGB_PALETTE_MASK];

            uint16_t td = (lut[td1] | (lut[td2] << 1)) >> (pstart * 2);
            for (int i = 0; i < count; i++, td >>= 2)
            {
                const uint8_t ci = td & 3;
                row[sx + i] = pal[ci];
                scanline_priority[sx + i] = ci;
                scanline_bg_priority[sx + i] = has_bg_pri;
            }
        };

        while (screen_x <= SCREEN_WIDTH - TILE_PIXEL_SIZE)
        {
            render_tile(tile_x, screen_x, 0, TILE_PIXEL_SIZE);
            screen_x += TILE_PIXEL_SIZE;
            tile_x++;
        }

        if (screen_x < SCREEN_WIDTH)
            render_tile(tile_x, screen_x, 0, SCREEN_WIDTH - screen_x);
    }
    else
    {
        auto render_tile = [&](uint8_t tx, int sx, int pstart, int count)
        {
            const uint16_t map_off = tile_map_base + (tile_y * 32) + tx;
            const uint8_t tile_idx = vram0[map_off];
            const uint16_t tile_addr = tile_data_base +
                (signed_tiles ? (int16_t)(int8_t)tile_idx : (int16_t)tile_idx) * TILE_SIZE_BYTES;

            const uint8_t td1 = vram0[tile_addr + pixel_y * 2];
            const uint8_t td2 = vram0[tile_addr + pixel_y * 2 + 1];
            uint16_t td = (tile_pixel_lut[td1] | (tile_pixel_lut[td2] << 1)) >> (pstart * 2);

            for (int i = 0; i < count; i++, td >>= 2)
            {
                const uint8_t ci = td & 3;
                row[sx + i] = dmg_palette[(scanline_bgp[sx + i] >> (ci << 1)) & 3];
                scanline_priority[sx + i] = ci;
            }
        };

        while (screen_x <= SCREEN_WIDTH - TILE_PIXEL_SIZE)
        {
            render_tile(tile_x, screen_x, 0, TILE_PIXEL_SIZE);
            screen_x += TILE_PIXEL_SIZE;
            tile_x++;
        }

        if (screen_x < SCREEN_WIDTH)
            render_tile(tile_x, screen_x, 0, SCREEN_WIDTH - screen_x);
    }

    this->window_line++;
}

void PPU::ScanOAM()
{
    this->object_count = 0;

    const uint8_t lcdc = *LCDC;
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t object_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;
    const uint8_t* oam_ptr = memory->OAMPtr();

    for (uint8_t i = 0; i < OAM_MAX_COUNT; i++)
    {
        const uint8_t* entry = oam_ptr + i * OAM_STRUCT_SIZE;
        const uint8_t obj_y = entry[0];
        const int obj_line = this->scanline - (obj_y - 16);

        if (obj_line < 0 || obj_line >= object_height)
            continue;

        this->objects[this->object_count++] = {obj_y, entry[1], entry[2], entry[3], i};

        if (this->object_count == OAM_MAX_SPRITES)
            break;
    }

    const bool cgb_priority = use_cgb_rendering && (*OPRI & OPRI_DMG_MODE_MASK) == 0;

    for (int i = 1; i < this->object_count; i++)
    {
        const OAMObject key = this->objects[i];
        int j = i - 1;
        while (j >= 0)
        {
            const bool should_swap = cgb_priority
                                         ? this->objects[j].oam_index > key.oam_index
                                         : (this->objects[j].x != key.x
                                                ? this->objects[j].x > key.x
                                                : this->objects[j].oam_index > key.oam_index);

            if (!should_swap)
                break;

            this->objects[j + 1] = this->objects[j];
            j--;
        }
        this->objects[j + 1] = key;
    }
}

void PPU::RenderObjects(uint16_t* row)
{
    const uint8_t lcdc = *LCDC;
    if ((lcdc & LCDC_OBJ_ENABLE) == 0)
        return;

    const uint8_t object_height = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;
    const uint8_t* vram0 = memory->VRAMPtr(false);
    const uint8_t* vram1 = memory->VRAMPtr(true);
    const bool bg_enable = lcdc & LCDC_BG_ENABLE;

    uint16_t obp_colors[2][4];
    if (!use_cgb_rendering)
    {
        const uint8_t obp0 = *OBP0;
        const uint8_t obp1 = *OBP1;
        for (int i = 0; i < 4; i++)
        {
            obp_colors[0][i] = dmg_palette[(obp0 >> (i << 1)) & 3];
            obp_colors[1][i] = dmg_palette[(obp1 >> (i << 1)) & 3];
        }
    }

    for (int8_t obj_idx = this->object_count - 1; obj_idx >= 0; obj_idx--)
    {
        const OAMObject& object = this->objects[obj_idx];
        const int object_x = object.x - 8;
        const int object_y = object.y - 16;

        const int px_start = object_x < 0 ? -object_x : 0;
        const int px_end = (object_x + TILE_PIXEL_SIZE > SCREEN_WIDTH)
                               ? SCREEN_WIDTH - object_x
                               : TILE_PIXEL_SIZE;
        if (px_start >= px_end) continue;

        const bool flip_y = object.attributes & OBJ_FLIP_Y_MASK;
        const bool flip_x = object.attributes & OBJ_FLIP_X_MASK;
        const bool has_priority = object.attributes & OBJ_PRIORITY_MASK;

        const int object_line = this->scanline - object_y;
        uint8_t tile_line = flip_y ? (object_height - object_line - 1) : object_line;
        uint16_t tile_index = object.tile_index;

        if (object_height == 16)
        {
            tile_index = (tile_index & 0xFE) | (tile_line >> 3);
            tile_line = tile_line & 7;
        }

        const uint8_t* vsrc =
            (use_cgb_rendering && (object.attributes & OBJ_BANK_MASK)) ? vram1 : vram0;

        const uint8_t td1 = vsrc[tile_index * TILE_SIZE_BYTES + tile_line * 2];
        const uint8_t td2 = vsrc[tile_index * TILE_SIZE_BYTES + tile_line * 2 + 1];

        const uint16_t* colors = use_cgb_rendering
                                     ? obj_palette_cache[object.attributes & OBJ_CGB_PALETTE_MASK]
                                     : obp_colors[(object.attributes & OBJ_DMG_PALETTE_MASK) ? 1 : 0];

        const uint16_t* lut = flip_x ? tile_pixel_lut_flipped : tile_pixel_lut;
        uint16_t td = (lut[td1] | (lut[td2] << 1)) >> (px_start * 2);

        if (use_cgb_rendering)
        {
            for (int px = px_start; px < px_end; px++, td >>= 2)
            {
                const uint8_t ci = td & 3;
                if (ci == 0) continue;
                const int sx = object_x + px;
                if (bg_enable && (scanline_bg_priority[sx] || has_priority) && scanline_priority[sx] != 0)
                    continue;
                row[sx] = colors[ci];
                scanline_priority[sx] = ci;
            }
        }
        else
        {
            for (int px = px_start; px < px_end; px++, td >>= 2)
            {
                const uint8_t ci = td & 3;
                if (ci == 0) continue;

                const int sx = object_x + px;
                if (has_priority && scanline_priority[sx] != 0) continue;

                row[sx] = colors[ci];
                scanline_priority[sx] = ci;
            }
        }
    }
}
