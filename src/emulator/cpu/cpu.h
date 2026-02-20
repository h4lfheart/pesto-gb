#pragma once
#include <cstdint>
#include "registers.h"
#include "../memory/memory.h"

#define CLOCK_RATE 4194304
#define CLOCK_CYCLE (1000000000 / CLOCK_RATE)
#define FRAMES_PER_SECOND 59.725
#define FRAME_INTERVAL (1000000000 / FRAMES_PER_SECOND)
#define CYCLES_PER_FRAME (CLOCK_RATE / FRAMES_PER_SECOND)

#define T_CYCLES_PER_M_CYCLE  4

#define INTERRUPT_VBLANK 0b00000001
#define INTERRUPT_STAT 0b00000010
#define INTERRUPT_TIMER 0b00000100
#define INTERRUPT_SERIAL 0b00001000
#define INTERRUPT_JOYPAD 0b00010000
#define INTERRUPT_MASK 0b00011111

#define IO_ADDR_INTERRUPT_FLAG 0x0F

#define INT_ADDR_VBLANK 0x40
#define INT_ADDR_LCD_STAT 0x48
#define INT_ADDR_TIMER 0x50
#define INT_ADDR_SERIAL 0x58
#define INT_ADDR_JOYPAD 0x60

#define INTERRUPT_MCYCLES 5

class Cpu
{
public:
    Cpu();

    void AttachMemory(Memory* mem);
    int Cycle();
    bool TryExecuteInterrupts();

    void Push16(uint16_t value);
    uint16_t Pop16();

    Registers reg = {};
    uint64_t cycles = 0;

    bool stop = false;
    bool halt = false;
    bool ime = false;
    bool ime_pending = false;

    Memory* memory = nullptr;
private:
    void ExecuteInterrupt(uint16_t addr);
};