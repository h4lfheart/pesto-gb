#include "instruction_def.h"

#include "instruction_set.h"


void InstructionRuntime::Execute(Cpu* cpu)
{
    this->def->func(cpu, this);
}

InstructionRuntime* InstructionRuntime::From(Memory* memory, uint16_t address)
{
    uint16_t cur_address = address;
    uint8_t op = memory->Read8(cur_address++);

    const bool is_prefix = op == 0xCB;
    if (is_prefix)
        op = memory->Read8(cur_address++);

    const InstructionDef* instruction = is_prefix ? InstructionSet::GetPrefixed(op) : InstructionSet::Get(op);
    if (instruction == nullptr)
        return nullptr;

    auto* runtime = new InstructionRuntime{
        .def = instruction,
        .cycles = instruction->main_cycles
    };

    if (!is_prefix)
    {
        if (instruction->size == 2)
        {
            runtime->imm.u8 = memory->Read8(cur_address++);
        }
        else if (instruction->size == 3)
        {
            runtime->imm.u16 = memory->Read16(cur_address++);
        }
    }

    return runtime;
}
