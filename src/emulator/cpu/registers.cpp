#include "registers.h"

void Registers::SetFlag(FlagType flag, uint8_t value)
{
    switch (flag)
    {
    case FlagType::FLAG_Z:
        if (value)
            *Reg8(RegisterType::REG_F) |= FLAG_MASK_Z;
        else
            *Reg8(RegisterType::REG_F) &= ~FLAG_MASK_Z;
        break;
    case FlagType::FLAG_N:
        if (value)
            *Reg8(RegisterType::REG_F) |= FLAG_MASK_N;
        else
            *Reg8(RegisterType::REG_F) &= ~FLAG_MASK_N;
        break;
    case FlagType::FLAG_H:
        if (value)
            *Reg8(RegisterType::REG_F) |= FLAG_MASK_H;
        else
            *Reg8(RegisterType::REG_F) &= ~FLAG_MASK_H;
        break;
    case FlagType::FLAG_C:
        if (value)
            *Reg8(RegisterType::REG_F) |= FLAG_MASK_C;
        else
            *Reg8(RegisterType::REG_F) &= ~FLAG_MASK_C;
        break;
    }
}

bool Registers::GetFlag(FlagType flag)
{
    switch (flag)
    {
    case FlagType::FLAG_Z:
        return *Reg8(RegisterType::REG_F) & FLAG_MASK_Z;
    case FlagType::FLAG_N:
        return *Reg8(RegisterType::REG_F) & FLAG_MASK_N;
    case FlagType::FLAG_H:
        return *Reg8(RegisterType::REG_F) & FLAG_MASK_H;
    case FlagType::FLAG_C:
        return *Reg8(RegisterType::REG_F) & FLAG_MASK_C;
    default:
        return false;
    }
}

uint8_t* Registers::Reg8(RegisterType reg)
{
    switch (reg)
    {
    case RegisterType::REG_A:
        return &A;
    case RegisterType::REG_F:
        return &F;
    case RegisterType::REG_B:
        return &B;
    case RegisterType::REG_C:
        return &C;
    case RegisterType::REG_D:
        return &D;
    case RegisterType::REG_E:
        return &E;
    case RegisterType::REG_H:
        return &H;
    case RegisterType::REG_L:
        return &L;
    default:
        return nullptr;
    }
}

uint16_t* Registers::Reg16(RegisterType reg)
{
    switch (reg)
    {
    case RegisterType::REG_AF:
        return &AF;
    case RegisterType::REG_BC:
        return &BC;
    case RegisterType::REG_DE:
        return &DE;
    case RegisterType::REG_HL:
        return &HL;
    case RegisterType::REG_PC:
        return &PC;
    case RegisterType::REG_SP:
        return &SP;
    default:
        return nullptr;
    }
}

void Registers::Write16(RegisterType reg, uint16_t value)
{
    switch (reg)
    {
    case RegisterType::REG_AF:
        AF = value;
        break;

    case RegisterType::REG_BC:
        BC = value;
        break;

    case RegisterType::REG_DE:
        DE = value;
        break;

    case RegisterType::REG_HL:
        HL = value;
        break;

    case RegisterType::REG_PC:
        PC = value;
        break;

    case RegisterType::REG_SP:
        SP = value;
        break;

    default:
        break;
    }
}
