#pragma once

#include "../memory/memory.h"

#define DIV_CYCLES_PER_TICK 256

#define IO_ADDR_DIV 0x04
#define IO_ADDR_TIMA 0x05
#define IO_ADDR_TMA 0x06
#define IO_ADDR_TAC 0x07

#define TAC_ENABLE 0b00000100
#define TAC_SELECT_MASK 0b00000011


class Timer
{
public:
    Timer();

    void AttachMemory(Memory* mem);

    void Cycle();

private:
    Memory* memory = nullptr;

    uint16_t tima_cycles = 0;
    uint16_t div_cycles = 0;

};