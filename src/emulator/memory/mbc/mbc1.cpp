#include "mbc1.h"

uint8_t MBC1::ReadRom(const uint8_t* rom_data, uint16_t address)
{
    if (address < 0x4000)
    {
        return rom_data[address];
    }

    const uint32_t mapped_address = (rom_bank * 0x4000) + (address - 0x4000);
    return rom_data[mapped_address];
}

void MBC1::WriteRom(uint16_t address, uint8_t value)
{
    if (address <= 0x1FFF)
    {
        ram_enabled = (value & 0x0F) == 0x0A;
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        rom_bank = (rom_bank & 0x60) | (value & 0x1F);
        if (rom_bank == 0) rom_bank = 1;
    }
    else if (address >= 0x4000 && address <= 0x5FFF)
    {
        ram_bank = value & 0x03;
        rom_bank = (rom_bank & 0x1F) | ((value & 0x03) << 5);
    }
    else if (address >= 0x6000 && address <= 0x7FFF)
    {
        use_ram_banking = (value & 0x01) != 0;
    }
}

uint8_t MBC1::ReadRam(const uint8_t* ram_data, uint16_t address)
{
    if (!ram_enabled)
        return 0xFF;

    uint32_t mapped_address = (ram_bank * 0x2000) + address;
    return ram_data[mapped_address];
}

void MBC1::WriteRam(uint8_t* ram_data, uint16_t address, uint8_t value)
{
    if (!ram_enabled)
        return;

    uint32_t mapped_address = (ram_bank * 0x2000) + address;
    ram_data[mapped_address] = value;
}