#pragma once
#include <cstdint>

#include "registers.h"
#include "../memory/memory.h"

#define CLOCK_RATE 4194304
#define CLOCK_CYCLE (1000000000 / CLOCK_RATE)
#define FRAME_PER_SECOND 59.7
#define LOGIC_FRAME_RATE 59.7
#define FRAME_INTERVAL (1000000000 / LOGIC_FRAME_RATE)
#define CYCLES_PER_FRAME (CLOCK_RATE / FRAME_PER_SECOND)

#define INTERRUPT_VBLANK 0b00000001
#define INTERRUPT_STAT 0b00000010
#define INTERRUPT_TIMER 0b00000100
#define INTERRUPT_SERIAL 0b00001000
#define INTERRUPT_JOYPAD 0b00010000
#define INTERRUPT_MASK 0b00011111

#define IO_ADDR_INTERRUPT_FLAG 0x0F

#define INT_ADDR_VBLANK   0x40
#define INT_ADDR_LCD_STAT 0x48
#define INT_ADDR_TIMER    0x50
#define INT_ADDR_SERIAL   0x58
#define INT_ADDR_JOYPAD   0x60


class Cpu
{
public:
    Cpu();

    void AttachMemory(Memory* mem);
    void Cycle();
    bool TryExecuteInterrupts();

    void Push16(uint16_t value);
    uint16_t Pop16();

    Registers reg = {};
    uint8_t cur_instr_cycles;
    uint64_t cycles;

    bool halt = false;
    bool ime = false;

    Memory* memory;
private:
    void ExecuteInterrupt(uint16_t addr);
};
