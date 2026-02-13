#include "cartridge.h"

#include <cstdio>
#include <cstdlib>

Cartridge::Cartridge()
{
}

void Cartridge::LoadRom(char* rom_path)
{
    FILE *rom = fopen(rom_path, "rb");

    fseek(rom, 0, SEEK_END);
    const size_t size = ftell(rom);
    rewind(rom);

    this->rom_data = static_cast<uint8_t*>(malloc(size));

    fread(this->rom_data, 1, size, rom);
    fclose(rom);
}
