#include "mbc5.h"

uint8_t MBC5::ReadRom(const uint8_t* rom_data, uint16_t address)
{
    if (address < 0x4000)
    {
        return rom_data[address];
    }

    const uint32_t mapped_address = (this->rom_bank * 0x4000) + (address - 0x4000);
    return rom_data[mapped_address];
}

void MBC5::WriteRom(uint16_t address, uint8_t value)
{
    if (address <= 0x1FFF)
    {
        this->ram_enabled = (value & 0x0F) == 0x0A;
    }
    else if (address >= 0x2000 && address <= 0x2FFF)
    {
        this->rom_bank = (this->rom_bank & 0xFF00) | value;
    }
    else if (address >= 0x3000 && address <= 0x3FFF)
    {
        this->rom_bank = (this->rom_bank & 0x00FF) | ((value & 0x01) << 8);
    }
    else if (address >= 0x4000 && address <= 0x5FFF)
    {
        this->ram_bank = value & 0x0F;
    }
}

uint8_t MBC5::ReadRam(const uint8_t* ram_data, uint16_t address)
{
    if (!this->ram_enabled)
        return 0xFF;

    const uint32_t mapped_address = (this->ram_bank * 0x2000) + address;
    return ram_data[mapped_address];
}

void MBC5::WriteRam(uint8_t* ram_data, uint16_t address, uint8_t value)
{
    if (!this->ram_enabled)
        return;

    const uint32_t mapped_address = (this->ram_bank * 0x2000) + address;
    ram_data[mapped_address] = value;
}