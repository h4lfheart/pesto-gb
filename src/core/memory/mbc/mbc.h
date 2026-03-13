#pragma once
#include <cstdint>
#include <memory>
#include <set>
#include <vector>

class MBC
{
public:
    explicit MBC(uint8_t type);
    virtual ~MBC() = default;

    virtual uint8_t ReadRom(const uint8_t* rom_data, uint16_t address) = 0;
    virtual void WriteRom(uint16_t address, uint8_t value) = 0;

    virtual uint8_t ReadRam(const uint8_t* ram_data, uint16_t address) = 0;
    virtual void WriteRam(uint8_t* ram_data, uint16_t address, uint8_t value) = 0;

    bool HasBattery();

    static std::unique_ptr<MBC> CreateMBC(uint8_t cartridge_type);

    uint8_t cartridge_type;

private:
    std::set<uint8_t> battery_types ={
        // MBC1
        0x03,

        // MBC 3
        0x0F, 0x10, 0x13,

        // MBC 5
        0x1B, 0x1E
    };
};
