#include "cpu.h"

#include <cstdio>
#include <process.h>

#include "instruction/instruction_def.h"
#include "instruction/instruction_set.h"

Cpu::Cpu()
{
    InstructionSet::Initialize();
}

void Cpu::AttachMemory(Memory* mem)
{
    this->memory = mem;
}

void Cpu::Cycle()
{
    this->cycles++;

    if (this->cur_instr_cycles > 0)
    {
        this->cur_instr_cycles--;
        return;
    }

    if (TryExecuteInterrupts())
    {
        return;
    }

    InstructionRuntime* instruction = InstructionRuntime::From(this->memory, this->reg.PC);
    if (instruction == nullptr)
    {
        uint8_t op = this->memory->Read8(this->reg.PC);
        fprintf(stderr, "Unknown Instruction at 0x%.4X: 0x%.2X\n\n", this->reg.PC, op == 0xCB ? op << 8 | this->memory->Read8(this->reg.PC + 1) : op);
        exit(1);
        return;
    }


    this->reg.PC += instruction->def->size;

    instruction->Execute(this);

    this->cur_instr_cycles = instruction->cycles;
}

bool Cpu::TryExecuteInterrupts()
{
    if (!this->ime)
        return false;

    const uint8_t interrupt_flag = this->memory->ReadIO(IO_ADDR_INTERRUPT_FLAG);
    const uint8_t interrupts = (this->memory->ie & interrupt_flag) & INTERRUPT_MASK;
    if (interrupts == 0)
        return false;

    this->ime = false;

    if (interrupts & INTERRUPT_VBLANK)
    {
        this->memory->WriteIO(IO_ADDR_INTERRUPT_FLAG, interrupt_flag & ~INTERRUPT_VBLANK);
        ExecuteInterrupt(INT_ADDR_VBLANK);
    }

    return true;
}

void Cpu::Push16(uint16_t value)
{
    this->reg.SP -= sizeof(uint16_t);
    this->memory->Write16(this->reg.SP, value);
}

uint16_t Cpu::Pop16()
{
    uint16_t value = this->memory->Read16(this->reg.SP);
    this->reg.SP += sizeof(uint16_t);

    return value;
}

void Cpu::ExecuteInterrupt(uint16_t addr)
{

    Push16(this->reg.PC);

    this->reg.PC = addr;
}
