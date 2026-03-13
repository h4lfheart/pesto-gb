#pragma once
#include <cstdint>
#include "../cpu.h"

struct InstructionDef;
typedef void (*InstructionFunc)(CPU* cpu, const InstructionDef* def);

struct InstructionDef
{
    const char*     name;
    uint8_t         opcode;
    InstructionFunc func;
    uint8_t         size;
    uint8_t         main_cycles;
    uint8_t         alt_cycles;
    RegisterType    op1   = RegisterType::REG_NONE;
    RegisterType    op2   = RegisterType::REG_NONE;
    uint16_t        param1 = 0;
    uint16_t        param2 = 0;
};
