#include "timer.h"

#include "../cpu/cpu.h"

static constexpr uint16_t clock_rates[4] =
{
    1024,
    16,
    64,
    256
};

void Timer::AttachMemory(Memory* mem)
{
    this->memory = mem;

    DIV = mem->PtrIO(IO_ADDR_DIV);
    TIMA = mem->PtrIO(IO_ADDR_TIMA);
    TMA = mem->PtrIO(IO_ADDR_TMA);
    TAC = mem->PtrIO(IO_ADDR_TAC);
}

void Timer::Cycle(uint8_t cycles)
{
    this->div_cycles += cycles;
    if (this->div_cycles >= DIV_CLOCK_RATE)
    {
        const uint16_t div_ticks = this->div_cycles / DIV_CLOCK_RATE;
        this->div_cycles -= div_ticks * DIV_CLOCK_RATE;
        *DIV += div_ticks;
    }

    if ((*TAC & TAC_ENABLE) == 0)
        return;

    const uint16_t tima_clock_rate = clock_rates[*TAC & TAC_SELECT_MASK];

    this->tima_cycles += cycles;
    if (this->tima_cycles >= tima_clock_rate)
    {
        const uint16_t tima_ticks = this->tima_cycles / tima_clock_rate;

        this->tima_cycles -= tima_ticks * tima_clock_rate;

        const uint16_t tima_new = *TIMA + tima_ticks;
        if (tima_new > 0xFF)
        {
            *TIMA = *TMA;
            this->memory->SetInterruptFlag(INTERRUPT_TIMER);
        }
        else
        {
            *TIMA = static_cast<uint8_t>(tima_new);
        }
    }
}
