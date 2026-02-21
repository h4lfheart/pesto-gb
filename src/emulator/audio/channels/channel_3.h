#pragma once
#include "base_channel.h"

#define CH3_NR30_ADDR 0x1A
#define CH3_NR31_ADDR 0x1B
#define CH3_NR32_ADDR 0x1C
#define CH3_NR33_ADDR 0x1D
#define CH3_NR34_ADDR 0x1E

#define CH3_WAVE_RAM_START 0x30
#define CH3_WAVE_RAM_END 0x3F
#define CH3_WAVE_SAMPLE_COUNT 32

#define CH3_NR30_DAC_MASK 0b10000000

#define CH3_NR32_VOLUME_MASK 0b01100000

class Channel3 : public BaseChannel
{
public:
    void Tick() override;
    void TickFrame(uint8_t frame_idx) override;
    void Reset() override;
    void AttachMemory(Memory* mem) override;
    bool IsDACEnabled() override;

    void Trigger();

    void TickLength();

    uint8_t* nr30 = nullptr;
    uint8_t* nr31 = nullptr;
    uint8_t* nr32 = nullptr;
    uint8_t* nr33 = nullptr;
    uint8_t* nr34 = nullptr;

    uint16_t period = 0;
    uint16_t period_timer = 0;

    uint8_t wave_step = 0;

    uint8_t volume = 0;
    uint16_t length_timer = 0;

    float dc_offset;
};