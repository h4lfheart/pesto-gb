#include "cpu.h"
#include <cstdio>
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

int Cpu::Cycle()
{
    bool ime_was_pending = this->ime_pending;

    if (this->halt)
    {
        const uint8_t interrupt_flag = this->memory->ReadIO(IO_ADDR_INTERRUPT_FLAG);
        const uint8_t pending = (this->memory->ie & interrupt_flag) & INTERRUPT_MASK;
        if (pending)
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
        InstructionRuntime* instr = InstructionRuntime::From(this->memory, this->reg.PC);
        if (instr == nullptr)
        {
            uint8_t op = this->memory->Read8(this->reg.PC);
            fprintf(stderr, "Unknown instruction at 0x%.4X: 0x%.2X\n", this->reg.PC, op == 0xCB? (op << 8) | this->memory->Read8(this->reg.PC + 1): op);
            exit(1);
        }

        this->reg.PC += instr->def->size;
        instr->Execute(this);
        mcycles = instr->cycles;
    }

    if (ime_was_pending)
    {
        this->ime_pending = false;
        this->ime = true;
    }

    this->cycles += mcycles;
    return mcycles;
}

bool Cpu::TryExecuteInterrupts()
{
    const uint8_t interrupt_flag = this->memory->ReadIO(IO_ADDR_INTERRUPT_FLAG);
    const uint8_t pending = (this->memory->ie & interrupt_flag) & INTERRUPT_MASK;
    if (pending == 0)
        return false;

    this->ime = false;

    struct { uint8_t mask; uint16_t addr; } handlers[] = {
        { INTERRUPT_VBLANK, INT_ADDR_VBLANK   },
        { INTERRUPT_STAT,   INT_ADDR_LCD_STAT },
        { INTERRUPT_TIMER,  INT_ADDR_TIMER    },
        { INTERRUPT_SERIAL, INT_ADDR_SERIAL   },
        { INTERRUPT_JOYPAD, INT_ADDR_JOYPAD   },
    };

    for (const auto& [mask, addr] : handlers)
    {
        if (pending & mask)
        {
            this->memory->WriteIO(IO_ADDR_INTERRUPT_FLAG, interrupt_flag & ~mask);
            ExecuteInterrupt(addr);
            return true;
        }
    }

    return false;
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
