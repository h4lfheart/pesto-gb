#include "registers.h"

uint8_t* Registers::Reg8(RegisterType reg) const
{
    return reg8_lut[static_cast<int>(reg)];
}

uint16_t* Registers::Reg16(RegisterType reg) const
{
    return reg16_lut[static_cast<int>(reg)];
}

void Registers::SetFlag(FlagType flag, uint8_t value)
{
    const uint8_t mask = FLAG_MASKS[static_cast<int>(flag)];
    if (value)
        F |= mask;
    else
        F &= ~mask;
}

bool Registers::GetFlag(FlagType flag) const
{
    return (F & FLAG_MASKS[static_cast<int>(flag)]) != 0;
}
