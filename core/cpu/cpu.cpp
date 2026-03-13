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

int CPU::ExecuteInstruction()
{
    uint16_t addr = this->reg.PC;
    uint8_t op = this->memory->Read8(addr++);

    const bool is_prefix = (op == 0xCB);

    const InstructionDef* def;
    if (is_prefix) [[unlikely]]
    {
        op = this->memory->Read8(addr++);
        def = InstructionSet::GetPrefixed(op);
    }
    else
    {
        def = InstructionSet::Get(op);
    }

    if (!is_prefix) [[likely]]
    {
        switch (def->size)
        {
        case 2:
            this->imm.u8 = this->memory->Read8(addr);
            addr++;
            break;
        case 3:
            this->imm.u16 = this->memory->Read16(addr);
            addr += 2;
            break;
        }
    }

    this->reg.PC = addr;
    this->exec_cycles = def->main_cycles;
    def->func(this, def);
    return this->exec_cycles;
}

int CPU::Cycle()
{
    const bool ime_was_pending = this->ime_pending;

    const uint8_t interrupt_pending = *IE & *IF & INTERRUPT_MASK;

    // quick case
    if (!this->halt && !(this->ime && interrupt_pending)) [[likely]]
    {
        const int cycles_elapsed = ExecuteInstruction();

        if (ime_was_pending) [[unlikely]]
        {
            this->ime_pending = false;
            this->ime = true;
        }

        this->cycles += cycles_elapsed;
        return cycles_elapsed;
    }


    if (this->halt && interrupt_pending)
        this->halt = false;

    int cycles_elapsed;
    if (this->ime && TryExecuteInterrupts(interrupt_pending))
    {
        cycles_elapsed = INTERRUPT_MCYCLES;
    }
    else if (this->halt)
    {
        cycles_elapsed = 1;
    }
    else
    {
        cycles_elapsed = ExecuteInstruction();
    }

    if (ime_was_pending)
    {
        this->ime_pending = false;
        this->ime = true;
    }

    this->cycles += cycles_elapsed;
    return cycles_elapsed;
}

static const struct
{
    uint8_t mask;
    uint16_t addr;
} interrupt_handlers[] = {
    {INTERRUPT_VBLANK, INT_ADDR_VBLANK},
    {INTERRUPT_STAT, INT_ADDR_LCD_STAT},
    {INTERRUPT_TIMER, INT_ADDR_TIMER},
    {INTERRUPT_SERIAL, INT_ADDR_SERIAL},
    {INTERRUPT_JOYPAD, INT_ADDR_JOYPAD},
};

bool CPU::TryExecuteInterrupts(uint8_t interrupt_pending)
{
    if (interrupt_pending == 0)
        return false;

    this->ime = false;

    for (const auto& [mask, addr] : interrupt_handlers)
    {
        if (interrupt_pending & mask)
        {
            *IF &= ~mask;
            ExecuteInterrupt(addr);
            return true;
        }
    }
    return false;
}

void CPU::Push16(uint16_t value)
{
    this->reg.SP -= sizeof(uint16_t);
    this->memory->Write16(this->reg.SP, value);
}

uint16_t CPU::Pop16()
{
    uint16_t value = this->memory->Read16(this->reg.SP);
    this->reg.SP += sizeof(uint16_t);
    return value;
}

void CPU::ExecuteInterrupt(uint16_t addr)
{
    Push16(this->reg.PC);
    this->reg.PC = addr;
}
