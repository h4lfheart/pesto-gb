#include "timer.h"

#include "../cpu/cpu.h"

static uint16_t clock_rates[4] =
{
    256,
    4,
    16,
    64
};

Timer::Timer()
{
}

void Timer::AttachMemory(Memory* mem)
{
    this->memory = mem;
}

void Timer::Cycle()
{
    this->div_cycles++;

    if (this->div_cycles >= DIV_CYCLES_PER_TICK)
    {
        this->div_cycles = 0;

        // TODO io pointers?
        this->memory->WriteIO(IO_ADDR_DIV, this->memory->ReadIO(IO_ADDR_DIV) + 1);
    }

    const uint8_t tac = this->memory->ReadIO(IO_ADDR_TAC);
    if (tac & TAC_ENABLE)
    {
        this->tima_cycles++;

        const uint8_t mode = tac & TAC_SELECT_MASK;

        if (this->tima_cycles >= clock_rates[mode])
        {
            this->tima_cycles = 0;

            const uint8_t tima = this->memory->ReadIO(IO_ADDR_TIMA);
            if (tima == 0xFF)
            {
                this->memory->WriteIO(IO_ADDR_TIMA,this->memory->ReadIO(IO_ADDR_TMA));

                const uint8_t interrupt_flag = this->memory->ReadIO(IO_ADDR_INTERRUPT_FLAG);
                this->memory->WriteIO(IO_ADDR_INTERRUPT_FLAG, interrupt_flag | INTERRUPT_TIMER);
            }
            else
            {
                this->memory->WriteIO(IO_ADDR_TIMA, tima + 1);
            }
        }
    }
}
