#pragma once

#include "../memory/memory.h"
#include "../cpu/cpu.h"
#include <cstdint>

#include "channels/channel_1.h"
#include "channels/channel_2.h"
#include "channels/channel_3.h"
#include "channels/channel_4.h"

#define APU_SAMPLE_RATE 44100
#define APU_CYCLES_PER_SAMPLE (CLOCK_RATE / APU_SAMPLE_RATE)

#define APU_NR50_ADDR 0x24
#define APU_NR51_ADDR 0x25
#define APU_NR52_ADDR 0x26

#define APU_NR50_RIGHT_VOLUME_MASK 0b00000111
#define APU_NR50_LEFT_VOLUME_MASK 0b01110000

#define APU_NR51_CH1_RIGHT_MASK 0b00000001
#define APU_NR51_CH2_RIGHT_MASK 0b00000010
#define APU_NR51_CH3_RIGHT_MASK 0b00000100
#define APU_NR51_CH4_RIGHT_MASK 0b00001000
#define APU_NR51_CH1_LEFT_MASK 0b00010000
#define APU_NR51_CH2_LEFT_MASK 0b00100000
#define APU_NR51_CH3_LEFT_MASK 0b01000000
#define APU_NR51_CH4_LEFT_MASK 0b10000000

#define APU_NR52_CH1_ENABLE_MASK 0b00000001
#define APU_NR52_CH2_ENABLE_MASK 0b00000010
#define APU_NR52_CH3_ENABLE_MASK 0b00000100
#define APU_NR52_CH4_ENABLE_MASK 0b00001000
#define APU_NR52_ENABLE_MASK 0b10000000

#define APU_FRAME_RATE 8192
#define APU_MAX_VOLUME 15.0f
#define APU_VOLUME_DIVISOR 7.0f
#define APU_MIX_SCALE 0.25f
#define APU_CHANNEL_COUNT 4

#define APU_REG_START 0x10
#define APU_REG_END 0x26

class APU
{
public:
    APU();

    void Cycle();
    void AttachMemory(Memory* mem);
    void GetSamples(float& left, float& right) const;

    bool ready_for_samples = false;
    Channel3* channel3 = nullptr;

private:
    void TickFrame();

    uint8_t APURead(uint8_t* io, uint16_t offset);
    void APUWrite(uint8_t* io, uint16_t offset, uint8_t value);

    Memory* memory = nullptr;

    bool enabled = false;

    Channel1* channel1 = nullptr;
    Channel2* channel2 = nullptr;
    Channel4* channel4 = nullptr;

    uint32_t sample_counter = 0;
    uint32_t frame_counter = 0;
    uint8_t frame_step = 0;

    float sample_left = 0.0f;
    float sample_right = 0.0f;
};