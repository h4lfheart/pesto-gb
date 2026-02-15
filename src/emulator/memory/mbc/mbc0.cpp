#include "mbc0.h"


uint8_t MBC0::ReadRom(const uint8_t* rom_data, uint16_t address)
{
    if (address < 0x8000)
        return rom_data[address];

    return 0xFF;
}

void MBC0::WriteRom(uint16_t address, uint8_t value)
{
}

uint8_t MBC0::ReadRam(const uint8_t* ram_data, uint16_t address)
{
    return 0xFF;
}

void MBC0::WriteRam(uint8_t* ram_data, uint16_t address, uint8_t value)
{

}