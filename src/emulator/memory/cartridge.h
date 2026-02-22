#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include "mbc/mbc.h"

#define GB_ROM_BANK_SIZE 0x4000
#define GB_ROM_EXTERNAL_RAM_SIZE 0x2000

#define HEADER_TITLE_ADDR 0x134
#define HEADER_CGB_FLAG_ADDR 0x143
#define HEADER_MBC_ADDR 0x147
#define HEADER_ROM_SIZE_ADDR 0x148
#define HEADER_RAM_SIZE_ADDR 0x149
#define HEADER_ROM_VERSION_ADDR 0x14C

#define HEADER_CGB_ENHANCED_MASK 0b10000000
#define HEADER_CGB_ONLY_MASK 0b01000000

#define HEADER_TITLE_LENGTH 16

struct CartridgeHeader
{
    char title[HEADER_TITLE_LENGTH];
    uint8_t cartridge_type;
    uint8_t rom_size;
    uint8_t ram_size;
    bool has_cgb_support;
};

class Cartridge
{
public:
    Cartridge();

    void LoadRom(char* rom_path);
    void ReadSave(const char* path);
    void WriteSave(const char* path);

    char* GetTitle();
    const char* GetCartridgeType() const;
    uint32_t GetRomSize() const;
    uint32_t GetRamSize() const;
    uint8_t GetRomVersion() const;
    uint16_t GetRomBankCount() const;
    uint8_t GetRamBankCount() const;
    bool HasCGBSupport() const;

    uint8_t ReadRom(uint16_t address) const;
    void WriteRom(uint16_t address, uint8_t value) const;
    uint8_t ReadRam(uint16_t address) const;
    void WriteRam(uint16_t address, uint8_t value) const;

private:
    void ReadHeader();

    CartridgeHeader header;

    uint8_t* rom_data = nullptr;
    uint8_t* ram = nullptr;

    std::unique_ptr<MBC> mbc = nullptr;
};