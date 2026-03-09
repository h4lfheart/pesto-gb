#include "cpu.h"
#include <cstdio>
#include "instruction/instruction_def.h"
#include "instruction/instruction_set.h"

CPU::CPU()
{
    InstructionSet::Initialize();
}

void CPU::AttachMemory(Memory* mem)
{
    this->memory = mem;

    IF = memory->PtrIO(IO_ADDR_INTERRUPT_FLAG);
    IE = &memory->ie;
}

int CPU::Cycle()
{
    const bool ime_was_pending = this->ime_pending;

    if (this->halt)
    {
        if (*IE & *IF & INTERRUPT_MASK)
            this->halt = false;
    }

    int mcycles = 0;

    if (this->ime && TryExecuteInterrupts())
    {
        mcycles = INTERRUPT_MCYCLES;
    }
    else if (this->halt)
    {
        mcycles = 1;
    }
    else
    {
        uint16_t addr = reg.PC;
        uint8_t op = memory->Read8(addr++);

        const bool is_prefix = (op == 0xCB);

        const InstructionDef* instruction;

        if (is_prefix) [[unlikely]]
        {
            op = memory->Read8(addr++);
            instruction = InstructionSet::GetPrefixed(op);
        }
        else
        {
            instruction = InstructionSet::Get(op);
        }

        InstructionRuntime instr {
            .def = instruction,
            .cycles = instruction->main_cycles
        };

        if (!is_prefix) [[likely]]
        {
            switch (instruction->size)
            {
            case 2:
                instr.imm.u8 = memory->Read8(addr);
                addr++;
                break;

            case 3:
                instr.imm.u16 = memory->Read16(addr);
                addr += 2;
                break;
            }
        }

        reg.PC = addr;

        instr.Execute(this);
        mcycles = instr.cycles;
    }

    if (ime_was_pending)
    {
        this->ime_pending = false;
        this->ime = true;
    }

    this->cycles += mcycles;
    return mcycles;
}

static const struct { uint8_t mask; uint16_t addr; } interrupt_handlers[] = {
    {INTERRUPT_VBLANK, INT_ADDR_VBLANK},
    {INTERRUPT_STAT,   INT_ADDR_LCD_STAT},
    {INTERRUPT_TIMER,  INT_ADDR_TIMER},
    {INTERRUPT_SERIAL, INT_ADDR_SERIAL},
    {INTERRUPT_JOYPAD, INT_ADDR_JOYPAD},
};

bool  CPU::TryExecuteInterrupts()
{
    const uint8_t pending = *IE & *IF & INTERRUPT_MASK;
    if (pending == 0)
        return false;

    this->ime = false;

    for (const auto& [mask, addr] : interrupt_handlers)
    {
        if (pending & mask)
        {
            *IF &= ~mask;
            ExecuteInterrupt(addr);
            return true;
        }
    }

    return false;
}

void  CPU::Push16(uint16_t value)
{
    this->reg.SP -= sizeof(uint16_t);
    this->memory->Write16(this->reg.SP, value);
}

uint16_t  CPU::Pop16()
{
    uint16_t value = this->memory->Read16(this->reg.SP);
    this->reg.SP += sizeof(uint16_t);

    return value;
}

void  CPU::ExecuteInterrupt(uint16_t addr)
{
    Push16(this->reg.PC);
    this->reg.PC = addr;
}
