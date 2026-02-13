#pragma once
#include <cstdint>

#define GB_ROM_BANK_SIZE 0x4000
#define GB_ROM_EXTERNAL_RAM_SIZE 0x2000

class Cartridge
{
public:
    Cartridge();

    void LoadRom(char* rom_path);

    uint8_t* rom_data = nullptr;

    uint8_t ram[GB_ROM_EXTERNAL_RAM_SIZE] = {};
};
