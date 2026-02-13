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
    REG_SP
};

enum class FlagType
{
    FLAG_Z, FLAG_N, FLAG_H, FLAG_C
};

struct Registers
{
    REGISTER(A, F);
    REGISTER(B, C);
    REGISTER(D, E);
    REGISTER(H, L);

    uint16_t PC = 0;
    uint16_t SP = 0;

    bool GetFlag(FlagType flag);
    void SetFlag(FlagType flag, uint8_t value);

    uint8_t* Reg8(RegisterType reg);
    uint16_t* Reg16(RegisterType reg);

    void Write16(RegisterType reg, uint16_t value);
};
