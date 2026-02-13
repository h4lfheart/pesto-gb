#pragma once
#include <cstdint>

#include "cartridge.h"

#define GB_ROM_SIZE 0x100
#define GB_BOOT_ROM_SIZE 0x100
#define GB_WRAM_SIZE 0x1000
#define GB_VRAM_SIZE 0x2000
#define GB_HRAM_SIZE 0x7F
#define GB_OAM_SIZE 0xA0
#define GB_IO_SIZE 0x80

class Memory
{
public:
    Memory();

    void LoadBootRom(char* boot_rom_path);

    uint8_t ReadIO(uint8_t address);
    uint8_t Read8(uint16_t address);
    uint16_t Read16(uint16_t address);

    void WriteIO(uint8_t address, uint8_t value);
    void Write8(uint16_t address, uint8_t value);
    void Write16(uint16_t address, uint16_t value);

    void AttachCartridge(Cartridge* cart);

    uint8_t ie;

protected:
    Cartridge* cartridge;

private:
    uint8_t Read(uint16_t address);
    void Write(uint16_t address, uint8_t value);

    bool use_boot_rom = false;
    uint8_t boot_rom[GB_BOOT_ROM_SIZE] = {};

public:

    uint8_t oam[GB_OAM_SIZE] = {};
    uint8_t wram1[GB_WRAM_SIZE] = {};
    uint8_t wram2[GB_WRAM_SIZE] = {};
    uint8_t hram[GB_HRAM_SIZE] = {};
    uint8_t vram[GB_VRAM_SIZE] = {};
    uint8_t io[GB_IO_SIZE] = {};
};