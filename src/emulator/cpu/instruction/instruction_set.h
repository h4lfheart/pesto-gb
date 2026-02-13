#pragma once
#include "instruction_def.h"

#define INSTRUCTION_SET_SIZE 0x100

class InstructionSet
{
public:
    static void Initialize();
    static InstructionDef* Get(uint8_t opcode);
    static InstructionDef* GetPrefixed(uint8_t opcode);

private:
    static InstructionDef* instructions[INSTRUCTION_SET_SIZE];
    static InstructionDef* prefix_instructions[INSTRUCTION_SET_SIZE];

    static int instruction_mappings[INSTRUCTION_SET_SIZE];
    static int prefix_instruction_mappings[INSTRUCTION_SET_SIZE];
};
