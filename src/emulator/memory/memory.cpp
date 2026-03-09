#include "memory.h"

#include <algorithm>
#include <cstdio>

#include "../cpu/cpu.h"
#include "../io/input.h"
#include "../timer/timer.h"

void Memory::RebuildPageTable()
{
    for (int i = 0; i < 256; i++)
        page_table[i] = {nullptr, nullptr};

    uint8_t* vram = use_extra_vram ? vram2 : vram1;
    for (int pg = 0x80; pg <= 0x9F; pg++)
    {
        uint8_t* base = vram + (pg - 0x80) * 256;
        page_table[pg] = {base, base};
    }

    for (int pg = 0xC0; pg <= 0xCF; pg++)
    {
        uint8_t* base = wram0 + (pg - 0xC0) * 256;
        page_table[pg] = {base, base};
    }

    uint8_t* wramx = wram_banks[wram_bank - 1];
    for (int pg = 0xD0; pg <= 0xDF; pg++)
    {
        uint8_t* base = wramx + (pg - 0xD0) * 256;
        page_table[pg] = {base, base};
    }
}

Memory::Memory()
{
}

void Memory::AttachCPU(CPU* cpu) { this->cpu = cpu; }

void Memory::AttachCartridge(Cartridge* cart)
{
    this->cartridge = cart;
    RebuildPageTable();
}

void Memory::LoadBootRom(char* boot_rom_path)
{
    FILE* rom = fopen(boot_rom_path, "rb");
    size_t bytes_read = fread(this->boot_rom, 1, GB_CGB_BOOT_ROM_SIZE, rom);
    fclose(rom);

    this->use_boot_rom = true;
    this->uses_cgb_bootrom = (bytes_read > GB_DMG_BOOT_ROM_SIZE);
}

void Memory::SetInterruptFlag(uint8_t flag)
{
    this->io[IO_ADDR_INTERRUPT_FLAG] |= flag;
}

uint8_t Memory::FallbackRead(uint16_t address)
{
    if (address < ADDR_ROM_END) [[likely]]
    {
        if (use_boot_rom && (address < ADDR_BOOT_ROM_END || (uses_cgb_bootrom && address >= ADDR_CGB_BOOT_ROM_BEGIN &&
            address < ADDR_CGB_BOOT_ROM_END)))
            return boot_rom[address];
        return cartridge->ReadRom(address);
    }

    if (address < ADDR_VRAM_END)
        return use_extra_vram ? vram2[address - ADDR_VRAM_BEGIN] : vram1[address - ADDR_VRAM_BEGIN];

    if (address < ADDR_CARTRIDGE_RAM_END)
        return cartridge->ReadRam(address - ADDR_CARTRIDGE_RAM_BEGIN);

    if (address < ADDR_WRAM0_END)
        return wram0[address - ADDR_WRAM0_BEGIN];

    if (address < ADDR_WRAM_BANK_END)
        return wram_banks[wram_bank - 1][address - ADDR_WRAM_BANK_BEGIN];

    if (address < ADDR_ECHO_END)
        return wram0[address - ADDR_ECHO_BEGIN];

    if (address < ADDR_OAM_END)
        return oam[address - ADDR_OAM_BEGIN];

    if (address < ADDR_INVALID_END)
        return 0xFF;

    if (address < ADDR_IO_END)
        return ReadIO(address - ADDR_IO_BEGIN);

    if (address < ADDR_HRAM_END)
        return hram[address - ADDR_HRAM_BEGIN];

    return ie;
}

void Memory::FallbackWrite(uint16_t address, uint8_t value)
{
    if (address < ADDR_ROM_END) [[likely]]
    {
        cartridge->WriteRom(address, value);
        return;
    }

    if (address < ADDR_VRAM_END)
    {
        if (use_extra_vram) vram2[address - ADDR_VRAM_BEGIN] = value;
        else vram1[address - ADDR_VRAM_BEGIN] = value;
        return;
    }

    if (address < ADDR_CARTRIDGE_RAM_END)
    {
        cartridge->WriteRam(address - ADDR_CARTRIDGE_RAM_BEGIN, value);
        return;
    }

    if (address < ADDR_WRAM0_END)
    {
        wram0[address - ADDR_WRAM0_BEGIN] = value;
        return;
    }

    if (address < ADDR_WRAM_BANK_END)
    {
        wram_banks[wram_bank - 1][address - ADDR_WRAM_BANK_BEGIN] = value;
        return;
    }

    if (address < ADDR_ECHO_END)
    {
        wram0[address - ADDR_ECHO_BEGIN] = value;
        return;
    }

    if (address < ADDR_OAM_END)
    {
        oam[address - ADDR_OAM_BEGIN] = value;
        return;
    }

    if (address < ADDR_INVALID_END)
        return;

    if (address < ADDR_IO_END)
    {
        WriteIO(address - ADDR_IO_BEGIN, value);
        return;
    }

    if (address < ADDR_HRAM_END)
    {
        hram[address - ADDR_HRAM_BEGIN] = value;
        return;
    }

    ie = value;
}

uint8_t Memory::ReadIO(uint16_t offset)
{
    if (io_lut[offset].read) [[unlikely]]
        return io_lut[offset].read(io_lut[offset].ctx, this->io, offset);
    return this->io[offset];
}

void Memory::WriteIO(uint16_t offset, uint8_t value)
{
    if (io_lut[offset].write) [[unlikely]]
    {
        io_lut[offset].write(io_lut[offset].ctx, this->io, offset, value);
        return;
    }

    if (offset == IO_ADDR_VBK)
    {
        this->use_extra_vram = value & VBK_ENABLE_MASK;
        RebuildPageTable();
    }

    if (offset == IO_ADDR_WBK)
    {
        this->wram_bank = std::max(1, value & WBK_BANK_MASK);
        RebuildPageTable();
    }

    if (offset == IO_ADDR_BOOT && value != 0)
        this->use_boot_rom = false;

    if (offset == IO_ADDR_OAM_DMA)
    {
        uint16_t src = value << 8;
        for (uint16_t dst = OAM_BEGIN; dst <= OAM_END; dst++, src++)
            this->oam[dst - OAM_BEGIN] = this->Read8(src);
    }

    this->io[offset] = value;
}

uint16_t Memory::Read16(uint16_t address)
{
    return Read8(address) | (Read8(address + 1) << 8);
}

void Memory::Write16(uint16_t address, uint16_t value)
{
    Write8(address, value & 0xFF);
    Write8(address + 1, value >> 8);
}

uint8_t Memory::ReadVRAMBank(bool use_extra_bank, uint16_t offset)
{
    return use_extra_bank ? vram2[offset] : vram1[offset];
}

void Memory::WriteVRAM(uint16_t offset, uint8_t value)
{
    if (use_extra_vram) vram2[offset] = value;
    else vram1[offset] = value;
}

uint8_t* Memory::VRAMPtr(bool use_extra_bank) { return use_extra_bank ? vram2 : vram1; }
uint8_t* Memory::OAMPtr() { return oam; }

bool Memory::IsCGB() const
{
    return this->cartridge->HasCGBSupport() && this->uses_cgb_bootrom;
}
