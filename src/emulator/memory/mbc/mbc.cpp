#include "mbc.h"

#include <memory>

#include "mbc0.h"
#include "mbc0.h"
#include "mbc0.h"
#include "mbc1.h"
#include "mbc3.h"
#include "mbc5.h"

MBC::MBC(const uint8_t type)
{
    this->cartridge_type = type;
}

bool MBC::HasBattery()
{
    return this->battery_types.contains(this->cartridge_type);
}

std::unique_ptr<MBC> MBC::CreateMBC(uint8_t cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x00: // ROM ONLY
        return std::make_unique<MBC0>(cartridge_type);

    case 0x01: // MBC1
    case 0x02: // MBC1+RAM
    case 0x03: // MBC1+RAM+BATTERY
        return std::make_unique<MBC1>(cartridge_type);

    case 0x0F: // MBC3+TIMER+BATTERY
    case 0x10: // MBC3+TIMER+RAM+BATTERY
    case 0x11: // MBC3
    case 0x12: // MBC3+RAM
    case 0x13: // MBC3+RAM+BATTERY
        return std::make_unique<MBC3>(cartridge_type);

    case 0x19: // MBC5
    case 0x1A: // MBC5+RAM
    case 0x1B: // MBC5+RAM+BATTERY
    case 0x1C: // MBC5+RUMBLE
    case 0x1D: // MBC5+RUMBLE+RAM
    case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
        return std::make_unique<MBC5>(cartridge_type);

    default:
        fprintf(stderr, "Unsupported cartridge type: 0x%02X\n", cartridge_type);
        return std::make_unique<MBC0>(cartridge_type);
    }
}
