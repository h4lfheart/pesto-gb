#pragma once
#include "mbc.h"

class MBC1 : public MBC
{
public:
    uint8_t ReadRom(const uint8_t* rom_data, uint16_t address) override;
    void WriteRom(uint16_t address, uint8_t value) override;
    uint8_t ReadRam(const uint8_t* ram_data, uint16_t address) override;
    void WriteRam(uint8_t* ram_data, uint16_t address, uint8_t value) override;

private:
    uint8_t rom_bank = 1;
    uint8_t ram_bank = 0;
    bool ram_enabled = false;
    bool use_ram_banking = false;
};