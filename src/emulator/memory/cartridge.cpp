#include "cartridge.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

Cartridge::Cartridge()
{
    this->ram = static_cast<uint8_t*>(calloc(1, 128 * 1024));
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

    ReadHeader();

    const uint32_t ram_size = this->GetRamSize();
    if (ram_size > 0)
    {
        this->ram = static_cast<uint8_t*>(calloc(1, ram_size));
    }

    mbc = MBC::CreateMBC(header.cartridge_type);
}

void Cartridge::ReadSave(const char* path)
{
    if (!this->mbc->HasBattery())
        return;

    FILE* file = fopen(path, "rb");
    if (file == nullptr)
        return;

    fread(ram, 1, GetRamSize(), file);
    fclose(file);
}

void Cartridge::WriteSave(const char* path)
{
    if (!this->mbc->HasBattery())
        return;

    FILE* file = fopen(path, "wb");
    if (file == nullptr)
        return;

    fwrite(ram, 1, GetRamSize(), file);
    fclose(file);
}

void Cartridge::ReadHeader()
{
    if (this->rom_data == nullptr)
        return;

    memcpy(this->header.title, &this->rom_data[HEADER_TITLE_ADDR], HEADER_TITLE_LENGTH);

    this->header.cartridge_type = rom_data[HEADER_MBC_ADDR];
    this->header.rom_size = rom_data[HEADER_ROM_SIZE_ADDR];
    this->header.ram_size = rom_data[HEADER_RAM_SIZE_ADDR];
    this->header.has_cgb_support = rom_data[HEADER_CGB_FLAG_ADDR] & HEADER_CGB_ENHANCED_MASK;
}

char* Cartridge::GetTitle()
{
    return this->header.title;
}

const char* Cartridge::GetCartridgeType() const
{
    switch (this->header.cartridge_type)
    {
    case 0x00: return "ROM ONLY";
    case 0x01: return "MBC1";
    case 0x02: return "MBC1+RAM";
    case 0x03: return "MBC1+RAM+BATTERY";
    case 0x05: return "MBC2";
    case 0x06: return "MBC2+BATTERY";
    case 0x08: return "ROM+RAM";
    case 0x09: return "ROM+RAM+BATTERY";
    case 0x0B: return "MMM01";
    case 0x0C: return "MMM01+RAM";
    case 0x0D: return "MMM01+RAM+BATTERY";
    case 0x0F: return "MBC3+TIMER+BATTERY";
    case 0x10: return "MBC3+TIMER+RAM+BATTERY";
    case 0x11: return "MBC3";
    case 0x12: return "MBC3+RAM";
    case 0x13: return "MBC3+RAM+BATTERY";
    case 0x19: return "MBC5";
    case 0x1A: return "MBC5+RAM";
    case 0x1B: return "MBC5+RAM+BATTERY";
    case 0x1C: return "MBC5+RUMBLE";
    case 0x1D: return "MBC5+RUMBLE+RAM";
    case 0x1E: return "MBC5+RUMBLE+RAM+BATTERY";
    case 0x20: return "MBC6";
    case 0x22: return "MBC7+SENSOR+RUMBLE+RAM+BATTERY";
    case 0xFC: return "POCKET CAMERA";
    case 0xFD: return "BANDAI TAMA5";
    case 0xFE: return "HuC3";
    case 0xFF: return "HuC1+RAM+BATTERY";
    default: return "UNKNOWN";
    }
}

uint32_t Cartridge::GetRomSize() const
{
    return 32 * 1024 * (1 << this->header.rom_size);
}

uint32_t Cartridge::GetRamSize() const
{
    switch (this->header.ram_size) {
        case 0x00: return 0;
        case 0x02: return 8 * 1024;
        case 0x03: return 32 * 1024;
        case 0x04: return 128 * 1024;
        case 0x05: return 64 * 1024;
        default: return 0;
    }
}

uint16_t Cartridge::GetRomBankCount() const
{
    return 2 * (1 << this->header.rom_size);
}

uint8_t Cartridge::GetRamBankCount() const
{
    switch (this->header.ram_size) {
        case 0x00: return 0;
        case 0x02: return 1;
        case 0x03: return 4;
        case 0x04: return 16;
        case 0x05: return 8;
        default: return 0;
    }
}

bool Cartridge::HasCGBSupport() const
{
    return this->header.has_cgb_support;
}

uint8_t Cartridge::ReadRom(uint16_t address) const
{
    return this->mbc->ReadRom(rom_data, address);
}

void Cartridge::WriteRom(uint16_t address, uint8_t value) const
{
    this->mbc->WriteRom(address, value);
}

uint8_t Cartridge::ReadRam(uint16_t address) const
{
    return this->mbc->ReadRam(ram, address);
}

void Cartridge::WriteRam(uint16_t address, uint8_t value) const
{
    if (header.ram_size == 0)
        return;

    this->mbc->WriteRam(ram, address, value);
}