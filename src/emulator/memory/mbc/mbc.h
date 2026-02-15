#pragma once
#include <cstdint>
#include <memory>

class MBC
{
public:
    virtual ~MBC() = default;

    virtual uint8_t ReadRom(const uint8_t* rom_data, uint16_t address) = 0;
    virtual void WriteRom(uint16_t address, uint8_t value) = 0;

    virtual uint8_t ReadRam(const uint8_t* ram_data, uint16_t address) = 0;
    virtual void WriteRam(uint8_t* ram_data, uint16_t address, uint8_t value) = 0;

    static std::unique_ptr<MBC> CreateMBC(uint8_t cartridge_type);
};
