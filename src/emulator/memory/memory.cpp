#include "memory.h"

#include <cstdio>
#include <process.h>

#include "../io/input.h"

Memory::Memory()
{
}

void Memory::LoadBootRom(char* boot_rom_path)
{
    FILE *rom = fopen(boot_rom_path, "rb");
    fread(this->boot_rom, 1, GB_BOOT_ROM_SIZE, rom);
    fclose(rom);

    this->use_boot_rom = true;
}

uint8_t Memory::ReadIO(uint8_t address)
{
    return this->io[address];
}

uint8_t Memory::Read8(uint16_t address)
{
    return this->Read(address);
}

uint16_t Memory::Read16(uint16_t address)
{
    return Read8(address) | Read8(address + 1) << 8;
}

void Memory::WriteIO(uint8_t address, uint8_t value)
{
    this->io[address] = value;
}

void Memory::Write8(uint16_t address, uint8_t value)
{
    Write(address, value);
}

void Memory::Write16(uint16_t address, uint16_t value)
{
    Write8(address, value & 0xFF);
    Write8(address + 1, value >> 8);
}

void Memory::AttachCartridge(Cartridge* cart)
{
    this->cartridge = cart;
    this->Write(0xFF00, 0xFF);
}

uint8_t Memory::Read(uint16_t address)
{
    // TODO use memory map entries
    if (this->use_boot_rom && address < 0x100)
    {
        return this->boot_rom[address];
    }

    if (address <= 0x7FFF)
    {
        return this->cartridge->rom_data[address];
    }

    if (address >= 0xA000 && address <= 0xBFFF)
    {
        return this->cartridge->ram[address - 0xA000];
    }

    if (address >= 0xC000 && address <= 0xCFFF)
    {
        return this->wram1[address - 0xC000];
    }

    if (address >= 0xD000 && address <= 0xDFFF)
    {
        return this->wram2[address - 0xD000];
    }

    if (address >= 0x8000 && address <= 0x9FFF)
    {
        return this->vram[address - 0x8000];
    }

    if (address >= 0xFE00 && address <= 0xFE9F)
    {
        return this->oam[address - 0xFE00];
    }

    if (address >= 0xFEA0 && address <= 0xFEFF)
        return 0xFF;

    if (address >= 0xFF00 && address <= 0xFF7F)
    {
        return this->io[address - 0xFF00];
    }

    if (address >= 0xFF80 && address <= 0xFFFE)
    {
        return this->hram[address - 0xFF80];
    }

    if (address == 0xFFFF)
    {
       return ie;
    }

    fprintf(stderr, "Invalid read at address 0x%X", address);
    exit(1);
    return 0;
}

void Memory::Write(uint16_t address, uint8_t value)
{
    if (address == 0xFF50 && value != 0)
    {
        this->use_boot_rom = false;
    }

    if (address <= 0x7FFF)
    {
        return;
    }

    if (address >= 0xA000 && address <= 0xBFFF)
    {
        this->cartridge->ram[address - 0xA000] = value;;
        return;
    }

    if (address >= 0xC000 && address <= 0xCFFF)
    {
        this->wram1[address - 0xC000] = value;
        return;
    }

    if (address >= 0xD000 && address <= 0xDFFF)
    {
        this->wram2[address - 0xD000] = value;
        return;
    }

    if (address >= 0x8000 && address <= 0x9FFF)
    {
        this->vram[address - 0x8000] = value;
        return;
    }

    if (address >= 0xFE00 && address <= 0xFE9F)
    {
        this->oam[address - 0xFE00] = value;
        return;
    }

    if (address >= 0xFEA0 && address <= 0xFEFF)
    {
        return;
    }

    if (address >= 0xFF00 && address <= 0xFF7F)
    {
        this->io[address - 0xFF00] = value;
        return;
    }

    if (address >= 0xFF80 && address <= 0xFFFE)
    {
        this->hram[address - 0xFF80] = value;
        return;
    }

    if (address == 0xFFFF)
    {
        ie = value;
        return;
    }

    fprintf(stderr, "Invalid write at address 0x%X", address);
    exit(1);
}
