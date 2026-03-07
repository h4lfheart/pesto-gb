#pragma once

#include <cstdint>

#define REGISTER(hi, lo)  \
    union {               \
        struct {          \
            uint8_t lo;   \
            uint8_t hi;   \
        };                \
        uint16_t hi##lo;  \
    }

#define FLAG_MASK_Z 0b10000000
#define FLAG_MASK_N 0b01000000
#define FLAG_MASK_H 0b00100000
#define FLAG_MASK_C 0b00010000

#define U4_MASK  0xF
#define U8_MASK  0xFF
#define U16_MASK 0xFFFF

#define HALF_CARRY_ADD(a, b) (((((a) & U4_MASK) + ((b) & U4_MASK)) & 0x10) == 0x10)
#define HALF_CARRY_SUB(a, b) ((((a) & U4_MASK) < ((b) & U4_MASK)) ? 1 : 0)
#define HALF_CARRY_ADD_16(a, b) (((((a) & 0xfff) + ((b) & 0xfff)) & 0x1000) == 0x1000)

enum class RegisterType
{
    REG_NONE,

    // u8
    REG_A,
    REG_F,
    REG_B,
    REG_C,
    REG_D,
    REG_E,
    REG_H,
    REG_L,

    // u16
    REG_AF,
    REG_BC,
    REG_DE,
    REG_HL,

    // special
    REG_PC,
    REG_SP,

    REG_MAX
};

enum class FlagType
{
    FLAG_Z, FLAG_N, FLAG_H, FLAG_C
};

static constexpr uint8_t FLAG_MASKS[4] = {
    FLAG_MASK_Z, FLAG_MASK_N, FLAG_MASK_H, FLAG_MASK_C
};

struct Registers
{
    REGISTER(A, F);

    REGISTER(B, C);

    REGISTER(D, E);

    REGISTER(H, L);

    uint16_t PC = 0;
    uint16_t SP = 0;

    Registers()
    {
        reg8_lut[static_cast<int>(RegisterType::REG_A)] = &A;
        reg8_lut[static_cast<int>(RegisterType::REG_F)] = &F;
        reg8_lut[static_cast<int>(RegisterType::REG_B)] = &B;
        reg8_lut[static_cast<int>(RegisterType::REG_C)] = &C;
        reg8_lut[static_cast<int>(RegisterType::REG_D)] = &D;
        reg8_lut[static_cast<int>(RegisterType::REG_E)] = &E;
        reg8_lut[static_cast<int>(RegisterType::REG_H)] = &H;
        reg8_lut[static_cast<int>(RegisterType::REG_L)] = &L;

        reg16_lut[static_cast<int>(RegisterType::REG_AF)] = &AF;
        reg16_lut[static_cast<int>(RegisterType::REG_BC)] = &BC;
        reg16_lut[static_cast<int>(RegisterType::REG_DE)] = &DE;
        reg16_lut[static_cast<int>(RegisterType::REG_HL)] = &HL;
        reg16_lut[static_cast<int>(RegisterType::REG_PC)] = &PC;
        reg16_lut[static_cast<int>(RegisterType::REG_SP)] = &SP;
    }

    uint8_t* Reg8(RegisterType reg) const;

    uint16_t* Reg16(RegisterType reg) const;

    void SetFlag(FlagType flag, uint8_t value);

    bool GetFlag(FlagType flag) const;

private:
    uint8_t* reg8_lut[static_cast<int>(RegisterType::REG_MAX)] = {};
    uint16_t* reg16_lut[static_cast<int>(RegisterType::REG_MAX)] = {};
};
