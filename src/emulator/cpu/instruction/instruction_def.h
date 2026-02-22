#pragma once
#include <cstdint>
#include "../cpu.h"

struct InstructionRuntime;
typedef void(*InstructionFunc)(CPU* cpu, InstructionRuntime* instruction);

struct InstructionDef
{
    const char* name;

    uint8_t opcode;
    InstructionFunc func;
    uint8_t size;

    uint8_t main_cycles = 0;
    uint8_t alt_cycles = 0;

    RegisterType op1 = RegisterType::REG_NONE;
    RegisterType op2 = RegisterType::REG_NONE;

    uint16_t param1 = 0;
    uint16_t param2 = 0;


};

struct InstructionRuntime
{
    const InstructionDef* def;

    union {
        uint16_t u16;
        uint8_t u8;
        int8_t s8;
    } imm;

    uint8_t cycles;

    void Execute(CPU* cpu);

    static InstructionRuntime* From(Memory* memory, uint16_t address);
};